# Public Repository Readiness Audit Report

**Date:** 2026-10-22  
**Target Release:** SnipCLI v1.0.0  
**Scope:** Full working tree and full git history (`--all`, `--full-history`)  
**Auditor:** Antigravity Release Automation  

---

## Executive Summary

Before transitioning the SnipCLI repository from private to public on GitHub, this readiness scan was executed to identify sensitive data, secret tokens, private personal information, unexpected large/binary assets, and potential license incompatibilities.

> [!IMPORTANT]
> In accordance with the project release rules, this scan is **report-only**. No git history has been rewritten, and no sensitive personal data has been modified without explicit human decision. The maintainers should review the recommendations below and decide on actions prior to executing `gh repo edit --visibility public`.

---

## 1. Secrets & Credentials Scan

### Methodology & Tools
- **Tool:** Gitleaks v8.21.2 (`gitleaks detect --source . --verbose --all`)
- **Secondary Tool:** Targeted `git log -p --all` regex sweep for credentials (`ghp_`, `github_pat_`, `AKIA`, `BEGIN .*PRIVATE KEY`, `password`, `secret`, `token`)

### Findings Table

| ID | Finding Description | Tool & Scope | Location / Evidence | Severity | Recommended Action |
| :--- | :--- | :--- | :--- | :---: | :--- |
| **SEC-01** | High-entropy secrets, API keys, private keys | Gitleaks v8.21.2 (80 commits scanned) | Full history (`git log -p -U0 --full-history --all`) | **CLEAN** | **Keep** — No leaks detected. |
| **SEC-02** | GitHub tokens / cloud credentials (`ghp_`, `AKIA`, etc.) | Regex pattern scan across `git log -p` | Full history | **CLEAN** | **Keep** — Zero hardcoded tokens found. |
| **SEC-03** | CI Secret References | Static analysis | `.github/workflows/ci.yml`, `release.yml` | **INFO** | **Keep** — Standard GitHub Actions context `${{ secrets.GITHUB_TOKEN }}` used correctly. |

---

## 2. Personal Data & Academic Privacy Scan

### 2.1 Student Identification Numbers
Student IDs associated with academic coursework appear in tracked files.

| ID | Data Found | Location | Context | Severity | Recommended Action |
| :--- | :--- | :--- | :--- | :---: | :--- |
| **PII-01** | Student ID `250233` (Chowdhury Rifat Ahmed)<br>Student ID `250201` (Lameya Tabasum) | `README.md:373` | Authors statement | **MEDIUM** | **Decide** — Remove or keep if desired for academic portfolio credit. |
| **PII-02** | Student ID `250233`<br>Student ID `250201` | `docs/ARCHITECTURE.md:5-6` | Architectural team metadata | **MEDIUM** | **Decide** — Redact IDs if public anonymity is preferred. |
| **PII-03** | Student ID `250233`<br>Student ID `250201` | `docs/TECHNICAL.md:4` | Engineers metadata | **MEDIUM** | **Decide** — Redact IDs if public anonymity is preferred. |
| **PII-04** | Student ID `250233`<br>Student ID `250201` | `docs/internal/COURSE.md:15-16` | Authors table & Viva focus | **MEDIUM** | **Decide** — Keep internal or redact prior to public launch. |
| **PII-05** | Student ID `250233`<br>Student ID `250201` | `docs/internal/REQUIREMENTS.md:4` | Authors header | **MEDIUM** | **Decide** — Redact IDs if public anonymity is preferred. |
| **PII-06** | Student ID `250233`<br>Student ID `250201` | `docs/internal/SPRINT_PLAN.md:15,16,27,33,46,51,65,70,83,88,101,108,123,126,141,144` | Sprint task ownership matrix | **MEDIUM** | **Decide** — Redact IDs if public anonymity is preferred. |

### 2.2 Personal Email Addresses & Commit Metadata

| ID | Data Found | Location | Context | Severity | Recommended Action |
| :--- | :--- | :--- | :--- | :---: | :--- |
| **PII-07** | `266280041+lameyatabasum@users.noreply.github.com` | Git commit metadata (`%ae`, `%ce`) and `Co-authored-by` trailers | Authorship on early commits | **MEDIUM** | **Decide** — Present in permanent git commit objects. Requires `git-filter-repo` to rewrite if email privacy is required. |
| **PII-08** | `67093774+chowdhuryrifat@users.noreply.github.com` | Git commit history diffs (`git log -p`) | Present in earlier commits of `CMakeLists.txt`, `SECURITY.md`, `CODE_OF_CONDUCT.md`, `packaging/aur/PKGBUILD` prior to Task 1 | **MEDIUM** | **Decide** — Replaced in working tree with GitHub noreply address (`67093774+chowdhuryrifat@users.noreply.github.com`), but visible in commit history. |
| **PII-09** | `67093774+chowdhuryrifat@users.noreply.github.com` | Tracked files: `CMakeLists.txt:148,154`, `CODE_OF_CONDUCT.md:62`, `SECURITY.md:21`, `packaging/aur/PKGBUILD:1` | Official public maintainer contact | **CLEAN** | **Keep** — Standard GitHub privacy-preserving noreply email address. |

### 2.3 Course & University Identifiers

| ID | Data Found | Location | Context | Severity | Recommended Action |
| :--- | :--- | :--- | :--- | :---: | :--- |
| **PII-10** | "Advanced Programming Laboratory", "viva defense", "professor challenge" | `docs/internal/COURSE.md:1,3,13,24,28`<br>`docs/internal/REQUIREMENTS.md:3,24`<br>`docs/internal/SPRINT_PLAN.md:3,138`<br>`README.md:44,367,373,375`<br>`docs/ARCHITECTURE.md:3`<br>`docs/TECHNICAL.md:3` | Coursework defense metadata | **LOW** | **Decide** — Normal for academic showcase projects. Maintainer may choose to retain for portfolio or move/strip for purely commercial presentation. |

### 2.4 User Home Paths (`/home/<user>/`) & Phone Numbers
- **Home Paths:** Scanned `git grep -n "/home/"` and `git log -p`. Zero occurrences found in tracked files and commit history.
- **Phone Numbers:** Scanned phone number regexes. Zero occurrences found.

---

## 3. Large, Binary, or Unwanted Generated Files

| ID | Asset Description | Location | Size / Type | Severity | Recommended Action |
| :--- | :--- | :--- | :--- | :---: | :--- |
| **BIN-01** | Review bundle archives | Working tree: `snipcli_review_bundle_4.zip` | ~1.5 MB ZIP | **LOW** | **Keep untracked / Ignore** — Ensure untracked ZIP files are ignored in `.gitignore` and never committed. |
| **BIN-02** | Tracked Git objects | Git index (`git ls-files`) | 100% text/source files | **CLEAN** | **Keep** — Zero binary blobs or generated binaries tracked in git. |
| **BIN-03** | Source file sizes | `tests/unit/test_repository.cpp` (58 KB)<br>`src/model/json_snippet_repository.cpp` (55 KB)<br>`tests/unit/test_commands.cpp` (54 KB) | Text source code | **CLEAN** | **Keep** — Legitimate comprehensive test suites and repository implementations. |

---

## 4. Third-Party Licenses & MIT Compatibility

| ID | Component | License | Compatibility with MIT | Severity | Recommended Action |
| :--- | :--- | :--- | :---: | :---: | :--- |
| **LIC-01** | SnipCLI Application Code | MIT (`LICENSE`) | Target License | **CLEAN** | **Keep** — Permissive MIT license is standard and clear. |
| **LIC-02** | Dependencies | None (Zero external libraries) | N/A | **CLEAN** | **Keep** — Implementation relies solely on ISO C++17 standard library and standard OS POSIX/Win32 APIs. No GPL, LGPL, or restrictive licenses present. |

---

## Summary of Human Decisions Required Before Public Release

1. **Student IDs (`250233`, `250201`):** Decide whether to keep student IDs in `README.md`, `docs/ARCHITECTURE.md`, `docs/TECHNICAL.md`, and `docs/internal/` as academic provenance, or redact them.
2. **Personal Email in Git History:** Note that `266280041+lameyatabasum@users.noreply.github.com` and `67093774+chowdhuryrifat@users.noreply.github.com` exist in historical commit objects. If absolute email privacy is desired, rewrite history before making the repository public; otherwise, accept historical presence since working tree is now clean.
3. **Academic Course Playbook:** Decide whether `docs/internal/COURSE.md` should be made public along with the repo or retained as an internal reference.
