# C++ Style Guide

## Overview

Follow C++ Core Guidelines with an emphasis on consistency,
modular design, immutability, and minimal mutation.

## Philosophy

Prioritize consistency over familiarity.
Embrace modern C++ for safety and maintainability.

## Tooling Integration

Complements existing clang-format and clang-tidy configurations
for formatting and extended linting.

## Type Deduction & Function Syntax

- **`auto` Usage**:
  - **MUST** use `auto` for all variable declarations.
  - Use `auto x = my_type{...}` for explicit typing.
  - Use `auto&&` by default to avoid copies; use `auto` only to explicitly copy.
- **Function Syntax**: **MUST** use trailing return types: `auto func() -> int`.
- **Initialization**:
  - Prefer brace initialization `{}` over `()`.
  - Use `{}` for default values instead of `nullptr`, `0`, etc.
- **Data Types**:
  - **Sum types**: Prefer `std::optional` and `std::variant`.
  - **Product types**: Prefer `std::tuple` and structured bindings.

## Templates

- **MUST** use concepts over SFINAE for readability.
- **MUST** use `typename` (not `class`) for template type parameters.
- Use `std::integral auto val` when a type is used once in a signature.
- Use `template<std::integral T>` when a type is used multiple times.
- **Friend declarations**: Omit template parameter names:
  `template<typename> friend class foo;`.

## Classes & Resource Management

### Class Structure

- **Member access order**: `public` → `protected` → `private`.
  Each specifier should appear at most once.
- **Declaration order**: types → static functions → ctors → dtor →
  copy/move assign → member operators → friend functions → methods →
  static data → data.
- **MUST** use default member initializers for primitive types: `int count_{};`.
- **Comparisons**: Prefer `operator<=>` (spaceship).
- **Friends**:
  - Declare `friend` functions in `public` - part of the class's public API.
  - Declare `friend` classes at the start of `private` - extend private access.

### Resource Management

- Use **`RAII`** wrappers extensively.
- Use raw pointers only for low-level resource classes.
- **`Rule of Five`**: Define all or none of the special members.
- **`Rule of Zero`**: Prefer classes with no custom special members.
- **`copy-and-swap`**: **MUST** use this pattern when defining special members.
  - Define copy and move assignment in terms of a `friend swap` function.
  - Use `std::exchange` in move constructors: `data_{std::exchange(other.data_, {})}`.

## Naming Conventions

| Scope | Casing | Example |
| :--- | :--- | :--- |
| Template parameters | `Pascal_Case` | `typename Value_Type` |
| All other code | `snake_case` | `my_class`, `my_var` |
| Private members | `snake_case_` | `member_` |

- **Getters/Setters**: `name()` and `set_name(...)` for a private member `name_`.
- **Scope resolution**: Use unqualified names within the same namespace/class.
- **Namespaces**:
  - **NEVER** use `using namespace` in production code.
  - **MUST** prefix `std::` for all standard library items.
  - `using some_ns::item` is acceptable in function bodies.

## Code Style & Organization

### Style Rules

- **Const placement**: "East const" style (`int const`, `std::string const&`).
- **Logical operators**: **MUST** use keyword forms (`not`, `and`, `or`).
- **Functions**: Prefer short, self-explanatory functions.
- **Whitespace**: Minimize vertical space; prefer compact, readable code.
- **Control flow**: Use early returns to reduce nesting.

### Comments and Prose

- Use semantic line breaks in all prose, including comments and documentation.
- Wrap lines at natural clause boundaries, aiming for a length of ~80 characters.
- Document only class/function contracts and complex algorithms.
- Do not comment self-explanatory functions (`swap`, `size`, `empty`, `clear`).
- Always use line comments `//`.

### File Organization

- **Include order**:
  - Source files: own header → project headers → standard library.
  - Header files: project headers → standard library.
- Sort includes lexicographically within groups.
- **Headers**:
  - Use `#pragma once`.
  - Must be self-contained and independently compilable.
  - Every header should have a corresponding source file that includes it.
