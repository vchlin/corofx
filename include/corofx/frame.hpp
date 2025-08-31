#pragma once

#include <coroutine>
#include <type_traits>
#include <utility>

namespace corofx {

template<typename P = void>
class frame {
public:
    explicit frame(std::coroutine_handle<P> data) noexcept : data_{data} {}

    template<typename Q>
    frame(frame<Q> that) noexcept
        requires std::is_void_v<P>
        : data_{std::exchange(that.data_, {})} {}

    frame(frame const&) = delete;
    frame(frame&& that) noexcept : data_{std::exchange(that.data_, {})} {}

    ~frame() {
        if (data_) data_.destroy();
    }

    auto operator=(frame const&) -> frame& = delete;

    auto operator=(frame&& that) noexcept -> frame& {
        auto left = std::move(that);
        swap(left, *this);
        return *this;
    }

    auto operator*() const noexcept -> std::coroutine_handle<P> { return data_; }

    auto operator->() const noexcept
        -> std::coroutine_handle<P> const* requires(not std::is_void_v<P>) { return &data_; }

    friend auto swap(frame& left, frame& right) noexcept -> void {
        using std::swap;
        swap(left.data_, right.data_);
    }

private:
    template<typename>
    friend class frame;

    std::coroutine_handle<P> data_;
};

} // namespace corofx
