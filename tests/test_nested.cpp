#include "corofx/check.hpp"
#include "corofx/task.hpp"

using namespace corofx;

struct bar {
    using return_type = int;

    int x{};
};

constexpr auto marker0 = __LINE__;
constexpr auto marker1 = __LINE__;

auto do_bar() -> task<int, bar> {
    co_await bar{marker0};
    check_unreachable();
}

auto inner() -> task<int, bar> {
    auto x = co_await do_bar().with(make_handler<bar>([](auto&& e, auto&&) -> task<int> {
        check(e.x == marker0);
        co_return marker1;
    }));
    check(x == marker1);
    co_return x;
}

auto outer() -> task<int, bar> {
    auto x = co_await inner().with(
        make_handler<bar>([](auto&&...) -> task<int> { check_unreachable(); }));
    check(x == marker1);
    co_return x;
}

auto main() -> int {
    auto x = inner().with(make_handler<bar>([](auto&&...) -> task<int> { check_unreachable(); }));
    check(std::move(x)() == marker1);
    auto y = outer().with(make_handler<bar>([](auto&&...) -> task<int> { check_unreachable(); }));
    check(std::move(y)() == marker1);
}
