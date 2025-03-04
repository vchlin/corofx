#include "corofx/check.hpp"
#include "corofx/task.hpp"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

using namespace corofx;

using vec = std::vector<std::size_t>;

struct foo {
    using return_type = std::unique_ptr<int>;

    std::unique_ptr<int> x;
};

struct bar {
    using return_type = vec;

    vec x;
};

constexpr auto len = 10;
constexpr auto marker = __LINE__;

auto do_foo() -> task<std::unique_ptr<int>, foo> {
    auto x = std::make_unique<int>(marker);
    x = co_await foo{std::move(x)};
    check(*x == marker);
    co_return x;
}

auto do_foo_outer() -> task<std::unique_ptr<int>, foo> { co_return co_await do_foo(); }

auto do_bar() -> task<vec, bar> {
    auto x = vec{};
    x.reserve(len);
    for (auto i = std::size_t{}; i < len; ++i) {
        x.push_back(i);
        x = co_await bar{std::move(x)};
        check(x[i] == i);
    }
    check(x.size() == len);
    co_return x;
}

auto do_bar_outer() -> task<vec, bar> { co_return co_await do_bar(); }

auto main() -> int {
    auto foo_handler = handler_of<foo>([](auto&& e, auto&& resume) -> task<std::unique_ptr<int>> {
        check(*e.x == marker);
        co_return resume(std::move(e.x));
    });
    auto x = do_foo().with(foo_handler);
    check(*std::move(x)() == marker);
    auto x2 = do_foo_outer().with(foo_handler);
    check(*std::move(x2)() == marker);

    auto i = std::size_t{};
    auto bar_handler = handler_of<bar>([&i](auto&& e, auto&& resume) -> task<vec> {
        check(e.x.size() == ++i);
        co_return resume(std::move(e.x));
    });
    auto y = do_bar().with(bar_handler);
    check(std::move(y)().size() == len);
    i = std::size_t{};
    auto y2 = do_bar_outer().with(bar_handler);
    check(std::move(y2)().size() == len);
}
