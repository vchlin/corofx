#pragma once

#include "corofx/detail/type_id.hpp"

#include <tuple>

namespace corofx {

// An effect handler entry.
template<typename E, typename F>
class handler {
public:
    using effect = E;

    handler(F fn) noexcept : fn_{std::move(fn)} {}

    auto handle(type_id id, void const* eff) -> bool {
        if (id != type_id_for<E>) return false;
        fn_(*static_cast<E const*>(eff));
        return true;
    }

private:
    F fn_;
};

// Creates an effect handler entry.
template<typename E, typename F>
auto make_handler(F fn) noexcept -> handler<E, F> {
    return handler<E, F>{std::move(fn)};
}

// A list of effect handlers.
class handler_list {
public:
    handler_list(handler_list const&) = default;
    handler_list(handler_list&&) = default;
    auto operator=(handler_list const&) -> handler_list& = default;
    auto operator=(handler_list&&) -> handler_list& = default;

    template<typename E>
    auto handle(E const& eff) -> bool {
        return handle_untyped(type_id_for<E>, &eff);
    }

protected:
    handler_list() noexcept = default;
    ~handler_list() = default;

private:
    virtual auto handle_untyped(type_id id, void const* eff) -> bool = 0;
};

template<typename... Hs>
class handler_list_impl : public handler_list {
public:
    handler_list_impl(Hs... handlers) noexcept
        : handlers_{std::make_tuple(std::move(handlers)...)} {}

private:
    auto handle_untyped(type_id id, void const* eff) -> bool override {
        return std::apply([=](auto&... hs) { return (hs.handle(id, eff) || ...); }, handlers_);
    }

    std::tuple<Hs...> handlers_{};
};

} // namespace corofx
