#pragma once

#include <coroutine>
#include <utility>

namespace corofx {

template<typename P = void>
class frame {
public:
    explicit frame(std::coroutine_handle<P> data) noexcept : data_{data} {}
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
    auto operator->() const noexcept -> std::coroutine_handle<P> const* { return &data_; }

    friend auto swap(frame& left, frame& right) noexcept -> void {
        using std::swap;
        swap(left.data_, right.data_);
    }

private:
    friend class frame<>;

    std::coroutine_handle<P> data_;
};

template<>
class frame<> {
public:
    template<typename P>
    frame(frame<P> that) noexcept : data_{std::exchange(that.data_, {})} {}
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

    auto operator*() const noexcept -> std::coroutine_handle<> { return data_; }

    friend auto swap(frame& left, frame& right) noexcept -> void {
        using std::swap;
        swap(left.data_, right.data_);
    }

private:
    std::coroutine_handle<> data_;
};

} // namespace corofx
