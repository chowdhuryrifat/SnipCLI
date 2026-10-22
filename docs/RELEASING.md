# Release Guide

This document outlines the standard release process for maintainers of **SnipCLI**.

---

## Release Checklist

1. **Verify Working Tree State:**
   - Ensure the working tree is clean on the target branch (`main`).
   - Run the full test and verification matrix locally:
     ```bash
     cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
     cmake --build build -j
     ctest --test-dir build --output-on-failure
     ./tests/smoke/smoke_test.sh ./build/snip
     ./scripts/snipcli_audit_repro.sh ./build/snip
     ```

2. **Check Documentation and Test Counts:**
   - Confirm `README.md` and `CONTRIBUTING.md` reflect accurate test counts and scenario breakdown.
   - Update `CHANGELOG.md`: promote the current `[Unreleased]` changes to a version header (e.g. `[1.0.0] - YYYY-MM-DD`).

3. **Verify Version Single-Sourcing:**
   - Version is defined in `CMakeLists.txt` via `project(SnipCLI VERSION X.Y.Z LANGUAGES CXX)`.
   - Ensure `snip --version` produces the exact expected banner:
     `SnipCLI version X.Y.Z (Modern C++17 MVC Architecture)`.

4. **Tagging:**
   - Create a signed annotated tag for the release:
     ```bash
     git tag -s v1.0.0 -m "Release v1.0.0"
     git push origin v1.0.0
     ```

5. **GitHub Release Automation:**
   - Pushing tag `v*.*.*` triggers `.github/workflows/release.yml`.
   - The workflow compiles Release binaries for:
     - Linux x86_64 (`.tar.gz` and `.deb`)
     - macOS ARM64 AppleClang (`.tar.gz`)
     - Windows x86_64 MSVC (`.zip`)
   - The workflow computes `SHA256SUMS.txt` for all assets and generates a GitHub Release draft.

6. **Distribution Channels:**
   - **WinGet:** Generate and submit manifest to `microsoft/winget-pkgs` using `wingetcreate`.
   - **Homebrew:** Update the formula in `homebrew-snipcli` tap with release URL and SHA-256.
   - **Debian / Ubuntu:** Provide the generated `.deb` package on GitHub Releases.
   - **AUR:** Update `PKGBUILD` and `.SRCINFO` in `aur/snipcli` with the release archive checksum.

7. **Review & Publish:**
   - Review the draft GitHub release notes.
   - Publish the release to make it public.
