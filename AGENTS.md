# Agent Instructions

## Overview

Follow C++ Core Guidelines with an emphasis on consistency,
modular design, immutability, and minimal mutation.

## Philosophy

Consistency SHOULD be prioritized over familiarity.
Modern C++ SHOULD be embraced for safety and maintainability.

## Type Deduction and Function Syntax

- **`auto` usage**:
  - MUST use `auto` for all variable declarations.
  - SHOULD use `auto x = my_type{...}` for explicit typing.
  - SHOULD use `auto&&` by default to avoid copies.
  - SHOULD use `auto` only to explicitly copy.
- **Function syntax**: MUST use trailing return types, e.g., `auto func() -> int`.
- **Initialization**:
  - SHOULD prefer brace initialization `{}` over `()`.
  - SHOULD use `{}` for default initialization instead of `nullptr`, `0`, etc.
- **Data types**:
  - **Sum types**: SHOULD use `std::optional` and `std::variant` over unions.
  - **Product types**: SHOULD use `std::tuple` and structured bindings.

## Templates

- MUST use concepts over SFINAE for readability.
- MUST use `typename` (not `class`) for template type parameters.
- SHOULD use `std::integral auto val` when the type is used only once in a signature.
- SHOULD use `template<std::integral T>` when the type is used multiple times.
- **Friend declarations**: SHOULD omit template parameter names:
  `template<typename> friend class foo`.

## Classes and Resource Management

### Class Structure

- **Member access order**: `public` → `protected` → `private`.
  Each section MUST appear at most once.
- **Declarations** MUST follow this order within each section:
  1. Types
  2. Static functions
  3. Constructors (including copy and move)
  4. Destructor
  5. Copy and move assignment
  6. Other member operators
  7. Friend functions
  8. Other methods
  9. Static data members
  10. Data members
- MUST use default member initializers for primitive types, e.g., `int count_{};`.
- **Comparisons**: SHOULD prefer `operator<=>` (the spaceship operator).
- **Friends**:
  - SHOULD declare `friend` functions in the `public` section:
    they are part of the class's public API.
  - SHOULD declare `friend` classes at the start of the `private` section:
    they extend private access.

### Resource Management

- MUST use **RAII** wrappers extensively.
- SHOULD use raw pointers only for low-level resource classes.
- **Rule of five**: MUST define all or none of the special members.
- **Rule of zero**: SHOULD prefer classes with no custom special members.
- **Copy-and-swap**: MUST use this pattern when defining special members.
  - SHOULD define copy and move assignments in terms of a `friend swap` function.
  - SHOULD use `std::exchange` in move constructors:
    `data_{std::exchange(other.data_, {})}`.

## Naming Conventions

| Scope | Casing | Example |
| :--- | :--- | :--- |
| Template parameters | `Ada_Case` | `typename Value_Type` |
| Private members | `snake_case` with trailing `_` | `member_` |
| All other code | `snake_case` | `my_class`, `my_var` |

- **Getters and setters**: SHOULD use `name()` and `set_name(...)`
  for a private member `name_`.
- **Scope resolution**: SHOULD use unqualified names within the same namespace
  or class.
- **Namespaces**:
  - MUST NOT use `using namespace` in production code.
  - MUST prefix all standard library items with `std::`.
  - `using some_ns::item` MAY be acceptable in function bodies.

## Code Style and Organization

### Style Rules

- **Const placement**: MUST use "East const" style
  (`int const`, `std::string const&`).
- **Logical operators**: MUST use keyword forms (`not`, `and`, `or`).
- **Functions**: SHOULD prefer short, self-explanatory functions.
- **Whitespace**: SHOULD minimize vertical space and prefer compact, readable code.
- **Control flow**: SHOULD use early returns to reduce nesting.

### Comments and Prose

- SHOULD use semantic line breaks in all prose, including comments and documentation.
- SHOULD wrap lines at natural clause boundaries,
  aiming for a length of ~80 characters.
- SHOULD document only class and function contracts and complex algorithms.
- SHOULD NOT comment self-explanatory functions (`swap`, `size`, `empty`, `clear`).
- MUST always use line comments `//`.

### File Organization

- MUST use this include order:
  - Source files: own header → project headers → standard library.
  - Header files: project headers → standard library.
- MUST sort includes lexicographically within groups.
- **Headers**:
  - MUST use `#pragma once`.
  - MUST be self-contained and independently compilable.
  - SHOULD have a corresponding source file that includes the header.
