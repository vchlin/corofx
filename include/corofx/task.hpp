#pragma once

#include "detail/type_set.hpp"
#include "handler.hpp"

#include <concepts>
#include <coroutine>
#include <exception>
#include <optional>
#include <utility>

namespace corofx {

// Base promise type.
class promise_base {
public:
    struct final_awaiter : std::suspend_always {
        template<std::derived_from<promise_base> U>
        auto await_suspend(std::coroutine_handle<U> frame) noexcept -> std::coroutine_handle<> {
            if (auto* k = frame.promise().cont_) return k->frame_;
            return std::noop_coroutine();
        }
    };

    template<typename P>
    class task_awaiter;
    template<typename HT>
    class handled_task_awaiter;
    template<typename E>
    class effect_awaiter;

    promise_base(promise_base const&) = delete;
    promise_base(promise_base&&) = delete;
    auto operator=(promise_base const&) -> promise_base& = delete;
    auto operator=(promise_base&&) -> promise_base& = delete;

    [[nodiscard]]
    constexpr auto initial_suspend() const noexcept -> std::suspend_always {
        return {};
    }

    [[noreturn]] auto unhandled_exception() noexcept -> void { std::terminate(); }
    [[nodiscard]] auto final_suspend() const noexcept -> final_awaiter { return {}; }

    auto unhandled_effect() -> void {
        if (auto r = unhandled_) std::rethrow_exception(r);
    }

protected:
    promise_base(std::coroutine_handle<> frame) noexcept : frame_{frame} {}
    ~promise_base() = default;

private:
    auto set_cont(promise_base& cont) noexcept -> std::coroutine_handle<> {
        cont_ = &cont;
        return frame_;
    }

    std::coroutine_handle<> const frame_;
    promise_base* cont_{};
    std::exception_ptr unhandled_;
    handler_list* handlers_{};
};

template<typename P>
class promise_base::task_awaiter : public std::suspend_always {
public:
    explicit task_awaiter(P& p) noexcept : p_{p} {}
    task_awaiter(task_awaiter const&) = delete;
    task_awaiter(task_awaiter&&) = delete;
    ~task_awaiter() = default;
    auto operator=(task_awaiter const&) -> task_awaiter& = delete;
    auto operator=(task_awaiter&&) -> task_awaiter& = delete;

    template<std::derived_from<promise_base> U>
    auto await_suspend(std::coroutine_handle<U> frame) noexcept -> std::coroutine_handle<> {
        return p_.set_cont(frame.promise());
    }

    auto await_resume() noexcept -> P::value_type { return p_.take_value(); }

private:
    P& p_;
};

template<typename Task, typename... Hs>
class handled_task {
    // TODO: Remove friends?
    template<typename U, std::movable... Gs>
    friend class task;
    friend promise_base;

public:
    using task = Task;
    using effects = Task::effects::template subtract<typename Hs::effect...>;

    handled_task(Task task, Hs... handlers) noexcept
        : task_{std::move(task)}, handlers_{std::move(handlers)...} {}

private:
    Task task_;
    handler_list_impl<Hs...> handlers_;
};

template<typename HT>
class promise_base::handled_task_awaiter : public std::suspend_always {
public:
    explicit handled_task_awaiter(HT ht) noexcept : ht_{std::move(ht)} {
        ht_.task_.frame_.promise().handlers_ = &ht_.handlers_;
    }

    template<std::derived_from<promise_base> U>
    auto await_suspend(std::coroutine_handle<U> frame) noexcept -> std::coroutine_handle<> {
        return ht_.task_.frame_.promise().set_cont(frame.promise());
    }

    auto await_resume() noexcept -> HT::task::promise_type::value_type {
        return ht_.task_.frame_.promise().take_value();
    }

private:
    HT ht_;
};

template<typename E>
class promise_base::effect_awaiter : public std::suspend_always {
public:
    explicit effect_awaiter(E eff) noexcept : eff_{std::move(eff)} {}

    template<std::derived_from<promise_base> U>
    auto await_suspend(std::coroutine_handle<U> frame) noexcept -> std::coroutine_handle<> {
        auto* p = (promise_base*){&frame.promise()};
        for (auto* k = p; k; p = k, k = k->cont_)
            if (k->handlers_ && k->handlers_->handle(eff_)) return frame;
        // TODO: WIP
        try {
            throw std::move(eff_);
        } catch (...) {
            p->unhandled_ = std::current_exception();
        }
        return std::noop_coroutine();
    }

private:
    E eff_;
};

template<typename T = void>
class promise_for;

template<>
class promise_for<> : public promise_base {
public:
    using promise_base::promise_base;

    constexpr auto return_void() const noexcept -> void {}

    constexpr auto take_value() const noexcept -> void {}
};

template<typename T>
class promise_for : public promise_base {
public:
    using promise_base::promise_base;

    auto return_value(T value) noexcept -> void { value_ = std::move(value); }

    auto take_value() noexcept -> T { return std::move(*value_); }

private:
    std::optional<T> value_;
};

// Represents a unit of computation that is potentially effectful.
template<typename T, std::movable... Es>
// TODO: Check if `Es...` are unique.
class task {
    template<typename U, std::movable... Gs>
    friend class task;
    // TODO: Remove friend?
    friend promise_base;

public:
    class promise_type;
    using handle_type = std::coroutine_handle<promise_type>;
    using effects = detail::type_set<Es...>;

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

    auto operator()() -> T {
        frame_.resume();
        auto& p = frame_.promise();
        p.unhandled_effect();
        return p.take_value();
    }

    // Runs the task with the provided handlers when awaited.
    template<typename... Hs>
    auto with(Hs... handlers) && -> handled_task<task, Hs...>
        requires(
            detail::type_set<Es...>::template contains<detail::type_set<typename Hs::effect...>>)
    {
        return handled_task{std::move(*this), std::move(handlers)...};
    }

private:
    explicit task(handle_type h) noexcept : frame_{h} {}

    auto swap(task& that) noexcept -> void {
        using std::swap;
        swap(frame_, that.frame_);
    }

    handle_type frame_{};
};

template<typename T, std::movable... Es>
class task<T, Es...>::promise_type : public promise_for<T> {
public:
    template<typename E>
    using effect_awaiter = promise_base::template effect_awaiter<E>;
    using value_type = T;

    promise_type() noexcept : promise_for<T>{handle_type::from_promise(*this)} {}

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
        requires(
            detail::type_set<Es...>::template contains<typename handled_task<Task, Hs...>::effects>)
    {
        return promise_base::handled_task_awaiter<handled_task<Task, Hs...>>{std::move(ht)};
    }

    template<std::movable E>
    auto await_transform(E eff) noexcept -> effect_awaiter<E>
        requires(detail::type_set<Es...>::template contains<E>)
    {
        return effect_awaiter<E>{std::move(eff)};
    }
};

} // namespace corofx
