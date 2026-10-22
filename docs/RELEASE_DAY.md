# SnipCLI v1.0.0 Release Day Runbook

This document is the exact, step-by-step operational runbook for the human maintainer releasing **SnipCLI v1.0.0**. Every step provides the exact GitHub Web UI actions or copy-paste terminal commands with expected outputs.

---

## Pre-Flight Checklist

Before beginning, ensure you have:
- [ ] Reviewed `docs/internal/PUBLIC_READINESS.md` and resolved all personal data/academic metadata decisions.
- [ ] SSH key configured and authenticated with GitHub (`ssh -T git@github.com`).
- [ ] GitHub CLI (`gh`) installed and authenticated (`gh auth status`).
- [ ] Clean local repository clone on `release/v1.0.0-final`.

---

## Phase 1: Repository Configuration & Public Transition

### Step 1.1: Enable Private Vulnerability Reporting **[GitHub web UI]**
1. Navigate to: `https://github.com/chowdhuryrifat/SnipCLI/settings`
2. In the left sidebar, click **Code security and analysis** (under Security).
3. Under **Private vulnerability reporting**, click **Enable**.
- **Expected Outcome:** The status displays "Private vulnerability reporting is enabled". A "Report a vulnerability" button becomes active under the Security tab.

### Step 1.2: Configure Branch Protection Rules **[GitHub web UI]**
1. Navigate to: `https://github.com/chowdhuryrifat/SnipCLI/settings/branches`
2. Under **Branch protection rules**, click **Add rule** (or edit existing rules for `main` and `dev`):
   - **Branch name pattern:** `main`
   - Check **Require a pull request before merging**
   - Check **Require approvals** (Set to `1`)
   - Check **Require status checks to pass before merging**
   - In status checks search box, select:
     - `Code Quality & Static Analysis`
     - `Build & Test Matrix (ubuntu-24.04 - GCC)`
     - `Build & Test Matrix (ubuntu-24.04 - GCC-Sanitizers)`
     - `Build & Test Matrix (ubuntu-24.04 - GCC-TSan)`
     - `Build & Test Matrix (macos-14 - AppleClang)`
     - `Build & Test Matrix (windows-latest - MSVC)`
   - Check **Require branches to be up to date before merging**
   - Click **Save changes**
3. Repeat the same protection rule configuration for `dev`.
- **Expected Outcome:** Both `main` and `dev` show active protection rules requiring CI green status and 1 approving peer review.

### Step 1.3: Make Repository Public **[GitHub web UI]**
> [!CAUTION]
> Verify that you have read `docs/internal/PUBLIC_READINESS.md` before executing this step.
1. Navigate to: `https://github.com/chowdhuryrifat/SnipCLI/settings`
2. Scroll to the bottom to the **Danger Zone**.
3. Next to **Change repository visibility**, click **Change visibility** -> **Change to public**.
4. Type `chowdhuryrifat/SnipCLI` to confirm and click **I understand, make this repository public**.
- **Expected Outcome:** Repository badge changes from "Private" to "Public".

---

## Phase 2: PR Stack Merging

### Step 2.1: Review and Merge Stack PRs in Order **[terminal]**

The feature and release preparation PR stack must be merged into `dev` in strict dependency order:

```bash
# 1. Inspect existing open pull requests
gh pr list --state open

# Expected output shows stack:
# #23 release/v1.0.0-final -> release/v1.0.0-packaging (or dev)

# 2. Check CI status of the head PR
gh pr checks <PR_NUMBER>

# Expected outcome: All 6 CI jobs pass (green)

# 3. Merge PR into its base branch
gh pr merge <PR_NUMBER> --merge --delete-branch
```

> [!NOTE]
> When a parent branch in a stack is merged, GitHub automatically retargets any child PRs targeting that branch to the new upstream branch (e.g. `dev`). Verify the base branch before merging child PRs.

### Step 2.2: Fast-Forward or Merge `dev` into `main` **[terminal]**

```bash
# Fetch latest commits
git checkout dev
git pull origin dev

# Verify local test suite on latest dev tip
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure
bash tests/smoke/smoke_test.sh build/snip

# Switch to main and merge dev
git checkout main
git pull origin main
git merge --ff-only dev || git merge dev -m "chore(release): merge dev into main for v1.0.0 release"

# Push main to origin
git push origin main

# Confirm CI passes on the merge commit on main
gh run list --branch main --limit 1
```
- **Expected Outcome:** All CI jobs complete with `conclusion: success` on `main`.

---

## Phase 3: Changelog Date, Tagging, and Release Workflow

### Step 3.1: Finalize CHANGELOG Release Date **[terminal]**

```bash
git checkout main
git pull origin main

# Update release date from placeholder to today (YYYY-MM-DD)
TODAY=$(date +%Y-%m-%d)
sed -i "s/## \[1.0.0\] - YYYY-MM-DD/## [1.0.0] - ${TODAY}/" CHANGELOG.md

# Verify diff
git diff CHANGELOG.md

# Commit and push directly to main (or via fast PR)
git add CHANGELOG.md
git commit -m "chore(release): set v1.0.0 release date to ${TODAY}"
git push origin main
```
- **Expected Output:**
  ```diff
  -## [1.0.0] - YYYY-MM-DD
  +## [1.0.0] - 2026-10-22
  ```

### Step 3.2: Verify Version Alignment and Create Annotated Tag **[terminal]**

```bash
# 1. Verify CMakeLists.txt version matches target release
CMAKE_VER=$(grep -E 'project\(SnipCLI' CMakeLists.txt -A 2 | grep -E 'VERSION' | sed -E 's/.*VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/')
echo "CMake project version: ${CMAKE_VER}"
[ "${CMAKE_VER}" = "1.0.0" ] || { echo "ERROR: Version mismatch!"; exit 1; }

# 2. Create signed (or annotated) release tag
git tag -a v1.0.0 -m "SnipCLI release v1.0.0"
# (Or if GPG signing is configured: git tag -s v1.0.0 -m "SnipCLI release v1.0.0")

# 3. Push tag to origin
git push origin v1.0.0

# 4. Watch the release automation workflow
gh run watch $(gh run list --workflow=release.yml --limit 1 --json databaseId --jq '.[0].databaseId')
```
- **Expected Outcome:** The `release.yml` workflow completes all jobs successfully:
  - `verify` (Unit tests + Smoke tests + Audit suites)
  - `build-linux` (Linux tarball + `.deb` package + `objdump` GLIBC check + `dpkg -I` / `dpkg -c`)
  - `build-macos` (Universal `arm64` + `x86_64` binary)
  - `build-windows` (Static CRT MSVC zip)
  - `draft-release` (Creates draft release with assets and `SHA256SUMS.txt`)

---

## Phase 4: Publish GitHub Draft Release

### Step 4.1: Review Draft Release and Publish **[GitHub web UI]**
1. Navigate to: `https://github.com/chowdhuryrifat/SnipCLI/releases`
2. Click **Edit** on the `v1.0.0` Draft Release.
3. Verify all 5 distribution assets are attached:
   - `snipcli-v1.0.0-linux-x86_64.tar.gz`
   - `snipcli_1.0.0_amd64.deb`
   - `snipcli-v1.0.0-macos-universal.tar.gz`
   - `snipcli-v1.0.0-windows-x86_64.zip`
   - `SHA256SUMS.txt`
4. Confirm release notes accurately list key highlights.
5. Uncheck **Set as a draft** (leave **Set as the latest release** checked).
6. Click **Publish release**.
- **Expected Outcome:** Release `v1.0.0` is public and tagged as `Latest`.

---

## Phase 5: Downstream Packaging Automation

### Step 5.1: Run Release Metadata Finalization **[terminal]**

Now that the release and source archive are published, execute the metadata finalization script to update manifests and generate submission trees:

```bash
git checkout main
git pull origin main

# Download SHA256SUMS.txt and verify asset URLs
gh release download v1.0.0 -p "SHA256SUMS.txt" -O SHA256SUMS.txt

# Run metadata finalizer with URL check
bash scripts/finalize_release_metadata.sh v1.0.0 --sums-file SHA256SUMS.txt --check-urls

# Review modified packaging manifests
git diff packaging/

# Create branch, commit, and submit PR for manifest checksums
git checkout -b chore/finalize-v1.0.0-manifests
git add packaging/
git commit -m "chore(packaging): finalize v1.0.0 release hashes and manifests"
git push -u origin chore/finalize-v1.0.0-manifests
gh pr create --title "chore(packaging): finalize v1.0.0 release hashes and manifests" --body "Populates official release checksums from v1.0.0 SHA256SUMS.txt across WinGet, Homebrew, and AUR manifests."
```
- **Expected Outcome:** `packaging/` files have valid 64-character SHA-256 hashes instead of `<SHA256_*>` placeholders. `dist-submit/` contains ready-to-copy package trees. Merge the PR after CI passes.

---

## Phase 6: Channel Submissions

### Step 6.1: Homebrew Tap Repository **[GitHub web UI & terminal]**

1. **[GitHub web UI]** Create public repository: `chowdhuryrifat/homebrew-snipcli`
   - Description: "Homebrew tap for SnipCLI"
   - Public: Yes
   - Initialize with README: No (will be pushed)
2. **[terminal]** Push tap contents from `dist-submit/homebrew-snipcli/`:
   ```bash
   cd dist-submit/homebrew-snipcli
   git init
   git branch -M main
   git add Formula/ README.md
   git commit -m "feat(formula): add snipcli v1.0.0 formula"
   git remote add origin git@github.com:chowdhuryrifat/homebrew-snipcli.git
   git push -u origin main
   cd ../..
   ```
3. **[terminal]** Test installation via Homebrew:
   ```bash
   brew tap chowdhuryrifat/snipcli
   brew install snipcli
   snip --version
   # Expected: SnipCLI version 1.0.0 (Modern C++17 MVC Architecture)
   ```

### Step 6.2: WinGet Package Submission **[terminal]**

Option A (via `wingetcreate`):
```bash
# Install wingetcreate if not already present
# Download from https://github.com/microsoft/winget-create/releases
wingetcreate submit \
  --token "<YOUR_GITHUB_TOKEN>" \
  --urls "https://github.com/chowdhuryrifat/SnipCLI/releases/download/v1.0.0/snipcli-v1.0.0-windows-x86_64.zip"
```

Option B (Manual PR into `microsoft/winget-pkgs`):
```bash
# 1. Fork microsoft/winget-pkgs on GitHub
# 2. Clone fork locally
git clone git@github.com:<YOUR_USERNAME>/winget-pkgs.git /tmp/winget-pkgs
cd /tmp/winget-pkgs
git checkout -b snipcli-1.0.0

# 3. Copy manifests generated in dist-submit
mkdir -p manifests/c/ChowdhuryRifat/SnipCLI/1.0.0
cp -r /path/to/SnipCLI/dist-submit/winget/manifests/c/ChowdhuryRifat/SnipCLI/1.0.0/* manifests/c/ChowdhuryRifat/SnipCLI/1.0.0/

# 4. Commit and push
git add manifests/c/ChowdhuryRifat/SnipCLI/1.0.0/
git commit -m "New version: ChowdhuryRifat.SnipCLI version 1.0.0"
git push -u origin snipcli-1.0.0

# 5. Open PR to microsoft/winget-pkgs
gh pr create --repo microsoft/winget-pkgs --title "New version: ChowdhuryRifat.SnipCLI version 1.0.0"
```
- **Expected PR Checks in `microsoft/winget-pkgs`:**
  - `Azure Pipelines` validation runs schema checks, installs the package, and verifies portable command alias `snip`.
  - Typical reviewer feedback: Check for correct licensing link (`https://github.com/chowdhuryrifat/SnipCLI/blob/main/LICENSE`) and release notes link. Manifests generated by `finalize_release_metadata.sh` satisfy these requirements.

### Step 6.3: Arch Linux AUR Package Submission **[terminal]**

Prerequisites:
- Registered account at `https://aur.archlinux.org/`
- SSH public key uploaded to AUR profile (`https://aur.archlinux.org/account/<username>/edit`)

```bash
# 1. Clone AUR repository
cd /tmp
git clone ssh://aur@aur.archlinux.org/snipcli.git aur-snipcli
cd aur-snipcli

# 2. Copy PKGBUILD and .SRCINFO from dist-submit
cp /path/to/SnipCLI/dist-submit/aur/PKGBUILD .
cp /path/to/SnipCLI/dist-submit/aur/.SRCINFO .

# 3. Test build locally in Arch Linux container or host
makepkg -f

# 4. Commit and push to AUR
git add PKGBUILD .SRCINFO
git commit -m "Release v1.0.0"
git push origin master
```
- **Expected Outcome:** Package appears at `https://aur.archlinux.org/packages/snipcli/` and is installable with `yay -S snipcli`.

---

## Phase 7: Post-Release Documentation Flip

Once downstream packaging channels are live, open a checklist PR to update `README.md` to remove "Coming soon" notices and add installation badges:

### Exact Lines to Update in `README.md`:
1. **Line 94:** Change:
   ```markdown
   ### 3. Package Managers (Coming soon — available after the v1.0.0 release is published)
   ```
   to:
   ```markdown
   ### 3. Package Managers
   ```
2. **Line 99:** Change:
   ```bash
   # macOS & Linux (Homebrew) — Coming Soon
   ```
   to:
   ```bash
   # macOS & Linux (Homebrew)
   ```
3. **Line 103:** Change:
   ```bash
   # Windows (WinGet) — Coming Soon
   ```
   to:
   ```bash
   # Windows (WinGet)
   ```
4. **Line 106:** Change:
   ```bash
   # Debian / Ubuntu (.deb via Releases or PPA) — Coming Soon
   ```
   to:
   ```bash
   # Debian / Ubuntu (.deb via Releases)
   ```
5. **Line 109:** Change:
   ```bash
   # Arch Linux (AUR) — Coming Soon
   ```
   to:
   ```bash
   # Arch Linux (AUR)
   ```

---

## Phase 8: Rollback, Yanking & Hotfix Procedures

### 8.1 Channel Rollback and Yanking Reference

| Channel | Rollback / Yank Procedure | Notes |
| :--- | :--- | :--- |
| **GitHub Releases** | Click "Edit release" -> Check "This is a pre-release" or delete the release asset. Do NOT delete git tags if already consumed. | If a fatal flaw exists, mark release as broken and publish v1.0.1 hotfix immediately. |
| **Homebrew** | Update `Formula/snipcli.rb` in `chowdhuryrifat/homebrew-snipcli` to point to the corrected release URL and commit to `main`. | Takes effect on next `brew update`. |
| **WinGet** | Submit a PR to `microsoft/winget-pkgs` deleting the version directory `manifests/c/ChowdhuryRifat/SnipCLI/1.0.0/` or submitting `1.0.1`. | Microsoft prefers submitting higher version hotfix over deleting published versions. |
| **AUR** | Bump `pkgrel=2` with an updated patch or increment `pkgver=1.0.1`. Push to `aur@aur.archlinux.org:snipcli.git`. | Packages cannot be "deleted" from AUR by users without submitting an orphan/merge request to AUR administrators. |

### 8.2 v1.0.1 Hotfix Procedure
1. Branch from `v1.0.0` tag: `git checkout -b hotfix/1.0.1 v1.0.0`.
2. Apply minimal bugfix commit.
3. Bump `VERSION` in `CMakeLists.txt` to `1.0.1`.
4. Update `CHANGELOG.md` with `## [1.0.1] - YYYY-MM-DD`.
5. Run full local verification matrix.
6. Merge `hotfix/1.0.1` into `main` and `dev`.
7. Tag `git tag -a v1.0.1 -m "SnipCLI release v1.0.1"` and push `git push origin v1.0.1`.
8. Re-run `scripts/finalize_release_metadata.sh v1.0.1` and submit downstream updates.
