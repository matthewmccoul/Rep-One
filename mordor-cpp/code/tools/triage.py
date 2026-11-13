"""Integration triage: an ant reads a symptom plus the functions involved and returns replacement functions.
usage: triage.py "<symptom text>" file.cpp::function [file.cpp::function ...] [--srd c02p1,c04p1] [--note "extra hint"]
The ant may replace any of the listed functions (full new definitions); helpers may be added in the same block."""
import asyncio, glob, os, re, subprocess, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
from partsplit import condense
CONV = open(f"{ROOT}/spec/CONVENTIONS.md").read()
SYS = ("You are a debugging ant for a C++ rules library. You get a SYMPTOM observed when running the game, the source of the functions involved, the state structs, and relevant declarations. "
       "Find the root cause and give corrected versions of the functions that need changing (only those listed). Keep the signatures. Follow the SRD context where given. Do not invent members or functions that are not shown. "
       "No I/O, globals or exceptions. Output ONE block per replaced function, each with the COMPLETE new definition (you may put small static helper functions before it inside the same block):\n"
       "=== REPLACE <file> :: <function> ===\n<code>\n=== END REPLACE ===\nThen one line `DIAGNOSIS: <root cause in one sentence>`.\n\n" + CONV)
def find_function(text, name):
    pat = re.compile(rf"^[A-Za-z_][\w:<>,&\*\s]*\b{re.escape(name)}\s*\(", re.M)
    for m in pat.finditer(text):
        i = m.start(); head_end = text.find("{", m.end()); semi = text.find(";", m.end())
        if head_end < 0 or (0 <= semi < head_end): continue
        depth = 0; j = head_end
        while j < len(text):
            if text[j] == "{": depth += 1
            elif text[j] == "}":
                depth -= 1
                if depth == 0: return i, j + 1
            j += 1
    return None
def apply(out):
    n = 0
    for m in re.finditer(r"=== REPLACE (\S+) :: (\w+) ===\n(.*?)\n=== END REPLACE ===", out, re.S):
        f, name, body = m.groups(); path = f"{ROOT}/{f}"
        if not os.path.exists(path): print("no file", f); continue
        t = open(path).read(); loc = find_function(t, name)
        if not loc: print("function not found", f, name); continue
        body = re.sub(r"^```[a-z+]*\n", "", body); body = re.sub(r"\n```\s*$", "", body)
        # keep leading comment lines attached to the old function out of the replacement span
        open(path, "w").write(t[:loc[0]] + body + t[loc[1]:]); n += 1
    return n
if __name__ == "__main__":
    args = sys.argv[1:]; srd = []; note = ""
    if "--srd" in args: i = args.index("--srd"); srd = args[i + 1].split(","); del args[i:i + 2]
    if "--note" in args: i = args.index("--note"); note = args[i + 1]; del args[i:i + 2]
    symptom, targets = args[0], args[1:]
    srcs = []; words = symptom + " " + note
    for tg in targets:
        f, fn = tg.split("::"); t = open(f"{ROOT}/{f}").read(); loc = find_function(t, fn)
        if not loc: print("not found:", tg); sys.exit(1)
        body = t[loc[0]:loc[1]]; srcs.append(f"// ---- {f} :: {fn}\n{body}"); words += " " + body
    spine = api_index.spine_digest(include_game=False) + "\n// ---- game_data.h (static tables; floor flag enum values are BIT POSITIONS: mask = 1u << FLAG_X; Party.x/y are 1-based, square index = (y-1)*width + (x-1))\n" + api_index.digest(f"{ROOT}/include/mordor/game_data.h")
    idx = api_index.relevant_index(words, cap=9000)
    srdtxt = "\n\n".join(condense(open(f"{ROOT}/spec/parts/{p}.md").read()) for p in srd)
    def mk(s):
        return f"SYMPTOM\n{symptom}\n{('NOTE: ' + note) if note else ''}\n\nFUNCTIONS\n" + "\n\n".join(srcs) + f"\n\nSTATE STRUCTS\n{spine}\n\nAPI INDEX\n{idx}\n\nSRD CONTEXT\n{s}"
    user = mk(srdtxt)
    while est_tokens(SYS, user) > 31_000 and len(srdtxt) > 2000: srdtxt = srdtxt[: int(len(srdtxt) * 0.8)]; user = mk(srdtxt)
    if est_tokens(SYS, user) > 31_000: spine = ""; user = mk(srdtxt)
    tid = re.sub(r"\W+", "_", symptom)[:40]
    asyncio.run(run_all([{"id": tid, "system": SYS, "user": user, "allow": [], "save": f"spec/triage/{tid}.out"}], "triage", 1))
    out = open(f"{ROOT}/spec/triage/{tid}.out").read()
    print("replaced", apply(out), "functions;", re.findall(r"DIAGNOSIS:.*", out)[:1])
