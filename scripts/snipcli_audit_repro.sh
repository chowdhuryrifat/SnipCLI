#!/usr/bin/env bash
# ==============================================================================
# SnipCLI Adversarial Security & Quality Audit Reproduction Suite
# Verifies resolution of findings: B1, B2, B3, B4, B6, B7, B8
# ==============================================================================

set -euo pipefail

BIN="${1:-./build/snip}"
TEMP_DIR=$(mktemp -d)

if command -v cygpath >/dev/null 2>&1; then
    TEST_STORAGE=$(cygpath -m "$TEMP_DIR/audit_snippets.json")
    TEMP_SRC=$(cygpath -m "$TEMP_DIR/raw_input.txt")
    TEMP_OUT=$(cygpath -m "$TEMP_DIR/raw_output.txt")
    export PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH"
else
    TEST_STORAGE="$TEMP_DIR/audit_snippets.json"
    TEMP_SRC="$TEMP_DIR/raw_input.txt"
    TEMP_OUT="$TEMP_DIR/raw_output.txt"
fi

export SNIPCLI_STORAGE_PATH="$TEST_STORAGE"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

pass() { echo -e "${GREEN}[PASS]${NC} $1"; }
fail() { echo -e "${RED}[FAIL]${NC} $1"; exit 1; }
info() { echo -e "${BLUE}[INFO]${NC} $1"; }

trap 'rm -rf "$TEMP_DIR"' EXIT

info "Running SnipCLI Audit Reproduction Test Suite using binary: $BIN"

# ------------------------------------------------------------------------------
# B1: Unquoted Multi-Word CLI Arguments & Positionals
# ------------------------------------------------------------------------------
info "Testing B1: Unquoted Multi-Word Positionals..."
$BIN add --title "Binary" --lang "cpp" --code "int a = 1;" > /dev/null
$BIN add --title "Search" --lang "cpp" --code "int b = 2;" > /dev/null
$BIN add --title "Binary Search" --lang "cpp" --code "int c = 3;" > /dev/null

# snip delete Binary Search must fail without deleting any snippet
if $BIN delete Binary Search > /dev/null 2>&1; then
    fail "B1: 'snip delete Binary Search' should have failed but succeeded!"
fi

# Verify none of the snippets were deleted
$BIN get --title "Binary" > /dev/null || fail "B1: Snippet 'Binary' was incorrectly deleted!"
$BIN get --title "Search" > /dev/null || fail "B1: Snippet 'Search' was incorrectly deleted!"
$BIN get --title "Binary Search" > /dev/null || fail "B1: Snippet 'Binary Search' was incorrectly deleted!"

# snip get 1 2 must fail
if $BIN get 1 2 > /dev/null 2>&1; then
    fail "B1: 'snip get 1 2' should have failed but succeeded!"
fi

# Quoted positional must succeed
$BIN delete "Binary Search" > /dev/null || fail "B1: Quoted 'Binary Search' delete failed!"
pass "B1: Unquoted multi-word CLI arguments properly rejected without unintended side-effects"

# ------------------------------------------------------------------------------
# B2: Option Schema & Valueless Option Handling
# ------------------------------------------------------------------------------
info "Testing B2: Valueless Option Handling..."
# Option followed by another option
if $BIN delete --id --title "Binary" > /dev/null 2>&1; then
    fail "B2: 'snip delete --id --title Binary' should have failed but succeeded!"
fi

# Option at end of line
if $BIN delete --id > /dev/null 2>&1; then
    fail "B2: 'snip delete --id' at end of arguments should have failed but succeeded!"
fi

# Option followed by flag
if $BIN add --title "Test" --code -h > /dev/null 2>&1; then
    fail "B2: 'snip add --title Test --code -h' should have failed but succeeded!"
fi

# Valueless short switch
if $BIN add -t > /dev/null 2>&1; then
    fail "B2: 'snip add -t' should have failed but succeeded!"
fi
pass "B2: Valueless options strictly validated and rejected"

# ------------------------------------------------------------------------------
# B3: Global Options Before Verb
# ------------------------------------------------------------------------------
info "Testing B3: Global Options Before Verb..."
# snip --format markdown list
OUTPUT_B3=$($BIN --format markdown list)
echo "$OUTPUT_B3" | grep -q "| Binary |" || fail "B3: '--format markdown list' failed to render snippet table!"
echo "$OUTPUT_B3" | grep -q "| ID | Title | Language | Lines | Tags |" || fail "B3: Markdown header missing!"

# snip -v list (version flag before verb)
OUTPUT_V=$($BIN -v list)
echo "$OUTPUT_V" | grep -qi "SnipCLI version" || fail "B3: '-v list' failed to render version!"
pass "B3: Global options before verb correctly parsed and routed"

# ------------------------------------------------------------------------------
# B4: Invalid --format Option Validation
# ------------------------------------------------------------------------------
info "Testing B4: Format Schema Validation..."
# Typos like markdwon
if $BIN --format markdwon list > /dev/null 2>&1; then
    fail "B4: 'snip --format markdwon list' should have failed but succeeded!"
fi

# Equals syntax with invalid format
if $BIN list --format=html > /dev/null 2>&1; then
    fail "B4: 'snip list --format=html' should have failed but succeeded!"
fi
pass "B4: Invalid --format specifications rejected with clear error"

# ------------------------------------------------------------------------------
# B6: Piped ANSI Escapes & NO_COLOR Support
# ------------------------------------------------------------------------------
info "Testing B6: NO_COLOR and Piped ANSI Escapes..."
# When NO_COLOR is set, no ANSI escape characters (\033 or \x1b) should appear
NO_COLOR=1 OUTPUT_NO_COLOR=$($BIN list)
if echo "$OUTPUT_NO_COLOR" | grep -q $'\033'; then
    fail "B6: Output contains ANSI escapes when NO_COLOR=1 is set!"
fi

# When output is redirected or piped to a file/grep, no ANSI escape should appear
$BIN list > "$TEMP_OUT"
if grep -q $'\033' "$TEMP_OUT"; then
    fail "B6: Redirected stdout contains ANSI escapes!"
fi
pass "B6: NO_COLOR and non-terminal output correctly suppress ANSI color codes"

# ------------------------------------------------------------------------------
# B7: Byte-Exact --code-only Output
# ------------------------------------------------------------------------------
info "Testing B7: Byte-Exact --code-only Output..."
# Create a code file without a trailing newline
printf 'int x = 42; /* no trailing newline */' > "$TEMP_SRC"
$BIN add --title "ExactSnippet" --lang "cpp" --file "$TEMP_SRC" > /dev/null

$BIN get --title "ExactSnippet" --code-only > "$TEMP_OUT"

# CRLF and LF byte-exact round-trip verification across platforms
TEMP_CRLF=$(cygpath -m "$TEMP_DIR/crlf.txt" 2>/dev/null || echo "$TEMP_DIR/crlf.txt")
TEMP_LF=$(cygpath -m "$TEMP_DIR/lf.txt" 2>/dev/null || echo "$TEMP_DIR/lf.txt")
TEMP_CRLF_OUT=$(cygpath -m "$TEMP_DIR/crlf_out.txt" 2>/dev/null || echo "$TEMP_DIR/crlf_out.txt")
TEMP_LF_OUT=$(cygpath -m "$TEMP_DIR/lf_out.txt" 2>/dev/null || echo "$TEMP_DIR/lf_out.txt")

printf "line1\r\nline2\r\n" > "$TEMP_CRLF"
printf "line1\nline2\n" > "$TEMP_LF"

$BIN add --title "CrlfExact" --lang "cpp" --file "$TEMP_CRLF" > /dev/null
$BIN add --title "LfExact" --lang "cpp" --file "$TEMP_LF" > /dev/null

$BIN get --title "CrlfExact" --code-only > "$TEMP_CRLF_OUT"
$BIN get --title "LfExact" --code-only > "$TEMP_LF_OUT"

if ! cmp -s "$TEMP_CRLF" "$TEMP_CRLF_OUT"; then
    fail "B7: CRLF file extraction failed byte-exact identity!"
fi
if ! cmp -s "$TEMP_LF" "$TEMP_LF_OUT"; then
    fail "B7: LF file extraction failed byte-exact identity!"
fi
pass "B7: Cross-platform byte-exact output verified for both CRLF and LF inputs"

# ------------------------------------------------------------------------------
# B8: Values Starting with '-' & '--' Terminator
# ------------------------------------------------------------------------------
info "Testing B8: Values Starting with '-' and '--' Terminator..."
# Code starting with dash: -x;
$BIN add --title "DashValue" --lang "cpp" --code "-x;" > /dev/null || fail "B8: Failed to add snippet with code '-x;'"
DASH_CODE=$($BIN get --title "DashValue" --code-only)
if [ "$DASH_CODE" != "-x;" ]; then
    fail "B8: Code starting with '-' was not preserved accurately: '$DASH_CODE'"
fi

# '--' terminator: arguments after '--' treated as positionals
if $BIN delete -- -5 > /dev/null 2>&1; then
    fail "B8: 'snip delete -- -5' should have failed ID validation but succeeded!"
fi
# Snippet titled "DashValue" must still exist
$BIN get --title "DashValue" > /dev/null || fail "B8: Snippet was unexpectedly deleted!"

# Delete '--' with multi-word positional rejected
if $BIN delete -- Binary Search > /dev/null 2>&1; then
    fail "B8: 'snip delete -- Binary Search' should have failed multi-word check!"
fi
pass "B8: Values starting with '-' and '--' terminator behave according to POSIX CLI standards"

# ------------------------------------------------------------------------------
# Regressions from Peer Review: Unknown Options, Add Positionals, Missing Command
# ------------------------------------------------------------------------------
info "Testing Peer Review Regressions..."
# 1. Unknown option rejection protects state: snip delete --titel Binary
if $BIN delete --titel "Binary" > /dev/null 2>&1; then
    fail "Peer Review Regression: Unknown option '--titel' should have failed but succeeded!"
fi
$BIN get --title "Binary" > /dev/null || fail "Peer Review Regression: Snippet 'Binary' was deleted by typo '--titel'!"

# 2. Add extra positionals rejected
if $BIN add Unquoted Multi Title --code "int x;" > /dev/null 2>&1; then
    fail "Peer Review Regression: Unquoted multi-word title in add should have failed!"
fi
if $BIN get --title "Unquoted" > /dev/null 2>&1; then
    fail "Peer Review Regression: Snippet 'Unquoted' was wrongly created!"
fi

# 3. Missing command with options exits non-zero
if $BIN --format markdown > /dev/null 2>&1; then
    fail "Peer Review Regression: 'snip --format markdown' without command should exit non-zero!"
fi

# 4. Command-specific option rejection protects state: snip delete --code unused Binary
if $BIN delete --code unused "Binary" > /dev/null 2>&1; then
    fail "Peer Review Regression: Command-unsupported option '--code' in delete should have failed!"
fi
$BIN get --title "Binary" > /dev/null || fail "Peer Review Regression: Snippet 'Binary' was deleted by 'delete --code'!"

# 5. Command-specific short switch rejection protects state: snip delete -c unused Binary
if $BIN delete -c unused "Binary" > /dev/null 2>&1; then
    fail "Peer Review Regression: Command-unsupported short switch '-c' in delete should have failed!"
fi
$BIN get --title "Binary" > /dev/null || fail "Peer Review Regression: Snippet 'Binary' was deleted by 'delete -c'!"

# 6. Reject positional when --id is provided: snip delete --id 1 Binary
if $BIN delete --id 1 "Binary" > /dev/null 2>&1; then
    fail "Peer Review Regression: 'snip delete --id 1 Binary' should have failed but succeeded!"
fi
$BIN get --title "Binary" > /dev/null || fail "Peer Review Regression: Snippet 'Binary' was deleted by 'delete --id 1 Binary'!"

# 7. Reject positional when --title is provided: snip delete --title Binary Extra
if $BIN delete --title "Binary" Extra > /dev/null 2>&1; then
    fail "Peer Review Regression: 'snip delete --title Binary Extra' should have failed but succeeded!"
fi
$BIN get --title "Binary" > /dev/null || fail "Peer Review Regression: Snippet 'Binary' was deleted by 'delete --title Binary Extra'!"

# 8. Reject conflicting selector options: snip delete --id 1 --title Binary
if $BIN delete --id 1 --title "Binary" > /dev/null 2>&1; then
    fail "Peer Review Regression: 'snip delete --id 1 --title Binary' should have failed but succeeded!"
fi
$BIN get --title "Binary" > /dev/null || fail "Peer Review Regression: Snippet 'Binary' was deleted by 'delete --id 1 --title Binary'!"

# 9. Get command rejects conflicting and extra selectors
if $BIN get --id 1 "Binary" > /dev/null 2>&1; then
    fail "Peer Review Regression: 'snip get --id 1 Binary' should have failed but succeeded!"
fi
if $BIN get --id 1 --title "Binary" > /dev/null 2>&1; then
    fail "Peer Review Regression: 'snip get --id 1 --title Binary' should have failed but succeeded!"
fi
pass "Peer Review Regressions: Unknown options, command schema, selector exclusivity, and missing command verified"

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------
echo ""
info "All adversarial audit checks (B1, B2, B3, B4, B6, B7, B8) PASSED successfully!"
exit 0
