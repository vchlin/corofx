# corofx

[![Build](https://github.com/vchlin/corofx/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/vchlin/corofx/actions/workflows/build.yml?query=branch%3Amain)

Typed effect handlers for C++20 using coroutines.

## Overview

corofx is a library for programming with effects and handlers. It uses standard C++20 features only, without external dependencies. The effect semantics are largely inspired by the [Koka](https://github.com/koka-lang/koka) language.

### Why Effects?

A useful computer program often needs to perform side effects like I/O operations. However, unrestricted side effects make program behavior difficult to reason about. Effect typing helps solve this by statically tracking which effects a function can perform, creating a clear separation between pure and effectful computations.

While this concept resembles checked exception specifications, effect handlers make it particularly powerful. Effect handling generalizes exception handling by allowing handlers to resume execution at the call site with a result. This provides some key benefits:

- Similar to dependency injection, effects and their handling logic can be decoupled, improving program modularity and composability.
- Interesting control flow patterns like generators can be implemented without requiring special language features.

### Why Coroutines?

[Coroutines](https://en.cppreference.com/w/cpp/language/coroutines) in C++ provide many customization points, allowing precise control over the behavior of the generated coroutine state machines. This makes them suitable as building blocks for an effect system. Since coroutines are a language feature, we can express interesting control flow structures using lightweight, standard C++ syntax.

> [!NOTE]
> Coroutines in C++ are state machines. They can be resumed only once, so only one-shot effect handlers are supported.

## Effect Handling

Here is a [generator example](examples/yield.cpp) adapted from [Koka](https://koka-lang.github.io/koka/doc/book.html#why-handlers):
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
        if (!co_await yield{x}) break;
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
```

The `yield` effect is defined as a simple `struct` with an associated `return_type` of `bool`.

`traverse` is a coroutine function that returns `task<void, yield>`. This describes an effectful computation that returns `void` on completion and may produce the `yield` effect.

`print_elems` provides a handler for `yield` when calling `traverse`. The handler transfers control back to the call site in `traverse` by performing a tail resumption.

> [!TIP]
> This library makes extensive use of [symmetric transfer](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0913r0.html), allowing repeated effect invocations without causing stack overflow.

Since the `yield` effect is handled, `print_elems` simply returns `task<void>` (a pure[^1] computation that returns `void`).

[^1]: In this example, the function isn't technically pure since it still prints to `stdout`. However, we can define a `console` effect and handle it accordingly. Note that there is nothing preventing the user from bypassing it and performing arbitrary I/O directly.

Finally, the `main` function simply calls the `print_elems` coroutine, as it does not produce any effects and therefore does not need effect handlers.

> [!TIP]
> The effects allowed in each `task` are checked at compile time, ensuring that all effects are handled at some level. Additionally, a handler can produce its own effects, which must be propagated upward.

When run, this program produces the following output:
```
yielded 1
yielded 2
yielded 3
```

## Examples

See [examples](examples) for more interesting use cases of effects and handlers.

## Requirements

A compiler with C++20 support is required. While older versions may work, the following compiler versions or later are recommended for best results:

| Compiler | Version |
| -------- | ------- |
| GCC      | 13.3.0  |
| Clang    | 18.1.3  |

## Usage

### CMake with FetchContent
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
