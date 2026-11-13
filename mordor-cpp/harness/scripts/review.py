"""One ant per harness group whose copies disagree with Core: is the mismatch from the test setup, or a real rule difference?"""
import asyncio, json, os, re, sys
S = sys.argv[1]; H = f"{S}/harness"; sys.path.insert(0, f"{S}/ants2/scripts"); from army import run_all, est
st = json.load(open(f"{H}/state_live.json")); tasks = {t["id"]: t for t in json.load(open(f"{H}/tasks_live.json"))}
SYS = """You review ONE differential test. The same inputs were run through a reference method in Mordor.Core (C#, the reference port of the 1995 RPG Mordor) and through a C++ re-implementation, and some outputs differ. Answer one question: is the difference caused by the TEST SETUP (the two sides were not given the same state or inputs, a unit or format conversion is wrong, a value was read from the wrong place, the C++ side was called differently from the way the game calls it), or is it a REAL difference in the rule?
Reply with exactly three lines:
VERDICT: <SETUP|REAL|UNSURE>
WHY: <one or two sentences pointing at the exact lines that cause the difference>
FIX: <for SETUP, the exact change to Case.cs or case.cpp that makes both sides equal in setup; for REAL, the inputs where the rule differs and how; else none>"""
out = []
for g, v in sorted(st.items()):
    if v["status"] != "DONE": continue
    bad = {k: c for k, c in v["copies"].items() if c["of"] and c["match"] < c["of"]}
    if not bad: continue
    d = f"{H}/groups/{g}"
    rd = lambda n: open(f"{d}/{n}").read() if os.path.exists(f"{d}/{n}") else ""
    ex = "\n".join(f"{k}: {c['match']}/{c['of']} match; examples: " + " ; ".join(c["examples"]) for k, c in bad.items())
    ref = tasks[g]["user"]; ref = ref[ref.find("C++ COPIES:"):][:9000]
    u = f"MISMATCHES:\n{ex}\n\nFIRST CASES (cases.json):\n{rd('cases.json')[:2500]}\n\nCase.cs:\n{rd('Case.cs')[:5000]}\n\ncase.cpp:\n{rd('case.cpp')[:6000]}\n\nTHE CODE UNDER TEST (C++ copy and Core reference):\n{ref}"
    out.append({"id": "rv" + g, "system": SYS, "user": u, "out": f"{H}/review/{g}.md"})
print(len(out), "review ants; max est", max(est(t["system"], t["user"]) for t in out), flush=True)
asyncio.run(run_all(out, f"{H}/review.log.jsonl", 20))
