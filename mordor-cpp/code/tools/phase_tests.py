"""Phase 6a: SRD-derived unit tests. One test ant per module writes tests/test_<module>.cpp from the SRD part and the header only (blind to the implementation).
usage: phase_tests.py [module ...]"""
import asyncio, json, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
from partsplit import SLUG, split_half, condense, header_view
import phase_impl_common as pic
SYS = ("You are a test-writer ant. You get the public header of a C++ rules module and the SRD text it implements. You have NOT seen the implementation. Write tests/test_<module>.cpp using the project harness: "
       '#include "test_support.h" and the module header; `TEST(name) { ... }` with CHECK(cond) and CHECK_EQ(actual, expected). `tsup::data()` gives GameData; `tsup::Env env({0.5, 0.9});` gives env.ctx with a scripted RNG (values in [0,1) are returned in order, the last repeats) or `tsup::Env env(seed)` for a seeded RNG; env.log is the EventLog. '
       "Build state structs by default-constructing them and setting fields. Derive every expected value from the SRD text (formulas, tables, worked examples, quirks) by doing the arithmetic yourself; where the result depends on randomness, drive it with ScriptedRng so the value is exact. "
       "Write 8 to 25 focused tests covering the main rules, boundary values and documented quirks. Do not test things the SRD marks UNKNOWN. Only call functions that are declared in the header or in the API INDEX, with their exact signatures. Each test must compile on its own. Keep the file under 600 lines.\n"
       "Output ONLY the file in this form:\n=== FILE: tests/test_<module>.cpp ===\n<code>\n=== END FILE ===")
mods = pic.modules()
want = sys.argv[1:] or list(mods)
core = api_index.core_digest(); tasks = []
for mod in want:
    if mod not in mods or os.path.exists(f"{ROOT}/tests/test_{mod}.cpp"): continue
    pid, half = mods[mod]; part = open(f"{ROOT}/spec/parts/{pid}.md").read()
    if half: part = split_half(part)[0 if half == "a" else 1]
    part = condense(part); header = header_view(open(f"{ROOT}/include/mordor/{mod}.h").read())
    spine = api_index.spine_for_header(open(f"{ROOT}/include/mordor/{mod}.h").read())
    idx = api_index.relevant_index(part + header, exclude=(f"{mod}.h",), cap=6000)
    user = f"MODULE: {mod}\n\nFOUNDATION\n{core}\n{spine}\n\nAPI INDEX\n{idx}\n\nHEADER include/mordor/{mod}.h\n{header}\n\nSRD PART {pid}\n{part}"
    if est_tokens(SYS, user) > 31_000:
        a, b = split_half(part)
        user = f"MODULE: {mod}\n\nFOUNDATION\n{core}\n{spine}\n\nHEADER include/mordor/{mod}.h\n{header}\n\nSRD PART {pid} (first half only)\n{a}"
    print(mod, "est", est_tokens(SYS, user))
    tasks.append({"id": mod, "system": SYS.replace("<module>", mod), "user": user, "allow": [f"tests/test_{mod}.cpp"], "save": f"spec/tests/{mod}.out"})
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, "tests_srd", 4))
