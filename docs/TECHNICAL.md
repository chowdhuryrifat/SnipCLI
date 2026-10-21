# SnipCLI Technical Reference Manual

**Course:** Advanced Programming Laboratory  
**Engineers:** Chowdhury Rifat Ahmed (ID: `250233`) & Lameya Tabasum (ID: `250201`)

---

## 1. Build & Toolchain Specifications

- **Language Standard:** C++17 (`std::filesystem`, `std::optional`, `std::string_view`)
- **Compilation Targets:**
  - `snipcli_core` (Static Library): Contains Model, View, and Controller implementations.
  - `snip` (Executable): CLI application binary.
  - `snip_tests` (Executable): Automated unit test suite.
- **Strict Compiler Diagnostics:**
  - GCC/Clang: `-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wconversion`
  - MSVC: `/W4 /permissive- /utf-8`

## 2. Memory Management & Exception Safety

- **Ownership Policy:**
  - Exclusive ownership: `std::unique_ptr` (e.g. `CommandDispatcher` owns `ICommand`).
  - Shared access: `std::shared_ptr` (e.g. `ISnippetRepository` injected into multiple commands).
  - Non-owning observers: `const T&` or raw pointer `const T*`.
- **Exception Quarantine:**
  - All exceptions inherit from `snipcli::common::SnipException`.
  - Commands catch exceptions, route the user-facing message to `ISnippetView::renderError()`, and return non-zero exit codes.
  - `main()` implements an outer catch-all barrier to prevent unhandled crashes.

- **Closed Schema & Field Contract (v1.0):**
  - Storage parses snippets from a JSON array of objects with strict UTF-8 encoding validation.
  - Mandatory fields: `id` (integer > 0), `title` (string, non-empty, case-insensitively unique), `language` (string, non-empty), `code` (string, non-empty), `tags` (array of strings).
  - Optional fields: `createdAt` (string ISO-8601), `updatedAt` (string ISO-8601).
  - Validation: Unknown fields are validated as strictly well-formed JSON (strings, booleans, null, numbers matching `-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?`, or nested arrays/objects up to a depth cap of 64).
  - Drop on Write: In SnipCLI v1.0, the schema is closed. Unknown fields are tolerated on read and dropped upon saving. Preserving arbitrary unknown attributes is scheduled for v1.1.
- **ID Allocation & Reuse Policy (v1.0):**
  - Snippet IDs are assigned sequentially as `max(id) + 1` across existing items in storage.
  - If the newest snippet (the one with the highest ID) is deleted, its numeric ID is reused by the next newly created snippet.
  - Deleting interior IDs does not cause reuse because higher IDs remain.
  - Clients requiring immutable, long-term identifiers across deletions must reference snippets by title rather than numeric ID. Persistent monotonic ID counters are slated for v1.1.

