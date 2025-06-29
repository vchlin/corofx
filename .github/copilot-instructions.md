# C++ Coding Guidelines

## Overview

C++ coding standards building upon the C++ Core Guidelines. Emphasizes:

- **Consistency**: Uniform patterns throughout the codebase
- **Modular design**: Clear separation of concerns and responsibilities  
- **Immutable data**: Prefer immutable state where possible
- **Minimal state mutation**: Reduce complexity through controlled state changes

## Philosophy

Prioritize **consistency over absolute familiarity** - uniform patterns even when unfamiliar initially.

Favor **modern C++ constructs** over legacy alternatives - embrace newer language features that improve safety, expressiveness, and maintainability.

## Tooling Integration

Complements existing clang-format and clang-tidy configurations for formatting and extended linting.

## Type Deduction & Function Syntax
- **MUST** use `auto` everywhere: `auto val = 42`, `auto const* ptr = &obj`, `auto val = some_function()`
- **MUST** use `auto x = my_type{...}` when explicit type needed
- **MUST** use trailing return types: `auto func() -> int`
- **Initialization:** prefer brace initialization `{}` over parentheses `()` for consistency
- Use `auto&&` by default to avoid unnecessary copies; use `auto` only when intent is to copy
- **Sum types:** prefer `std::optional` and `std::variant` over nullable pointers or unions
- **Product types:** prefer `std::tuple` and structured bindings for safer grouped data access

## Templates
- **MUST** use concepts over SFINAE for readability
- **MUST** use `typename` (not `class`) for template type parameters
- Use `std::integral auto val` when type doesn't appear elsewhere in signature
- Use explicit `template<std::integral T>` when type appears multiple times

## Classes & Resource Management

### Class Structure
- **Member access order:** public → protected → private (each appears at most once)
- **Declaration order:** types → static functions → ctors → dtor → copy/move assign → member operators → friend functions → methods → static data → data
- **MUST** use default member initializers: `int count_{};`, `char* ptr_{};`
- Non-primitive types can omit `{}` if using default constructor
- **Comparisons:** prefer `operator<=>` (spaceship) over individual comparison operators
- Declare `friend` functions in `public`: part of class's public API
- Declare `friend` classes at start of `private`: extend private access

### Resource Management
- Use RAII wrappers extensively; raw pointers only for low-level resource classes
- **Rule of Five:** define all or none of: dtor/copy ctor/copy assign/move ctor/move assign
- **Rule of Zero:** prefer classes without custom special members
- **Copy-and-swap:** use copy-and-swap + move pattern when implementing special members manually
- Use `std::exchange` in move constructors: `data_{std::exchange(other.data_, {})}`

## Naming Conventions
- **PascalCase:** template parameters (`typename Value_Type`)
- **snake_case:** everything else (classes, variables, functions, types, namespaces)
- **Local variables:** short names (`i`, `it`, `ptr`, `val`, `str`, `vec`)
- **API elements:** descriptive names (`calculate_distance`, `file_manager`)
- **Private member fields:** snake_case with trailing underscore (`data_`, `count_`, `ptr_`)
- **Getters/Setters:** getter for private member `name_` should be `name()`, setter should be `set_name(...)`
- **Scope resolution:** use unqualified names within same namespace/class
- **Namespaces:**
  - **NEVER** `using namespace` in production (tests/examples only)
  - **MUST** prefix `std::` for all standard library items
  - `using some_ns::item` acceptable in function bodies

## Code Style & Organization

### Style Rules
- **Const placement:** "east const" style (`int const`, `std::string const&`)
- **Functions:** prefer short, self-explanatory functions; extract one-off functions if it helps readability
- **Whitespace:** minimize vertical space, prefer compact readable code
- **Control flow:** use early returns to reduce nesting

### Comments
- Only for class/function docs and complex algorithms, not inside function bodies
- Do not comment self-explanatory functions (`swap`, `size`, `empty`, `clear`, etc.)
- **NEVER** use block comments `/* */`, always use line comments `//`
- Break long comments using semantic line breaks at natural logical points
- Allow flexibility: multiple sentence parts up to ~80% of column limit are acceptable

### File Organization
- **Include order:**
  - Source files: own header → project headers → standard library
  - Header files: project headers → standard library
  - Sort lexicographically within groups
- **Headers:** use `#pragma once`, must be self-contained and independently compilable
- Every header should have corresponding source file that includes it
