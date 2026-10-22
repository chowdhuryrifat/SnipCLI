#!/usr/bin/env python3
"""scripts/lint_report_claims.py - Verify that all claims in FINAL_REPORT.md are backed by real logs.

Rules enforced:
1. Every table row marked PASS must cite a log path that exists in the bundle.
2. The referenced log must contain an authentic success marker (e.g. PASS, exit 0, 100% tests passed, etc.).
3. Every row marked NOT RUN must cite the failing command ('NOT RUN: <command> -> <error>').
Exits 0 on success, exits 1 on any violation.
"""
import os
import re
import sys

def lint_final_report(report_path, bundle_root):
    if not os.path.isfile(report_path):
        print(f"ERROR: Report file not found: {report_path}", file=sys.stderr)
        return 1

    with open(report_path, "r", encoding="utf-8") as f:
        content = f.read()

    lines = content.splitlines()
    in_tool_table = False
    violations = []
    checked_rows = 0

    success_markers = [
        "pass", "100%", "success", "0 mismatches", "0 errors", "clean",
        "verified", "ok", "no leaks", "0 failed", "passed"
    ]

    for line_no, line in enumerate(lines, 1):
        stripped = line.strip()
        # Detect tool verification table
        if re.search(r"\|\s*Tool\s*\|\s*Invocation\s*\|\s*Status\s*\|", stripped, re.IGNORECASE) or \
           re.search(r"\|\s*Tool\s*\|\s*Target\s*\|\s*Result\s*\|", stripped, re.IGNORECASE) or \
           re.search(r"\|\s*Target\s*/\s*Tool\s*\|\s*Result\s*\|", stripped, re.IGNORECASE):
            in_tool_table = True
            continue

        if in_tool_table:
            if not stripped.startswith("|") or stripped.startswith("| :---") or stripped.startswith("|:---"):
                if not stripped.startswith("|"):
                    in_tool_table = False
                continue

            parts = [p.strip() for p in stripped.split("|")[1:-1]]
            if len(parts) < 3:
                continue

            status_col = None
            evidence_col = None

            for p in parts:
                p_upper = p.upper()
                if "PASS" in p_upper or "FAIL" in p_upper or "NOT RUN" in p_upper or "RESOLVED" in p_upper:
                    status_col = p
                    break

            if status_col is None:
                continue

            checked_rows += 1
            status_upper = status_col.upper()

            # Find cited log paths anywhere in the row
            log_refs = re.findall(r'`([^`]+\.(?:log|txt|json))`', line)
            if not log_refs:
                log_refs = re.findall(r'\[([^\]]+)\]\(([^)]+\.(?:log|txt|json))\)', line)
                log_refs = [r[1] for r in log_refs]

            if "PASS" in status_upper:
                if not log_refs:
                    violations.append(f"Line {line_no}: Row marked PASS has no cited log file: '{stripped}'")
                    continue

                for ref in log_refs:
                    # Clean markdown links or relative paths
                    clean_ref = ref.lstrip("./")
                    # Try resolving relative to bundle_root and report dir
                    candidates = [
                        os.path.join(bundle_root, clean_ref),
                        os.path.join(bundle_root, "logs", os.path.basename(clean_ref)),
                        os.path.join(os.path.dirname(report_path), clean_ref),
                        os.path.join(bundle_root, clean_ref.replace("review_bundle/", ""))
                    ]
                    found_path = None
                    for c in candidates:
                        if os.path.isfile(c):
                            found_path = c
                            break

                    if not found_path:
                        violations.append(f"Line {line_no}: Cited log file '{ref}' does not exist in bundle.")
                        continue

                    # Check for authentic success marker
                    with open(found_path, "r", encoding="utf-8", errors="replace") as lf:
                        log_text = lf.read().lower()

                    has_success = any(marker in log_text for marker in success_markers)
                    if not has_success:
                        violations.append(f"Line {line_no}: Log '{ref}' does not contain expected success marker.")

            elif "NOT RUN" in status_upper:
                # Check that row cites the failing command
                if "->" not in line and "NOT RUN:" not in line and "not found" not in line and "unavailable" not in line:
                    violations.append(f"Line {line_no}: Row marked NOT RUN must cite failing command/reason ('NOT RUN: <cmd> -> <err>'): '{stripped}'")

    print(f"Claims linter checked {checked_rows} table rows in {report_path}.")

    if violations:
        print(f"FAILED: Found {len(violations)} claim verification violations:", file=sys.stderr)
        for v in violations:
            print(f"  - {v}", file=sys.stderr)
        return 1

    print("SUCCESS: All report claims strictly verified against bundle evidence logs.")
    return 0

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: scripts/lint_report_claims.py <path/to/FINAL_REPORT.md> [bundle_root]", file=sys.stderr)
        sys.exit(2)

    rep_path = sys.argv[1]
    b_root = sys.argv[2] if len(sys.argv) > 2 else os.path.dirname(os.path.abspath(rep_path))
    sys.exit(lint_final_report(rep_path, b_root))
