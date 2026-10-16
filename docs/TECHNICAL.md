# SnipCLI Technical Reference Manual

**Course:** Advanced Programming Laboratory  
**Engineers:** Lameya Tabasum (ID: `250201`) & Chowdhury Rifat Ahmed (ID: `250233`)

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

## 3. Storage & Concurrency

- **Persistence Format:** JSON file with standard field schema (`id`, `title`, `language`, `code`, `tags`, `createdAt`, `updatedAt`).
- **Thread Safety:** `JsonSnippetRepository` guards read/write operations using `std::mutex` and `std::lock_guard<std::mutex>`.
- **File System Portability:** Storage location dynamically resolves via `SNIPCLI_STORAGE_PATH`, `%APPDATA%` (Windows), or `$HOME/.snipcli` (POSIX).
