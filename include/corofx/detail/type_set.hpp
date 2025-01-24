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

    static constexpr bool value = (std::is_base_of_v<std::type_identity<Us>, set> && ...);
};

template<typename S, typename S2, typename... SN>
struct add_impl {
    using result = add_impl<typename add_impl<S, S2>::result, SN...>::result;
};

template<typename Head, typename Tail, typename... Us>
struct subtract_impl {
    using result = Tail;
};

template<typename... Ts>
struct type_set {
    template<typename... Us>
    using add = add_impl<type_set, Us...>::result;

    template<typename... Us>
    using subtract = subtract_impl<type_set<>, type_set<Ts...>, Us...>::result;

    static constexpr bool empty = sizeof...(Ts) == 0;

    template<typename U>
    static constexpr bool contains = contains_impl<type_set, U>::value;
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
template<
    template<typename...>
    typename S,
    typename... Head,
    typename T,
    typename... Ts,
    typename U,
    typename... Us>
struct subtract_impl<S<Head...>, S<T, Ts...>, U, Us...> {
    using result = std::conditional_t<
        std::is_same_v<T, U>,
        subtract_impl<type_set<>, S<Head..., Ts...>, Us...>,
        subtract_impl<S<Head..., T>, S<Ts...>, U, Us...>>::result;
};

template<template<typename...> typename S, typename... Head, typename U, typename... Us>
struct subtract_impl<S<Head...>, S<>, U, Us...> {
    using result = subtract_impl<S<>, S<Head...>, Us...>::result;
};

} // namespace corofx::detail
