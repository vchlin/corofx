#include "corofx/task.hpp"

#include <iostream>
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
    auto divide_add = []() -> task<int, raise> {
        co_return 8 + co_await safe_divide(1, 0); // NOLINT
    };
    co_return co_await divide_add().with(handler_of<raise>([](auto&& e, auto&&) -> task<int> {
        std::cout << "error: " << e.msg << "\n";
        co_return 42; // NOLINT
    }));
}

auto main() -> int {
    auto x = raise_const()();
    std::cout << "x: " << x << "\n";
}
