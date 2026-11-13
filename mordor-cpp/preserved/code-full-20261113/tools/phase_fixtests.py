"""Failure triage for the SRD-derived tests. One ant per failing test file decides per test: TEST wrong / CODE wrong / SRD AMBIGUOUS, and patches.
usage: phase_fixtests.py  (reads logs/tests_run*.txt of the latest run: pass a log path as argv[1])"""
import asyncio, collections, glob, json, os, re, subprocess, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
from partsplit import condense
from triage import find_function
import phase_impl_common as pic
LOG = sys.argv[1] if len(sys.argv) > 1 else f"{ROOT}/logs/tests_run1.txt"
CONV = open(f"{ROOT}/spec/CONVENTIONS.md").read()
SYS = ("You are a test-triage ant. A unit test written (blind) from the SRD fails against the implementation. You get the failing tests, the failure lines, the source of the functions they call, and the SRD text. "
       "For EACH failing test decide: TEST (the test's expected value or setup contradicts the SRD or the function's signature/units: fix the test), CODE (the function deviates from the SRD: fix the function), or AMBIGUOUS (the SRD does not settle it or the sources conflict: make the test agree with the code and comment `// SRD-AMBIGUOUS`). "
       "Prefer the SRD over both. Keep signatures. Output, for each change, a block:\n"
       "=== REPLACE_TEST <name> ===\n<the complete new TEST(<name>) { ... } definition>\n=== END ===\n"
       "=== REPLACE <file> :: <function> ===\n<the complete new function definition>\n=== END REPLACE ===\n"
       "and finally one line per failing test: `VERDICT <name>: TEST|CODE|AMBIGUOUS - <reason in 10 words>`. No I/O, globals or exceptions.\n\n" + CONV)
if os.environ.get("ENGINE"):
    SYS += ("\n\nIMPORTANT: these tests are translations of assertions from the REFERENCE implementation (the original engine's own tests); the expected values are GROUND TRUTH. "
            "Prefer verdict CODE: fix the function so it returns the reference value. Choose TEST only when the translation is wrong (wrong units: Currency = gold*10000, wrong index base: party members and Party.x/y, wrong argument meaning, impossible setup); then fix the test's setup but NEVER change an expected value. Use AMBIGUOUS only when the setup cannot be reproduced with the new API.")
def parse_failures(path):
    cur = collections.defaultdict(list); order = []
    for ln in open(path, errors="replace"):
        m = re.match(r"FAIL \[(\w+)\] \S*/(tests/[\w.]+):(\d+): (.*)", ln)
        if m: cur[m.group(1)].append((m.group(2), int(m.group(3)), m.group(4)))
    return cur
def test_defs():
    where = {}
    for f in glob.glob(f"{ROOT}/tests/test_*.cpp"):
        for m in re.finditer(r"^TEST\((\w+)\)", open(f).read(), re.M): where[m.group(1)] = f
    return where
def test_body(path, name):
    t = open(path).read(); m = re.search(rf"^TEST\({name}\)\s*\{{", t, re.M)
    if not m: return None
    depth, i = 0, m.end() - 1
    while i < len(t):
        if t[i] == "{": depth += 1
        elif t[i] == "}":
            depth -= 1
            if depth == 0: return m.start(), i + 1
        i += 1
def src_function_index():
    idx = {}
    for f in glob.glob(f"{ROOT}/src/*.cpp"):
        if os.path.basename(f).startswith(("game_data", "json_lite")): continue
        t = open(f).read()
        for m in re.finditer(r"^[A-Za-z_][\w:<>,&\*\s]*?\b(\w+)\s*\([^;{)]*\)\s*(?:const)?\s*\{", t, re.M): idx.setdefault(m.group(1), []).append(f)
    return idx
def apply(out):
    n = 0
    for m in re.finditer(r"=== REPLACE_TEST (\w+) ===\n(.*?)\n=== END ===", out, re.S):
        name, body = m.groups(); f = test_defs().get(name)
        if not f: continue
        loc = test_body(f, name)
        if not loc: continue
        t = open(f).read(); body = re.sub(r"^```[a-z+]*\n", "", body); body = re.sub(r"\n```\s*$", "", body)
        open(f, "w").write(t[:loc[0]] + body + t[loc[1]:]); n += 1
    for m in re.finditer(r"=== REPLACE (\S+) :: (\w+) ===\n(.*?)\n=== END REPLACE ===", out, re.S):
        f, name, body = m.groups(); path = f"{ROOT}/{f}"
        if not os.path.exists(path): continue
        t = open(path).read(); loc = find_function(t, name)
        if not loc: continue
        body = re.sub(r"^```[a-z+]*\n", "", body); body = re.sub(r"\n```\s*$", "", body)
        open(path, "w").write(t[:loc[0]] + body + t[loc[1]:]); n += 1
    return n
if __name__ == "__main__":
    fails = parse_failures(LOG); where = test_defs(); fidx = src_function_index(); byfile = collections.defaultdict(list)
    for name in fails:
        if name in where: byfile[where[name]].append(name)
    print(len(fails), "failing tests in", len(byfile), "files", flush=True)
    mods = pic.modules(); core = api_index.core_digest(); tasks = []
    for f, names in sorted(byfile.items()):
        mod = os.path.basename(f)[5:-4]
        bodies, fnames = [], set()
        for n in names:
            loc = test_body(f, n); b = open(f).read()[loc[0]:loc[1]] if loc else ""
            bodies.append(b); fnames |= set(re.findall(r"\b([a-z]\w{4,})\s*\(", b))
        defs = []; used = 0
        for fn in sorted(fnames):
            if fn in fidx:
                src = fidx[fn][0]; t = open(src).read(); loc = find_function(t, fn)
                if loc and used < 38_000:
                    chunk = f"// ---- {os.path.relpath(src, ROOT)} :: {fn}\n{t[loc[0]:loc[1]]}"; defs.append(chunk); used += len(chunk)
        flines = "\n".join(f"{n}: line {ln} {ex}" for n in names for (_, ln, ex) in fails[n][:6])
        pid, half = mods.get(mod, (None, ""))
        srd = condense(open(f"{ROOT}/spec/parts/{pid}.md").read()) if pid else ""
        hdr = open(f"{ROOT}/include/mordor/{mod}.h").read() if os.path.exists(f"{ROOT}/include/mordor/{mod}.h") else ""
        spine = api_index.spine_for_header(hdr) if hdr else ""
        def mk(srd_t, spine_t, defs_t):
            return f"FAILING TESTS in {os.path.relpath(f, ROOT)}\n" + "\n\n".join(bodies) + f"\n\nFAILURE LINES\n{flines}\n\nFUNCTIONS UNDER TEST\n" + "\n\n".join(defs_t) + f"\n\nSTATE STRUCTS\n{spine_t}\n\nSRD CONTEXT\n{srd_t}"
        user = mk(srd, spine, defs)
        while est_tokens(SYS, user) > 31_000 and len(srd) > 3000: srd = srd[: int(len(srd) * 0.8)]; user = mk(srd, spine, defs)
        if est_tokens(SYS, user) > 31_000: spine = ""; user = mk(srd, spine, defs)
        while est_tokens(SYS, user) > 31_000 and len(defs) > 1: defs = defs[:-1]; user = mk(srd, spine, defs)
        print(mod, len(names), "failing tests; est", est_tokens(SYS, user), flush=True)
        tasks.append({"id": mod, "system": SYS, "user": user, "allow": [], "save": f"spec/fixtests/{mod}.out"})
    asyncio.run(run_all(tasks, "fixtests", 3))
    total, verdicts = 0, collections.Counter()
    for t in tasks:
        p = f"{ROOT}/{t['save']}"
        if os.path.exists(p):
            out = open(p).read(); total += apply(out)
            for v in re.findall(r"VERDICT \w+: (TEST|CODE|AMBIGUOUS)", out): verdicts[v] += 1
    print("applied", total, "patches; verdicts", dict(verdicts))
    json.dump(dict(verdicts), open(f"{ROOT}/logs/fixtests_verdicts_{os.path.basename(LOG)}.json", "w"))
