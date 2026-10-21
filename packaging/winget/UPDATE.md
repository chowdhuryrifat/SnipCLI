# Updating SnipCLI on WinGet (`microsoft/winget-pkgs`)

This guide describes how to publish new versions of **SnipCLI** to the official Microsoft Community Package Repository (`microsoft/winget-pkgs`) using `wingetcreate`.

---

## Prerequisites

1. Install `winget-create`:
   ```powershell
   winget install Microsoft.WingetCreate
   ```
2. Verify installation:
   ```powershell
   wingetcreate --version
   ```
3. A GitHub Personal Access Token (classic or fine-grained with `public_repo` scope) to allow `wingetcreate` to fork, commit, and submit PRs to `microsoft/winget-pkgs`.

---

## Submitting New Version Updates

Once a new release tag (e.g., `v1.0.0`) is published on GitHub Releases and the Windows archive (`snipcli-v1.0.0-windows-x86_64.zip`) is attached:

```powershell
wingetcreate update ChowdhuryRifat.SnipCLI `
  --version 1.0.0 `
  --urls "https://github.com/chowdhuryrifat/SnipCLI/releases/download/v1.0.0/snipcli-v1.0.0-windows-x86_64.zip" `
  --token "<YOUR_GITHUB_TOKEN>" `
  --submit
```

`wingetcreate` automatically:
1. Downloads the release `.zip` from GitHub Releases.
2. Calculates the SHA-256 cryptographic hash.
3. Updates the multi-file manifest YAML files to the current schema.
4. Forks `microsoft/winget-pkgs`, creates a branch, commits the manifest changes, and opens a Pull Request.

---

## Local Validation (Optional)

To test and validate the WinGet manifest files locally before submitting:

```powershell
winget validate packaging/winget/
```
