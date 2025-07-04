#pragma once

#include "check.hpp"
#include "detail/type_set.hpp"
#include "effect.hpp"
#include "handler.hpp"
#include "promise.hpp"

#include <coroutine>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

namespace corofx {

template<typename Task>
class task_awaiter;

template<typename Task, typename... Hs>
class handled_task {
public:
    using task_type = Task;
    using value_type = task_type::value_type;
    using effect_types = task_type::effect_types::template subtract<
        typename Hs::effect_type...>::template add<typename Hs::task_type::effect_types...>;

    handled_task(Task task, Hs... handlers) noexcept
        : task_{std::move(task)}, handlers_{std::move(handlers)...} {
        bind_handlers();
    }

    handled_task(handled_task const&) = delete;

    handled_task(handled_task&& that) noexcept
        : task_{std::move(that.task_)}, handlers_{std::move(that.handlers_)} {
        bind_handlers();
    }

    ~handled_task() = default;

    auto operator=(handled_task const&) -> handled_task& = delete;

    auto operator=(handled_task&& that) noexcept -> handled_task& {
        auto left = std::move(that);
        swap(left, *this);
        bind_handlers();
        return *this;
    }

    [[nodiscard]]
    auto operator()() && noexcept -> value_type
        requires(effect_types::empty)
    {
        auto output = std::optional<value_holder<value_type>>{};
        set_output(output);
        task_.call_unchecked(output);
        if constexpr (not std::is_void_v<value_type>) return std::move(*output);
    }

    friend auto swap(handled_task& left, handled_task& right) noexcept -> void {
        using std::swap;
        swap(left.task_, right.task_);
        swap(left.handlers_, right.handlers_);
    }

private:
    template<typename T, effect... Es>
    friend class task;
    friend class task_awaiter<handled_task>;

    auto bind_handlers() noexcept -> void {
        std::apply(
            [&](auto&... hs) { (task_.frame_->promise().set_handler(&hs), ...); }, handlers_);
    }

    template<typename Task2>
    auto copy_handlers(Task2& t) noexcept -> void {
        task_type::effect_types::template subtract<typename Hs::effect_type...>::apply(
            [&]<effect... Es>() {
                (task_.frame_->promise().set_handler(t.template get_handler<Es>()), ...);
            });
        std::apply([&](auto&... hs) { (hs.copy_handlers(t), ...); }, handlers_);
    }

    auto set_cont(std::coroutine_handle<> cont) noexcept -> void {
        task_.frame_->promise().set_cont(cont);
        std::apply([=](auto&... hs) { (hs.set_cont(cont), ...); }, handlers_);
    }

    [[nodiscard]]
    auto get_frame() const noexcept -> task_type::handle_type {
        return *task_.frame_;
    }

    auto set_output(std::optional<value_holder<value_type>>& output) noexcept -> void {
        task_.frame_->promise().set_output(output);
        std::apply([&](auto&... hs) { (hs.set_output(output), ...); }, handlers_);
    }

    task_type task_;
    std::tuple<Hs...> handlers_;
};

// Represents a unit of computation that is potentially effectful.
template<typename T, effect... Es>
// TODO: Check if `Es...` are unique.
class task {
public:
    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    using value_type = T;
    using effect_types = detail::type_set<Es...>;

    [[nodiscard]]
    auto operator()() && noexcept -> T
        requires(effect_types::empty)
    {
        auto output = std::optional<value_holder<T>>{};
        call_unchecked(output);
        if constexpr (not std::is_void_v<T>) return std::move(*output);
    }

    operator frame<>() && noexcept { return std::move(frame_); }

    // Runs the task with the provided handlers when awaited.
    // TODO: Disallow multiple handlers for the same effect?
    template<typename... Hs>
    [[nodiscard]]
    auto with(Hs... handlers) && noexcept -> handled_task<task, Hs...>
        requires(
            effect_types::template contains<detail::type_set<typename Hs::effect_type...>> and
            (std::same_as<T, typename Hs::task_type::value_type> and ...))
    {
        return handled_task{std::move(*this), std::move(handlers)...};
    }

private:
    template<effect E, typename F>
    friend class handler_impl;
    template<typename Task, typename... Hs>
    friend class handled_task;
    friend class task_awaiter<task>;

    explicit task(handle_type h) noexcept : frame_{h} {}

    [[nodiscard]]
    auto get_frame() const noexcept -> handle_type {
        return *frame_;
    }

    auto set_output(std::optional<value_holder<T>>& output) noexcept -> void {
        frame_->promise().set_output(output);
    }

    auto call_unchecked(std::optional<value_holder<T>>& output) noexcept -> void {
        check(not frame_->done());
        set_output(output);
        frame_->resume();
    }

    frame<promise_type> frame_;
};

template<typename Task>
class task_awaiter : public std::suspend_always {
public:
    using value_type = Task::value_type;

    explicit task_awaiter(Task t) noexcept : task_{std::move(t)} { task_.set_output(value_); }

    task_awaiter(task_awaiter const&) = delete;
    task_awaiter(task_awaiter&&) = delete;
    ~task_awaiter() = default;
    auto operator=(task_awaiter const&) -> task_awaiter& = delete;
    auto operator=(task_awaiter&&) -> task_awaiter& = delete;

    [[nodiscard]]
    auto await_suspend(std::coroutine_handle<> frame) const noexcept -> std::coroutine_handle<> {
        auto h = task_.get_frame();
        h.promise().set_cont(frame);
        return h;
    }

    [[nodiscard]]
    auto await_resume() noexcept -> value_type {
        if constexpr (not std::is_void_v<value_type>) return std::move(*value_);
    }

private:
    Task task_;
    std::optional<value_holder<value_type>> value_;
};

template<typename T, effect... Es>
class task<T, Es...>::promise_type : public promise_impl<T> {
public:
    [[nodiscard]]
    auto get_return_object() noexcept -> task {
        return task{handle_type::from_promise(*this)};
    }

    template<typename U, effect... Gs>
    [[nodiscard]]
    auto await_transform(task<U, Gs...> t) noexcept -> task_awaiter<decltype(t)>
        requires(effect_types::template contains<typename decltype(t)::effect_types>)
    {
        auto& p = t.frame_->promise();
        p.copy_handlers(*this);
        return task_awaiter{std::move(t)};
    }

    template<typename Task, typename... Hs>
    [[nodiscard]]
    auto await_transform(handled_task<Task, Hs...> t) noexcept -> task_awaiter<decltype(t)>
        requires(effect_types::template contains<typename decltype(t)::effect_types>)
    {
        t.copy_handlers(*this);
        t.set_cont(handle_type::from_promise(*this));
        return task_awaiter{std::move(t)};
    }

    template<effect E>
    [[nodiscard]]
    auto await_transform(E eff) noexcept -> effect_awaiter<E>
        requires(effect_types::template contains<E>)
    {
        return effect_awaiter<E>{
            ev_vec_.template get_handler<E>(), handle_type::from_promise(*this), std::move(eff)};
    }

    template<effect E>
    auto set_handler(handler<E>* h) noexcept -> void {
        ev_vec_.set_handler(h);
    }

    template<effect E>
    [[nodiscard]]
    auto get_handler() const noexcept -> handler<E>* {
        return ev_vec_.template get_handler<E>();
    }

    template<typename Task>
    auto copy_handlers(Task& t) noexcept -> void {
        (set_handler(t.template get_handler<Es>()), ...);
    }

private:
    evidence_vec<Es...> ev_vec_;
};

} // namespace corofx
