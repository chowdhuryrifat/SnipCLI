# Contributing to SnipCLI

Thank you for your interest in contributing to **SnipCLI**! We welcome bug reports, documentation improvements, code fixes, and feature proposals.

This guide details the development setup, coding standards, testing workflow, and Pull Request (PR) protocols.

---

## 1. Development Environment Setup

### Prerequisites
- **C++ Compiler:** Supporting C++17:
  - GCC >= 9.0
  - Clang >= 10.0 (or AppleClang >= 12.0)
  - MSVC >= 2019 (19.28)
- **Build System:** CMake >= 3.16 and Make or Ninja
- **Code Formatter:** `clang-format` version 18
- **Static Analysis:** `cppcheck` >= 2.7 (optional for local checks, verified in CI)
- **External Dependencies:** **Zero**. SnipCLI has no external library dependencies.

### Building from Source

```bash
# Clone the repository
git clone https://github.com/chowdhuryrifat/SnipCLI.git
cd SnipCLI

# Configure build with tests enabled
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSNIPCLI_BUILD_TESTS=ON

# Compile in parallel
cmake --build build --parallel "$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"
```

---

## 2. Testing & Verification

SnipCLI maintains zero tolerance for regressions. Every change must be verified against our multi-tier test suite.

### 2.1 Running the Unit Test Suite

The zero-dependency unit testing framework tests 139 scenarios across 7 domain suites:
- **`Snippet` (10 scenarios):** Invariants, normalization, timestamps, line counts.
- **`SnippetValidator` (12 scenarios):** Validation constraints, bad characters, empty payloads.
- **`JsonSnippetRepository` (25 scenarios):** Persistence, atomic save, lock timeouts, UTF-8, symlink defenses.
- **`Strategy` (10 scenarios):** Title substring, tag matching, language filtering.
- **`Parser` (31 scenarios):** Flags, positionals, delimiters, schema rejection.
- **`Command Wiring` (31 scenarios):** Execution flows, selector exclusivity, error paths.
- **`View` (20 scenarios):** Terminal ANSI colors, markdown tables, raw byte outputs, `NO_COLOR`.

```bash
# Run unit tests via CTest
ctest --test-dir build --output-on-failure

# Or execute the test binary directly
./build/snip_tests
```

### 2.2 Integration Smoke Tests

```bash
# End-to-end integration smoke tests across all CLI commands
./tests/smoke/smoke_test.sh ./build/snip
```

### 2.3 Adversarial Reproduction & Differential Fuzzing

```bash
# Verify regression benchmarks and adversarial edge cases
./scripts/snipcli_audit_repro.sh ./build/snip

# Differential fuzzing against Python RFC 8259 json parser oracle
python3 scripts/snipcli_loader_fuzz.py --bin ./build/snip --count 1500
```

---

## 3. Code Style & Standards

We enforce strict automated formatting and compiler diagnostics:
- **Formatter:** `clang-format-18` adhering to the repository's `.clang-format` configuration (Google C++ style modified).
  ```bash
  # Check formatting
  clang-format-18 --dry-run --Werror src/**/*.cpp include/**/*.hpp tests/**/*.cpp

  # Apply formatting
  clang-format-18 -i src/**/*.cpp include/**/*.hpp tests/**/*.cpp
  ```
- **Compiler Warnings:** Must compile with zero warnings under `-Wall -Wextra -Wpedantic` on GCC/Clang and `/W4` on MSVC.
- **Memory Safety:** Zero raw owning pointers; always use `std::unique_ptr` or `std::shared_ptr`.
- **Hermetic Tests:** When modifying environment variables in unit tests, use the `ScopedEnv` RAII helper to ensure test isolation.

---

## 4. Branching & Commit Guidelines

### 4.1 Branching Strategy
- **`main`**: Production-ready branch. Releases and tagged versions only.
- **`dev`**: Active integration branch. All feature and fix branches target `dev`.
- **Feature Branches:** `feat/<short-name>`
- **Bug Fix Branches:** `fix/<short-name>`

### 4.2 Conventional Commits

Commit messages must adhere to the [Conventional Commits v1.0.0](https://www.conventionalcommits.org/) format:

```text
<type>(<scope>): <description>

[optional body explaining motivation and trade-offs]

[optional footer: Closes #issue]
```

**Common types:**
- `feat`: A new user-facing feature, command, or option.
- `fix`: A bug fix or security patch.
- `docs`: Documentation updates.
- `refactor`: Code change that neither fixes a bug nor adds a feature.
- `test`: Adding or updating test cases.
- `ci`: Changes to GitHub Actions or packaging scripts.

---

## 5. Pull Request Process

1. Open your PR against the `dev` branch.
2. Complete the checklist in `.github/pull_request_template.md`.
3. Ensure all CI checks pass:
   - Code Quality (clang-format and cppcheck)
   - Linux GCC (Release)
   - Linux Sanitizers (ASan + UBSan)
   - Linux ThreadSanitizer (TSan)
   - macOS AppleClang (ARM64)
   - Windows MSVC (x86_64)
4. A maintainer will review your PR. At least one approving review is required before merging.
