# SnipCLI Project Constitution & Technical Ruleset (`RULES.md`)

> **Notice:** This document serves as the binding project constitution for all contributors and acts as system instructions for AI coding assistants (`.cursorrules`, `CLAUDE.md`, or Antigravity rules). Every line of code written in SnipCLI must adhere to the standards outlined below.

---

## 1. Architectural Boundaries & MVC Integrity

SnipCLI strictly enforces the **Model-View-Controller (MVC)** architectural pattern in a command-line environment:

### 1.1 Model Invariants (`snipcli::model`)
- **No Presentation Knowledge:** The Model layer must NEVER include headers or reference concepts related to ANSI escape sequences, terminal colors, CLI arguments (`argc`/`argv`), or output formatting.
- **Pure Domain & Persistence:** The Model is exclusively responsible for data encapsulation (`Snippet`), business validation (`SnippetValidator`), persistence contracts (`ISnippetRepository`), and search logic (`ISearchStrategy`).
- **Data Transfer Objects (DTOs):** All data passed from the Model to other layers must be immutable or copied value types/DTOs.

### 1.2 View Invariants (`snipcli::view`)
- **No Business Logic & No Direct Persistence:** The View layer must NEVER instantiate or query a repository, modify snippets, perform validation, or parse raw command-line flags.
- **Abstract Rendering Contract:** Views implement `ISnippetView` and output to an abstract `std::ostream` (defaults to `std::cout` or `std::cerr`).
- **Format Decoupling:** Different formatting paradigms (ANSI terminal, Markdown table, raw JSON) must be isolated into distinct View implementations (`TerminalView`, `MarkdownView`, `JsonView`).

### 1.3 Controller Invariants (`snipcli::controller`)
- **Orchestration & Command Routing:** The Controller is the mediator. It coordinates between the View, the Model repository, and input commands.
- **Command Pattern Strictness:** Every CLI verb (`add`, `list`, `get`, `delete`, `search`, `stats`) is encapsulated as a standalone `ICommand` subclass.
- **Error Quarantine:** The Controller catches domain and repository exceptions (`SnippetException`, `StorageException`) and forwards error messages to the View; raw exceptions must never escape `main()`.

---

## 2. Naming Conventions

Consistency across both team members is mandatory to prevent merge disputes and cognitive overhead.

| Element | Format | Example | Notes |
| :--- | :--- | :--- | :--- |
| **Namespaces** | `snake_case` | `snipcli::model`, `snipcli::view` | Root namespace: `snipcli` |
| **Classes / Structs** | `PascalCase` | `SnippetRepository`, `AddCommand` | Structs used only for POD/DTOs |
| **Interfaces (Pure Virtual)** | `IPascalCase` | `ISnippetRepository`, `ICommand` | Prefixed with uppercase `I` |
| **Methods / Functions** | `camelCase` | `getSnippetById()`, `renderList()` | Verbs or verb-noun phrases |
| **Member Variables** | `m_camelCase` | `m_repository`, `m_title` | Always prefixed with `m_` |
| **Local Variables / Params** | `snake_case` | `snippet_id`, `search_query` | Concise yet descriptive |
| **Constants & Enum Values** | `kPascalCase` | `kMaxTitleLength`, `kDefaultPath` | Prefixed with `k` |
| **Source / Header Files** | `snake_case.cpp` / `.hpp` | `json_snippet_repository.cpp` | Matches the primary class name |

---

## 3. Modularity & Header Rules

### 3.1 Include Guards & File Organization
- Every header file must use `#pragma once` at line 1.
- Header files must have the `.hpp` extension; implementation files must have `.cpp`.
- Inline method definitions in headers are strictly forbidden, except for:
  - 1-line trivial getters/setters (`return m_title;`).
  - Template declarations.

### 3.2 Forward Declarations & Include Hygiene
- Use forward declarations in header files whenever possible (pointers and references) to minimize compilation cascade.
- Never use `#include <bits/stdc++.h>` or wildcard includes.
- Order includes strictly from local to global:
  1. Primary corresponding header (e.g. in `snippet.cpp`, `#include "snipcli/model/snippet.hpp"`)
  2. Local project headers (`#include "snipcli/..."`)
  3. Third-party libraries (`#include <nlohmann/json.hpp>`)
  4. Standard C++ library headers (`#include <string>`, `#include <vector>`)
- Absolutely NO `using namespace std;` in header files. Inside `.cpp` files, prefer scoped `using std::string;` if brevity is required.

---

## 4. Error Handling, Memory Safety & Modern C++ Standards

### 4.1 Memory Safety & Ownership
- **Zero Raw Owning Pointers:** Use `std::unique_ptr` for exclusive ownership (e.g., `CommandRegistry` holding commands, `ViewFactory` returning views) and `std::shared_ptr` only when shared ownership is genuinely needed.
- Raw pointers (`T*`) or references (`T&`) are allowed only as non-owning observer parameters.
- **RAII (Resource Acquisition Is Initialization):** File streams, mutexes, and resources must be managed exclusively via RAII wrappers (`std::ifstream`, `std::ofstream`, `std::lock_guard`).

### 4.2 `const` Correctness
- Mark all member functions that do not alter state as `const`.
- Pass non-primitive parameters by `const&` (e.g., `const std::string& query`).
- Variables initialized once and never modified must be marked `const`.

### 4.3 Custom Exception Hierarchy
All application exceptions derive from `snipcli::common::SnipException` which inherits from `std::runtime_error`:
```cpp
namespace snipcli::common {
    class SnipException : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };
    class ValidationException : public SnipException { using SnipException::SnipException; };
    class StorageException : public SnipException { using SnipException::SnipException; };
    class CommandException : public SnipException { using SnipException::SnipException; };
}
```

---

## 5. Commenting & Documentation Standards

- All interfaces, public classes, and methods must have **Doxygen-compliant docstrings**:
```cpp
/**
 * @brief Searches for snippets matching the provided query within specified tags.
 * @param query The search substring or regex pattern.
 * @param tag The tag category filter (e.g., "dp", "graph").
 * @return std::vector<Snippet> A list of matching snippet DTOs.
 * @throws StorageException If the underlying JSON file is corrupt or unreadable.
 */
[[nodiscard]] virtual std::vector<Snippet> search(const std::string& query, const std::string& tag) const = 0;
```
- Inline comments must describe **WHY**, never **WHAT**. (e.g., Explain algorithmic complexity or edge case handling, not that an index is being incremented).
- Use `[[nodiscard]]` on all query methods returning values to prevent discarded returns.
