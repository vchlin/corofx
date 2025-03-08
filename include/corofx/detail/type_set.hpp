#pragma once

#include <type_traits>

namespace corofx::detail {

template<typename S, typename U>
struct contains_impl {
    static constexpr bool value = false;
};

template<template<typename...> typename S, typename... Ts, typename U>
struct contains_impl<S<Ts...>, U> {
    struct set : std::type_identity<Ts>... {};

    static constexpr bool value = std::is_base_of_v<std::type_identity<U>, set>;
};

template<template<typename...> typename S, typename... Ts, typename... Us>
struct contains_impl<S<Ts...>, S<Us...>> {
    struct set : std::type_identity<Ts>... {};

    static constexpr bool value = (std::is_base_of_v<std::type_identity<Us>, set> and ...);
};

template<typename S, typename S2, typename... Ss>
struct add_impl {
    using result = add_impl<typename add_impl<S, S2>::result, Ss...>::result;
};

template<typename S, typename S2, typename Remain>
struct subtract_impl {
    using result = Remain;
};

// TODO: Down with ::template and .template
template<typename... Ts>
struct type_set {
    template<typename... Ss>
    using add = add_impl<type_set, Ss...>::result;

    template<typename... Us>
    using subtract = subtract_impl<type_set, type_set<Us...>, type_set<>>::result;

    static constexpr bool empty = sizeof...(Ts) == 0;

    template<typename U>
    static constexpr bool contains = contains_impl<type_set, U>::value;

    template<typename F>
    static constexpr auto apply(F&& f) -> decltype(auto) {
        return f.template operator()<Ts...>();
    }

    template<template<typename...> typename S>
    using unpack_to = S<Ts...>;
};

template<template<typename...> typename S, typename... Ts>
struct add_impl<S<Ts...>, S<>> {
    using result = S<Ts...>;
};

template<template<typename...> typename S, typename... Ts, typename U, typename... Us>
struct add_impl<S<Ts...>, S<U, Us...>> {
    using result = std::conditional_t<
        S<Ts...>::template contains<U>,
        add_impl<S<Ts...>, S<Us...>>,
        add_impl<S<Ts..., U>, S<Us...>>>::result;
};

// TODO: Double check if template instantiation is lazy enough?
template<template<typename...> typename S, typename T, typename... Ts, typename S2, typename... Us>
struct subtract_impl<S<T, Ts...>, S2, S<Us...>> {
    using result = std::conditional_t<
        S2::template contains<T>,
        subtract_impl<S<Ts...>, S2, S<Us...>>,
        subtract_impl<S<Ts...>, S2, S<Us..., T>>>::result;
};

} // namespace corofx::detail
