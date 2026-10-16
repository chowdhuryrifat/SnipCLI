# SnipCLI 7-Day Sprint Plan & Daily Work Breakdown

**Course:** Advanced Programming Laboratory  
**Project:** SnipCLI (Algorithmic Snippet Manager)  
**Timeline:** 1-Week Sprint (7 Days)

---

## 👥 Engineering Team & Core Responsibilities

<div align="center">

| Engineer | Student ID | Architectural Role | Domain Ownership |
| :--- | :---: | :--- | :--- |
| **Chowdhury Rifat Ahmed** | `250233` | **Backend & Core Architect** | Model Layer, JSON Persistence, Validation Rules (SRP), Strategy Pattern (`ISearchStrategy`), CTest Unit Tests |
| **Lameya Tabasum** | `250201` | **Frontend, Routing & DevOps Lead** | CLI Parser, Command Pattern (`ICommand`), View Layer & Factory (ANSI/Markdown), GitHub Actions CI/CD, Smoke Tests |

</div>

---

## 📅 Day-by-Day Execution Matrix

### Day 1: Architecture Inception & Contract Freeze
> **Goal:** Establish project infrastructure, agree on abstract interfaces, and freeze headers to unblock parallel work.

- **Chowdhury Rifat Ahmed (`250233`):**
  - Design domain model entity: `include/snipcli/model/snippet.hpp`.
  - Design segregated repository interfaces per Interface Segregation Principle (ISP):
    - `ISnippetReader` (read-only queries).
    - `ISnippetWriter` (state-mutating persistence).
    - `ISnippetRepository` (combining interface for DIP/LSP).
- **Lameya Tabasum (`250201`):**
  - Set up root `CMakeLists.txt` (C++17, compiler warning flags, library targets).
  - Draft project constitution `RULES.md` and git guidelines `CONTRIBUTING.md`.
  - Define Command Pattern abstraction: `include/snipcli/controller/i_command.hpp`.
  - Define View presentation abstraction: `include/snipcli/view/i_snippet_view.hpp`.
- **🤝 Daily Sync Milestone (Git Gate):**
  - **Milestone 1 — Interface Freeze:** Review and merge all abstract header files into `dev`. All public contracts are frozen; neither developer will have compile-time blocking dependencies.

---

### Day 2: Core Domain Logic & CLI Parsing
> **Goal:** Implement domain data handling and input tokenization.

- **Chowdhury Rifat Ahmed (`250233`):**
  - Implement `src/model/snippet.cpp` (getters, setters, line counting).
  - Implement `src/model/snippet_validator.cpp` enforcing Single Responsibility Principle (SRP):
    - Title length, syntax checks, language verification, empty payload rejection.
  - Author initial unit tests for domain invariants: `tests/unit/test_validator.cpp`.
- **Lameya Tabasum (`250201`):**
  - Implement `src/controller/cli_parser.cpp`:
    - Positional arguments, options (`--flag value`), boolean switches (`-h`, `-v`).
  - Implement `src/controller/command_dispatcher.cpp`:
    - Command registry mapping CLI verbs to `ICommand` instances.
    - Global `--help` and `--version` dispatching.
- **🤝 Daily Sync Milestone (Git Gate):**
  - **Milestone 2 — Foundation Validation:** Rifat reviews Lameya's parser PR; Lameya reviews Rifat's domain validator PR. Both run local unit tests with zero warnings.

---

### Day 3: Persistence Engine & Terminal Presentation
> **Goal:** Enable disk storage and colorized terminal output.

- **Chowdhury Rifat Ahmed (`250233`):**
  - Implement `src/model/json_snippet_repository.cpp`:
    - Thread-safe file persistence with `std::mutex` and `std::lock_guard`.
    - JSON serialization and parsing using C++17 filesystem and streams.
    - Path resolution: environment variable `SNIPCLI_STORAGE_PATH`, fallback to `~/.snipcli`.
- **Lameya Tabasum (`250201`):**
  - Implement `src/view/terminal_view.cpp`:
    - ANSI color highlighting (bold green for success, bold red for errors, cyan for titles).
    - Tabular formatting with column padding (`std::setw`) and line numbering.
    - Telemetry presentation (`renderStats`).
- **🤝 Daily Sync Milestone (Git Gate):**
  - **Milestone 3 — Storage & Presentation:** Repository saves and loads sample snippets; Terminal view renders test tables.

---

### Day 4: GoF Patterns Implementation (Strategy & Command Wiring)
> **Goal:** Connect business actions through GoF Design Patterns.

- **Chowdhury Rifat Ahmed (`250233`):**
  - Implement GoF Strategy Pattern for snippet searching:
    - `include/snipcli/model/strategy/i_search_strategy.hpp`
    - `src/model/strategy/tag_search_strategy.cpp` (filter by category tag).
    - `src/model/strategy/title_search_strategy.cpp` (case-insensitive substring match).
- **Lameya Tabasum (`250201`):**
  - Implement core GoF Command subclasses:
    - `src/controller/commands/add_command.cpp` (file reading and validation).
    - `src/controller/commands/list_command.cpp` (filtering by tag/language).
    - `src/controller/commands/get_command.cpp` (retrieval with `--code-only` pipe mode).
- **🤝 Daily Sync Milestone (Git Gate):**
  - **Milestone 4 — Command Wiring:** End-to-end execution of `snip add`, `snip list`, and `snip get` working across Model, Controller, and View.

---

### Day 5: Multi-Format View Engine & Advanced Commands
> **Goal:** Add Markdown export and complete the full CLI command set.

- **Chowdhury Rifat Ahmed (`250233`):**
  - Author complete unit test suite in `tests/unit/`:
    - `test_snippet.cpp`
    - `test_validator.cpp`
    - `test_repository.cpp`
    - `test_strategy.cpp`
    - Master runner `test_main.cpp` integrated with CTest.
- **Lameya Tabasum (`250201`):**
  - Implement GoF Factory Pattern:
    - `src/view/markdown_view.cpp` (generates GitHub-Flavored Markdown tables).
    - `src/view/view_factory.cpp` (swaps View at runtime based on `--format`).
  - Implement remaining commands:
    - `src/controller/commands/delete_command.cpp`
    - `src/controller/commands/search_command.cpp` (invokes Rifat's Search Strategy).
- **🤝 Daily Sync Milestone (Git Gate):**
  - **Milestone 5 — Feature Completeness:** All planned CLI verbs active; dual terminal/markdown presentation functional.

---

### Day 6: CI/CD Automation & Telemetry Command
> **Goal:** Automate testing pipeline and prepare metrics telemetry.

- **Chowdhury Rifat Ahmed (`250233`):**
  - Implement `src/controller/commands/stats_command.cpp` (calculates total snippets, total lines, language breakdown).
  - Conduct memory-safety audit using Valgrind / AddressSanitizer (ensuring zero raw owning pointers).
- **Lameya Tabasum (`250201`):**
  - Author automated end-to-end integration suite: `tests/smoke/smoke_test.sh`.
  - Author GitHub Actions multi-platform workflow: `.github/workflows/ci.yml`:
    - Linux GCC + Windows MSVC matrix.
    - Clang-Format 18 check and Cppcheck static analysis.
    - Automated binary artifact release.
  - Finalize user-facing documentation in `README.md`.
- **🤝 Daily Sync Milestone (Git Gate):**
  - **Milestone 6 — CI/CD Quality Gate:** Pull Request checks fully passing with 100% green status on GitHub Actions.

---

### Day 7: Viva Rehearsal & Live Modification Drills
> **Goal:** Run mock defenses and practice live code modifications under a 5-minute timer.

- **Chowdhury Rifat Ahmed (`250233`):**
  - Rehearse explaining SOLID principles (SRP, ISP, LSP, DIP) and the GoF Strategy Pattern.
  - Practice Live Modification Scenario B: Modifying `SnippetValidator` to enforce new validation rules (e.g. line limits or duplicate tags) in under 90 seconds.
- **Lameya Tabasum (`250201`):**
  - Rehearse explaining CLI MVC boundaries, Command Pattern routing, and Factory Pattern view creation.
  - Practice Live Modification Scenarios A & C: Registering a new command or adding a new view export format in under 3 minutes.
- **🤝 Final Milestone — Defense Ready:**
  - Both engineers sign off on `dev` $\rightarrow$ `main` release.
  - Tag release `v1.0.0` on GitHub.

---

## 🔒 Conflict Prevention Protocol (Zero-Collision Strategy)

1. **Strict File Boundaries:**
   - **Rifat's Exclusive Files:** `include/snipcli/model/*`, `src/model/*`, `tests/unit/*`.
   - **Lameya's Exclusive Files:** `include/snipcli/view/*`, `include/snipcli/controller/*`, `src/view/*`, `src/controller/*`, `tests/smoke/*`, `.github/*`.
2. **Mutual Peer Review Required:**
   - Neither developer merges their own PR. Every PR requires an explicit approving review from the other developer.
3. **No Direct Pushes:**
   - Protected branch rules enforce that all changes arrive via feature branches (`feat/*`, `fix/*`) and pass GitHub Actions CI checks before merging.
