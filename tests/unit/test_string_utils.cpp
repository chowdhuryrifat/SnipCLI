#include <cstdint>
#include <iostream>
#include <string>

#include "snipcli/common/string_utils.hpp"

namespace {

void assertTrue(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "[FAIL] String Utils Assertion Failed: " << message << std::endl;
        std::exit(1);
    }
}

}  // namespace

void runStringUtilsTests() {
    using namespace snipcli::common;
    std::cout << "[TEST] Running shared string utility tests..." << std::endl;

    // toLower: ASCII only, other bytes untouched
    assertTrue(toLower("DiJkStRa_42") == "dijkstra_42", "toLower lowercases ASCII");
    assertTrue(toLower("\xC3\x89") == "\xC3\x89", "toLower leaves UTF-8 bytes untouched");

    // trim: space, tab, CR, LF on both ends
    assertTrue(trim(" \t graph \r\n") == "graph", "trim strips surrounding whitespace");
    assertTrue(trim("   ").empty() && trim("").empty(), "trim of blank input is empty");
    assertTrue(trim("a b") == "a b", "trim keeps interior whitespace");

    // equalsIgnoreCase
    assertTrue(equalsIgnoreCase("BFS", "bfs"), "equalsIgnoreCase ignores case");
    assertTrue(!equalsIgnoreCase("BFS", "BFSx"), "equalsIgnoreCase rejects length mismatch");

    // looksNumeric: optional sign then digits
    assertTrue(looksNumeric("42") && looksNumeric("-5") && looksNumeric("+7"),
               "looksNumeric accepts signed integers");
    assertTrue(
        !looksNumeric("") && !looksNumeric("-") && !looksNumeric("4a") && !looksNumeric("1.5"),
        "looksNumeric rejects non-integers");

    // parseStrictId: positive, unsigned, no junk, no overflow
    assertTrue(parseStrictId("1") == uint64_t{1}, "parseStrictId parses 1");
    assertTrue(parseStrictId("18446744073709551615") == UINT64_MAX, "parseStrictId accepts max");
    assertTrue(!parseStrictId("0") && !parseStrictId("-1") && !parseStrictId("+1") &&
                   !parseStrictId(" 1") && !parseStrictId("1x") && !parseStrictId("") &&
                   !parseStrictId("18446744073709551616"),
               "parseStrictId rejects zero, signs, whitespace, junk, empty and overflow");

    std::cout << "[PASS] Shared string utility tests passed. (7/7 Invariants Verified)"
              << std::endl;
}
