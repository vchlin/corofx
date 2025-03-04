#include "corofx/detail/type_set.hpp"

#include <type_traits>

using namespace corofx::detail;

static_assert(std::is_same_v<type_set<int, char>::add<type_set<char>>, type_set<int, char>>);
static_assert(std::is_same_v<type_set<int, char>::add<type_set<bool>>, type_set<int, char, bool>>);
static_assert(
    std::is_same_v<type_set<int, char>::add<type_set<bool, int, char>>, type_set<int, char, bool>>);
static_assert(std::is_same_v<
              type_set<int, char>::add<type_set<bool>, type_set<char, int>>,
              type_set<int, char, bool>>);

static_assert(std::is_same_v<type_set<int, char>::subtract<int>, type_set<char>>);
static_assert(std::is_same_v<type_set<int, char>::subtract<char, int>, type_set<>>);
static_assert(std::is_same_v<type_set<int, char>::subtract<bool, char>, type_set<int>>);
static_assert(std::is_same_v<type_set<int, char>::subtract<char, bool>, type_set<int>>);
static_assert(
    std::is_same_v<type_set<int, char, short>::subtract<char, bool>, type_set<int, short>>);

auto main() -> int {}
