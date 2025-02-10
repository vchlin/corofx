#pragma once

#include "corofx/effect.hpp"
#include "detail/type_set.hpp"
#include "promise.hpp"

#include <coroutine>
#include <tuple>
#include <utility>

namespace corofx {

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
        if constexpr (std::is_void_v<value_type>) {
            task_.call_unchecked();
        } else {
            auto output = std::optional<value_type>{};
            set_output(output);
            task_.call_unchecked(output);
            return std::move(*output);
        }
    }

    friend auto swap(handled_task& left, handled_task& right) noexcept -> void {
        using std::swap;
        swap(left.task_, right.task_);
        swap(left.handlers_, right.handlers_);
    }

private:
    template<typename T, effect... Es>
    friend class task;
    template<typename T>
    friend class task_awaiter;

    auto bind_handlers() noexcept -> void {
        std::apply([&](auto&... hs) { (task_.frame_.promise().set_handler(&hs), ...); }, handlers_);
    }

    template<typename Task2>
    auto copy_handlers(Task2& t) noexcept -> void {
        task_type::effect_types::template subtract<typename Hs::effect_type...>::apply(
            [&]<effect... Es>() {
                (task_.frame_.promise().set_handler(t.template get_handler<Es>()), ...);
            });
        std::apply([&](auto&... hs) { (hs.copy_handlers(t), ...); }, handlers_);
    }

    auto set_cont(std::coroutine_handle<> cont) noexcept -> void {
        task_.frame_.promise().set_cont(cont);
        std::apply([=](auto&... hs) { (hs.set_cont(cont), ...); }, handlers_);
    }

    [[nodiscard]]
    auto get_promise() noexcept -> task_type::promise_type& {
        return task_.frame_.promise();
    }

    auto set_output(std::optional<value_type>& output) noexcept -> void {
        task_.frame_.promise().set_output(output);
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

    task(task const&) = delete;
    task(task&& that) noexcept : frame_{std::exchange(that.frame_, {})} {}

    ~task() {
        if (frame_) frame_.destroy();
    }

    auto operator=(task const&) -> task& = delete;

    auto operator=(task&& that) noexcept -> task& {
        auto left = std::move(that);
        swap(left, *this);
        return *this;
    }

    [[nodiscard]]
    auto operator()() && noexcept -> T
        requires(effect_types::empty)
    {
        if constexpr (std::is_void_v<T>) {
            call_unchecked();
        } else {
            auto output = std::optional<T>{};
            call_unchecked(output);
            return std::move(*output);
        }
    }

    friend auto swap(task& left, task& right) noexcept -> void {
        using std::swap;
        swap(left.frame_, right.frame_);
    }

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
    template<typename U>
    friend class task_awaiter;

    explicit task(handle_type h) noexcept : frame_{h} {}

    [[nodiscard]]
    auto release() noexcept -> handle_type {
        return std::exchange(frame_, {});
    }

    [[nodiscard]]
    auto get_promise() noexcept -> promise_type& {
        return frame_.promise();
    }

    auto set_output(std::optional<T>& output) noexcept -> void
        requires(not std::is_void_v<T>)
    {
        frame_.promise().set_output(output);
    }

    auto call_unchecked() noexcept -> void
        requires(std::is_void_v<T>)
    {
        frame_.resume();
    }

    auto call_unchecked(std::optional<T>& output) noexcept -> void
        requires(not std::is_void_v<T>)
    {
        set_output(output);
        frame_.resume();
    }

    handle_type frame_;
};

template<typename T, effect... Es>
class task<T, Es...>::promise_type : public promise_impl<T> {
public:
    promise_type() noexcept : promise_impl<T>{handle_type::from_promise(*this)} {}

    [[nodiscard]]
    auto get_return_object() noexcept -> task {
        return task{handle_type::from_promise(*this)};
    }

    template<typename U, effect... Gs>
    [[nodiscard]]
    auto await_transform(task<U, Gs...> t) noexcept -> task_awaiter<decltype(t)>
        requires(effect_types::template contains<typename decltype(t)::effect_types>)
    {
        auto& p = t.frame_.promise();
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
        return effect_awaiter<E>{ev_vec_.template get_handler<E>(), this, std::move(eff)};
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
