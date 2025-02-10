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

auto main() -> int {
    auto x = do_bar().with(make_handler<bar>([](auto&& e, auto&&) -> task<int> {
        check(e.x == marker0);
        co_return marker1;
    }));
    auto y = std::move(x);
    check(std::move(y)() == marker1);
}
