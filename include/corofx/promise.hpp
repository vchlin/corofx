#pragma once

#include "corofx/check.hpp"
#include "corofx/effect.hpp"

#include <coroutine>
#include <optional>
#include <type_traits>
#include <utility>
#include <variant> // TODO: Remove for C++26.

namespace corofx {

class promise_base;

template<typename T = void>
class promise_impl;

class resumer_tag {
public:
    resumer_tag(resumer_tag const&) = delete;
    resumer_tag(resumer_tag&&) = delete;
    ~resumer_tag() = default;
    auto operator=(resumer_tag const&) -> resumer_tag& = delete;
    auto operator=(resumer_tag&&) -> resumer_tag& = delete;

private:
    template<effect E>
    friend class resumer;
    friend promise_base;

    explicit resumer_tag(std::coroutine_handle<> resume) noexcept : resume_{resume} {}

    std::coroutine_handle<> resume_;
};

template<effect E>
class effect_awaiter;

template<effect E>
class resumer {
public:
    resumer(resumer const&) = delete;
    resumer(resumer&&) = delete;
    ~resumer() = default;
    auto operator=(resumer const&) -> resumer& = delete;
    auto operator=(resumer&&) -> resumer& = delete;

    // TODO: Handle void return_type.
    [[nodiscard]]
    auto operator()(E::return_type value) noexcept -> resumer_tag {
        effect_.set_value(std::move(value));
        return resumer_tag{resume_};
    }

private:
    friend class effect_awaiter<E>;

    explicit resumer(std::coroutine_handle<> resume, effect_awaiter<E>& effect) noexcept
        : resume_{resume}, effect_{effect} {}

    std::coroutine_handle<> resume_;
    effect_awaiter<E>& effect_;
};

class untyped_task {
public:
    explicit untyped_task(std::coroutine_handle<> frame) noexcept : frame_{frame} {}
    untyped_task(untyped_task const&) = delete;
    untyped_task(untyped_task&& that) noexcept : frame_{std::exchange(that.frame_, {})} {}

    ~untyped_task() {
        if (frame_) frame_.destroy();
    }

    auto operator=(untyped_task const&) -> untyped_task& = delete;

    auto operator=(untyped_task&& that) noexcept -> untyped_task& {
        auto left = std::move(that);
        swap(left, *this);
        return *this;
    }

    friend auto swap(untyped_task& left, untyped_task& right) noexcept -> void {
        using std::swap;
        swap(left.frame_, right.frame_);
    }

private:
    template<effect E>
    friend class effect_awaiter;

    [[nodiscard]]
    auto get_frame() const noexcept -> std::coroutine_handle<> {
        return frame_;
    }

    std::coroutine_handle<> frame_;
};

// Base promise type.
class promise_base {
public:
    struct final_awaiter : std::suspend_always {
        template<std::derived_from<promise_base> U>
        [[nodiscard]]
        auto await_suspend(std::coroutine_handle<U> frame) noexcept -> std::coroutine_handle<> {
            if (auto k = frame.promise().cont_) return k;
            return std::noop_coroutine();
        }
    };

    promise_base(promise_base const&) = delete;
    promise_base(promise_base&&) = delete;
    auto operator=(promise_base const&) -> promise_base& = delete;
    auto operator=(promise_base&&) -> promise_base& = delete;

    [[nodiscard]]
    constexpr auto initial_suspend() const noexcept -> std::suspend_always {
        return {};
    }

    auto return_value(resumer_tag const& resume) noexcept -> void { set_cont(resume.resume_); }

    [[noreturn]]
    auto unhandled_exception() noexcept -> void {
        unreachable("unhandled exception");
    }

    [[nodiscard]]
    auto final_suspend() const noexcept -> final_awaiter {
        return {};
    }

    auto set_cont(std::coroutine_handle<> cont) noexcept -> void { cont_ = cont; }

protected:
    promise_base() noexcept = default;
    ~promise_base() = default;

private:
    std::coroutine_handle<> cont_;
};

template<>
class promise_impl<> : public promise_base {
public:
    using promise_base::return_value;

    constexpr auto return_value(std::monostate) noexcept -> void {}
};

template<typename T>
class promise_impl : public promise_base {
public:
    using promise_base::return_value;

    auto return_value(T value) noexcept -> void { *output_ = std::move(value); }

    auto set_output(std::optional<T>& output) noexcept -> void { output_ = &output; }

private:
    std::optional<T>* output_{};
};

template<typename T>
using value_holder = std::conditional_t<std::is_void_v<T>, std::monostate, std::optional<T>>;

template<typename Task>
class task_awaiter : public std::suspend_always {
public:
    using value_type = Task::value_type;

    explicit task_awaiter(Task t) noexcept : task_{std::move(t)} {
        if constexpr (not std::is_void_v<value_type>) task_.set_output(value_);
    }

    task_awaiter(task_awaiter const&) = delete;
    task_awaiter(task_awaiter&&) = delete;
    ~task_awaiter() = default;
    auto operator=(task_awaiter const&) -> task_awaiter& = delete;
    auto operator=(task_awaiter&&) -> task_awaiter& = delete;

    template<std::derived_from<promise_base> U>
    [[nodiscard]]
    auto await_suspend(std::coroutine_handle<U> frame) noexcept -> std::coroutine_handle<> {
        auto h = task_.get_frame();
        h.promise().set_cont(frame);
        return h;
    }

    auto await_resume() noexcept -> value_type {
        if constexpr (not std::is_void_v<value_type>) return std::move(*value_);
    }

private:
    Task task_;
    value_holder<value_type> value_;
};

template<effect E>
class handler {
public:
    [[nodiscard]]
    virtual auto handle(E&& eff, resumer<E>& resume) noexcept -> untyped_task = 0;
};

template<effect E>
class effect_awaiter : public std::suspend_always {
public:
    using value_type = E::return_type;

    explicit effect_awaiter(handler<E>* h, std::coroutine_handle<> k, E eff) noexcept
        : handler_{h},
          eff_{std::move(eff)},
          resumer_{k, *this},
          task_{h->handle(std::move(eff_), resumer_)} {}

    template<std::derived_from<promise_base> U>
    [[nodiscard]]
    auto await_suspend(std::coroutine_handle<U>) noexcept -> std::coroutine_handle<> {
        return task_.get_frame();
    }

    auto await_resume() noexcept -> value_type { return std::move(*value_); }

    auto set_value(value_type value) noexcept -> void { value_ = std::move(value); }

private:
    handler<E>* handler_{};
    E eff_; // NOTE: This effect will not be moved until the task starts running.
    resumer<E> resumer_;
    untyped_task task_;
    std::optional<value_type> value_;
};

template<effect E>
class evidence {
protected:
    auto set_handler(handler<E>* h) noexcept -> void { handler_ = h; }

    [[nodiscard]]
    auto get_handler() const noexcept -> handler<E>* {
        return handler_;
    }

private:
    handler<E>* handler_{};
};

template<effect... Es>
class evidence_vec : public evidence<Es>... {
public:
    using evidence<Es>::set_handler...;

    template<effect E>
    [[nodiscard]]
    auto get_handler() const noexcept -> handler<E>* {
        return evidence<E>::get_handler();
    }
};

// An effect handler entry.
template<effect E, typename F>
class handler_impl : public handler<E> {
public:
    using effect_type = E;
    using task_type = std::invoke_result_t<F, E&&, resumer<E>&>;
    using value_type = task_type::value_type;

    handler_impl(F fn) noexcept : handler<E>{}, fn_{std::move(fn)} {}

    [[nodiscard]]
    auto handle(E&& eff, resumer<E>& resume) noexcept -> untyped_task final {
        auto task = fn_(std::move(eff), resume);
        auto frame = task.release();
        auto& p = frame.promise();
        p.set_cont(cont_);
        if constexpr (not std::is_void_v<value_type>) p.set_output(*output_);
        task_type::effect_types::apply(
            [&]<effect... Es>() { (p.set_handler(ev_vec_.template get_handler<Es>()), ...); });
        return untyped_task{frame};
    }

    template<typename Task>
    auto copy_handlers(Task& t) noexcept -> void {
        if constexpr (not task_type::effect_types::empty) {
            task_type::effect_types::apply(
                [&]<effect... Es>() { (ev_vec_.set_handler(t.template get_handler<Es>()), ...); });
        }
    }

    auto set_cont(std::coroutine_handle<> cont) noexcept -> void { cont_ = cont; }

    auto set_output(std::optional<value_type>& output) noexcept -> void
        requires(not std::is_void_v<value_type>)
    {
        output_ = &output;
    }

private:
    F fn_;
    std::coroutine_handle<> cont_;
    value_holder<value_type>* output_{};
    task_type::effect_types::template unpack_to<evidence_vec> ev_vec_;
};

// Creates an effect handler entry.
template<effect E, typename F>
[[nodiscard]]
auto handler_of(F fn) noexcept -> handler_impl<E, F> {
    return handler_impl<E, F>{std::move(fn)};
}

} // namespace corofx
