#include "corofx/task.hpp"

#include <iostream>
#include <vector>

using namespace corofx;

// Example adapted from https://koka-lang.github.io/koka/doc/book.html#why-handlers.
struct yield {
    using return_type = bool;

    int i{};
};

auto traverse(std::vector<int> xs) -> task<void, yield> {
    for (auto x : xs) {
        if (not co_await yield{x}) break;
    }
    co_return {};
}

auto print_elems() -> task<void> {
    co_await traverse(std::vector{1, 2, 3, 4})
        .with(make_handler<yield>([](auto&& y, auto&& resume) -> task<void> {
            std::cout << "yielded " << y.i << std::endl;
            co_return resume(y.i <= 2);
        }));
    co_return {};
}

auto main() -> int { print_elems()(); }
