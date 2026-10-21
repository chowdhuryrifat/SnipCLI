# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - YYYY-MM-DD

### Added
- Zero-dependency modern C++17 command-line snippet manager (`snip`) designed for competitive programmers and software engineers.
- Commands: `add`, `list`, `get`, `delete`, `search`, and `stats`.
- Search strategies: query filtering by tag (`--tag`), title substring (`--query`), or programming language (`--language`).
- Multiple rendering views: human-friendly colored terminal view (`--format terminal`) and Markdown output (`--format markdown`).
- `--code-only` flag on `get` for byte-exact code extraction suitable for piping into compilers and clipboards.
- Strict MVC architecture, GoF Command/Strategy/Factory patterns, and Interface Segregation Principle (ISP) repository abstractions.
- Cross-platform file locking with bounded polling (`SNIPCLI_LOCK_TIMEOUT_MS`) to protect storage concurrency on POSIX (`flock`) and Windows (`LockFileEx`).
- Atomic file writes using temporary files with restrictive permissions (`0600`), fsync, and atomic rename.
- Comprehensive test harness covering 139 scenarios across 7 unit suites and end-to-end integration smoke tests (`tests/smoke/smoke_test.sh`).
- Shell completions for Bash, Zsh, Fish, and PowerShell.
- Manual page (`man/snip.1`) formatted with groff.

### Changed
- Characterized and documented ID allocation strategy (`max(id) + 1` policy) and ID reuse semantics upon deleting the highest ID (PR #22, F4).
- Unit test harness modernized with `ScopedEnv` RAII helper to ensure hermetic environment variable isolation across test runs (PR #22, F2).
- CI workflow matrix pinned to `ubuntu-24.04`, added `macos-14` (AppleClang) runner, added `permissions: contents: read`, added `setarch x86_64 -R` for TSan ASLR safety, and configured weekly Dependabot updates (PR #22, F7).
- Version reporting unified through CMake single-source configuration (`src/version.hpp.in`).

### Fixed
- Fixed recursive symlink resolution crash by capping symlink traversal to 40 links, preventing stack exhaustion on circular symlinks (PR #22, F1).
- Hardened JSON repository loader: enforced RFC 3629 UTF-8 validation, unescaping of JSON keys, strict syntax validation for unknown fields, and capped JSON nesting depth to 64 levels in `validateAndSkipJsonValue` to prevent recursion stack overflow (PR #22, F3).
- Added `O_NOFOLLOW` flag and `ELOOP` handling to file locking on POSIX to prevent symlink traversal attacks targeting lock files (PR #22, F5).
- Hardened lock timeout parsing using `std::from_chars` bounded to `[1, 600000]` ms with default fallback on non-numeric or negative values; added `fcntl(fd, F_FULLFSYNC)` on macOS and parent directory synchronization after atomic rename on POSIX (PR #22, F6).

### Security
- Hardened file lock acquisition against symlink traversal attacks (`O_NOFOLLOW`).
- Enforced strict JSON validation rejecting malformed Unicode surrogates, unescaped control characters, duplicate keys, and deeply nested attack vectors.
- Guaranteed restrictive file creation permissions (`0600`) and safe temporary file creation (`O_EXCL | O_NOFOLLOW`).
