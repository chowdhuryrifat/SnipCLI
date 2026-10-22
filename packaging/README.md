# Distribution and Packaging

This directory contains packaging definitions, manifest templates, and build configuration files for distributing **SnipCLI** across package managers and operating systems.

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
- **`debian/`**: Debian package configuration and CPack settings for building `.deb` packages with correct control metadata, architectures, and dependencies.
- **`aur/`**: Arch Linux `PKGBUILD` and `.SRCINFO` configuring standard CMake compilation and FHS installation for pacman.

---

## Build Automation

All distribution packages are integrated into the release automation workflow (`.github/workflows/release.yml`) and validated by the packaging smoke test script (`scripts/packaging_smoke.sh`).
