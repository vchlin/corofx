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
        .with(handler_of<yield>([](auto&& e, auto&& resume) -> task<void> {
            std::cout << "yielded " << e.i << "\n";
            co_return resume(e.i <= 2);
        }));
    co_return {};
}

auto main() -> int { print_elems()(); }
