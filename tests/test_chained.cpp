#include "corofx/check.hpp"
#include "corofx/task.hpp"

#include <utility>

using namespace corofx;

struct bar {
    using return_type = int;

    int x{};
};

constexpr auto marker0 = __LINE__;
constexpr auto marker1 = __LINE__;
constexpr auto marker2 = __LINE__;
constexpr auto marker3 = __LINE__;

auto do_bar() -> task<int, bar> {
    co_await bar{marker0};
    check_unreachable();
}

auto inner() -> task<int, bar> {
    std::ignore = co_await do_bar().with(handler_of<bar>([](auto&& e, auto&&) -> task<int, bar> {
        check(e.x == marker0);
        co_await bar{marker1};
        check_unreachable();
    }));
    check_unreachable();
}

auto outer() -> task<int, bar> {
    auto x = co_await inner().with(handler_of<bar>([](auto&& e, auto&&) -> task<int> {
        check(e.x == marker1);
        co_return marker2;
    }));
    check(x == marker2);
    co_return x;
}

auto main() -> int {
    auto x = outer().with(handler_of<bar>([](auto&&...) -> task<int> { check_unreachable(); }));
    check(std::move(x)() == marker2);
    auto y = inner().with(handler_of<bar>([](auto&& e, auto&&) -> task<int> {
        check(e.x == marker1);
        co_return marker3;
    }));
    check(std::move(y)() == marker3);
}
