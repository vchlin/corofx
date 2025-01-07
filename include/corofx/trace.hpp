#pragma once

#include <iostream>

namespace corofx {

template<typename... Args>
auto trace(Args&&... args) -> void {
    (std::cout << ... << args) << '\n';
}

} // namespace corofx
