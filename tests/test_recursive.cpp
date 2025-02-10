#include "corofx/check.hpp"
#include "corofx/task.hpp"

#include <utility>

using namespace corofx;

struct bar {
    using return_type = int;

    int x{};
};

constexpr auto large_value = 1'000'000;

auto do_bar() -> task<int, bar> {
    auto i = 0;
    for (; i < large_value; ++i) {
        check(co_await bar{i} == i);
    }
    co_return i;
}

auto main() -> int {
    auto i = 0;
    auto res = do_bar().with(handler_of<bar>([&i](auto&& b, auto&& resume) -> task<int> {
        check(b.x == i);
        ++i;
        co_return resume(b.x);
    }));
    check(std::move(res)() == large_value);
}
