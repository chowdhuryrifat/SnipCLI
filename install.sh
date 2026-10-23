#!/usr/bin/env bash
set -euo pipefail

# SnipCLI Official APT Repository Setup & Installation Script
# https://github.com/chowdhuryrifat/SnipCLI

REPO_URL="https://chowdhuryrifat.github.io/SnipCLI"
KEYRING_DIR="/etc/apt/keyrings"
KEYRING_FILE="${KEYRING_DIR}/snipcli-archive-keyring.gpg"
SOURCES_FILE="/etc/apt/sources.list.d/snipcli.list"

echo "=========================================================="
echo "           SnipCLI APT Repository Setup                   "
echo "=========================================================="

if [ "$(id -u)" -ne 0 ]; then
    echo "ERROR: This script must be run with root privileges (sudo)." >&2
    echo "Usage: curl -fsSL ${REPO_URL}/install.sh | sudo bash" >&2
    exit 1
fi

ARCH=$(dpkg --print-architecture 2>/dev/null || uname -m)
case "$ARCH" in
    amd64|x86_64)
        REPO_ARCH="amd64"
        ;;
    *)
        echo "ERROR: Unsupported architecture '$ARCH'. SnipCLI APT currently supports amd64." >&2
        exit 1
        ;;
esac

command_exists() { command -v "$1" >/dev/null 2>&1; }

MISSING_DEPS=()
if ! command_exists curl && ! command_exists wget; then
    MISSING_DEPS+=(curl)
fi
if ! command_exists gpg; then
    MISSING_DEPS+=(gpg)
fi

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    echo "--> Installing missing prerequisites: ${MISSING_DEPS[*]} ca-certificates..."
    apt-get update -qq
    apt-get install -y -qq --no-install-recommends "${MISSING_DEPS[@]}" ca-certificates
fi

echo "--> Installing SnipCLI signing keyring to ${KEYRING_FILE}..."
install -m 0755 -d "${KEYRING_DIR}"
if command_exists curl; then
    curl -fsSL "${REPO_URL}/snipcli-archive-keyring.gpg" -o "${KEYRING_FILE}"
else
    wget -qO "${KEYRING_FILE}" "${REPO_URL}/snipcli-archive-keyring.gpg"
fi
chmod 0644 "${KEYRING_FILE}"

echo "--> Writing repository configuration to ${SOURCES_FILE}..."
cat <<SOURCES_EOF > "${SOURCES_FILE}"
# SnipCLI Official APT Repository
deb [arch=${REPO_ARCH} signed-by=${KEYRING_FILE}] ${REPO_URL} stable main
SOURCES_EOF
chmod 0644 "${SOURCES_FILE}"

echo "--> Updating APT package index for SnipCLI..."
apt-get update -o Dir::Etc::sourcelist="${SOURCES_FILE}" -o Dir::Etc::sourceparts="-" -o APT::Get::List-Cleanup="0" -qq

if [ "${1:-}" != "--no-install" ]; then
    echo "--> Installing snipcli..."
    apt-get install -y snipcli
    echo ""
    echo "=========================================================="
    echo "✓ SnipCLI successfully installed!"
    if command_exists snip; then
        echo "  Version: $(snip --version 2>&1 || true)"
    fi
    echo "  Run 'snip help' to explore commands."
    echo "=========================================================="
else
    echo ""
    echo "=========================================================="
    echo "✓ SnipCLI repository configured successfully!"
    echo "  To install now: sudo apt install snipcli"
    echo "=========================================================="
fi
