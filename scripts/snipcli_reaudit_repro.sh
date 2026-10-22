#!/usr/bin/env bash
# SnipCLI re-audit repro (dev @ 956621b, after PR #19 and #20). Linux/macOS, bash + coreutils (+ python3 and flock optional).
# Usage:  bash snipcli_reaudit_repro.sh ./build/snip
# Each block prints EXPECTED (the fixed behavior) and OBSERVED. Run before and after a fix.
set -u
SNIP="$(cd "$(dirname "${1:-./build/snip}")" && pwd)/$(basename "${1:-./build/snip}")"
[ -x "$SNIP" ] || { echo "snip binary not found/executable: $SNIP"; exit 2; }
W="$(mktemp -d)"; trap 'rm -rf "$W"' EXIT; cd "$W" || exit 2
strip_ansi() { sed 's/\x1b\[[0-9;]*m//g'; }
fresh() { rm -rf "$W/db.json" "$W/db.json.lock"; export SNIPCLI_STORAGE_PATH="$W/db.json"; }
hdr() { printf '\n### %s\n    EXPECTED: %s\n' "$1" "$2"; }

hdr "R1 --help with no HOME/APPDATA and no SNIPCLI_STORAGE_PATH (containers, cron, env -i)" "help text, exit 0"
env -i "$SNIP" --help >out.txt 2>err.txt; rc=$?
echo "    OBSERVED: rc=$rc first line: $( (head -1 out.txt; head -1 err.txt) | head -1 | strip_ansi )"

hdr "R2 regression guard for the cross-process lock (no automated test exists today)" "40/40 stored; 10 deletes + 10 adds leave exactly 10 new"
fresh; for i in $(seq 1 40); do ( "$SNIP" add --title "P$i" --code "int x$i;" >/dev/null 2>&1 ) & done; wait
echo "    OBSERVED: parallel adds stored $(grep -c '"title"' "$W/db.json")/40, distinct ids $(grep '"id"' "$W/db.json" | sort -u | wc -l)"
fresh; for i in $(seq 1 10); do "$SNIP" add --title "K$i" --code "a;" >/dev/null; done
for i in $(seq 1 10); do ( "$SNIP" delete "K$i" >/dev/null 2>&1 ) & ( "$SNIP" add --title "N$i" --code "b;" >/dev/null 2>&1 ) & done; wait
echo "    OBSERVED: leftover K=$(grep -c '"title": "K' "$W/db.json") (want 0), new N=$(grep -c '"title": "N' "$W/db.json") (want 10)"

hdr "R3 a held lock makes snip wait forever with no message" "error within ~10 s: 'storage is locked by another snip process'"
if command -v flock >/dev/null 2>&1; then
  fresh; "$SNIP" add --title Seed --code "a;" >/dev/null
  ( flock -x "$W/db.json.lock" sleep 6 ) & sleep 0.5
  SNIPCLI_LOCK_TIMEOUT_MS=2000 timeout 3 "$SNIP" add --title Blocked --code "b;" >lock_out.txt 2>&1; rc=$?; wait
  echo "    OBSERVED: rc=$rc output: [$(strip_ansi < lock_out.txt | head -1)]"
else echo "    SKIPPED: flock(1) not installed"; fi

hdr "R4 duplicate titles (case variants) in a hand-edited file: delete removes ALL matches but reports one" "load rejects the file as corrupt, or delete removes only one / asks"
fresh; printf '[{"id":1,"title":"bfs","language":"cpp","code":"a","tags":[],"createdAt":"x","updatedAt":"x"},{"id":2,"title":"BFS","language":"cpp","code":"b","tags":[],"createdAt":"x","updatedAt":"x"}]' > db.json
"$SNIP" delete bfs 2>&1 | strip_ansi | head -1 | sed 's/^/    OBSERVED: /'; echo "    OBSERVED: rows left in file: $(grep -c '"id"' db.json)"

hdr "R5 color decision looks at stdout only: ANSI leaks into a redirected stderr when stdout is a terminal" "no ESC bytes in the stderr file"
if command -v python3 >/dev/null 2>&1; then
  fresh; python3 - "$SNIP" <<'PY'
import os, pty, subprocess, sys
env = {k: v for k, v in os.environ.items() if k != "NO_COLOR"}
m, s = pty.openpty()
with open("err_tty.txt", "wb") as ef:
    subprocess.run([sys.argv[1], "delete", "zzz"], env=env, stdout=s, stderr=ef)
os.close(s)
print("    OBSERVED: ESC bytes in redirected stderr:", b"\x1b" in open("err_tty.txt", "rb").read())
PY
else echo "    SKIPPED: python3 not installed"; fi

hdr "R7 values that start with '--' are rejected; extra positionals on list/stats are ignored" "documented behavior (use --code=VALUE), consistent extra-argument errors"
fresh
"$SNIP" add --title Q1 --code "-- sql comment" 2>&1 | strip_ansi | head -1 | sed 's/^/    OBSERVED (--code "-- sql comment"): /'
"$SNIP" add --title Q2 --code="-- sql comment" 2>&1 | strip_ansi | head -1 | sed 's/^/    OBSERVED (--code="-- sql comment"): /'
"$SNIP" stats extra 2>&1 | strip_ansi | head -1 | sed 's/^/    OBSERVED (stats extra): /'
echo; echo "done."
