# corofx

[![Build](https://github.com/vchlin/corofx/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/vchlin/corofx/actions/workflows/build.yml?query=branch%3Amain)

Algebraic effects for C++20 using coroutines.

corofx is implemented using standard C++20 features only without external dependencies.

## Features

- Typed effect handling: Allowed effects in each scope are checked at compile time.
- Resuming (one-shot): Allows resuming back to the call site with a result.

## An Example

A generator example adapted from [Koka](https://koka-lang.github.io/koka/doc/book.html#why-handlers):
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

## Requirements

A compiler with C++20 support is required. While older versions may work, the following compiler versions or later are recommended for best results:

| Compiler | Version |
| -------- | ------- |
| GCC      | 13.3.0  |
| Clang    | 18.1.3  |

## Usage

### FetchContent
```CMake
cmake_minimum_required(VERSION 3.28)

project(MyProj
    LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    corofx
    GIT_REPOSITORY https://github.com/vchlin/corofx.git
    GIT_TAG main
    GIT_SHALLOW ON
    FIND_PACKAGE_ARGS NAMES CoroFX
)
FetchContent_MakeAvailable(corofx)

add_executable(MyExe main.cpp)
target_link_libraries(MyExe CoroFX::CoroFX)
```

## Limitations

- Only one-shot handlers are supported.
