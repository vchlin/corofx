#pragma once

#include <type_traits>

namespace corofx::detail {

template<typename T>
struct identity {
    using type = T;
};

template<typename S, typename U>
struct contains_impl {
    static constexpr bool value = false;
};

template<template<typename...> typename S, typename... Ts, typename U>
struct contains_impl<S<Ts...>, U> {
    struct set : identity<Ts>... {};

    static constexpr bool value = std::is_base_of_v<identity<U>, set>;
};

template<template<typename...> typename S, typename... Ts, typename... Us>
struct contains_impl<S<Ts...>, S<Us...>> {
    struct set : identity<Ts>... {};

    static constexpr bool value = (std::is_base_of_v<identity<Us>, set> && ...);
};

template<typename Head, typename Tail, typename... Us>
struct subtract_impl {
    using result = Tail;
};

template<typename... Ts>
struct type_set {
    template<typename... Us>
    using subtract = subtract_impl<type_set<>, type_set<Ts...>, Us...>::result;

    template<typename U>
    static constexpr bool contains = contains_impl<type_set, U>::value;
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

// TODO: Move these to a test.
static_assert(std::is_same_v<type_set<int, char>::subtract<int>, type_set<char>>);
static_assert(std::is_same_v<type_set<int, char>::subtract<char, int>, type_set<>>);
static_assert(std::is_same_v<type_set<int, char>::subtract<bool, char>, type_set<int>>);
static_assert(std::is_same_v<type_set<int, char>::subtract<char, bool>, type_set<int>>);

} // namespace corofx::detail
