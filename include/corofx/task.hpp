#pragma once

#include "corofx/effect.hpp"
#include "detail/type_set.hpp"
#include "promise.hpp"

#include <concepts>
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
        handled_task{std::move(that)}.swap(*this);
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

    auto set_cont(promise_impl<value_type>& cont) noexcept -> void {
        task_.frame_.promise().set_cont(&cont);
        std::apply([&](auto&... hs) { (hs.set_cont(&cont), ...); }, handlers_);
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
        task{std::move(that)}.swap(*this);
        return *this;
    }

    [[nodiscard]]
    auto operator()() && noexcept -> T
        requires(sizeof...(Es) == 0)
    {
        if constexpr (std::is_void_v<T>) {
            call_unchecked();
        } else {
            auto output = std::optional<T>{};
            call_unchecked(output);
            return std::move(*output);
        }
    }

    // Runs the task with the provided handlers when awaited.
    template<typename... Hs>
    [[nodiscard]]
    auto with(Hs... handlers) && noexcept -> handled_task<task, Hs...>
        requires(
            detail::type_set<Es...>::template contains<
                detail::type_set<typename Hs::effect_type...>> and
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

    auto swap(task& that) noexcept -> void {
        using std::swap;
        swap(frame_, that.frame_);
    }

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

    handle_type frame_{};
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
    auto await_transform(task<U, Gs...>& task) noexcept -> task_awaiter<U>
        requires(detail::type_set<Es...>::template contains<detail::type_set<Gs...>>)
    {
        auto& p = task.frame_.promise();
        p.copy_handlers(*this);
        return task_awaiter<U>{task};
    }

    template<std::movable Task, typename... Hs>
    [[nodiscard]]
    auto await_transform(handled_task<Task, Hs...>& task) noexcept
        -> task_awaiter<typename Task::value_type>
        requires(detail::type_set<Es...>::template contains<
                 typename handled_task<Task, Hs...>::effect_types>)
    {
        task.copy_handlers(*this);
        task.set_cont(*this);
        return task_awaiter<typename Task::value_type>{task};
    }

    template<effect E>
    [[nodiscard]]
    auto await_transform(E eff) noexcept -> effect_awaiter<E>
        requires(detail::type_set<Es...>::template contains<E>)
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
