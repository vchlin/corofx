#pragma once

#include "check.hpp"
#include "effect.hpp"

#include <concepts>
#include <coroutine>
#include <optional>
#include <utility>
#include <variant> // TODO: Remove for C++26.

namespace corofx {

// Base promise type.
class promise_base {
public:
    struct final_awaiter : std::suspend_always {
        template<std::derived_from<promise_base> U>
        [[nodiscard]]
        auto await_suspend(std::coroutine_handle<U> frame) const noexcept
            -> std::coroutine_handle<> {
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

template<typename T = void>
class promise_impl : public promise_base {
public:
    using promise_base::return_value;

    auto return_value(T value) const noexcept -> void { *output_ = std::move(value); }

    auto set_output(std::optional<T>& output) noexcept -> void { output_ = &output; }

private:
    std::optional<T>* output_{};
};

template<>
class promise_impl<> : public promise_base {
public:
    using promise_base::return_value;

    constexpr auto return_value(std::monostate) const noexcept -> void {}
};

} // namespace corofx
