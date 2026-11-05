# SnipCLI Technical Reference Manual

**Maintainers:** Chowdhury Rifat Ahmed & Lameya Tabasum

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
  - Snippet IDs are always the contiguous sequence `1..N`; a new snippet receives `N + 1`.
  - After every deletion (by ID or title), the remaining snippets are renumbered to `1..N`, preserving their relative order. Renumbering does not modify `createdAt`/`updatedAt`.
  - Consequence: an ID is a *position*, not a permanent handle. Deleting a snippet shifts every higher ID down by one, so scripts and aliases that need a stable reference should use the snippet title.
- **Timestamps:** `createdAt` / `updatedAt` are ISO 8601 in the user's local timezone with an explicit UTC offset (e.g. `2026-11-04T22:17:18+06:00`). Files written by v1.0.0 contain UTC `...Z` timestamps; both forms are accepted and preserved as-is.
  - Clients requiring immutable, long-term identifiers across deletions must reference snippets by title rather than numeric ID. Persistent monotonic ID counters are slated for v1.1.
- **Lock Timeout Parsing & Durability Hardening (v1.0):**
  - `SNIPCLI_LOCK_TIMEOUT_MS` environment variable is parsed using `std::from_chars`. Valid range is `[1, 600000]` ms (1 ms to 10 minutes). Values above 600,000 clamp to 600,000; non-numeric, empty, negative, zero, or overflowing values safely fall back to the 10,000 ms default.
  - Advisory lock files are opened with `O_NOFOLLOW` on POSIX to prevent symlink traversal.
  - Atomic saves issue `fcntl(fd, F_FULLFSYNC)` on macOS with fallback to `fsync`, and perform a best-effort `fsync` on the parent directory following atomic rename on POSIX. Temporary file permissions default safely to 0600.

