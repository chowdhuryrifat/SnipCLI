# Distribution and Packaging

This directory contains packaging definitions, manifest templates, and build configuration files for distributing **SnipCLI** across package managers and operating systems.

---

## Registry Name Collision Analysis & Naming Policy

Prior to v1.0.0 release packaging, name availability for `snip` and `snipcli` was audited across upstream package registries:

| Registry | Status for `snip` | Status for `snipcli` | Decision / Policy |
|---|---|---|---|
| **WinGet (`microsoft/winget-pkgs`)** | Unclaimed | Unclaimed | Package Identifier: **`ChowdhuryRifat.SnipCLI`** (command: `snip`). |
| **Homebrew (`homebrew/core` & taps)** | Legacy tap existed (`sniptt-official/snip`, discontinued) | Available | Formula: **`snipcli`** in tap `chowdhuryrifat/homebrew-snipcli` (binary: `snip`). |
| **Debian / Ubuntu (`apt`)** | Unclaimed (only `sniproxy` / `vim-snippets`) | Available | Package name: **`snipcli`** (binary installed to `/usr/bin/snip`). |
| **Arch Linux (`AUR`)** | **Claimed** (legacy Ultisnips tool) | Available | Package name: **`snipcli`** (binary installed to `/usr/bin/snip`). |

### Universal Rule:
- **Binary Name:** Everywhere installed, the executable binary is named **`snip`**.
- **Package Name:** In all package registries, the package identifier is **`snipcli`** (or `ChowdhuryRifat.SnipCLI` on WinGet).

---

## Supported Distribution Targets

| Channel | Platform | Directory | Artifact / Format | Publishing Method |
|---|---|---|---|---|
| **WinGet** | Windows 10/11 | `packaging/winget/` | `ChowdhuryRifat.SnipCLI.yaml` | Pull request to `microsoft/winget-pkgs` |
| **Homebrew** | macOS & Linux | `packaging/homebrew/` | `snipcli.rb` | Custom tap (`chowdhuryrifat/homebrew-snipcli`) |
| **Debian / Ubuntu** | Linux (Debian-based) | `packaging/debian/` | `.deb` binary package (CPack) | GitHub Releases / PPA |
| **AUR** | Linux (Arch-based) | `packaging/aur/` | `PKGBUILD` | Arch User Repository git push |

---

## Directory Overview

- **`winget/`**: Manifest files adhering to the Windows Package Manager schema version 1.6.0 (`ChowdhuryRifat.SnipCLI.yaml`, installer manifest, locale manifest).
- **`homebrew/`**: Homebrew ruby formula `snipcli.rb` compiling from source and installing completions and man pages.
- **`debian/`**: Debian package generation is integrated into root `CMakeLists.txt` via CPack (`CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON`) as the single source of truth, computing dynamic shared library dependencies automatically via `dpkg-shlibdeps`.
- **`aur/`**: Arch Linux `PKGBUILD` and `.SRCINFO` configuring standard CMake compilation and FHS installation for pacman.

---

## Setting Up the Homebrew Tap

To maintain a dedicated Homebrew tap:
1. Create a public repository named `homebrew-snipcli` on GitHub under the `chowdhuryrifat` account.
2. Place `packaging/homebrew/snipcli.rb` in `Formula/snipcli.rb` in that repo.
3. Users install via:
   ```bash
   brew tap chowdhuryrifat/snipcli
   brew install snipcli
   ```

---

## Build Automation

All distribution packages are integrated into the release automation workflow (`.github/workflows/release.yml`) and validated by the packaging smoke test script (`scripts/packaging_smoke.sh`).
