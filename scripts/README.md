# Verification, Packaging, and Release Scripts

This directory contains automated audit reproduction scripts, fuzzing tools, packaging validators, and release metadata automation used to verify the security, correctness, and reliability of SnipCLI.

---

## Scripts

### 1. `snipcli_audit_repro.sh`
- **Purpose:** End-to-end regression reproduction test suite covering findings from the initial code audit (R1–R14).
- **Execution:**
  ```bash
  bash scripts/snipcli_audit_repro.sh [path/to/snip]
  ```
- **Coverage:** Checks file locking concurrency, storage corruption handling, CLI argument parsing edge cases, strict ID validation, `--code-only` exact output, and directory symlink resolution.

### 2. `snipcli_reaudit_repro.sh`
- **Purpose:** Follow-up verification suite for second-round audit findings (R15–R17).
- **Execution:**
  ```bash
  bash scripts/snipcli_reaudit_repro.sh [path/to/snip]
  ```
- **Coverage:** Tests atomic save permissions, non-blocking lock polling timeout behavior, and lock canonicalization.

### 3. `snipcli_loader_fuzz.py`
- **Purpose:** Differential fuzzing oracle comparing `JsonSnippetRepository`'s custom JSON parser against Python's strict RFC 8259 `json.loads` parser.
- **Execution:**
  ```bash
  python3 scripts/snipcli_loader_fuzz.py [path/to/snip] [N_MUTATIONS] [SEED]
  # Example (standard verification):
  python3 scripts/snipcli_loader_fuzz.py ./build/snip 1500 7
  ```
- **Coverage:** Performs prefix truncation testing (every strict prefix of a valid JSON snippet file) and pseudo-random byte mutation fuzzing (e.g. 1500 iterations) with ASCII/UTF-8 control character injection, lone surrogate generation, number format mutations, and unknown object field tests. Asserts 0 differential classification mismatches.

### 4. `packaging_smoke.sh`
- **Purpose:** Local and CI validation of package builds, file hierarchies, and CLI runtime functionality.
- **Execution:**
  ```bash
  # Test an existing binary directly:
  bash scripts/packaging_smoke.sh /usr/bin/snip
  # Full local build & packaging verification:
  bash scripts/packaging_smoke.sh
  ```
- **Coverage:** Verifies CPack `.deb` generation, FHS file placement, man page, shell completions, Homebrew formula presence, WinGet YAML manifests, and AUR PKGBUILD/.SRCINFO syntax.

### 5. `finalize_release_metadata.sh`
- **Purpose:** Automates downstream packaging release metadata updates by parsing release checksums from `SHA256SUMS.txt`, replacing `<SHA256_*>` placeholders in manifests, and generating ready-to-submit channel directory trees under `dist-submit/`.
- **Execution:**
  ```bash
  # Test without writing changes:
  bash scripts/finalize_release_metadata.sh v1.0.0 --sums-file path/to/SHA256SUMS.txt --dry-run
  # Apply changes and generate dist-submit trees:
  bash scripts/finalize_release_metadata.sh v1.0.0 --sums-file path/to/SHA256SUMS.txt --check-urls
  ```
- **Outputs:**
  - `dist-submit/winget/manifests/c/ChowdhuryRifat/SnipCLI/<version>/` (WinGet package manifests)
  - `dist-submit/homebrew-snipcli/Formula/snipcli.rb` & `README.md` (Homebrew tap repository)
  - `dist-submit/aur/` (PKGBUILD and `.SRCINFO` for Arch User Repository)
