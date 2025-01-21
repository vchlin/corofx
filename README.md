# corofx

[![Build](https://github.com/vchlin/corofx/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/vchlin/corofx/actions/workflows/build.yml?query=branch%3Amain)

Algebraic effects for C++20 using coroutines.

## Overview

corofx is a library for programming with algebraic effects. It is implemented using standard C++20 features only without external dependencies. The effect semantics are largely inspired by the [Koka](https://github.com/koka-lang/koka) language.

Some use cases of effects include dependency injection, emulating checked exceptions, etc.

## Features

### Typed Effect Handling

Allowed effects in each scope are checked at compile time.

```C++
struct foo {
    using return_type = int;
};

struct bar {
    using return_type = bool;
};

struct baz {
    using return_type = char;
};

auto do_something() -> task<int, foo, baz> {
    //                           ^~~~~~~~ Allowed effects.
    auto x = co_await foo{}; // Okay.
    auto y = co_await bar{}; // Error: Cannot perform a `bar` effect in this context.
    auto z = co_await baz{}; // Okay.
    co_return x + y + z;
}
```

### Resuming with a Result

An effect handler can tail-resume (one-shot) back to the call site with a result. This library makes use of [symmetric transfer](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0913r0.html), allowing repeated effect invocations without stack overflow.

```C++
constexpr auto large_value = 1'000'000;

struct yield {
    using return_type = bool;

    int i{};
};

auto traverse() -> task<void, yield> {
    for (auto i = 0;; ++i) {
        if (!co_await yield{i}) break;
    }
    co_return {};
}

auto print_elems() -> task<void> {
    co_await traverse()
        .with(make_handler<yield>([](auto&& y, auto&& resume) -> task<void> {
            std::cout << "yielded " << y.i << '\n';
            co_return resume(y.i < large_value);
        }));
    co_return {};
}
```

See [examples](examples) for more.

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

## Limitations

- Only one-shot handlers are supported.
