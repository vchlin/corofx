#include "corofx/task.hpp"

using namespace corofx;

struct state_get {
    using return_type = int;
};

struct state_put {
    using return_type = int;

    int x{};
};

constexpr auto max_depth = 100;

auto stateful(int depth) -> task<void, state_get, state_put> { // NOLINT(misc-no-recursion)
    if (depth == 0) {
        for (auto i = co_await state_get{}; i > 0; i = co_await state_get{}) {
            co_await state_put{i - 1};
        }
    } else {
        co_await stateful(depth - 1);
    }
    co_return {};
}

auto do_state() -> task<void> {
    auto x = 0;
    co_await stateful(max_depth).with(
        handler_of<state_put>([&](auto&& e, auto&& resume) -> task<void> {
            x = e.x;
            co_return resume(x);
        }),
        handler_of<state_get>([&](auto&&, auto&& resume) -> task<void> { co_return resume(x); }));
    co_return {};
}

auto main() -> int { do_state()(); }
