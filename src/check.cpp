#include "corofx/check.hpp"

#include "corofx/trace.hpp"

#include <exception>

namespace corofx {

auto unreachable(std::string_view msg) noexcept -> void {
    trace(msg);
    std::terminate();
}

auto check(bool pred, std::source_location loc) noexcept -> void {
    if (pred) return;
    trace(loc.file_name(), ":", loc.line(), ": ", loc.function_name(), ": check failed");
    std::terminate();
}

auto check_unreachable(std::source_location loc) noexcept -> void {
    trace(
        loc.file_name(),
        ":",
        loc.line(),
        ": ",
        loc.function_name(),
        ": unreachable statement reached");
    std::terminate();
}

} // namespace corofx
