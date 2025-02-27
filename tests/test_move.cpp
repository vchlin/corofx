#include "corofx/check.hpp"
#include "corofx/task.hpp"

#include <cstddef>
#include <utility>
#include <vector>

using namespace corofx;

using vec = std::vector<std::size_t>;

struct bar {
    using return_type = vec;

    vec x;
};

constexpr auto len = 10;

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

auto main() -> int {
    auto i = std::size_t{};
    auto res = do_bar().with(handler_of<bar>([&i](auto&& e, auto&& resume) -> task<vec> {
        check(e.x.size() == ++i);
        co_return resume(std::move(e.x));
    }));
    check(std::move(res)().size() == len);
}
