#pragma once

namespace corofx {

using type_tag = char;

template<typename T>
struct type_id_impl {
    static constexpr type_tag tag = 0;
};

using type_id = type_tag const*;

// TODO: Don't use if RTTI is already enabled?
template<typename T>
constexpr type_id type_id_for = &type_id_impl<T>::tag;

} // namespace corofx
