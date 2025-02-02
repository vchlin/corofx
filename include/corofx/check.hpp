#pragma once

#include <source_location>
#include <string_view>

#include "corofx/config.hpp"

namespace corofx {

[[noreturn]]
COROFX_PUBLIC auto unreachable(std::string_view msg = "") noexcept -> void;

COROFX_PUBLIC auto check(
    bool pred,
    std::source_location loc = std::source_location::current()) noexcept -> void;

[[noreturn]]
COROFX_PUBLIC auto check_unreachable(
    std::source_location loc = std::source_location::current()) noexcept -> void;

} // namespace corofx
