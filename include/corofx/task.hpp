#pragma once

#include "detail/type_set.hpp"
#include "promise.hpp"

#include <concepts>
#include <coroutine>
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
        task_.frame_.promise().handlers_ = &handlers_;
    }

    auto operator()() && noexcept -> value_type
        requires(effect_types::empty)
    {
        // TODO: Remove duplicate code.
        task_.frame_.resume();
        return task_.frame_.promise().take_value();
    }

private:
    friend promise_base;

    task_type task_;
    handler_list_impl<Hs...> handlers_;
};

// Represents a unit of computation that is potentially effectful.
template<typename T, std::movable... Es>
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

    auto operator()() && noexcept -> T
        requires(sizeof...(Es) == 0)
    {
        frame_.resume();
        return frame_.promise().take_value();
    }

    // Runs the task with the provided handlers when awaited.
    template<typename... Hs>
    auto with(Hs... handlers) && noexcept -> handled_task<task, Hs...>
        requires(
            detail::type_set<Es...>::template contains<
                detail::type_set<typename Hs::effect_type...>> &&
            (std::same_as<value_type, typename Hs::task_type::value_type> && ...))
    {
        return handled_task{std::move(*this), std::move(handlers)...};
    }

private:
    template<typename U, std::movable... Gs>
    friend class task;
    // TODO: Remove friend?
    friend promise_base;
    template<typename E, typename F>
    friend class handler;
    template<typename Task, typename... Hs>
    friend class handled_task;

    explicit task(handle_type h) noexcept : frame_{h} {}

    auto swap(task& that) noexcept -> void {
        using std::swap;
        swap(frame_, that.frame_);
    }

    [[nodiscard]]
    auto release() noexcept -> handle_type {
        return std::exchange(frame_, {});
    }

    handle_type frame_{};
};

template<typename T, std::movable... Es>
class task<T, Es...>::promise_type : public promise_impl<T> {
public:
    using value_type = T;

    promise_type() noexcept : promise_impl<T>{handle_type::from_promise(*this)} {}

    auto get_return_object() noexcept -> task { return task{handle_type::from_promise(*this)}; }

    template<typename U, std::movable... Gs>
    auto await_transform(task<U, Gs...> task) noexcept
        -> promise_base::task_awaiter<typename corofx::task<U, Gs...>::promise_type>
        requires(detail::type_set<Es...>::template contains<detail::type_set<Gs...>>)
    {
        return promise_base::task_awaiter{task.frame_.promise()};
    }

    template<std::movable Task, typename... Hs>
    auto await_transform(handled_task<Task, Hs...> ht) noexcept
        -> promise_base::handled_task_awaiter<handled_task<Task, Hs...>>
        requires(detail::type_set<Es...>::template contains<
                 typename handled_task<Task, Hs...>::effect_types>)
    {
        return promise_base::handled_task_awaiter<handled_task<Task, Hs...>>{std::move(ht)};
    }

    template<std::movable E>
    auto await_transform(E eff) noexcept -> effect_awaiter<E>
        requires(detail::type_set<Es...>::template contains<E>)
    {
        return effect_awaiter<E>{this, std::move(eff)};
    }
};

} // namespace corofx
