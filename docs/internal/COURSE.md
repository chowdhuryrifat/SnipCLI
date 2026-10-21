# Course & Viva Defense Metadata

**Course:** Advanced Programming Laboratory  
**Project:** SnipCLI (Algorithmic Snippet Manager)  
**Engineering Team:** Chowdhury Rifat Ahmed & Lameya Tabasum  

---

## 👥 Authors & Team Responsibilities

<div align="center">

| Developer | Student ID | Primary Architectural Role & Domain Ownership | Viva Defense Focus |
| :--- | :---: | :--- | :--- |
| **Chowdhury Rifat Ahmed** | `250233` | **Backend & Core Architect**<br>• Domain Entity & Invariants (`SnippetValidator`)<br>• Storage Engine & JSON Persistence (`JsonSnippetRepository`)<br>• Search Strategy Pattern (`ISearchStrategy`)<br>• CTest Unit Testing Framework (`tests/unit/`) | • SOLID (SRP, ISP, LSP, DIP)<br>• Strategy Pattern<br>• Cross-process advisory locking<br>• Live Validation Scenario |
| **Lameya Tabasum** | `250201` | **Frontend, Routing & DevOps Lead**<br>• Controller & Command Pattern (`ICommand`)<br>• View Layer & Factory (`TerminalView`, `MarkdownView`)<br>• CLI Argument Parser & Router (`CliParser`)<br>• CI/CD Pipeline & End-to-End Smoke Tests | • CLI MVC Architecture<br>• Command & Factory Patterns<br>• GitHub Actions Multi-OS Matrix<br>• Live Command/View Scenarios |

</div>

---

## 🛡️ Defense & Live Modification Playbook

SnipCLI is engineered specifically to satisfy the **5-minute live on-the-spot viva modification constraint**. Because the codebase strictly decouples Model, View, and Controller layers and applies GoF design patterns, candidate changes touch minimal isolated files:

| Scenario | Professor Challenge | Files Touched | Defense Rationale & Pattern Applied |
| :--- | :--- | :--- | :--- |
| **A: New Command** | *"Add a `stats` command showing line totals across all snippets."* | 2 files (`stats_command.cpp`, `main.cpp`) | Demonstrates **OCP** and the **Command Pattern**: no core router, model, or existing commands modified. |
| **B: New Invariant** | *"Reject snippets with > 100 lines or duplicate tags."* | 1 file (`snippet_validator.cpp`) | Demonstrates **SRP**: validation invariants are strictly quarantined from parser and storage layers. |
| **C: Format Swap** | *"Export snippet listings as Markdown tables instead of terminal output."* | 2 files (`markdown_view.cpp`, `view_factory.cpp`) | Demonstrates **DIP** & **Factory Pattern**: Controller depends on abstract `ISnippetView`; view swap requires zero controller logic edits. |

---

## 📚 Related Academic Documentation

- [System Architecture](../../docs/ARCHITECTURE.md) — MVC layer separation and SOLID design details.
- [Technical Specification](../../docs/TECHNICAL.md) — Storage engine, locking mechanics, and JSON schema invariants.
- [7-Day Sprint Plan](SPRINT_PLAN.md) — Daily execution matrix and interface freeze milestones.
- [Project Constitution & Coding Standards](RULES.md) — Architectural rules, naming conventions, and error handling.
