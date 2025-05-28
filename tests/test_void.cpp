#include "corofx/check.hpp"
#include "corofx/task.hpp"

#include <utility>

using namespace corofx;

struct foo {
    using return_type = void;

    int value{};
};

constexpr auto marker0 = __LINE__;
constexpr auto marker1 = __LINE__;
constexpr auto marker2 = __LINE__;

auto test_void_effect() -> task<int, foo> {
    co_await foo{marker0};
    co_await foo{marker1};
    co_return marker2;
}

auto main() -> int {
    auto counter = 0;
    auto result =
        test_void_effect().with(handler_of<foo>([&](auto&& e, auto&& resume) -> task<int> {
            counter += e.value;
            co_return resume();
        }));
    auto final_result = std::move(result)();
    check(counter == marker0 + marker1);
    check(final_result == marker2);
}
