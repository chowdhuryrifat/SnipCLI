# Verification and Audit Scripts

This directory contains automated audit reproduction scripts and fuzzing tools used to verify the security, correctness, and reliability of SnipCLI.

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
  python3 scripts/snipcli_loader_fuzz.py --bin [path/to/snip] --count 1500
  ```
- **Coverage:** Performs prefix truncation testing (657 prefixes) and pseudo-random byte mutation fuzzing (1500 iterations) with ASCII/UTF-8 control character injection, lone surrogate generation, number format mutations, and unknown object field tests. Asserts 0 differential classification mismatches.
