#include "corofx/handler.hpp"
#include "corofx/task.hpp"

#include <iostream>

using corofx::make_handler;
using corofx::task;

struct foo {
    using return_type = int;

    int x;
};
struct bar {};
struct baz {};

auto blah() -> task<char, bar, baz> {
    std::cout << "blah: 0\n";
    // co_await baz{};
    co_await bar{};
    std::cout << "blah: 1\n";
    co_return 5; // NOLINT
}

auto bleh() -> task<void> {
    std::cout << "bleh: 0\n";
    co_return;
}

auto meh() -> task<int, foo, bar, baz> {
    std::cout << "meh: 0\n";
    co_await bleh();
    std::cout << "meh: 1\n";
    auto x = co_await blah().with(
        make_handler<bar>([](auto&&) { std::cout << "meh: handling bar\n"; }),
        make_handler<baz>([](auto&&) { std::cout << "meh: handling baz\n"; }));
    std::cout << "meh: 2\n";
    co_return x + 1;
    std::cout << "meh: 3\n";
}

auto uhh() -> task<int, foo, baz> {
    auto x =
        co_await meh().with(make_handler<bar>([](auto&&) { std::cout << "uhh: handling bar\n"; }));
    co_return x;
}

auto main() -> int {
    auto m = uhh();
    auto m2 = std::move(m);
    try {
        auto res = m2();
        std::cout << "main: success: " << res << std::endl;
    } catch (foo const&) {
        std::cout << "main: unhandled foo\n";
    } catch (bar const&) {
        std::cout << "main: unhandled bar\n";
    } catch (baz const&) {
        std::cout << "main: unhandled baz\n";
    } catch (...) {
        std::cout << "main: failed to handle <unknown>\n";
    }
}
