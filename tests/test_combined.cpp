#include "corofx/check.hpp"
#include "corofx/task.hpp"

#include <utility>

using namespace corofx;

struct foo {
    using return_type = int;

    char x{};
};

struct bar {
    using return_type = int;

    int x{};
};

constexpr auto marker0 = __LINE__;
constexpr auto marker1 = __LINE__;
constexpr auto marker2 = __LINE__;

auto do_something() -> task<int, foo, bar> {
    check(co_await bar{marker0} == marker1);
    check(co_await foo{marker1} == marker0);
    co_return marker2;
}

auto inner() -> task<int, bar, foo> {
    auto x = co_await do_something().with(
        handler_of<bar>([](auto&& e, auto&& resume) -> task<int, foo, bar> {
            check(e.x == marker0);
            check(co_await foo{marker0} == marker1);
            check(co_await bar{marker1} == marker0);
            co_return resume(marker1);
        }));
    co_return x;
}

auto main() -> int {
    auto res = inner().with(
        handler_of<foo>([](auto&& e, auto&& resume) -> task<int> {
            switch (e.x) {
            case marker0:
                co_return resume(marker1);
            case marker1:
                co_return resume(marker0);
            }
            check_unreachable();
        }),
        handler_of<bar>([](auto&& e, auto&& resume) -> task<int> {
            check(e.x == marker1);
            co_return resume(marker0);
        }));
    check(std::move(res)() == marker2);
}
