#pragma once

#include "frame.hpp"

#include <concepts>
#include <coroutine>
#include <optional>

namespace corofx {

// clang-format off
template<typename T>
concept effect = std::movable<T> and
    requires(T)
{
    typename T::return_type;
};
// clang-format on

template<effect E>
class resumer;

template<effect E>
class handler {
public:
    [[nodiscard]]
    virtual auto handle(E&& eff, resumer<E>& resume) noexcept -> frame<> = 0;
};

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
    friend class promise_base;

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

template<effect E>
class effect_awaiter : public std::suspend_always {
public:
    using value_type = E::return_type;

    explicit effect_awaiter(handler<E>* h, std::coroutine_handle<> k, E eff) noexcept
        : eff_{std::move(eff)}, resumer_{k, *this}, frame_{h->handle(std::move(eff_), resumer_)} {}

    effect_awaiter(effect_awaiter const&) = delete;
    effect_awaiter(effect_awaiter&&) = delete;
    ~effect_awaiter() = default;
    auto operator=(effect_awaiter const&) -> effect_awaiter& = delete;
    auto operator=(effect_awaiter&&) -> effect_awaiter& = delete;

    [[nodiscard]]
    auto await_suspend(std::coroutine_handle<>) const noexcept -> std::coroutine_handle<> {
        return *frame_;
    }

    auto await_resume() noexcept -> value_type { return std::move(*value_); }

    auto set_value(value_type value) noexcept -> void { value_ = std::move(value); }

private:
    E eff_; // NOTE: This effect will not be moved until the task starts running.
    resumer<E> resumer_;
    frame<> frame_;
    std::optional<value_type> value_;
};

} // namespace corofx
