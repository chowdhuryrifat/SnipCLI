#!/usr/bin/env bash
# scripts/packaging_smoke.sh — Validate packaging targets and manifests locally
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="/tmp/snipcli_packaging_smoke_build"
STAGE_DIR="/tmp/snipcli_packaging_smoke_stage"
DIST_DIR="/tmp/snipcli_packaging_smoke_dist"

rm -rf "${BUILD_DIR}" "${STAGE_DIR}" "${DIST_DIR}"
mkdir -p "${BUILD_DIR}" "${STAGE_DIR}" "${DIST_DIR}"

echo "=== [1/5] Building Release Binary & CPack Debian (.deb) Package ==="
cmake -S "${REPO_DIR}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DSNIPCLI_BUILD_TESTS=OFF
cmake --build "${BUILD_DIR}" -j"$(nproc 2>/dev/null || echo 4)"

(cd "${BUILD_DIR}" && cpack -G DEB)

DEB_PKG="$(ls -1 "${BUILD_DIR}"/snipcli_*.deb | head -n 1)"
echo "Generated DEB: ${DEB_PKG}"

echo "Verifying Debian package contents via dpkg -c..."
DEB_CONTENTS="$(dpkg -c "${DEB_PKG}")"
echo "${DEB_CONTENTS}"

REQUIRED_DEB_ENTRIES=(
    "usr/bin/snip"
    "usr/share/man/man1/snip.1"
    "usr/share/bash-completion/completions/snip"
    "usr/share/zsh/site-functions/_snip"
    "usr/share/fish/vendor_completions.d/snip.fish"
)

for entry in "${REQUIRED_DEB_ENTRIES[@]}"; do
    if ! echo "${DEB_CONTENTS}" | grep -q "${entry}"; then
        echo "ERROR: Missing expected Debian package entry: ${entry}" >&2
        exit 1
    fi
done
echo "Debian package structure: OK"

echo "=== [2/5] Building & Verifying Tarball (TGZ) Archive ==="
cmake --install "${BUILD_DIR}" --prefix "${STAGE_DIR}"
tar -czf "${DIST_DIR}/snipcli-test-linux-x86_64.tar.gz" -C "${STAGE_DIR}" .

TGZ_CONTENTS="$(tar -ztvf "${DIST_DIR}/snipcli-test-linux-x86_64.tar.gz")"
REQUIRED_TGZ_ENTRIES=(
    "bin/snip"
    "share/man/man1/snip.1"
    "share/bash-completion/completions/snip"
    "share/zsh/site-functions/_snip"
    "share/fish/vendor_completions.d/snip.fish"
)

for entry in "${REQUIRED_TGZ_ENTRIES[@]}"; do
    if ! echo "${TGZ_CONTENTS}" | grep -q "${entry}"; then
        echo "ERROR: Missing expected TGZ archive entry: ${entry}" >&2
        exit 1
    fi
done
echo "Tarball FHS layout: OK"

echo "=== [3/5] Checking Homebrew Formula File Presence ==="
FORMULA="${REPO_DIR}/packaging/homebrew/snipcli.rb"
if [ ! -f "${FORMULA}" ]; then
    echo "ERROR: Homebrew formula file missing: ${FORMULA}" >&2
    exit 1
fi
echo "Homebrew formula file presence check: OK"
if command -v brew >/dev/null 2>&1; then
    echo "Running brew audit --strict..."
    brew audit --strict "${FORMULA}"
elif command -v ruby >/dev/null 2>&1; then
    echo "Running ruby syntax check..."
    ruby -c "${FORMULA}"
    echo "Homebrew ruby syntax check: OK"
else
    echo "NOTE: Neither brew nor ruby found; full syntax/audit check skipped (presence check only)"
fi

echo "=== [4/5] Checking WinGet Manifests (Presence & PackageIdentifier) ==="
python3 -c "
import yaml, glob, sys
files = sorted(glob.glob('${REPO_DIR}/packaging/winget/*.yaml'))
if len(files) < 3:
    print(f'ERROR: Expected at least 3 WinGet manifest files, found {len(files)}', file=sys.stderr)
    sys.exit(1)
for path in files:
    with open(path, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    pkg_id = data.get('PackageIdentifier')
    if not pkg_id:
        print(f'ERROR: PackageIdentifier missing in {path}', file=sys.stderr)
        sys.exit(1)
    print(f'WinGet manifest presence check: {path} (PackageIdentifier: {pkg_id})')
"
echo "WinGet manifests presence check: OK (NOTE: winget validate NOT RUN, requires Windows environment)"

echo "=== [5/5] Checking Arch Linux PKGBUILD & .SRCINFO ==="
PKGBUILD="${REPO_DIR}/packaging/aur/PKGBUILD"
SRCINFO="${REPO_DIR}/packaging/aur/.SRCINFO"
if [ ! -f "${PKGBUILD}" ] || [ ! -f "${SRCINFO}" ]; then
    echo "ERROR: Missing Arch Linux PKGBUILD or .SRCINFO in ${REPO_DIR}/packaging/aur" >&2
    exit 1
fi
bash -n "${PKGBUILD}"
echo "AUR PKGBUILD bash syntax: OK"
if command -v namcap >/dev/null 2>&1; then
    echo "Running namcap on PKGBUILD..."
    namcap "${PKGBUILD}"
    echo "namcap check: OK"
else
    echo "NOTE: namcap not found; skipping namcap audit (presence check only)"
fi
echo "Arch Linux PKGBUILD & .SRCINFO presence check: OK"

echo "=== ALL PACKAGING SMOKE CHECKS PASSED ==="
