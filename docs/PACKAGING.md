# Downstream Packaging Guide

This guide is intended for Linux, BSD, and macOS package maintainers (Homebrew, Debian/Ubuntu, Arch AUR, Fedora, etc.) who package **SnipCLI**.

---

## Build Requirements

- **C++ Compiler:** C++17 compliant compiler:
  - GCC >= 9
  - Clang >= 10 (or AppleClang >= 12)
  - MSVC >= 2019 (19.28)
- **Build System:** CMake >= 3.16 and Make or Ninja
- **Dependencies:** **None**. SnipCLI has zero external runtime or third-party dependencies.

---

## Standard Build and Install

SnipCLI uses standard CMake build semantics and honors GNUInstallDirs (`CMAKE_INSTALL_BINDIR`, `CMAKE_INSTALL_MANDIR`, `CMAKE_INSTALL_DATADIR`, `CMAKE_INSTALL_DOCDIR`).

```bash
# Configure
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DSNIPCLI_BUILD_TESTS=OFF

# Build
cmake --build build -j"$(nproc)"

# Install (destdir staging)
DESTDIR="/path/to/pkgdir" cmake --install build
```

---

## CMake Options

| Option | Default | Description |
|---|---|---|
| `CMAKE_BUILD_TYPE` | `Release` | Recommended optimization level (`Release` enables `-O3` / `/O2`). |
| `SNIPCLI_BUILD_TESTS` | `ON` | Set to `OFF` to avoid compiling the test suite binary (`snip_tests`). |

---

## Installed Artifacts and Paths

Under standard `/usr` prefix, CMake installs:

| File | Destination | Description |
|---|---|---|
| Binary | `/usr/bin/snip` | Executable binary |
| Man page | `/usr/share/man/man1/snip.1` | Groff manual page |
| Bash completion | `/usr/share/bash-completion/completions/snip` | Bash completion script |
| Zsh completion | `/usr/share/zsh/site-functions/_snip` | Zsh completion function |
| Fish completion | `/usr/share/fish/vendor_completions.d/snip.fish` | Fish completion script |
| Docs & License | `/usr/share/doc/snip/` | LICENSE, NOTICE, README.md |

---

## Running Test Suite During Package Build

If packagers wish to run test checks during the package build (`check()` or `test()` step):

```bash
# Build with tests enabled
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSNIPCLI_BUILD_TESTS=ON
cmake --build build -j

# Execute test suite via ctest
ctest --test-dir build --output-on-failure

# Execute integration smoke test
./tests/smoke/smoke_test.sh ./build/snip
```

---

## Debian Package Generation (`.deb`)

Debian packages are generated using CPack as the single source of truth, configured in root `CMakeLists.txt`:

```bash
# Configure and build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DSNIPCLI_BUILD_TESTS=OFF
cmake --build build -j"$(nproc)"

# Package with CPack
cd build && cpack -G DEB
```

Package dependencies (`Depends:`) are automatically determined at package time from the compiled binary via `dpkg-shlibdeps` (`CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON`), guaranteeing exact glibc and libstdc++ compatibility without hardcoded version strings. The generated package filename is `snipcli_1.0.0_amd64.deb`.

