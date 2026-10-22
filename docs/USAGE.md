# 📖 SnipCLI Complete User Manual & Command Reference

**SnipCLI** is a zero-dependency, high-performance command-line code snippet manager engineered in modern C++17. It allows competitive programmers, software engineers, and DevOps practitioners to instantly save, search, view, and pipe algorithmic templates and boilerplate directly from the terminal with sub-millisecond latency.

---

## 📑 Table of Contents

1. [Installation Guide](#1-installation-guide)
   - [Prebuilt Binaries](#prebuilt-binaries-recommended)
   - [Package Managers (Homebrew, Debian, AUR, WinGet)](#package-managers)
   - [Building from Source](#building-from-source)
   - [Shell Tab-Completions (Bash, Zsh, Fish, PowerShell)](#shell-tab-completions)
   - [Man Page Installation](#manual-page-man-snip)
2. [CLI Core Concepts & Global Options](#2-cli-core-concepts--global-options)
3. [Exhaustive Command Reference](#3-exhaustive-command-reference)
   - [`add` — Create and Store a Snippet](#snip-add)
   - [`list` — Display All Snippets in Tabular View](#snip-list)
   - [`get` — Retrieve and Pipe Snippets](#snip-get)
   - [`search` — Algorithmic Strategy Search](#snip-search)
   - [`delete` — Remove a Snippet](#snip-delete)
   - [`stats` — View Repository Telemetry](#snip-stats)
4. [Output Presentation Engines (Terminal vs Markdown)](#4-output-presentation-engines)
5. [Environment Variables & Storage Internals](#5-environment-variables--storage-internals)
6. [Exit Status & Error Handling](#6-exit-status--error-handling)
7. [Contest & Pipeline Workflows](#7-contest--pipeline-workflows)

---

## 1. Installation Guide

### Prebuilt Binaries (Recommended)

Standalone binaries with zero runtime dependencies are attached to every [GitHub Release](https://github.com/chowdhuryrifat/SnipCLI/releases) accompanied by SHA-256 checksums (`SHA256SUMS.txt`).

#### Linux (x86_64)
```bash
# 1. Download latest archive
curl -fsSLO https://github.com/chowdhuryrifat/SnipCLI/releases/download/v1.0.0/snipcli-v1.0.0-linux-x86_64.tar.gz

# 2. Extract binary to /usr/local/bin
sudo tar -xzf snipcli-v1.0.0-linux-x86_64.tar.gz -C /usr/local --strip-components=1

# 3. Verify
snip --version
```

#### macOS (Universal: Apple Silicon & Intel)
```bash
# 1. Download latest macOS archive
curl -fsSLO https://github.com/chowdhuryrifat/SnipCLI/releases/download/v1.0.0/snipcli-v1.0.0-darwin-universal.tar.gz

# 2. Extract binary
sudo tar -xzf snipcli-v1.0.0-darwin-universal.tar.gz -C /usr/local --strip-components=1

# 3. Verify
snip --version
```

#### Windows (x86_64)
Download `snipcli-v1.0.0-windows-x86_64.zip` from Releases, unzip `snip.exe`, and place it anywhere in your system `%PATH%` (e.g. `C:\Program Files\SnipCLI` or `C:\tools\snip`).

---

### Package Managers

#### macOS & Linux via Homebrew
```bash
brew tap chowdhuryrifat/snipcli
brew install snipcli
```

#### Debian / Ubuntu (`.deb`)
```bash
# Download Debian package from Releases
curl -fsSLO https://github.com/chowdhuryrifat/SnipCLI/releases/download/v1.0.0/snipcli_1.0.0_amd64.deb

# Install package (installs binary, man page, and completions)
sudo dpkg -i snipcli_1.0.0_amd64.deb
```

#### Arch Linux (AUR)
```bash
yay -S snipcli
# or via paru
paru -S snipcli
```

#### Windows via WinGet
```powershell
winget install ChowdhuryRifat.SnipCLI
```

---

### Building from Source

Requires a C++17 compliant compiler (`gcc` 9+, `clang` 10+, or `MSVC` 2019+) and CMake 3.16+:

```bash
# 1. Clone repository
git clone https://github.com/chowdhuryrifat/SnipCLI.git
cd SnipCLI

# 2. Configure Release build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# 3. Compile in parallel
cmake --build build -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)"

# 4. (Optional) Run test suite
ctest --test-dir build --output-on-failure

# 5. Install system-wide
sudo cmake --install build
```

---

### Shell Tab-Completions

SnipCLI includes native completion scripts for Bash, Zsh, Fish, and PowerShell located under `completions/`.

#### Bash
```bash
# System-wide
sudo cp completions/snip.bash /etc/bash_completion.d/snip

# Or per-user (add to ~/.bashrc)
source /path/to/completions/snip.bash
```

#### Zsh
```bash
# Copy into your zsh fpath (e.g. ~/.zfunc)
mkdir -p ~/.zfunc
cp completions/_snip ~/.zfunc/_snip

# Add to ~/.zshrc before compinit:
fpath=(~/.zfunc $fpath)
autoload -Uz compinit && compinit
```

#### Fish
```bash
mkdir -p ~/.config/fish/completions
cp completions/snip.fish ~/.config/fish/completions/snip.fish
```

#### PowerShell
Add the following line to your PowerShell `$PROFILE`:
```powershell
. "C:\Path\To\completions\snip.ps1"
```

---

### Manual Page (`man snip`)

```bash
sudo cp man/snip.1 /usr/share/man/man1/snip.1
sudo mandb  # Update man database
man snip
```

---

## 2. CLI Core Concepts & Global Options

### Syntax
```text
snip <command> [options]
snip [global-options] <command> [options]
```

### Global Options

| Option | Short | Description |
| :--- | :---: | :--- |
| `--help` | `-h` | Display help screen (global or per-command) |
| `--version` | `-v` | Display application version and architecture banner |
| `--format <fmt>` | — | Override output format: `terminal` (default) or `markdown` |

### Interactive Help Commands
- `snip --help` or `snip -h` — Display global commands overview, common examples, and environment variables.
- `snip <command> --help` or `snip help <command>` — Display detailed syntax, flags, constraints, and examples for a specific command (e.g., `snip add --help`).

---

## 3. Exhaustive Command Reference

### `snip add`
Create and persist a new code snippet into the storage repository.

#### Syntax
```bash
snip add --title <title> --lang <language> [--code <inline_code> | --file <file_path>] [--tags <tag1,tag2>]
```

#### Options & Flags

| Flag | Short | Required | Description |
| :--- | :---: | :---: | :--- |
| `--title <string>` | `-t` | **Yes** | Unique snippet title. Allowed: letters, numbers, spaces, hyphens (`-`), underscores (`_`). |
| `--lang <string>`<br>`--language <string>` | `-l` | **Yes** | Programming language identifier (e.g. `cpp`, `python`, `rust`, `java`, `go`). |
| `--code <string>` | `-c` | *Conditional* | Inline code content string. *(Must provide `--code` OR `--file`).* |
| `--file <filepath>` | `-f` | *Conditional* | Path to an existing file containing snippet source code. |
| `--tags <list>`<br>`--tag <list>` | — | No | Comma-separated category tags (e.g. `graph,shortest_path,algo`). |
| `--format <fmt>` | — | No | Presentation format: `terminal` or `markdown`. |

#### Validation Invariants
- Duplicate titles are rejected defensively to prevent overwriting existing templates.
- Empty titles, empty language tags, and empty payloads are rejected.
- If code starts with dashes (like SQL comments `--`), use equals syntax: `--code="-- SQL comment"`.

#### Examples
```bash
# Add with inline code and multiple category tags
snip add --title "Dijkstra" --lang cpp --tags "graph,shortest_path" \
  --code "vector<int> dist(n, INF); priority_queue<pair<int,int>> pq;"

# Add directly from a source file
snip add --title "SegmentTree" --lang cpp --file ./templates/segtree.cpp --tags "tree,range_query"

# Add Python snippet with spaces in title
snip add --title "Binary Exponentiation" --lang python --code "def power(b, e): return pow(b, e, 10**9+7)"
```

---

### `snip list`
Display all stored snippets in tabular format with optional category or language filtering.

#### Syntax
```bash
snip list [options]
```

#### Options & Flags

| Flag | Short | Description |
| :--- | :---: | :--- |
| `--lang <string>`<br>`--language <string>` | `-l` | Filter snippets by exact programming language (e.g. `cpp`, `python`). |
| `--tag <string>`<br>`--tags <string>` | — | Filter snippets by category tag (e.g. `graph`, `dp`). |
| `--format <fmt>` | — | Presentation format: `terminal` (ANSI table) or `markdown` (GFM table). |

#### Examples
```bash
# List all snippets
snip list

# List only C++ snippets
snip list --lang cpp

# List only snippets tagged with 'graph'
snip list --tag graph

# Output as a GitHub-Flavored Markdown table (ready for documentation)
snip list --format markdown
```

---

### `snip get`
Retrieve, view, or extract a specific snippet by its unique numeric ID or exact title.

#### Syntax
```bash
snip get [--id <id> | --title <title> | <query>] [options]
```

#### Options & Flags

| Flag | Short | Description |
| :--- | :---: | :--- |
| `--id <integer>` | — | Exact numeric ID of the snippet (e.g. `--id 1`). |
| `--title <string>` | `-t` | Exact title of the snippet (e.g. `--title "Dijkstra"`). |
| `--code-only` | `-c` | **Pipe mode:** Emits raw source code byte-for-byte to stdout with zero headers, line numbers, or ANSI escapes. |
| `--format <fmt>` | — | Output format: `terminal` (default) or `markdown`. |

> **Note:** `--id` and `--title` are mutually exclusive. Specify either `--id` or `--title`, not both.

#### Pipe-Native Power Workflows
Using `--code-only` (or `-c`), SnipCLI acts as a pure UNIX pipe source:

```bash
# Inspect snippet with formatted banner and line numbers
snip get --title "Dijkstra"

# Pipe pure snippet code straight into your system clipboard (macOS)
snip get --title "Dijkstra" --code-only | pbcopy

# Pipe pure snippet code straight into system clipboard (Linux X11)
snip get --id 1 -c | xclip -selection clipboard

# Pipe pure snippet code straight into system clipboard (Linux Wayland)
snip get --id 1 -c | wl-copy

# Pipe snippet directly into a source file or compiler
snip get --title "Dijkstra" --code-only > solution.cpp
g++ -O3 solution.cpp -o solution
```

---

### `snip search`
Dynamically search through snippet titles, tags, or languages using the GoF Strategy Pattern.

#### Syntax
```bash
snip search [<query>] [options]
```

#### Options & Flags

| Flag | Short | Strategy Applied | Description |
| :--- | :---: | :--- | :--- |
| `<query>` | — | `TitleSearchStrategy` | Case-insensitive title substring search (positional argument). |
| `--title <query>` | `-t` | `TitleSearchStrategy` | Search by case-insensitive title substring. |
| `--tag <tag>`<br>`--tags <tag>` | — | `TagSearchStrategy` | Search by exact category tag match. |
| `--lang <lang>`<br>`--language <lang>` | `-l` | `LanguageSearchStrategy` | Search by exact programming language. |
| `--format <fmt>` | — | — | Output format: `terminal` or `markdown`. |

#### Examples
```bash
# Substring search: finds "Dijkstra", "dijkstra_dense", "DijkstraShortestPath"
snip search dijk

# Search by tag
snip search --tag tree

# Search by programming language
snip search --lang rust

# Output search results as Markdown
snip search --tag math --format markdown
```

---

### `snip delete`
Remove a snippet permanently from the repository by numeric ID or title.

#### Syntax
```bash
snip delete [--id <id> | --title <title> | <query>] [options]
```

#### Options & Flags

| Flag | Short | Description |
| :--- | :---: | :--- |
| `--id <integer>` | — | Exact numeric ID of the snippet to delete. |
| `--title <string>` | `-t` | Exact title of the snippet to delete. |
| `<query>` | — | Positional identifier (interpreted as ID if numeric, else title). |

> **Note:** `--id` and `--title` are mutually exclusive. Specify either `--id` or `--title`.

#### Examples
```bash
# Delete by unique title
snip delete --title "Dijkstra"

# Delete by numeric ID
snip delete --id 2

# Delete using positional argument
snip delete 3
```

---

### `snip stats`
Compute and display aggregate code metrics and repository telemetry across all stored snippets.

#### Syntax
```bash
snip stats [options]
```

#### Options & Flags

| Flag | Description |
| :--- | :--- |
| `--format <fmt>` | Output format: `terminal` (ANSI box) or `markdown` (formatted stats table). |

#### Metrics Computed
- **Total Snippets Volume:** Count of all active snippets stored.
- **Total Lines of Code:** Total sum of all source lines across all templates.
- **Language Breakdown:** Breakdown of snippets per language, sorted in descending order by volume.

#### Examples
```bash
# View terminal statistics
snip stats

# Export repository telemetry to Markdown
snip stats --format markdown
```

---

## 4. Output Presentation Engines

SnipCLI incorporates a polymorphic GoF Factory presentation engine (`ViewFactory` $\rightarrow$ `TerminalView`, `MarkdownView`):

### Terminal Engine (`--format terminal`, Default)
- Formatted tabular layouts with automatic column alignment.
- Syntax banners with metadata (ID, Language, Lines, Tags).
- ANSI color highlighting: Bold Green for successes, Bold Red for errors, Cyan for titles, Yellow for languages.
- Safe per-stream emission: Stdout receives formatted data; Stderr receives isolated diagnostics.
- Responsive color suppression when standard output is redirected or when `NO_COLOR` is active.

### Markdown Engine (`--format markdown`)
- Outputs strictly valid GitHub-Flavored Markdown tables.
- Perfect for pasting snippet indexes into repository documentation, contest team wikis, or project READMEs.

---

## 5. Environment Variables & Storage Internals

| Variable | Type / Range | Default | Purpose |
| :--- | :--- | :--- | :--- |
| `SNIPCLI_STORAGE_PATH` | File path | `~/.snipcli/snippets.json` | Location of the JSON database. Parent directories are created automatically if missing. |
| `SNIPCLI_LOCK_TIMEOUT_MS` | Integer in `[1, 600000]` ms | `10000` (10 seconds) | Cross-process advisory lock acquisition timeout. Protects against lock contention under high concurrency. Safe fallback on invalid or out-of-range values. |
| `NO_COLOR` | Any non-empty string | *(unset)* | Completely suppresses ANSI color escape sequences per [no-color.org](https://no-color.org/). |

### Storage Engine Architecture & Concurrency Guarantees
- **Thread Safety:** In-process concurrent threads are coordinated using `std::mutex` and `std::lock_guard`.
- **Cross-Process Concurrency:** Inter-process mutations are protected by advisory file locking:
  - **POSIX (Linux, macOS):** `flock` with `LOCK_EX` guarded with `O_NOFOLLOW` (rejects symlinks to prevent symlink traversal attacks).
  - **Windows:** `LockFileEx` with `LOCKFILE_EXCLUSIVE_LOCK` and `FILE_FLAG_OPEN_REPARSE_POINT` rejection.
- **Durability & Crash Consistency:** Saves write to a temporary file (`.tmp`), flush with POSIX parent directory `fsync` and macOS `F_FULLFSYNC`, followed by an atomic filesystem rename (`std::filesystem::rename` / `MoveFileExW`).
- **Symlink Protection:** Directory resolution iteratively resolves symlinks capped at `kMaxSymlinkHops = 40` to guarantee resilience against circular symlink loops.

---

## 6. Exit Status & Error Handling

| Exit Code | Meaning | Description |
| :---: | :--- | :--- |
| **`0`** | **Success** | Command completed successfully with valid output. |
| **`1`** | **Error** | Command failed (validation error, non-existent snippet, duplicate title, unrecognized option, or lock timeout). |

### Common Error Messages & Diagnoses
- `Error: Duplicate snippet title '<title>' already exists.`
  $\rightarrow$ Provide a distinct title or delete the existing snippet first.
- `Error: No code provided. Use --code="<code_string>" or --file=<file_path>.`
  $\rightarrow$ Ensure you pass either inline code or a readable file path.
- `Error: Conflicting selector options: specify either --id or --title, not both.`
  $\rightarrow$ In `get` or `delete`, select your snippet using either `--id` or `--title`.
- `Error: Unknown option '--<flag>' for command '<verb>'.`
  $\rightarrow$ Run `snip <verb> --help` to check supported flags for that command.

---

## 7. Contest & Pipeline Workflows

### 1. Rapid Algorithm Lookup During Contests
Add this alias to your `.bashrc` or `.zshrc`:
```bash
# Add quick copy alias
alias scp='snip get --code-only --title'
```
During a contest round:
```bash
# Grab your template directly into your clipboard in 1 command
scp "Dijkstra" | pbcopy
```

### 2. Batch Ingesting a Folder of Algorithm Templates
```bash
for file in ./templates/*.cpp; do
    name=$(basename "$file" .cpp)
    snip add --title "$name" --lang cpp --file "$file" --tags "contest,algo"
done
```

### 3. Synchronizing Snippets with Git
Because SnipCLI stores all snippets in a clean, human-readable JSON file (`~/.snipcli/snippets.json`), you can track it in a private dotfiles repository:
```bash
cd ~/.snipcli
git init
git add snippets.json
git commit -m "chore: backup snippets"
```
