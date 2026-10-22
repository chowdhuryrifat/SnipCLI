# Homebrew Tap Guide for SnipCLI

This directory contains the official Homebrew formula (`snipcli.rb`) for installing **SnipCLI** on macOS and Linux via a custom Homebrew tap.

---

## 1. Setting Up the Tap Repository

1. Create a new public repository on GitHub named `homebrew-snipcli` under your GitHub account:
   `https://github.com/chowdhuryrifat/homebrew-snipcli`
2. Clone the tap repository locally:
   ```bash
   git clone https://github.com/chowdhuryrifat/homebrew-snipcli.git
   cd homebrew-snipcli
   mkdir -p Formula
   ```
3. Copy `snipcli.rb` into `Formula/snipcli.rb` in the tap repository:
   ```bash
   cp /path/to/SnipCLI/packaging/homebrew/snipcli.rb Formula/snipcli.rb
   ```
4. Commit and push:
   ```bash
   git add Formula/snipcli.rb
   git commit -m "feat: add snipcli formula v1.0.0"
   git push origin main
   ```

---

## 2. Auditing and Testing the Formula

Before publishing or upon updating releases, test the formula against Homebrew's official style and compilation gates:

```bash
# Run strict Homebrew formula audit
brew audit --strict --new Formula/snipcli.rb

# Test source build and installation
brew install --build-from-source Formula/snipcli.rb

# Run formula test block (verifies --version and snippet add/get)
brew test Formula/snipcli.rb
```

---

## 3. User Installation

Once published, users install SnipCLI directly with:

```bash
brew tap chowdhuryrifat/snipcli
brew install snipcli
```
