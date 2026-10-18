#!/usr/bin/env bash
# ==============================================================================
# SnipCLI Automated Smoke & Integration Test Suite
# Tests core CLI verbs: add, list, get, search, stats, delete
# ==============================================================================

set -euo pipefail

BIN="${1:-./build/snip}"
TEMP_DIR=$(mktemp -d)
TEST_STORAGE="$TEMP_DIR/test_snippets.json"
export SNIPCLI_STORAGE_PATH="$TEST_STORAGE"

RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m'

pass() { echo -e "${GREEN}[PASS]${NC} $1"; }
fail() { echo -e "${RED}[FAIL]${NC} $1"; exit 1; }
info() { echo -e "${BLUE}[INFO]${NC} $1"; }

trap 'rm -rf "$TEMP_DIR"' EXIT

info "Executing SnipCLI Smoke Tests using binary: $BIN"

# Test 1: Binary execution and Help command
$BIN --help > /dev/null || fail "Help command failed"
pass "Help command executed successfully"

# Test 2: Add snippet
$BIN add --title "Dijkstra" --lang "cpp" --tags "graph,shortest_path" --code "vector<int> dist;" > /dev/null || fail "Failed to add Dijkstra snippet"
pass "Snippet 'Dijkstra' added"

# Test 3: Add second snippet
$BIN add --title "BinarySearch" --lang "cpp" --tags "binary_search,array" --code "int l = 0, r = n - 1;" > /dev/null || fail "Failed to add BinarySearch snippet"
pass "Snippet 'BinarySearch' added"

# Test 4: List snippets
OUTPUT_LIST=$($BIN list)
echo "$OUTPUT_LIST" | grep -q "Dijkstra" || fail "List does not contain 'Dijkstra'"
echo "$OUTPUT_LIST" | grep -q "BinarySearch" || fail "List does not contain 'BinarySearch'"
pass "Snippet listing verified"

# Test 5: Get snippet
OUTPUT_GET=$($BIN get --title "Dijkstra")
echo "$OUTPUT_GET" | grep -q "vector<int> dist;" || fail "Get command failed to retrieve snippet body"
pass "Snippet retrieval by title verified"

# Test 5b: Get snippet with --code-only pipe mode
OUTPUT_CODE_ONLY=$($BIN get --title "Dijkstra" --code-only)
if [[ "$OUTPUT_CODE_ONLY" != *"vector<int> dist;"* ]]; then
    fail "Get command --code-only failed to retrieve pure code body"
fi
pass "Snippet retrieval with --code-only verified"

# Test 6: Search snippet by tag
OUTPUT_SEARCH=$($BIN search --tag "graph")
echo "$OUTPUT_SEARCH" | grep -q "Dijkstra" || fail "Search by tag failed to locate 'Dijkstra'"
pass "Search by tag verified"

# Test 7: Stats command
OUTPUT_STATS=$($BIN stats --format markdown)
echo "$OUTPUT_STATS" | grep -q "Total Snippets:\*\* 2" || fail "Stats command did not report correct count"
pass "Snippet stats verified"

# Test 8: Delete snippet
$BIN delete --title "Dijkstra" > /dev/null || fail "Failed to delete Dijkstra snippet"
OUTPUT_POST_DELETE=$($BIN list)
if echo "$OUTPUT_POST_DELETE" | grep -q "Dijkstra"; then
    fail "Dijkstra still present after deletion"
fi
pass "Snippet deletion verified"

info "All SnipCLI smoke tests passed with 100% success!"
exit 0
