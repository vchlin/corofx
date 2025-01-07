#include "corofx/check.hpp"
#include "corofx/task.hpp"

using namespace corofx;

struct bar {
    using return_type = int;

    int x{};
};

constexpr auto marker0 = __LINE__;
constexpr auto marker1 = __LINE__;
constexpr auto marker2 = __LINE__;

auto do_bar() -> task<int, bar> {
    co_await bar{marker0};
    check_unreachable();
}

auto inner() -> task<int, bar> {
    std::ignore = co_await do_bar().with(make_handler<bar>([](auto&& b, auto&&) -> task<int, bar> {
        check(b.x == marker0);
        co_await bar{marker1};
        check_unreachable();
    }));
    check_unreachable();
}

auto outer() -> task<int, bar> {
    auto x = co_await inner().with(make_handler<bar>([](auto&& b, auto&&) -> task<int> {
        check(b.x == marker1);
        co_return marker2;
    }));
    check(x == marker2);
    co_return x;
}

auto main() -> int {
    auto res = outer().with(make_handler<bar>([](auto&&...) -> task<int> { check_unreachable(); }));
    check(res() == marker2);
}
