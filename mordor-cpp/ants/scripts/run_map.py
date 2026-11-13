"""One ant per Mordor.Core packet: for each public/internal method, does the C++ demo implement it? usage: run_map.py <ids|all> [conc]"""
import asyncio, json, os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from army import run_all, est
A2 = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SYS = """You are one ant in a team comparing two implementations of the rules of the 1995 dungeon RPG Mordor: Mordor.Core (C#, the reference) and a C++ re-implementation written separately from a rules document (different names, camelCase, free functions, often several functions per rule).
You receive ONE Mordor.Core source file (or part) and a list of C++ declarations that a script found as possible counterparts (by name similarity or because the C++ cites the C# name). You see C++ declarations only, not their bodies.
For EVERY public or internal method in the C# packet, output one block:
### M <ClassName.MethodName>
Status: PRESENT | PARTIAL | MISSING | OUT_OF_SCOPE
C++: <the counterpart name(s) copied EXACTLY from the candidate list, comma separated, or NONE>
Note: <one short sentence: what the C# method does, and for PARTIAL what the C++ side seems to lack>
PRESENT = a candidate clearly implements the same rule (matching purpose and inputs). PARTIAL = a candidate covers part of it, or only a related rule. MISSING = no candidate does this. OUT_OF_SCOPE = the method only reads the original game's files, installers, art, help or old save formats, draws or lays out a window page, plays sound, or handles app settings and updates (not game rules).
Judge only from what is shown; when unsure between PRESENT and PARTIAL choose PARTIAL. Never invent C++ names. Trivial members (ToString, Equals, property-like getters) may be grouped into one block named <ClassName.(trivial)>. Output only the blocks."""
man = json.load(open(f"{A2}/notes/manifest.json")); sel = sys.argv[1]; conc = int(sys.argv[2]) if len(sys.argv) > 2 else 4
ids = [m["id"] for m in man] if sel == "all" else sel.split(",")
os.makedirs(f"{A2}/map", exist_ok=True)
tasks = [{"id": f"cm{i}", "system": SYS, "user": open(f"{A2}/packets/{i}.txt").read(), "out": f"{A2}/map/{i}.md"} for i in ids]
print("ants", len(tasks), "max est", max(est(t["system"], t["user"]) for t in tasks), flush=True)
asyncio.run(run_all(tasks, f"{A2}/map.log.jsonl", conc))
