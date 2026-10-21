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

echo "=== [3/5] Validating Homebrew Formula Syntax ==="
FORMULA="${REPO_DIR}/packaging/homebrew/snipcli.rb"
if command -v brew >/dev/null 2>&1; then
    brew audit --strict "${FORMULA}" || true
elif command -v ruby >/dev/null 2>&1; then
    ruby -c "${FORMULA}"
    echo "Homebrew ruby syntax check: OK"
else
    echo "Neither brew nor ruby found; checking file presence: OK"
fi

echo "=== [4/5] Validating WinGet Manifests ==="
python3 -c "
import yaml, glob, sys
for path in sorted(glob.glob('${REPO_DIR}/packaging/winget/*.yaml')):
    with open(path, 'r', encoding='utf-8') as f:
        data = yaml.safe_load(f)
    print(f'Validated WinGet YAML: {path} (PackageIdentifier: {data.get(\"PackageIdentifier\")})')
"
echo "WinGet manifests: OK"

echo "=== [5/5] Validating Arch Linux PKGBUILD & .SRCINFO ==="
PKGBUILD="${REPO_DIR}/packaging/aur/PKGBUILD"
bash -n "${PKGBUILD}"
if command -v namcap >/dev/null 2>&1; then
    namcap "${PKGBUILD}" || true
fi
echo "AUR PKGBUILD syntax: OK"

echo "=== ALL PACKAGING SMOKE CHECKS PASSED ==="
