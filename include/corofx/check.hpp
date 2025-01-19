#pragma once

#include <source_location>
#include <string_view>

namespace corofx {

[[noreturn]]
auto unreachable(std::string_view msg = "") noexcept -> void;

auto check(bool pred, std::source_location loc = std::source_location::current()) noexcept -> void;

[[noreturn]]
auto check_unreachable(std::source_location loc = std::source_location::current()) noexcept -> void;

} // namespace corofx
