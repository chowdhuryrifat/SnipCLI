#!/usr/bin/env bash
set -euo pipefail

# update_apt_repo.sh
# Generates and maintains the official SnipCLI Debian/APT repository for GitHub Pages.
# https://github.com/chowdhuryrifat/SnipCLI

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

REPO_DIR="${PROJECT_ROOT}/gh-pages"
DEB_FILE=""
RELEASE_TAG=""
GPG_HOME=""
GPG_KEY_NAME="SnipCLI APT Archive"
BASE_URL="https://chowdhuryrifat.github.io/SnipCLI"

usage() {
    cat <<EOF
Usage: $(basename "$0") [OPTIONS]

Options:
  --repo-dir <dir>       Path to target APT repository directory (default: ${REPO_DIR})
  --deb <path>           Path to .deb package to add to repository pool
  --tag <tag>            Download .deb package from GitHub Release tag (e.g. v1.0.0)
  --gpg-home <dir>       GNUPGHOME directory containing signing key
  --gpg-key <name|id>    GPG key identifier or user ID (default: "${GPG_KEY_NAME}")
  --base-url <url>       Base URL where repo is served (default: ${BASE_URL})
  -h, --help             Show this help message
EOF
    exit 0
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --repo-dir) REPO_DIR="$2"; shift 2 ;;
        --deb) DEB_FILE="$2"; shift 2 ;;
        --tag) RELEASE_TAG="$2"; shift 2 ;;
        --gpg-home) GPG_HOME="$2"; shift 2 ;;
        --gpg-key) GPG_KEY_NAME="$2"; shift 2 ;;
        --base-url) BASE_URL="$2"; shift 2 ;;
        -h|--help) usage ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
done

echo "=== Building SnipCLI APT Repository ==="
echo "Target directory: ${REPO_DIR}"
echo "Base URL:         ${BASE_URL}"

# Setup temporary GNUPGHOME if APT_GPG_PRIVATE_KEY secret is provided in environment
TEMP_GPG_DIR=""
if [ -n "${APT_GPG_PRIVATE_KEY:-}" ] && [ -z "${GPG_HOME}" ]; then
    echo "Importing signing key from APT_GPG_PRIVATE_KEY environment variable..."
    TEMP_GPG_DIR="$(mktemp -d -t snipcli-gpg-XXXXXX)"
    chmod 700 "${TEMP_GPG_DIR}"
    export GNUPGHOME="${TEMP_GPG_DIR}"
    echo "${APT_GPG_PRIVATE_KEY}" | gpg --batch --import --quiet
    GPG_HOME="${TEMP_GPG_DIR}"
elif [ -n "${GPG_HOME}" ]; then
    export GNUPGHOME="${GPG_HOME}"
fi

cleanup() {
    if [ -n "${TEMP_GPG_DIR}" ] && [ -d "${TEMP_GPG_DIR}" ]; then
        rm -rf "${TEMP_GPG_DIR}"
    fi
}
trap cleanup EXIT

# Verify GPG signing key is available
if ! gpg --list-keys "${GPG_KEY_NAME}" >/dev/null 2>&1; then
    echo "ERROR: Signing key '${GPG_KEY_NAME}' not found in GNUPGHOME (${GNUPGHOME:-~/.gnupg})." >&2
    exit 1
fi
GPG_FINGERPRINT=$(gpg --with-colons --list-keys "${GPG_KEY_NAME}" | grep '^fpr:' | head -n 1 | cut -d: -f10)
echo "Using GPG key:    ${GPG_KEY_NAME} (${GPG_FINGERPRINT})"

# Ensure directory layout
POOL_DIR="${REPO_DIR}/pool/main/s/snipcli"
BINARY_DIR="${REPO_DIR}/dists/stable/main/binary-amd64"
mkdir -p "${POOL_DIR}" "${BINARY_DIR}"

# Download .deb if release tag was requested
if [ -n "${RELEASE_TAG}" ] && [ -z "${DEB_FILE}" ]; then
    echo "Downloading .deb asset for release ${RELEASE_TAG} via gh CLI..."
    TEMP_DL_DIR="$(mktemp -d -t snipcli-dl-XXXXXX)"
    gh release download "${RELEASE_TAG}" -p "snipcli_*_amd64.deb" -D "${TEMP_DL_DIR}"
    DEB_FILE=$(find "${TEMP_DL_DIR}" -name "snipcli_*_amd64.deb" -print -quit)
fi

# Copy deb into pool if provided
if [ -n "${DEB_FILE}" ] && [ -f "${DEB_FILE}" ]; then
    echo "Ingesting package: $(basename "${DEB_FILE}")"
    cp -f "${DEB_FILE}" "${POOL_DIR}/"
fi

# Ensure at least one deb exists in pool
DEB_COUNT=$(find "${POOL_DIR}" -maxdepth 1 -name "*.deb" | wc -l)
if [ "${DEB_COUNT}" -eq 0 ]; then
    echo "ERROR: No .deb packages found in ${POOL_DIR}." >&2
    exit 1
fi
echo "Packages in pool: ${DEB_COUNT}"

# Generate Packages & Packages.gz
echo "Generating Packages index..."
(
    cd "${REPO_DIR}"
    dpkg-scanpackages --multiversion pool/ > "${BINARY_DIR}/Packages"
    gzip -9fk "${BINARY_DIR}/Packages"
)

# Generate Release file
echo "Generating dists/stable/Release..."
STABLE_DIR="${REPO_DIR}/dists/stable"
DATE_STR=$(date -Ru)

if command -v apt-ftparchive >/dev/null 2>&1; then
    apt-ftparchive -c /dev/null \
        -o APT::FTPArchive::Release::Origin="SnipCLI" \
        -o APT::FTPArchive::Release::Label="SnipCLI" \
        -o APT::FTPArchive::Release::Suite="stable" \
        -o APT::FTPArchive::Release::Codename="stable" \
        -o APT::FTPArchive::Release::Architectures="amd64" \
        -o APT::FTPArchive::Release::Components="main" \
        -o APT::FTPArchive::Release::Description="Official SnipCLI Debian/Ubuntu Repository" \
        release "${STABLE_DIR}" > "${STABLE_DIR}/Release"
else
    # Fallback portable Release generator
    cat <<EOF > "${STABLE_DIR}/Release"
Architectures: amd64
Codename: stable
Components: main
Date: ${DATE_STR}
Description: Official SnipCLI Debian/Ubuntu Repository
Label: SnipCLI
Origin: SnipCLI
Suite: stable
SHA256:
EOF
    for rel_file in main/binary-amd64/Packages main/binary-amd64/Packages.gz; do
        if [ -f "${STABLE_DIR}/${rel_file}" ]; then
            F_SHA=$(sha256sum "${STABLE_DIR}/${rel_file}" | cut -d' ' -f1)
            F_SIZE=$(wc -c < "${STABLE_DIR}/${rel_file}" | tr -d ' ')
            printf " %s %16s %s\n" "${F_SHA}" "${F_SIZE}" "${rel_file}" >> "${STABLE_DIR}/Release"
        fi
    done
fi

# Sign Release file: InRelease (clearsign) and Release.gpg (detached)
echo "Cryptographically signing Release metadata with GPG..."
rm -f "${STABLE_DIR}/InRelease" "${STABLE_DIR}/Release.gpg"
gpg --batch --yes --default-key "${GPG_KEY_NAME}" --clearsign -o "${STABLE_DIR}/InRelease" "${STABLE_DIR}/Release"
gpg --batch --yes --default-key "${GPG_KEY_NAME}" -abs -o "${STABLE_DIR}/Release.gpg" "${STABLE_DIR}/Release"

# Export public keys to repository root
echo "Exporting public keyring files..."
gpg --armor --export "${GPG_KEY_NAME}" > "${REPO_DIR}/KEY.gpg"
gpg --export "${GPG_KEY_NAME}" > "${REPO_DIR}/snipcli-archive-keyring.gpg"
chmod 0644 "${REPO_DIR}/KEY.gpg" "${REPO_DIR}/snipcli-archive-keyring.gpg"

# Create .nojekyll for GitHub Pages
touch "${REPO_DIR}/.nojekyll"

# Generate 1-liner installer script (install.sh)
echo "Generating install.sh..."
cat << 'EOF' > "${REPO_DIR}/install.sh"
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
EOF
chmod 0755 "${REPO_DIR}/install.sh"

# Generate landing page index.html
echo "Generating index.html..."
cat << 'EOF' > "${REPO_DIR}/index.html"
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SnipCLI APT Repository</title>
  <style>
    :root {
      --bg: #0d1117;
      --card-bg: #161b22;
      --border: #30363d;
      --text: #c9d1d9;
      --heading: #f0f6fc;
      --accent: #58a6ff;
      --green: #238636;
      --code-bg: #090d13;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Helvetica, Arial, sans-serif;
      background: var(--bg);
      color: var(--text);
      line-height: 1.6;
      padding: 2rem 1rem;
    }
    .container {
      max-width: 860px;
      margin: 0 auto;
    }
    header {
      text-align: center;
      margin-bottom: 2.5rem;
    }
    h1 {
      font-size: 2.4rem;
      color: var(--heading);
      margin-bottom: 0.5rem;
    }
    .tagline {
      font-size: 1.15rem;
      color: #8b949e;
    }
    .card {
      background: var(--card-bg);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 1.75rem;
      margin-bottom: 1.75rem;
    }
    h2 {
      font-size: 1.35rem;
      color: var(--heading);
      margin-bottom: 1rem;
      display: flex;
      align-items: center;
      gap: 0.5rem;
    }
    pre {
      background: var(--code-bg);
      border: 1px solid var(--border);
      border-radius: 6px;
      padding: 1rem;
      overflow-x: auto;
      font-family: ui-monospace, SFMono-Regular, "SF Mono", Menlo, Consolas, monospace;
      font-size: 0.92rem;
      color: #7ee787;
      margin: 0.75rem 0;
    }
    code {
      font-family: ui-monospace, SFMono-Regular, "SF Mono", Menlo, Consolas, monospace;
      color: #79c0ff;
    }
    a {
      color: var(--accent);
      text-decoration: none;
    }
    a:hover {
      text-decoration: underline;
    }
    .badges {
      display: flex;
      gap: 0.5rem;
      justify-content: center;
      margin-top: 1rem;
      flex-wrap: wrap;
    }
    .badge {
      display: inline-block;
      padding: 0.25rem 0.6rem;
      border-radius: 20px;
      font-size: 0.8rem;
      font-weight: 600;
      background: #21262d;
      border: 1px solid var(--border);
      color: #8b949e;
    }
    footer {
      text-align: center;
      color: #8b949e;
      font-size: 0.85rem;
      margin-top: 3rem;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>SnipCLI APT Repository</h1>
      <p class="tagline">Official Debian/Ubuntu package repository for SnipCLI</p>
      <div class="badges">
        <span class="badge">Debian / Ubuntu / Mint / Pop!_OS</span>
        <span class="badge">Architecture: amd64</span>
        <span class="badge">GPG Clearsigned</span>
      </div>
    </header>

    <div class="card">
      <h2>🚀 One-Line Installation (Recommended)</h2>
      <p>Configures the GPG keyring, registers the repository, updates your index, and installs SnipCLI:</p>
      <pre><code>curl -fsSL https://chowdhuryrifat.github.io/SnipCLI/install.sh | sudo bash</code></pre>
    </div>

    <div class="card">
      <h2>📦 Manual Repository Configuration</h2>
      <p>If you prefer to configure your APT sources manually:</p>

      <p style="margin-top: 0.75rem;"><strong>1. Download and install the archive signing keyring:</strong></p>
      <pre><code>sudo install -m 0755 -d /etc/apt/keyrings
curl -fsSL https://chowdhuryrifat.github.io/SnipCLI/snipcli-archive-keyring.gpg | sudo tee /etc/apt/keyrings/snipcli-archive-keyring.gpg &gt; /dev/null
sudo chmod 0644 /etc/apt/keyrings/snipcli-archive-keyring.gpg</code></pre>

      <p style="margin-top: 0.75rem;"><strong>2. Register the SnipCLI repository:</strong></p>
      <pre><code>echo "deb [arch=amd64 signed-by=/etc/apt/keyrings/snipcli-archive-keyring.gpg] https://chowdhuryrifat.github.io/SnipCLI stable main" | sudo tee /etc/apt/sources.list.d/snipcli.list &gt; /dev/null</code></pre>

      <p style="margin-top: 0.75rem;"><strong>3. Update and install:</strong></p>
      <pre><code>sudo apt update
sudo apt install snipcli</code></pre>
    </div>

    <div class="card">
      <h2>🔗 Links &amp; Other Platforms</h2>
      <ul style="padding-left: 1.5rem; margin-top: 0.5rem;">
        <li><a href="https://github.com/chowdhuryrifat/SnipCLI">GitHub Repository (chowdhuryrifat/SnipCLI)</a></li>
        <li><a href="https://github.com/chowdhuryrifat/SnipCLI/releases">Releases &amp; Release Notes</a></li>
        <li><strong>macOS &amp; Linux (Homebrew):</strong> <code>brew install chowdhuryrifat/snipcli/snipcli</code></li>
        <li><strong>Windows (WinGet):</strong> <code>winget install chowdhuryrifat.SnipCLI</code></li>
      </ul>
    </div>

    <footer>
      &copy; 2026 Chowdhury Rifat Ahmed &bull; SnipCLI is open-source under the MIT License.
    </footer>
  </div>
</body>
</html>
EOF

echo "=== APT Repository successfully prepared at ${REPO_DIR} ==="
