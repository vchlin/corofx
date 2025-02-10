#pragma once

#include <concepts>

namespace corofx {

template<typename T>
concept effect = std::movable<T> and
    requires(T)
{
    typename T::return_type;
};

} // namespace corofx
