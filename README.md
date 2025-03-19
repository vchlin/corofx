# corofx

[![CI](https://github.com/vchlin/corofx/actions/workflows/ci.yml/badge.svg?branch=main)](https://github.com/vchlin/corofx/actions/workflows/ci.yml?query=branch%3Amain)

Typed effect handlers for C++20 using coroutines.

## Overview

corofx is a library for programming with effects and handlers.
It uses standard C++20 features only, without external dependencies.
The effect semantics are largely inspired
by the [Koka](https://github.com/koka-lang/koka) language.

### Why Effects?

A useful computer program often needs to perform side effects like I/O operations.
However, unrestricted side effects make program behavior difficult to reason about.
Effect typing solves this by statically tracking which effects a function can perform,
creating a clear separation between pure and effectful computations.

While this concept resembles checked exception specifications,
effect handlers make it particularly powerful.
Effect handling generalizes exception handling by allowing handlers to resume execution
at the call site with a result.
This provides some key benefits:
- Similar to dependency injection,
  effects and their handling logic can be decoupled,
  improving program modularity and composability.
- Interesting control flow patterns like generators can be implemented
  without language extensions.

### Why Coroutines?

C++ [coroutines](https://en.cppreference.com/w/cpp/language/coroutines)
provide an ideal foundation for building an effect system.
Their extensive customization points enable precise control over execution flow.
By leveraging coroutines as a native language feature,
we can express complex control patterns using standard C++ syntax.

> [!NOTE]
> C++ coroutines are state machines that can be resumed only once.
> As a result, this library supports only one-shot effect handlers.

## A Motivating Example

Here is a [generator example](examples/yield.cpp)
adapted from [Koka](https://koka-lang.github.io/koka/doc/book.html#why-handlers):
```C++
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
```

The key components in this example are:
- Effect Definition:
  The `yield` effect is a simple `struct` with an `int` payload and a `bool` return type.
- Effect Producer:
  `traverse` returns `task<void, yield>`,
  indicating it's an effectful computation that:
    - May produce the `yield` effect.
    - Returns `void` when complete.
- Effect Handler:
  `print_elems` discharges the `yield` effect from `traverse` with a handler, which:
    - Produces no effects[^1].
    - Prints each yielded value.
    - Passes a `bool` and transfers control back to the effect producer via tail resumption.

After handling `yield`,
`print_elems` returns `task<void>` - a pure computation returning `void`.
Finally, `main` simply calls the `print_elems` coroutine
since no effects remain to be handled.

[^1]: This example still performs I/O by printing to `stdout`.
While we could define a `console` effect to track and handle such operations,
C++ doesn't prevent direct I/O operations outside our effect system.

When run, this program produces the following output:
```
yielded 1
yielded 2
yielded 3
```

> [!TIP]
> Effect types are checked at compile time,
> ensuring proper handling throughout the call chain.

> [!TIP]
> The library uses
> [symmetric transfer](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0913r0.html)
> to handle repeated effect invocations without stack overflow.

See [examples](examples) for more interesting use cases of effects and handlers.

## Getting Started

### Requirements

A compiler with C++20 support is required.
While older compiler versions may work,
the following versions are recommended for best results:

| Compiler | Version |
| -------- | ------- |
| GCC      | 13.3.0  |
| Clang    | 18.1.3  |
| MSVC     | 19.42   |

### Building with CMake

To build and use this library in your project, use CMake with `FetchContent`:

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
