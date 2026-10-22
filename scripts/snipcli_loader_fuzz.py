#!/usr/bin/env python3
"""SnipCLI storage-loader fuzzer (no dependencies beyond Python 3).

Usage:  python3 snipcli_loader_fuzz.py ./build/snip [mutations=1500] [seed=7]
Best run against an ASan+UBSan build (cmake -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -g").

What it checks, using the real binary and a real storage file:
  1. Truncation sweep: every strict prefix of a valid file (ignoring trailing whitespace) must be REJECTED
     (exit 1). Accepting a truncated file would mean silent data loss on the next save.
  2. Random mutations (byte flip/delete/insert/duplicate/swap): the process must exit 0 or 1, never crash,
     hang (10 s timeout), or print sanitizer / runtime-error output.
Exit status: 0 = no problems, 1 = problems found (details printed).
"""
import os
import random
import subprocess
import sys
import tempfile

SNIP = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "./build/snip")
N_MUT = int(sys.argv[2]) if len(sys.argv) > 2 else 1500
SEED = int(sys.argv[3]) if len(sys.argv) > 3 else 7

work = tempfile.mkdtemp(prefix="snipcli_fuzz_")
db = os.path.join(work, "db.json")
env = dict(os.environ, SNIPCLI_STORAGE_PATH=db, ASAN_OPTIONS="detect_leaks=0")


def run(data):
    with open(db, "wb") as f:
        f.write(data)
    try:
        p = subprocess.run([SNIP, "list", "--format", "markdown"], env=env,
                           capture_output=True, timeout=10, stdin=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        return "TIMEOUT", b"", b""
    return p.returncode, p.stdout, p.stderr


# Build an authentic valid file through the real binary (escapes, brackets, unicode, tags).
if os.path.exists(db):
    os.remove(db)
seeds = [
    ("Alpha", 'int a=1; // "q" \\ back\n\tx'),
    ("Beta", "auto f=[&](int x){ return v[x]; };"),
    ("Gamma", '// caf\u00e9 \u4e2d\u6587 \U0001F600\n}]{["'),
]
for title, code in seeds:
    subprocess.run([SNIP, "add", "--title", title, "--code", code, "--tags", "x,y"],
                   env=env, capture_output=True)
valid = open(db, "rb").read()
rc, out, err = run(valid)
if rc != 0:
    print("Could not build a valid baseline file:", rc, err[:200])
    sys.exit(2)
print("baseline: %d bytes, %d snippets" % (len(valid), out.count(b"\n| ")))

import json

def _strict_pairs_hook(pairs):
    seen = set()
    for k, v in pairs:
        if k in seen:
            raise ValueError(f"duplicate key {k}")
        seen.add(k)
    return dict(pairs)

def _reject_constant(c):
    raise ValueError(f"invalid JSON constant {c}")

def python_accepts_json(data):
    try:
        text = data.decode("utf-8-sig")
        if not text.strip():
            return True
        json.loads(text, parse_constant=_reject_constant, object_pairs_hook=_strict_pairs_hook)
        return True
    except Exception:
        return False

problems = []


def check(label, data, must_fail=False):
    rc, out, err = run(data)
    if rc not in (0, 1) or b"Sanitizer" in err or b"runtime error" in err:
        problems.append((label, "CRASH/UB/HANG", rc, err[:160]))
    if must_fail and rc == 0:
        problems.append((label, "ACCEPTED A TRUNCATED FILE", rc, out[-100:]))
    if rc == 0 and not python_accepts_json(data):
        problems.append((label, "ACCEPTED MALFORMED JSON (DIFFERENTIAL ORACLE)", rc, data[:100]))


for n in range(len(valid.rstrip())):
    check("truncate@%d" % n, valid[:n], must_fail=(n > 0 and valid[:n].strip() != b""))
print("truncation sweep: %d prefixes checked" % len(valid.rstrip()))

random.seed(SEED)
alphabet = b'"{}[],:\\u0123\n\t -'
for i in range(N_MUT):
    b = bytearray(valid)
    kind = random.choice(["flip", "del", "ins", "dup", "swap"])
    p = random.randrange(len(b))
    if kind == "flip":
        b[p] = random.randrange(256)
    elif kind == "del":
        del b[p]
    elif kind == "ins":
        b.insert(p, random.choice(alphabet))
    elif kind == "dup":
        b[p:p] = b[p:p + random.randint(1, 8)]
    else:
        q = random.randrange(len(b))
        b[p], b[q] = b[q], b[p]
    check("mutation%d:%s@%d" % (i, kind, p), bytes(b))
print("random mutations: %d checked" % N_MUT)

print("problems:", len(problems))
for item in problems[:20]:
    print("  ", item)
sys.exit(1 if problems else 0)
