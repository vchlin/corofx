#include "corofx/check.hpp"
#include "corofx/task.hpp"

#include <utility>

#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wself-move"
#endif

using namespace corofx;

struct bar {
    using return_type = int;

    int x{};
};

constexpr auto marker0 = __LINE__;
constexpr auto marker1 = __LINE__;
constexpr auto marker2 = __LINE__;

auto do_foo() -> task<int> { co_return marker0; }

auto do_bar() -> task<int, bar> {
    co_await bar{marker1};
    check_unreachable();
}

auto main() -> int {
    auto x = do_foo();
    auto x2 = std::move(x);
    check(std::move(x2)() == marker0);
    x = do_foo();
    check(std::move(x)() == marker0);
    auto x3 = do_foo();
    x3 = std::move(x3);
    check(std::move(x3)() == marker0);
    auto y = do_bar().with(handler_of<bar>([](auto&& e, auto&&) -> task<int> {
        check(e.x == marker1);
        co_return marker2;
    }));
    auto y2 = std::move(y);
    check(std::move(y2)() == marker2);
    auto y3 = do_bar().with(handler_of<bar>([](auto&& e, auto&&) -> task<int> {
        check(e.x == marker1);
        co_return marker2;
    }));
    y3 = std::move(y3);
    check(std::move(y3)() == marker2);
}
