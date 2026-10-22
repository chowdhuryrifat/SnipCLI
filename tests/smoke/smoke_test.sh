#!/usr/bin/env bash
# ==============================================================================
# SnipCLI Automated Smoke & End-to-End Integration Test Suite
# Tests all core CLI verbs, GoF patterns, multi-format presentation, and error quarantine
# ==============================================================================

set -euo pipefail

BIN="${1:-./build/snip}"
TEMP_DIR=$(mktemp -d)

if command -v cygpath >/dev/null 2>&1; then
    TEST_STORAGE=$(cygpath -m "$TEMP_DIR/test_snippets.json")
    SAMPLE_FILE=$(cygpath -m "$TEMP_DIR/sample.java")
    export PATH="/c/Program Files/CodeBlocks/MinGW/bin:$PATH"
else
    TEST_STORAGE="$TEMP_DIR/test_snippets.json"
    SAMPLE_FILE="$TEMP_DIR/sample.java"
fi

export SNIPCLI_STORAGE_PATH="$TEST_STORAGE"

# ANSI Color Codes for reporting
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[0;33m'
NC='\033[0m'

pass() { echo -e "${GREEN}[PASS]${NC} $1"; }
fail() { echo -e "${RED}[FAIL]${NC} $1"; exit 1; }
info() { echo -e "${BLUE}[INFO]${NC} $1"; }

trap 'rm -rf "$TEMP_DIR"' EXIT

info "Executing SnipCLI Automated End-to-End Smoke Test Suite using binary: $BIN"

# ------------------------------------------------------------------------------
# Test 1: Binary Existence & CLI Global Flags (--help, --version)
# ------------------------------------------------------------------------------
$BIN --help > /dev/null || fail "Help command (--help) failed"
$BIN -h > /dev/null || fail "Help shortcut (-h) failed"
OUTPUT_VERSION=$($BIN --version)
echo "$OUTPUT_VERSION" | grep -qi "SnipCLI" || fail "Version output does not contain application name"
$BIN -v > /dev/null || fail "Version shortcut (-v) failed"
pass "Global CLI flags (--help, -h, --version, -v) verified"

env -i "$BIN" --help > /dev/null || fail "Help command failed under isolated environment (env -i)"
pass "Help command verified under isolated container/cron environment (env -i)"

# ------------------------------------------------------------------------------
# Test 2: Add Snippets with Diverse Metadata & File Loading
# ------------------------------------------------------------------------------
# 2a: Add C++ snippet with multiple tags
$BIN add --title "Dijkstra" --lang "cpp" --tags "graph,shortest_path" --code "vector<int> dist(n, INF);\npriority_queue<pair<int,int>> pq;" > /dev/null || fail "Failed to add Dijkstra snippet"
pass "Snippet 'Dijkstra' added"

# 2b: Add second C++ snippet
$BIN add --title "BinarySearch" --lang "cpp" --tags "binary_search,array" --code "int l = 0, r = n - 1;\nwhile (l <= r) { int mid = l + (r - l) / 2; }" > /dev/null || fail "Failed to add BinarySearch snippet"
pass "Snippet 'BinarySearch' added"

# 2c: Add Python snippet
$BIN add --title "QuickSort" --lang "python" --tags "sorting,divide_conquer" --code "def quicksort(arr):\n    if len(arr) <= 1: return arr" > /dev/null || fail "Failed to add QuickSort snippet"
pass "Snippet 'QuickSort' added"

# 2d: Add snippet loaded from an external file
cat << 'EOF' > "$SAMPLE_FILE"
public class Factorial {
    public static long compute(int n) {
        return (n <= 1) ? 1 : n * compute(n - 1);
    }
}
EOF
$BIN add --title "Factorial" --lang "java" --tags "math,recursion" --file "$SAMPLE_FILE" > /dev/null || fail "Failed to add Factorial snippet from file"
pass "Snippet 'Factorial' added via --file input"

# ------------------------------------------------------------------------------
# Test 3: Defensive Domain Invariants & Validation Quarantine
# ------------------------------------------------------------------------------
# 3a: Duplicate title rejection
if $BIN add --title "Dijkstra" --lang "cpp" --code "int x = 0;" > /dev/null 2>&1; then
    fail "Duplicate title 'Dijkstra' was incorrectly accepted"
fi
pass "Defensive check: Duplicate snippet title successfully rejected"

# 3b: Empty title rejection
if $BIN add --title "" --lang "cpp" --code "int x = 0;" > /dev/null 2>&1; then
    fail "Empty snippet title was incorrectly accepted"
fi
pass "Defensive check: Empty snippet title rejected"

# 3c: Missing code and file payload rejection
if $BIN add --title "InvalidSnippet" --lang "cpp" > /dev/null 2>&1; then
    fail "Snippet without code payload was incorrectly accepted"
fi
pass "Defensive check: Missing code/file payload rejected"

# ------------------------------------------------------------------------------
# Test 4: Tabular Listing & Multi-Format Rendering (Terminal vs Markdown)
# ------------------------------------------------------------------------------
# 4a: Terminal table listing
OUTPUT_LIST=$($BIN list)
echo "$OUTPUT_LIST" | grep -q "Dijkstra" || fail "Terminal list missing 'Dijkstra'"
echo "$OUTPUT_LIST" | grep -q "BinarySearch" || fail "Terminal list missing 'BinarySearch'"
echo "$OUTPUT_LIST" | grep -q "QuickSort" || fail "Terminal list missing 'QuickSort'"
echo "$OUTPUT_LIST" | grep -q "Factorial" || fail "Terminal list missing 'Factorial'"
pass "Snippet listing verified in terminal view"

# 4b: Markdown table listing
OUTPUT_LIST_MD=$($BIN list --format markdown)
echo "$OUTPUT_LIST_MD" | grep -q "| ID | Title | Language | Lines | Tags |" || fail "Markdown list header missing"
echo "$OUTPUT_LIST_MD" | grep -q "| Dijkstra | cpp |" || fail "Markdown list entry for Dijkstra missing"
pass "Snippet listing verified in GitHub-Flavored Markdown table format"

# 4c: Filter listing by tag
OUTPUT_TAG_LIST=$($BIN list --tag "graph")
echo "$OUTPUT_TAG_LIST" | grep -q "Dijkstra" || fail "Filtered list missing 'Dijkstra'"
if echo "$OUTPUT_TAG_LIST" | grep -q "QuickSort"; then
    fail "Filtered list incorrectly included 'QuickSort'"
fi
pass "Snippet listing filtered by category tag verified"

# 4d: Filter listing by language
OUTPUT_LANG_LIST=$($BIN list --lang "python")
echo "$OUTPUT_LANG_LIST" | grep -q "QuickSort" || fail "Filtered list missing 'QuickSort'"
if echo "$OUTPUT_LANG_LIST" | grep -q "Dijkstra"; then
    fail "Filtered list incorrectly included 'Dijkstra'"
fi
pass "Snippet listing filtered by language verified"

# ------------------------------------------------------------------------------
# Test 5: Snippet Retrieval & Pipe Native Support (get)
# ------------------------------------------------------------------------------
# 5a: Standard terminal retrieval by title
OUTPUT_GET=$($BIN get --title "Dijkstra")
echo "$OUTPUT_GET" | grep -q "vector<int> dist" || fail "Failed to retrieve Dijkstra code in terminal format"
pass "Snippet retrieval by title verified"

# 5b: Markdown formatted snippet retrieval
OUTPUT_GET_MD=$($BIN get --title "Dijkstra" --format markdown)
echo "$OUTPUT_GET_MD" | grep -q "# Dijkstra" || fail "Markdown snippet title header missing"
echo "$OUTPUT_GET_MD" | grep -q "\`\`\`cpp" || fail "Markdown language fence block missing"
pass "Snippet retrieval in GitHub-Flavored Markdown format verified"

# 5c: Pipe-friendly pure source code extraction (--code-only)
OUTPUT_CODE_ONLY=$($BIN get --title "Dijkstra" --code-only)
if [[ "$OUTPUT_CODE_ONLY" != *"vector<int> dist"* ]]; then
    fail "Get command --code-only failed to output pure code"
fi
if echo "$OUTPUT_CODE_ONLY" | grep -q "Language:"; then
    fail "Get command --code-only leaked metadata headers"
fi
pass "Snippet retrieval with pipe-native --code-only verified"

# 5d: Retrieval of non-existent snippet should fail with exit code 1
if $BIN get --title "NonExistentSnippet" > /dev/null 2>&1; then
    fail "Querying non-existent snippet succeeded unexpectedly"
fi
pass "Quarantine: Non-existent snippet retrieval correctly exited with error"

# ------------------------------------------------------------------------------
# Test 6: GoF Strategy Pattern Searching (Substrings, Tags, Languages)
# ------------------------------------------------------------------------------
# 6a: Title substring search strategy
OUTPUT_SEARCH_TITLE=$($BIN search "Quick")
echo "$OUTPUT_SEARCH_TITLE" | grep -q "QuickSort" || fail "Search by substring failed to locate 'QuickSort'"
pass "Strategy search by title substring verified"

# 6b: Tag search strategy
OUTPUT_SEARCH_TAG=$($BIN search --tag "graph")
echo "$OUTPUT_SEARCH_TAG" | grep -q "Dijkstra" || fail "Search by tag failed to locate 'Dijkstra'"
pass "Strategy search by category tag verified"

# 6c: Language search strategy
OUTPUT_SEARCH_LANG=$($BIN search --lang "java")
echo "$OUTPUT_SEARCH_LANG" | grep -q "Factorial" || fail "Search by language failed to locate 'Factorial'"
pass "Strategy search by language verified"

# ------------------------------------------------------------------------------
# Test 7: Repository Telemetry & Aggregation (stats)
# ------------------------------------------------------------------------------
# 7a: Terminal stats output
OUTPUT_STATS=$($BIN stats)
echo "$OUTPUT_STATS" | grep -q "Total Snippets:" || fail "Terminal stats missing total snippets metric"
echo "$OUTPUT_STATS" | grep -q "Total Lines of Code:" || fail "Terminal stats missing total lines metric"
echo "$OUTPUT_STATS" | grep -q "Language Breakdown:" || fail "Terminal stats missing language breakdown"
pass "Repository telemetry verified in terminal view"

# 7b: Markdown stats output
OUTPUT_STATS_MD=$($BIN stats --format markdown)
echo "$OUTPUT_STATS_MD" | grep -q "## SnipCLI Repository Statistics" || fail "Markdown stats header missing"
echo "$OUTPUT_STATS_MD" | grep -q "| Language | Snippet Count |" || fail "Markdown stats table header missing"
echo "$OUTPUT_STATS_MD" | grep -q "| cpp | 2 |" || fail "Markdown stats cpp count mismatch"
pass "Repository telemetry verified in GitHub-Flavored Markdown format"

# ------------------------------------------------------------------------------
# Test 8: Snippet Deletion & State Mutation (delete)
# ------------------------------------------------------------------------------
# 8a: Delete snippet by title
$BIN delete --title "Factorial" > /dev/null || fail "Failed to delete snippet 'Factorial' by title"
OUTPUT_CHECK=$($BIN list)
if echo "$OUTPUT_CHECK" | grep -q "Factorial"; then
    fail "Snippet 'Factorial' still found in repository after deletion"
fi
pass "Snippet deletion by title verified"

# 8b: Delete snippet by ID
$BIN delete --id 1 > /dev/null || fail "Failed to delete snippet ID #1"
OUTPUT_CHECK_2=$($BIN list)
if echo "$OUTPUT_CHECK_2" | grep -q "Dijkstra"; then
    fail "Snippet 'Dijkstra' (ID #1) still found in repository after deletion"
fi
pass "Snippet deletion by ID verified"

# 8c: Deleting non-existent snippet should fail cleanly
if $BIN delete --id 9999 > /dev/null 2>&1; then
    fail "Deleting non-existent snippet ID #9999 succeeded unexpectedly"
fi
pass "Quarantine: Non-existent snippet deletion cleanly rejected"

# 8d: Add snippet titled "0" to verify positional '0' does NOT delete it
$BIN add --title "0" --lang "cpp" --code "int zero = 0;" > /dev/null || fail "Failed to add snippet titled '0'"
if $BIN delete 0 > /dev/null 2>&1; then
    fail "Positional delete with '0' succeeded unexpectedly"
fi
$BIN get --title "0" > /dev/null || fail "Snippet titled '0' was wrongly deleted by positional delete 0"
$BIN delete --title "0" > /dev/null || fail "Failed to delete snippet titled '0' via explicit --title"
pass "Quarantine: Positional delete 0 rejected and protected snippet titled '0'"

# 8e: Spaced negative ID in delete and get rejected
if $BIN delete --id -5 > /dev/null 2>&1; then
    fail "Delete with spaced negative ID '--id -5' succeeded unexpectedly"
fi
if $BIN get --id -5 > /dev/null 2>&1; then
    fail "Get with spaced negative ID '--id -5' succeeded unexpectedly"
fi
pass "Defensive check: Spaced negative ID (--id -5) cleanly rejected"

# ------------------------------------------------------------------------------
# Test 9: Unknown Command Dispatch Handling
# ------------------------------------------------------------------------------
if $BIN unrecognized_verb > /dev/null 2>&1; then
    fail "Unrecognized command was accepted unexpectedly"
fi
pass "CLI Dispatcher: Unrecognized command verb cleanly rejected with error"

# ------------------------------------------------------------------------------
# Test 10: Option Schema, Multi-Word Quarantine, & Global Flags (Findings B1-B8)
# ------------------------------------------------------------------------------
# 10a: Unquoted multi-word positional rejection
$BIN add --title "Binary Search" --lang "cpp" --code "int bs = 1;" > /dev/null || fail "Failed to add Binary Search"
if $BIN delete Binary Search > /dev/null 2>&1; then
    fail "Unquoted multi-word delete 'Binary Search' was accepted unexpectedly"
fi
$BIN get --title "Binary Search" > /dev/null || fail "Snippet 'Binary Search' was deleted by unquoted delete"
pass "B1: Multi-word unquoted positionals rejected defensively"

# 10b: Valueless option handling
if $BIN delete --id --title "Binary Search" > /dev/null 2>&1; then
    fail "Valueless --id followed by --title was accepted unexpectedly"
fi
if $BIN delete --id > /dev/null 2>&1; then
    fail "Valueless --id at end of arguments was accepted unexpectedly"
fi
pass "B2: Valueless options correctly rejected with CommandException"

# 10c: Global options before verb
OUTPUT_GLOBAL=$($BIN --format markdown list)
echo "$OUTPUT_GLOBAL" | grep -q "| Binary Search |" || fail "Global option before verb failed"
pass "B3: Global options before verb (--format markdown list) verified"

# 10d: Invalid --format rejection
if $BIN --format markdwon list > /dev/null 2>&1; then
    fail "Invalid format 'markdwon' was accepted unexpectedly"
fi
if $BIN list --format=invalid > /dev/null 2>&1; then
    fail "Invalid format '--format=invalid' was accepted unexpectedly"
fi
pass "B4: Invalid format correctly rejected"

# 10e: Values starting with dash accepted
$BIN add --title "DashCode" --lang "cpp" --code "-x;" > /dev/null || fail "Failed to add snippet with code '-x;'"
pass "B8: Snippet code starting with dash ('-x;') accepted"

# 10f: Byte-exact code-only extraction
EXTRACTED_DASH=$($BIN get --title "DashCode" --code-only)
if [ "$EXTRACTED_DASH" != "-x;" ]; then
    fail "Byte-exact code extraction failed for '-x;'"
fi
pass "B7: Byte-exact --code-only output verified"

# 10g: Unknown option rejection
$BIN add --title "GuardSnippet" --lang "cpp" --code "int guard = 1;" > /dev/null
if $BIN delete --titel "GuardSnippet" > /dev/null 2>&1; then
    fail "Unknown option '--titel' was accepted unexpectedly"
fi
$BIN get --title "GuardSnippet" > /dev/null || fail "Snippet 'GuardSnippet' was wrongly deleted by unknown option!"
$BIN delete --title "GuardSnippet" > /dev/null
pass "Unknown option rejection: typo --titel cleanly rejected, snippet protected"

# 10h: Extra positionals for add rejected
if $BIN add Unquoted Multi Title --code "int x;" > /dev/null 2>&1; then
    fail "Add with unquoted multi-word title was accepted unexpectedly"
fi
if $BIN get --title "Unquoted" > /dev/null 2>&1; then
    fail "Snippet 'Unquoted' was wrongly created by unquoted add!"
fi
if $BIN add --title "SingleTitle" ExtraArg --code "int x;" > /dev/null 2>&1; then
    fail "Add with explicit --title and extra positional was accepted unexpectedly"
fi
if $BIN get --title "SingleTitle" > /dev/null 2>&1; then
    fail "Snippet 'SingleTitle' was wrongly created by malformed add!"
fi
pass "Add command extra positional rejection verified"

# 10i: Missing command verb with options returns non-zero error
if $BIN --format markdown > /dev/null 2>&1; then
    fail "Invocation with options but no command unexpectedly exited with 0"
fi
pass "Missing command verb with options returns non-zero error code"

# 10j: Cross-platform CRLF and LF byte-exact round-trip verification
CRLF_FILE="$TEMP_DIR/test_crlf.txt"
LF_FILE="$TEMP_DIR/test_lf.txt"
CRLF_OUT="$TEMP_DIR/out_crlf.txt"
LF_OUT="$TEMP_DIR/out_lf.txt"
if command -v cygpath >/dev/null 2>&1; then
    CRLF_FILE=$(cygpath -m "$CRLF_FILE")
    LF_FILE=$(cygpath -m "$LF_FILE")
    CRLF_OUT=$(cygpath -m "$CRLF_OUT")
    LF_OUT=$(cygpath -m "$LF_OUT")
fi

printf "int a = 1;\r\nint b = 2;\r\n" > "$CRLF_FILE"
printf "int c = 3;\nint d = 4;\n" > "$LF_FILE"

$BIN add --title "CrlfSnippet" --lang "cpp" --file "$CRLF_FILE" > /dev/null || fail "Failed to add CRLF snippet"
$BIN add --title "LfSnippet" --lang "cpp" --file "$LF_FILE" > /dev/null || fail "Failed to add LF snippet"

$BIN get --title "CrlfSnippet" --code-only > "$CRLF_OUT"
$BIN get --title "LfSnippet" --code-only > "$LF_OUT"

if ! cmp -s "$CRLF_FILE" "$CRLF_OUT"; then
    fail "CRLF byte-exact roundtrip failed!"
fi
if ! cmp -s "$LF_FILE" "$LF_OUT"; then
    fail "LF byte-exact roundtrip failed!"
fi
pass "Byte-exact file roundtrip verified across both CRLF and LF formats"

$BIN delete --title "CrlfSnippet" > /dev/null
$BIN delete --title "LfSnippet" > /dev/null

# 10k: Command-specific option rejection protects state: snip delete --code unused Binary Search
if $BIN delete --code unused "Binary Search" > /dev/null 2>&1; then
    fail "Command-unsupported option '--code' in delete was accepted unexpectedly"
fi
$BIN get --title "Binary Search" > /dev/null || fail "Snippet 'Binary Search' was deleted by 'delete --code'!"
pass "Command-specific option rejection: delete --code cleanly rejected, state protected"

# 10l: Unsupported short switch rejection protects state: snip delete -c unused Binary Search
if $BIN delete -c unused "Binary Search" > /dev/null 2>&1; then
    fail "Command-unsupported short switch '-c' in delete was accepted unexpectedly"
fi
$BIN get --title "Binary Search" > /dev/null || fail "Snippet 'Binary Search' was deleted by 'delete -c'!"
pass "Command-specific short switch rejection: delete -c cleanly rejected, state protected"

# 10m: Reject positional when --id is provided
if $BIN delete --id 1 "Binary Search" > /dev/null 2>&1; then
    fail "Delete with --id and positional should have failed but succeeded!"
fi
$BIN get --title "Binary Search" > /dev/null || fail "Snippet was deleted by 'delete --id with positional'!"
pass "Selector exclusivity: delete --id with positional cleanly rejected"

# 10n: Reject positional when --title is provided
if $BIN delete --title "Binary Search" Extra > /dev/null 2>&1; then
    fail "Delete with --title and positional should have failed but succeeded!"
fi
$BIN get --title "Binary Search" > /dev/null || fail "Snippet was deleted by 'delete --title with positional'!"
pass "Selector exclusivity: delete --title with positional cleanly rejected"

# 10o: Reject conflicting selector options
if $BIN delete --id 1 --title "Binary Search" > /dev/null 2>&1; then
    fail "Delete with conflicting --id and --title should have failed but succeeded!"
fi
$BIN get --title "Binary Search" > /dev/null || fail "Snippet was deleted by conflicting selector options!"
pass "Selector exclusivity: conflicting --id and --title cleanly rejected"

# 10p: Get command rejects conflicting and extra selectors
if $BIN get --id 1 "Binary Search" > /dev/null 2>&1; then
    fail "Get with --id and positional should have failed but succeeded!"
fi
if $BIN get --id 1 --title "Binary Search" > /dev/null 2>&1; then
    fail "Get with conflicting --id and --title should have failed but succeeded!"
fi
pass "Selector exclusivity in get command cleanly rejected"

# 10q: Reject boolean flags with equals syntax
if $BIN get --code-only=true 1 > /dev/null 2>&1; then
    fail "Get with --code-only=true should have failed but succeeded!"
fi
if $BIN list --help=false > /dev/null 2>&1; then
    fail "List with --help=false should have failed but succeeded!"
fi
if $BIN list --version=false > /dev/null 2>&1; then
    fail "List with --version=false should have failed but succeeded!"
fi
if $BIN list -h=true > /dev/null 2>&1; then
    fail "List with -h=true should have failed but succeeded!"
fi
if $BIN list -v=true > /dev/null 2>&1; then
    fail "List with -v=true should have failed but succeeded!"
fi
if $BIN get -c=true 1 > /dev/null 2>&1; then
    fail "Get with -c=true should have failed but succeeded!"
fi
pass "Boolean flags with equals syntax cleanly rejected before execution"

# Clean up temporary test snippets
$BIN delete --title "Binary Search" > /dev/null || fail "Failed to delete 'Binary Search'"
$BIN delete --title "DashCode" > /dev/null || fail "Failed to delete 'DashCode'"

# ------------------------------------------------------------------------------
# Test 11: Cross-Process Concurrency & Strict Command Schema Validation
# ------------------------------------------------------------------------------
# 11a: Concurrent cross-process additions (R2b)
if [ -z "${WINDIR:-}" ] && [ -z "${COMSPEC:-}" ]; then
    PARALLEL_STORAGE="$TEMP_DIR/parallel_snippets.json"
    export SNIPCLI_STORAGE_PATH="$PARALLEL_STORAGE"
    for i in $(seq 1 20); do
        ( "$BIN" add --title "P$i" --code "int x$i;" > /dev/null 2>&1 ) &
    done
    wait

    COUNT_PARALLEL=$("$BIN" list --format markdown | grep -c '^| [0-9]' || true)
    [ "$COUNT_PARALLEL" -eq 20 ] || fail "Parallel adds lost updates: expected 20 snippets, found $COUNT_PARALLEL"
    pass "Concurrent cross-process writes preserved: 20/20 parallel adds persisted"

    # Same-title race: 8 parallel adds with identical title
    SAME_RACE_STORAGE="$TEMP_DIR/same_race.json"
    export SNIPCLI_STORAGE_PATH="$SAME_RACE_STORAGE"
    for i in $(seq 1 8); do
        ( "$BIN" add --title "RaceTitle" --code "int val = $i;" > /dev/null 2>&1 ) &
    done
    wait

    COUNT_RACE=$("$BIN" list --format markdown | grep -c '^| [0-9]' || true)
    [ "$COUNT_RACE" -eq 1 ] || fail "Concurrent same-title race failed: expected exactly 1 snippet, found $COUNT_RACE"
    pass "Concurrent same-title race safety verified: exactly 1 snippet persisted"

    # 11c: Storage path alias locking across symlinked parent directory
    ALIAS_REAL_DIR="$TEMP_DIR/alias_real_dir"
    ALIAS_LINK_DIR="$TEMP_DIR/alias_link_dir"
    mkdir -p "$ALIAS_REAL_DIR"
    ln -s "$ALIAS_REAL_DIR" "$ALIAS_LINK_DIR"
    for i in $(seq 1 20); do
        if [ $((i % 2)) -eq 0 ]; then
            ( SNIPCLI_STORAGE_PATH="$ALIAS_REAL_DIR/db.json" "$BIN" add --title "Alias$i" --code "int a$i;" > /dev/null 2>&1 ) &
        else
            ( SNIPCLI_STORAGE_PATH="$ALIAS_LINK_DIR/db.json" "$BIN" add --title "Alias$i" --code "int a$i;" > /dev/null 2>&1 ) &
        fi
    done
    wait

    COUNT_ALIAS=$(SNIPCLI_STORAGE_PATH="$ALIAS_REAL_DIR/db.json" "$BIN" list --format markdown | grep -c '^| [0-9]' || true)
    [ "$COUNT_ALIAS" -eq 20 ] || fail "Storage path alias adds lost updates: expected 20 snippets, found $COUNT_ALIAS"
    pass "Storage path alias locking: 20/20 writes preserved across symlinked parent directories"

    # 11d: Symlink loop storage path defensively rejected without crash (F1)
    SYMLOOP_DIR="$TEMP_DIR/symloop_smoke"
    mkdir -p "$SYMLOOP_DIR"
    ln -s "$SYMLOOP_DIR/loop_b.json" "$SYMLOOP_DIR/loop_a.json"
    ln -s "$SYMLOOP_DIR/loop_a.json" "$SYMLOOP_DIR/loop_b.json"
    LOOP_OUT=$(SNIPCLI_STORAGE_PATH="$SYMLOOP_DIR/loop_a.json" "$BIN" list 2>&1 || true)
    LOOP_RC=0
    SNIPCLI_STORAGE_PATH="$SYMLOOP_DIR/loop_a.json" "$BIN" list > /dev/null 2>&1 || LOOP_RC=$?
    [ "$LOOP_RC" -eq 1 ] || fail "Symlink loop did not exit with code 1 (got $LOOP_RC)"
    echo "$LOOP_OUT" | grep -q "Too many levels of symbolic links" || fail "Symlink loop output missing expected error message"
    pass "Symlink loop defense: loop path exited cleanly with code 1 and error message"
    export SNIPCLI_STORAGE_PATH="$TEST_STORAGE"
fi

# 11b: Extra positional arguments on list and stats rejected (R7)
if $BIN stats extra > /dev/null 2>&1; then
    fail "Stats with extra positional argument should have failed but succeeded!"
fi
if $BIN list extra > /dev/null 2>&1; then
    fail "List with extra positional argument should have failed but succeeded!"
fi
pass "Command positionals schema: unexpected positionals on list/stats rejected"

# ------------------------------------------------------------------------------
# Completion Notice
# ------------------------------------------------------------------------------
echo ""
info "All SnipCLI automated smoke & end-to-end integration tests passed with 100% success!"
exit 0
