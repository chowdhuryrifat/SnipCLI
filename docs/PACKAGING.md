# Downstream Packaging Guide

This guide is intended for Linux, BSD, macOS, and Windows package maintainers (Homebrew, Debian/Ubuntu, Arch AUR, WinGet, Fedora, Alpine, etc.) packaging **SnipCLI**.

---

## Build Requirements

- **C++ Compiler:** ISO C++17 compliant compiler:
  - GCC >= 9 (Recommended: GCC 11+)
  - Clang >= 10 (or AppleClang >= 12)
  - MSVC >= 2019 (19.28+)
- **Build System:** CMake >= 3.16 and Make or Ninja
- **Runtime Dependencies:** **None**. SnipCLI has zero external runtime or third-party dependencies. It links exclusively against the standard C++ runtime (`libstdc++`/`libc++`) and standard C library (`glibc`/`musl`/`MSVCRT`).

---

## Standard Build and Installation (FHS Compliant)

SnipCLI uses standard CMake build semantics and strictly honors `GNUInstallDirs` (`CMAKE_INSTALL_BINDIR`, `CMAKE_INSTALL_MANDIR`, `CMAKE_INSTALL_DATADIR`, `CMAKE_INSTALL_DOCDIR`).

```bash
# Configure
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DSNIPCLI_BUILD_TESTS=OFF

# Build
cmake --build build -j"$(nproc)"

# Install into staging root (destdir staging)
DESTDIR="/path/to/pkgdir" cmake --install build
```

---

## CMake Options

| Option | Default | Description |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `Release` | Recommended optimization level (`Release` enables `-O3` on GCC/Clang, `/O2` on MSVC). |
| `SNIPCLI_BUILD_TESTS` | `ON` | Set to `OFF` in production packages to avoid building the unit test runner (`snip_tests`). |

---

## Installed Artifacts and Paths

Under standard `/usr` prefix, CMake installs:

| File | Destination | Description |
|---|---|---|
| Binary | `/usr/bin/snip` | Main CLI executable |
| Manual Page | `/usr/share/man/man1/snip.1` | Formatted groff man page |
| Bash Completion | `/usr/share/bash-completion/completions/snip` | Bash completion script |
| Zsh Completion | `/usr/share/zsh/site-functions/_snip` | Zsh completion function |
| Fish Completion | `/usr/share/fish/vendor_completions.d/snip.fish` | Fish completion script |
| Documentation & License | `/usr/share/doc/snipcli/` | `LICENSE`, `NOTICE`, `README.md`, `CHANGELOG.md` |

---

## Running Test Suites During Package Build

Downstream package build scripts that include a `check()` or `test()` step can run:

```bash
# 1. Unit tests (all 7 suites, 139 scenarios)
ctest --test-dir build -C Release --output-on-failure

# 2. Integration smoke test against built binary
bash tests/smoke/smoke_test.sh build/snip
```

---

## Supported Packaging Channels & Manifests

Pre-authored, verified manifests are maintained in the repository under `packaging/`:

### 1. Debian & Ubuntu (`.deb`)
Debian packages are generated via CPack in root `CMakeLists.txt`:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSNIPCLI_BUILD_TESTS=OFF
cmake --build build -j"$(nproc)"
cd build && cpack -G DEB
```
- Shared library dependencies (`Depends:`) are determined dynamically via `dpkg-shlibdeps` (`CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON`), ensuring exact libc compatibility.
- Package Maintainer: `Chowdhury Rifat Ahmed <67093774+chowdhuryrifat@users.noreply.github.com>`.

### 2. Homebrew (`packaging/homebrew/snipcli.rb`)
- Formula maintained in tap repository: `chowdhuryrifat/homebrew-snipcli`.
- Uses standard Homebrew CMake install flags (`*std_cmake_args`).
- Formula includes unit and CLI smoke test in its `test do` block.

### 3. Windows WinGet (`packaging/winget/*.yaml`)
- Manifest files conforming to WinGet Manifest Specification v1.9.0:
  - `ChowdhuryRifat.SnipCLI.yaml` (Version manifest)
  - `ChowdhuryRifat.SnipCLI.installer.yaml` (Installer manifest for zip portable)
  - `ChowdhuryRifat.SnipCLI.locale.en-US.yaml` (Locale and metadata manifest)
- Targets `snipcli-v1.0.0-windows-x86_64.zip` with portable alias `snip`.

### 4. Arch Linux AUR (`packaging/aur/PKGBUILD`, `.SRCINFO`)
- PKGBUILD builds from source tarball using CMake.
- Executes `ctest` during `check()`.
- Installs license to `/usr/share/licenses/snipcli/LICENSE`.

---

## Validation & Release Automation

1. **Automated CI Packaging Validation:**
   `.github/workflows/packaging-validate.yml` runs full end-to-end linting and installation tests on every PR:
   - Debian `.deb` build, `lintian --fail-on error`, and container installations on `ubuntu:22.04`, `ubuntu:24.04`, `debian:12`.
   - WinGet schema validation against official v1.9.0 JSON schemas.
   - Homebrew `brew style`, `brew audit`, and `brew install --build-from-source` using a local `file://` tarball.
   - Arch Linux container build via `makepkg -f` and `namcap` audit.

2. **Release Metadata Automation:**
   `scripts/finalize_release_metadata.sh <tag>` updates manifests with release SHA-256 hashes and generates ready-to-submit channel trees under `dist-submit/`. See [**Release Day Runbook**](RELEASE_DAY.md).
