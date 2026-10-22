#!/usr/bin/env bash
# scripts/finalize_release_metadata.sh
# Automate filling release checksums in downstream package manifests and generate dist-submit trees.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

TAG=""
SUMS_FILE=""
CHECK_URLS=false
DRY_RUN=false

usage() {
    cat << 'EOF'
Usage: scripts/finalize_release_metadata.sh <tag> [options]

Arguments:
  <tag>                  Release tag (e.g. v1.0.0)

Options:
  --sums-file <path>     Path to SHA256SUMS.txt (default: search local or download via gh)
  --check-urls           Verify each asset URL resolves via HTTP (curl -sIL)
  --dry-run              Print the manifest diffs without modifying files or writing dist-submit/
  -h, --help             Show this help message
EOF
    exit 1
}

# Parse options
while [[ $# -gt 0 ]]; do
    case "$1" in
        --sums-file)
            SUMS_FILE="$2"
            shift 2
            ;;
        --check-urls)
            CHECK_URLS=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        -*)
            echo "ERROR: Unknown option: $1" >&2
            usage
            ;;
        *)
            if [ -z "${TAG}" ]; then
                TAG="$1"
            else
                echo "ERROR: Unexpected argument: $1" >&2
                usage
            fi
            shift
            ;;
    esac
done

if [ -z "${TAG}" ]; then
    echo "ERROR: Missing required <tag> argument." >&2
    usage
fi

# Normalize tag and version
if [[ "${TAG}" != v* ]]; then
    VERSION="${TAG}"
    TAG="v${VERSION}"
else
    VERSION="${TAG#v}"
fi

echo "=== Finalizing Release Metadata for SnipCLI ${TAG} (version ${VERSION}) ==="

# Resolve SHA256SUMS.txt
if [ -z "${SUMS_FILE}" ]; then
    CANDIDATES=(
        "${REPO_DIR}/SHA256SUMS.txt"
        "${REPO_DIR}/dist/SHA256SUMS.txt"
        "${REPO_DIR}/release_assets/SHA256SUMS.txt"
        "SHA256SUMS.txt"
    )
    for c in "${CANDIDATES[@]}"; do
        if [ -f "$c" ]; then
            SUMS_FILE="$c"
            break
        fi
    done
fi

if [ -z "${SUMS_FILE}" ] || [ ! -f "${SUMS_FILE}" ]; then
    if command -v gh >/dev/null 2>&1; then
        echo "Attempting to download SHA256SUMS.txt for ${TAG} via gh release download..."
        TMP_SUMS=$(mktemp)
        if gh release download "${TAG}" -p "SHA256SUMS.txt" -O "${TMP_SUMS}" 2>/dev/null; then
            SUMS_FILE="${TMP_SUMS}"
            echo "Downloaded SHA256SUMS.txt successfully."
        fi
    fi
fi

if [ -z "${SUMS_FILE}" ] || [ ! -f "${SUMS_FILE}" ]; then
    echo "ERROR: SHA256SUMS.txt could not be located. Provide via --sums-file <path>." >&2
    exit 1
fi

echo "Using SHA256SUMS file: ${SUMS_FILE}"

# Extract hashes from sums file
# Expected asset formats:
#   snipcli-v1.0.0-windows-x86_64.zip
#   snipcli-v1.0.0-linux-x86_64.tar.gz
#   snipcli-v1.0.0-macos-universal.tar.gz
#   snipcli_1.0.0_amd64.deb
#   Source tarball: SnipCLI-v1.0.0.tar.gz or v1.0.0.tar.gz or snipcli-1.0.0.tar.gz

lookup_hash() {
    local pattern="$1"
    local match
    match=$(grep -E "${pattern}" "${SUMS_FILE}" | head -n 1 | awk '{print $1}')
    echo "${match}"
}

WIN_ZIP_HASH=$(lookup_hash "(snipcli-.*windows.*\.zip|windows.*\.zip)")
SRC_TAR_HASH=$(lookup_hash "(SnipCLI-.*\.tar\.gz|source.*\.tar\.gz|snipcli-${VERSION}\.tar\.gz|${TAG}\.tar\.gz)")

# Fallback: check if source archive can be computed or extracted
if [ -z "${SRC_TAR_HASH}" ]; then
    # Look for any .tar.gz that is not linux or macos dist
    SRC_TAR_HASH=$(grep -vE "(linux|macos|darwin)" "${SUMS_FILE}" | grep -E "\.tar\.gz" | head -n 1 | awk '{print $1}' || true)
fi

validate_sha256() {
    local name="$1"
    local hash="$2"
    if [ -z "${hash}" ]; then
        echo "ERROR: Could not find SHA256 for ${name} in ${SUMS_FILE}" >&2
        exit 1
    fi
    if ! [[ "${hash}" =~ ^[0-9a-fA-F]{64}$ ]]; then
        echo "ERROR: Invalid SHA256 hash format for ${name}: '${hash}' (expected 64 hex characters)" >&2
        exit 1
    fi
    echo "  [FOUND] ${name} -> ${hash}"
}

echo "Resolving asset checksums:"
validate_sha256 "Windows ZIP (<SHA256_WINDOWS_ZIP>)" "${WIN_ZIP_HASH}"
validate_sha256 "Source Tarball (<SHA256_SOURCE_TARBALL>)" "${SRC_TAR_HASH}"

# URL verification if requested
if [ "${CHECK_URLS}" = true ]; then
    echo "=== Verifying Asset URLs (--check-urls) ==="
    URLS_TO_CHECK=(
        "https://github.com/chowdhuryrifat/SnipCLI/releases/download/${TAG}/snipcli-${TAG}-windows-x86_64.zip"
        "https://github.com/chowdhuryrifat/SnipCLI/archive/refs/tags/${TAG}.tar.gz"
        "https://github.com/chowdhuryrifat/SnipCLI/releases/download/${TAG}/snipcli-${TAG}-linux-x86_64.tar.gz"
        "https://github.com/chowdhuryrifat/SnipCLI/releases/download/${TAG}/snipcli-${TAG}-macos-universal.tar.gz"
        "https://github.com/chowdhuryrifat/SnipCLI/releases/download/${TAG}/snipcli_${VERSION}_amd64.deb"
    )
    for url in "${URLS_TO_CHECK[@]}"; do
        echo -n "Checking ${url} ... "
        HTTP_CODE=$(curl -sIL -o /dev/null -w "%{http_code}" "${url}" || echo "000")
        if [[ "${HTTP_CODE}" =~ ^(200|302)$ ]]; then
            echo "OK (${HTTP_CODE})"
        else
            echo "WARNING: HTTP status ${HTTP_CODE} (Release may be draft or not yet published)"
        fi
    done
fi

# Prepare substitutions
TMP_WORK="$(mktemp -d)"
trap 'rm -rf "${TMP_WORK}"' EXIT

mkdir -p "${TMP_WORK}/winget" "${TMP_WORK}/homebrew" "${TMP_WORK}/aur"

# 1. WinGet
cp "${REPO_DIR}/packaging/winget/"*.yaml "${TMP_WORK}/winget/"
sed -i "s/<SHA256_WINDOWS_ZIP>/${WIN_ZIP_HASH}/g" "${TMP_WORK}/winget/ChowdhuryRifat.SnipCLI.installer.yaml"

# 2. Homebrew
cp "${REPO_DIR}/packaging/homebrew/snipcli.rb" "${TMP_WORK}/homebrew/snipcli.rb"
sed -i "s/<SHA256_SOURCE_TARBALL>/${SRC_TAR_HASH}/g" "${TMP_WORK}/homebrew/snipcli.rb"

# 3. AUR PKGBUILD & .SRCINFO
cp "${REPO_DIR}/packaging/aur/PKGBUILD" "${TMP_WORK}/aur/PKGBUILD"
sed -i "s/<SHA256_SOURCE_TARBALL>/${SRC_TAR_HASH}/g" "${TMP_WORK}/aur/PKGBUILD"

if command -v makepkg >/dev/null 2>&1; then
    (cd "${TMP_WORK}/aur" && makepkg --printsrcinfo > .SRCINFO)
else
    # Fallback reproduction of .SRCINFO with identical formatting
    cp "${REPO_DIR}/packaging/aur/.SRCINFO" "${TMP_WORK}/aur/.SRCINFO"
    sed -i "s/<SHA256_SOURCE_TARBALL>/${SRC_TAR_HASH}/g" "${TMP_WORK}/aur/.SRCINFO"
fi

# Verify no remaining <SHA256_*> placeholders
REMAINING=$(grep -rn "<SHA256_" "${TMP_WORK}" || true)
if [ -n "${REMAINING}" ]; then
    echo "ERROR: Unreplaced placeholders detected:" >&2
    echo "${REMAINING}" >&2
    exit 1
fi

echo "Verified all placeholders successfully replaced."

# Dry run diff
if [ "${DRY_RUN}" = true ]; then
    echo "=== Dry Run Mode: Diffs against repository manifests ==="
    echo "--- WinGet installer diff ---"
    diff -u "${REPO_DIR}/packaging/winget/ChowdhuryRifat.SnipCLI.installer.yaml" "${TMP_WORK}/winget/ChowdhuryRifat.SnipCLI.installer.yaml" || true
    echo "--- Homebrew formula diff ---"
    diff -u "${REPO_DIR}/packaging/homebrew/snipcli.rb" "${TMP_WORK}/homebrew/snipcli.rb" || true
    echo "--- AUR PKGBUILD diff ---"
    diff -u "${REPO_DIR}/packaging/aur/PKGBUILD" "${TMP_WORK}/aur/PKGBUILD" || true
    echo "--- AUR .SRCINFO diff ---"
    diff -u "${REPO_DIR}/packaging/aur/.SRCINFO" "${TMP_WORK}/aur/.SRCINFO" || true
    echo "=== Dry Run Completed (No files written) ==="
    exit 0
fi

# Apply in-place to packaging/ manifests
cp "${TMP_WORK}/winget/"*.yaml "${REPO_DIR}/packaging/winget/"
cp "${TMP_WORK}/homebrew/snipcli.rb" "${REPO_DIR}/packaging/homebrew/snipcli.rb"
cp "${TMP_WORK}/aur/PKGBUILD" "${REPO_DIR}/packaging/aur/PKGBUILD"
cp "${TMP_WORK}/aur/.SRCINFO" "${REPO_DIR}/packaging/aur/.SRCINFO"

# Build ready-to-copy dist-submit/ trees
DIST_SUBMIT="${REPO_DIR}/dist-submit"
rm -rf "${DIST_SUBMIT}"
mkdir -p "${DIST_SUBMIT}"

# 1. WinGet: manifests/c/ChowdhuryRifat/SnipCLI/<version>/
WINGET_TARGET="${DIST_SUBMIT}/winget/manifests/c/ChowdhuryRifat/SnipCLI/${VERSION}"
mkdir -p "${WINGET_TARGET}"
cp "${TMP_WORK}/winget/"*.yaml "${WINGET_TARGET}/"

# 2. Homebrew: homebrew-snipcli/Formula/snipcli.rb + README.md
BREW_TARGET="${DIST_SUBMIT}/homebrew-snipcli/Formula"
mkdir -p "${BREW_TARGET}"
cp "${TMP_WORK}/homebrew/snipcli.rb" "${BREW_TARGET}/snipcli.rb"

cat << EOF > "${DIST_SUBMIT}/homebrew-snipcli/README.md"
# Homebrew Tap for SnipCLI

Official Homebrew tap for **SnipCLI** — Fast, modern C++17 CLI snippet manager for competitive programmers.

## Installation

\`\`\`bash
brew tap chowdhuryrifat/snipcli
brew install snipcli
\`\`\`

## Verify Installation

\`\`\`bash
snip --version
\`\`\`
EOF

# 3. AUR: aur/ (PKGBUILD + .SRCINFO)
AUR_TARGET="${DIST_SUBMIT}/aur"
mkdir -p "${AUR_TARGET}"
cp "${TMP_WORK}/aur/PKGBUILD" "${AUR_TARGET}/PKGBUILD"
cp "${TMP_WORK}/aur/.SRCINFO" "${AUR_TARGET}/.SRCINFO"

echo "=== Packaging manifests updated in packaging/ ==="
echo "=== Submission trees generated in dist-submit/ ==="
find "${DIST_SUBMIT}" -type f | sort | while read -r f; do
    echo "  ${f#${REPO_DIR}/}"
done

echo "SUCCESS: Release metadata finalization complete for ${TAG}."
