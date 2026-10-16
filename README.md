# SnipCLI 🚀

**A fast, modern C++17 CLI snippet manager built for competitive programmers.**

[![CI/CD Pipeline](https://github.com/example/SnipCLI/actions/workflows/ci.yml/badge.svg)](https://github.com/example/SnipCLI/actions)
[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

SnipCLI empowers competitive programmers to store, categorize, search, and retrieve code templates directly from the terminal without breaking focus during timed rounds.

---

## Key Features

- ⚡ **Instant Search & Retrieval:** Query snippets by title substring or category tags (e.g., `#graph`, `#dp`, `#math`) using the **GoF Strategy Pattern**.
- 🎨 **Multi-Format Presentation:** Output ANSI-colorized terminal tables with syntax banners or export clean GitHub-Flavored Markdown tables via the **GoF Factory Pattern**.
- 🧩 **Strict MVC & SOLID Architecture:** Zero coupling between presentation, business rules, and storage persistence.
- 📋 **Terminal Pipe Friendly:** Pipe raw snippet code directly to clipboard or compilers via `snip get <title> --code-only | pbcopy`.
- 📊 **Metric Analytics:** Track code volume and language distribution with `snip stats`.

---

## Installation & Quick Start

### Prerequisites
- C++17 compliant compiler (`GCC 9+`, `Clang 10+`, or `MSVC 2019+`)
- CMake 3.16 or higher

### Building from Source
```bash
git clone https://github.com/your-org/SnipCLI.git
cd SnipCLI
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

### Running Tests
```bash
# Run CTest Unit Tests
ctest --test-dir build --output-on-failure

# Run Integration Smoke Tests
./tests/smoke/smoke_test.sh ./build/snip
```

---

## Usage Guide

```bash
# 1. Add a snippet from an inline string
./build/snip add --title "Dijkstra" --lang "cpp" --tags "graph,shortest_path" --code "vector<int> dist(n, INF);"

# 2. Add a snippet from a source file
./build/snip add --title "SegmentTree" --lang "cpp" --tags "tree,range_query" --file ./templates/segtree.cpp

# 3. List all snippets (Terminal view)
./build/snip list

# 4. List snippets filtered by tag or language
./build/snip list --tag "graph"
./build/snip list --lang "cpp"

# 5. Search snippets by title or tag (Strategy Pattern)
./build/snip search "dijkstra"
./build/snip search --tag "tree"

# 6. Retrieve a snippet
./build/snip get --title "Dijkstra"

# 7. Pipe raw code to clipboard or file
./build/snip get --title "Dijkstra" --code-only > solution.cpp

# 8. View repository metrics and statistics
./build/snip stats

# 9. Export output in GitHub-Flavored Markdown
./build/snip list --format markdown
./build/snip stats --format markdown

# 10. Delete a snippet
./build/snip delete --title "Dijkstra"
```

---

## Architecture & Governance

- [Executive Architecture & Class Diagrams](docs/ARCHITECTURE.md)
- [Functional & Non-Functional Requirements](docs/REQUIREMENTS.md)
- [Technical Specification & Safety](docs/TECHNICAL.md)
- [Coding Standards & AI Rules (`RULES.md`)](RULES.md)
- [Git Flow & Contribution Protocols (`CONTRIBUTING.md`)](CONTRIBUTING.md)

---

## Authors & Course Context

- **Developers:**
  - **Lameya Tabasum** (Student ID: `@lameyatabasum`) - Backend & Core Model Architect
  - **Chowdhury Rifat Ahmed** (Student ID: `@chowdhuryrifat`) - Frontend & Routing Lead

