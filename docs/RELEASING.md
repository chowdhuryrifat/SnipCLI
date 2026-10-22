# Release Guide

This document outlines the standard release architecture, asset conventions, and verification workflows for maintainers of **SnipCLI**. For the step-by-step release-day execution commands, refer to [**Release Day Runbook**](RELEASE_DAY.md).

---

## Release Architecture & Asset Conventions

SnipCLI uses an automated GitHub Actions release workflow (`.github/workflows/release.yml`) triggered by annotated git tags (`v*.*.*`). The workflow generates release artifacts conforming to standard naming conventions:

| Distribution Asset | Platform / Architecture | Package Format | Build Description |
| :--- | :--- | :--- | :--- |
| `snipcli-v1.0.0-linux-x86_64.tar.gz` | Linux x86_64 | Gzip Tarball (FHS) | Release binary (`bin/snip`), manual page (`man/snip.1`), shell completions (Bash, Zsh, Fish), LICENSE, NOTICE, README, CHANGELOG |
| `snipcli_1.0.0_amd64.deb` | Debian / Ubuntu amd64 | Debian Package (`.deb`) | CPack-generated Debian package with automated shared library dependency resolution (`dpkg-shlibdeps`) |
| `snipcli-v1.0.0-macos-universal.tar.gz` | macOS (Universal) | Gzip Tarball | Universal Mach-O fat binary combining Apple Silicon (`arm64`) and Intel (`x86_64`) slices, completions, and manual page |
| `snipcli-v1.0.0-windows-x86_64.zip` | Windows x86_64 | Zip Archive | Standalone `snip.exe` compiled with MSVC static runtime (`/MT`, zero dynamic MSVC CRT DLL dependencies) |
| `SHA256SUMS.txt` | Multi-platform | Plaintext Checksums | Standard SHA-256 checksums file covering all published release assets |

---

## Quality Gates and Verification

Every release must pass three automated quality gates prior to publishing:

1. **Continuous Integration (`.github/workflows/ci.yml`):**
   - Clang-Format 18 style verification (`git diff --exit-code`)
   - Cppcheck static analysis (`--enable=warning,performance,portability`)
   - 6-job multi-OS matrix:
     - Ubuntu 24.04 (GCC 13)
     - Ubuntu 24.04 (GCC AddressSanitizer + UndefinedBehaviorSanitizer)
     - Ubuntu 24.04 (GCC ThreadSanitizer with `setarch x86_64 -R`)
     - macOS 14 (AppleClang 15)
     - Windows Latest (MSVC 2022)
   - 139 unit test scenarios across 7 CTest suites (`ctest --output-on-failure`)
   - End-to-end CLI integration smoke test suite (`tests/smoke/smoke_test.sh`)
   - Adversarial audit reproduction suite (`scripts/snipcli_audit_repro.sh`)

2. **Packaging Validation (`.github/workflows/packaging-validate.yml`):**
   - Debian packaging validation: CPack build, `dpkg -I`, `dpkg -c`, `lintian --fail-on error`, and container smoke tests in `ubuntu:22.04`, `ubuntu:24.04`, and `debian:12`.
   - WinGet validation: schema compliance against official v1.9.0 JSON schemas.
   - Homebrew validation: `brew style`, `brew audit`, and a real `brew install --build-from-source` test via a local `file://` tarball.
   - Arch Linux AUR validation: `makepkg --printsrcinfo` diff against `.SRCINFO`, `namcap PKGBUILD`, and `makepkg -f` container build.
   - Distribution archive smoke tests across Linux, macOS, and Windows.

3. **Release Automation (`.github/workflows/release.yml`):**
   - Pre-build verification gate on Ubuntu 24.04.
   - Linux GLIBC symbol version check via `objdump -T` compared against declared `control` dependencies.
   - Cryptographic build provenance attestation via GitHub Actions (`actions/attest-build-provenance`).

---

## Local Pre-Release Verification Matrix

Before pushing any release tag, run the local matrix across environments:

```bash
# 1. Clean build and unit tests
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSNIPCLI_BUILD_TESTS=ON
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure

# 2. Run under hermetic env and NO_COLOR=1
NO_COLOR=1 ctest --test-dir build --output-on-failure
env -i PATH="$PATH" build/tests/snip_tests

# 3. Integration smoke suite and audit suites
bash tests/smoke/smoke_test.sh build/snip
bash scripts/snipcli_audit_repro.sh build/snip
bash scripts/snipcli_reaudit_repro.sh build/snip

# 4. Fuzzing oracle (1500 mutations, seeds 7 and 99)
python3 scripts/snipcli_loader_fuzz.py build/snip 1500 7
python3 scripts/snipcli_loader_fuzz.py build/snip 1500 99
```

---

## Downstream Packaging Finalization

Once the release is published on GitHub, run:

```bash
bash scripts/finalize_release_metadata.sh v1.0.0 --check-urls
```

This populates official release checksums from `SHA256SUMS.txt` across `packaging/` and outputs ready-to-submit channel trees under `dist-submit/`. See [**Release Day Runbook**](RELEASE_DAY.md) for channel-specific submission commands.
