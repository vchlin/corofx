#pragma once

#include <concepts>

namespace corofx {

template<typename E>
concept is_void_effect = std::same_as<typename E::return_type, void>;

template<typename T>
struct resume {};

// TODO: Resume awaiter.

} // namespace corofx
