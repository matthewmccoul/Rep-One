"""Acceptance tests from the reference implementation's own assertions (extracted earlier into ../testcheck/cases). One ant per batch of cases writes tests/test_engine_<chapter>_<k>.cpp.
Expected values come from the reference tests and must be used verbatim. usage: phase_enginetests.py"""
import asyncio, collections, glob, json, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
from partsplit import SLUG, header_view
M = os.path.dirname(ROOT)
TOPIC_CH = {"T01": "06", "T02": "02", "T03": "03", "T04": "04", "T05": "05", "T06": "07", "T07": "07", "T08": "07", "T09": "09", "T10": "10", "T11": "08", "T12": "11", "T13": "12", "T14": "13", "T15": "14", "T16": "15", "T17": "16", "T18": "17", "T19": "18", "T24": "22"}
SYS = ("You translate assertions from a reference implementation's unit tests into C++ tests for a new implementation with a different API. You get cases (question = the inputs and the call, expected = what the reference asserts) and the public headers of the new modules. "
       'Write tests/test_engine_<name>.cpp using `#include "test_support.h"` and the module headers: `TEST(name) { ... }`, CHECK(cond), CHECK_EQ(actual, expected). `tsup::data()` is GameData; `tsup::Env env({0.5, 0.9});` gives env.ctx with a scripted RNG, `tsup::Env env(seed)` a seeded one; env.log is the EventLog. '
       "For each case whose inputs you can set up with the new API and whose call has an equivalent function in the headers, write a test named engine_<caseid>_<what> that asserts the expected value EXACTLY as given (convert units: Currency = gold*10000). "
       "For every other case write only a comment line `// SKIP <caseid>: <reason>` (no equivalent function, needs UI, needs a simulation). Never invent functions: use only functions and members that appear in the headers/state structs shown. Never change an expected value to make it pass. Keep the file under 500 lines.\n"
       "Output ONLY:\n=== FILE: tests/test_engine_<name>.cpp ===\n<code>\n=== END FILE ===")
cases = []
for fn in sorted(glob.glob(f"{M}/testcheck/cases/B_*.jsonl")) + sorted(glob.glob(f"{M}/testcheck/cases/A_0[123].jsonl")):
    for ln in open(fn):
        ln = ln.strip()
        if ln.startswith("{"):
            try: o = json.loads(ln)
            except Exception: continue
            if o.get("topic") in TOPIC_CH and o.get("question") and o.get("expected") not in (None, ""): cases.append(o)
seen, uniq = set(), []
for c in cases:
    k = c["question"][:160]
    if k in seen: continue
    seen.add(k); uniq.append(c)
by = collections.defaultdict(list)
for c in uniq: by[TOPIC_CH[c["topic"]]].append(c)
print({k: len(v) for k, v in sorted(by.items())}, "total", len(uniq), flush=True)
core = api_index.core_digest(); tasks = []
for ch, cs in sorted(by.items()):
    mods = sorted(f[:-2] for f in os.listdir(f"{ROOT}/include/mordor") if re.match(rf"^{SLUG[ch]}\d*[ab]?\.h$", f))
    hdrs = []
    for m in mods:
        hdrs.append(f"// ---- include/mordor/{m}.h\n" + header_view(open(f"{ROOT}/include/mordor/{m}.h").read()))
    htxt = "\n".join(hdrs)
    spine = api_index.spine_for_header("\n".join(open(f"{ROOT}/include/mordor/{m}.h").read() for m in mods))
    for k in range(0, len(cs), 10):
        batch = cs[k:k + 10]; name = f"{SLUG[ch]}_{k // 10 + 1}"
        if os.path.exists(f"{ROOT}/tests/test_engine_{name}.cpp"): continue
        ctxt = "\n".join(f"case {c['id']}: {c['question']}\n  expected: {c['expected']}" for c in batch)
        def mk(h, s): return f"CASES\n{ctxt}\n\nFOUNDATION\n{core}\n{s}\n\nHEADERS OF THE MODULES FOR THIS CHAPTER\n{h}"
        user = mk(htxt, spine)
        h2 = htxt
        while est_tokens(SYS, user) > 31_000 and len(h2) > 6000: h2 = h2[: int(len(h2) * 0.85)]; user = mk(h2, spine)
        if est_tokens(SYS, user) > 31_000: user = mk(h2, ""); 
        print(name, len(batch), "cases est", est_tokens(SYS, user), flush=True)
        tasks.append({"id": name, "system": SYS.replace("<name>", name), "user": user, "allow": [f"tests/test_engine_{name}.cpp"], "save": f"spec/enginetests/{name}.out"})
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, "enginetests", 3))
