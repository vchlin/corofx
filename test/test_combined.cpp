#include "corofx/check.hpp"
#include "corofx/task.hpp"

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
        make_handler<bar>([](auto&& b, auto&& resume) -> task<int, foo, bar> {
            check(b.x == marker0);
            check(co_await foo{marker0} == marker1);
            check(co_await bar{marker1} == marker0);
            co_return resume(marker1);
        }));
    co_return x;
}

auto main() -> int {
    auto res = inner().with(
        make_handler<foo>([](auto&& f, auto&& resume) -> task<int> {
            switch (f.x) {
            case marker0:
                co_return resume(marker1);
            case marker1:
                co_return resume(marker0);
            }
            check_unreachable();
        }),
        make_handler<bar>([](auto&& b, auto&& resume) -> task<int> {
            check(b.x == marker1);
            co_return resume(marker0);
        }));
    check(res() == marker2);
}
