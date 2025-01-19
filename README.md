# corofx

[![Build](https://github.com/vchlin/corofx/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/vchlin/corofx/actions/workflows/build.yml?query=branch%3Amain)

Algebraic effects for C++20 using coroutines.

corofx is implemented using standard C++20 features only without external dependencies.

## Features

- Typed effect handling: Allowed effects in each scope are checked at compile time.
- Resuming (one-shot): Allows resuming back to the call site with a result.

## Examples

### Generator

An example adapted from [Koka](https://koka-lang.github.io/koka/doc/book.html#why-handlers):
```C++
#include "corofx/task.hpp"
#include "corofx/trace.hpp"

#include <vector>

using namespace corofx;

// Example adapted from https://koka-lang.github.io/koka/doc/book.html#why-handlers.
struct yield {
    using return_type = bool;

    int i{};
};

auto traverse(std::vector<int> xs) -> task<void, yield> {
    for (auto x : xs) {
        if (!co_await yield{x}) break;
    }
    co_return {};
}

auto print_elems() -> task<void> {
    co_await traverse(std::vector{1, 2, 3, 4})
        .with(make_handler<yield>([](auto&& y, auto&& resume) -> task<void> {
            trace("yielded ", y.i);
            co_return resume(y.i <= 2);
        }));
    co_return {};
}

auto main() -> int { print_elems()(); }
```

Output:
```
yielded 1
yielded 2
yielded 3
```

## Limitations

- Only one-shot handlers are supported.
- Limited type inference because a C++ coroutine must name its return type.
