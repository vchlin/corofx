#include "corofx/task.hpp"
#include "corofx/trace.hpp"

#include <string>

using namespace corofx;

// Example adapted from https://koka-lang.github.io/koka/doc/book.html#sec-handling.
struct raise {
    using return_type = int;

    std::string msg;
};

auto safe_divide(int x, int y) -> task<int, raise> {
    if (y == 0) co_await raise{"division by zero"};
    co_return x / y;
}

auto raise_const() -> task<int> {
    auto add_divide = []() -> task<int, raise> {
        co_return 8 + co_await safe_divide(1, 0); // NOLINT
    };
    co_return co_await add_divide().with(make_handler<raise>([](auto&& r, auto&&) -> task<int> {
        trace("error: ", r.msg);
        co_return 42; // NOLINT
    }));
}

auto main() -> int {
    auto x = raise_const()();
    trace("x: ", x);
}
