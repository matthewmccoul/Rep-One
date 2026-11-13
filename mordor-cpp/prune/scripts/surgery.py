"""Phase-1 surgery: one ant per copy group writes the edits; the coordinator's script applies them one group at a time through the gate.
usage: surgery.py <W> <A2scripts> [plan indices comma-separated | all] [--round N]"""
import asyncio, glob, json, os, re, sys, collections
W, AS = sys.argv[1], sys.argv[2]; sys.path.insert(0, AS); sys.path.insert(0, f"{W}/scripts")
from army import run_all, est
from gate import Gate
C = f"{W}/code"; EXCLUDE = {55, 59}
P = json.load(open(os.environ.get("PLAN", f"{W}/notes/plan.json")))
sel = sys.argv[3] if len(sys.argv) > 3 else "all"; rnd = int(sys.argv[sys.argv.index("--round") + 1]) if "--round" in sys.argv else 1
ids = [i for i, p in enumerate(P) if any(l["phase"] == 1 for l in p["losers"]) and i not in EXCLUDE]
if sel != "all": ids = [int(x) for x in sel.split(",")]
def strip(t):
    t = re.sub(r"/\*.*?\*/", lambda m: " " * len(m.group(0)), t, flags=re.S); return re.sub(r"//[^\n]*", lambda m: " " * len(m.group(0)), t)
def func_span(path, name, near_line):
    raw = open(path, errors="replace").read(); t = strip(raw); best = None
    for m in re.finditer(rf"^(?:static\s+|inline\s+|constexpr\s+)*[A-Za-z_][\w:<>,&\*\s]*?\b{re.escape(name)}\s*\(", t, re.M):
        j = t.find("{", m.end()); semi = t.find(";", m.end())
        if j < 0 or (0 <= semi < j): continue
        d, k = 0, j
        while k < len(t):
            if t[k] == "{": d += 1
            elif t[k] == "}":
                d -= 1
                if d == 0: break
            k += 1
        line = raw[:m.start()].count("\n") + 1
        if best is None or abs(line - near_line) < abs(best[2] - near_line): best = (m.start(), k + 1, line)
    if not best: return None
    s = best[0]; ls = raw.rfind("\n", 0, s) + 1
    while True:   # include comment lines directly above
        prev = raw.rfind("\n", 0, ls - 1) + 1
        if ls > 0 and raw[prev:ls].strip().startswith("//"): ls = prev
        else: break
    return ls, best[1], raw
def decls(name):
    out = []
    for h in glob.glob(f"{C}/include/mordor/*.h"):
        for i, l in enumerate(open(h, errors="replace").read().split("\n"), 1):
            if re.search(rf"\b{re.escape(name)}\s*\(", l) and l.rstrip().endswith(";"): out.append(f"{os.path.relpath(h, C)}:{i}: {l.strip()}")
    return out
def tests_using(name):
    out = []
    for f in glob.glob(f"{C}/tests/*.cpp"):
        t = open(f, errors="replace").read()
        for m in re.finditer(r"^TEST\((\w+)\)\s*\{", t, re.M):
            d, k = 0, m.end() - 1
            while k < len(t):
                if t[k] == "{": d += 1
                elif t[k] == "}":
                    d -= 1
                    if d == 0: break
                k += 1
            body = t[m.start():k + 1]
            if re.search(rf"\b{re.escape(name)}\s*\(", body): out.append((os.path.relpath(f, C), m.group(1), body))
    return out
SYS = """You are a surgery ant doing ONE cleanup step on a C++17 code base (a re-implementation of the rules of the 1995 RPG Mordor). The coordinator has already decided which duplicate copies of a rule to remove; you only carry the decision out. You get: the KEEPER function (the copy that stays), each LOSER with its action, the header declarations of each loser, and every test that calls a loser.
Actions: DELETE = the loser's definition and its header declaration(s) go away. FORWARD = keep the loser's signature but replace its body with a call to the keeper (adapt arguments and return value exactly; if the keeper cannot express it, say CANNOT_FORWARD). REDIRECT = delete the loser's definition and declaration(s), and change EVERY call site shown under CALL SITES to call the keeper instead (adapt the arguments; use one EDIT per call site, with enough surrounding text to be unique), adding the keeper's header with ADD_INCLUDE where the file needs it. If the keeper is file-local and a loser's file must call it, make the keeper visible as described below.
For every test that calls a DELETE loser: if the same expected values must also hold for the keeper (same rule, compatible inputs), rewrite the test to call the keeper instead, keeping every expected value; otherwise remove the test. Never change an expected value.
Output ONLY these blocks:
=== DELETE_FUNCTION <src file> :: <name> ===
=== EDIT <file> ===
<old text copied exactly, occurring once in that file>
=== TO ===
<new text>
=== END EDIT ===
=== REPLACE_TEST <test name> ===
<complete new TEST(<name>) { ... }>
=== END TEST ===
=== REMOVE_TEST <test name> | <one-line reason> ===
If the keeper is file-local (static, or inside an anonymous namespace) and another file must call it, make it visible: move it out of the anonymous namespace (or drop static) with an EDIT in its own file, and declare it in the matching header under include/mordor/ with an EDIT. Use EDIT blocks to delete header declarations (new text empty) and for FORWARD bodies. To add an include to a file whose top you were not shown, write one line: === ADD_INCLUDE <src or include file> :: <header path as written in quotes, e.g. mordor/death3.h> === (the applier puts it after the file's last top-level #include). Finish with one line: SUMMARY: <what you did>."""
def packet(i, extra=""):
    p = P[i]; k = p["keeper"]; parts = [f"RULE: {p['rule']}"]
    ks = func_span(f"{C}/{k['file']}", k["name"], k["line"])
    parts.append(f"KEEPER {k['name']} in {k['file']} (matches reference: {k['verdict']})\n" + (ks[2][ks[0]:ks[1]][:4000] if ks else "(not found)") + "\nDeclarations: " + "; ".join(decls(k["name"])[:4]))
    for l in p["losers"]:
        if l["phase"] != 1: continue
        ls = func_span(f"{C}/{l['file']}", l["name"], l["line"])
        parts.append(f"LOSER {l['name']} in {l['file']} near line {l['line']} -> ACTION {l['action']}\n" + (ls[2][ls[0]:ls[1]][:3500] if ls else "(already gone)") + "\nDeclarations:\n" + "\n".join(decls(l["name"])[:6]))
        for f, tn, body in tests_using(l["name"])[:14]: parts.append(f"TEST {tn} in {f}\n{body[:2500]}")
        if l["action"] == "REDIRECT":
            sites = []
            for g in sorted(glob.glob(f"{C}/src/*.cpp") + glob.glob(f"{C}/include/mordor/*.h") + glob.glob(f"{C}/tests/*.cpp") + glob.glob(f"{C}/apps/*/*.cpp")):
                raw = open(g, errors="replace").read(); L = raw.splitlines()
                if os.path.basename(g) != os.path.basename(l["file"]) and re.search(rf"^[A-Za-z_][\w:<>,&\*\s]*\b{re.escape(l['name'])}\s*\([^;]*\)\s*(const)?\s*\{{", raw, re.M): continue   # file has its own definition of that name
                for n, line in enumerate(L):
                    if re.search(rf"\b{re.escape(l['name'])}\s*\(", line) and not re.match(rf"^[A-Za-z_][\w:<>,&\*\s]*\b{re.escape(l['name'])}\s*\([^;]*\)\s*(const)?\s*\{{?\s*$", line):
                        sites.append(f"{os.path.relpath(g, C)}:{n+1}:\n" + "\n".join(L[max(0, n - 2):n + 3]))
            parts.append(f"CALL SITES of {l['name']} ({len(sites)}):\n" + "\n---\n".join(sites[:40]))
    if extra: parts.append("PREVIOUS ATTEMPT FAILED:\n" + extra)
    return "\n\n".join(parts)
def apply(out, i):
    n = 0; problems = []
    for m in re.finditer(r"=== ADD_INCLUDE (\S+) :: (\S+) ===", out):
        f, h = m.groups(); p = f"{C}/{f}"
        if not os.path.exists(p): problems.append(f"add_include: {f} not found"); continue
        t = open(p).read(); inc = f'#include "{h.strip(chr(34))}"'
        if inc in t: continue
        last = [mm.end() for mm in re.finditer(r"^#include[^\n]*\n", t, re.M)]
        pos = last[-1] if last else 0; open(p, "w").write(t[:pos] + inc + "\n" + t[pos:]); n += 1
    for m in re.finditer(r"=== DELETE_FUNCTION (\S+) :: (\w+) ===", out):
        f, name = m.groups(); l = next((l for l in P[i]["losers"] if l["name"] == name and l["file"] == f), None)
        sp = func_span(f"{C}/{f}", name, l["line"] if l else 0) if os.path.exists(f"{C}/{f}") else None
        if not sp: problems.append(f"delete: {f}::{name} not found"); continue
        a, b, raw = sp; open(f"{C}/{f}", "w").write(raw[:a] + raw[b:].lstrip("\n")); n += 1
    for m in re.finditer(r"=== EDIT (\S+) ===\n(.*?)\n=== TO ===\n(.*?)\n?=== END EDIT ===", out, re.S):
        f, old, new = m.groups(); p = f"{C}/{f}"
        if not os.path.exists(p): problems.append(f"edit: no file {f}"); continue
        t = open(p).read()
        if t.count(old) != 1: problems.append(f"edit: old text found {t.count(old)}x in {f}"); continue
        open(p, "w").write(t.replace(old, new)); n += 1
    for m in re.finditer(r"=== (REPLACE_TEST (\w+) ===\n(.*?)\n=== END TEST ===|REMOVE_TEST (\w+)\s*\|\s*(.*?) ===)", out, re.S):
        name = m.group(2) or m.group(4)
        for f in glob.glob(f"{C}/tests/*.cpp"):
            t = open(f).read(); mm = re.search(rf"^TEST\({name}\)\s*\{{", t, re.M)
            if not mm: continue
            d, k = 0, mm.end() - 1
            while k < len(t):
                if t[k] == "{": d += 1
                elif t[k] == "}":
                    d -= 1
                    if d == 0: break
                k += 1
            new = m.group(3) if m.group(2) else ""
            open(f, "w").write(t[:mm.start()] + new + t[k + 1:]); n += 1; break
    # tests that now call the keeper need the keeper's header
    k = P[i]["keeper"]; hs = sorted({d.split(":")[0] for d in decls(k["name"])})
    if hs:
        inc = '#include "mordor/' + os.path.basename(hs[0]) + '"'
        for f in glob.glob(f"{C}/tests/*.cpp"):
            t = open(f).read()
            if re.search(rf"\b{re.escape(k['name'])}\s*\(", t) and inc not in t and not any(('mordor/' + os.path.basename(h)) in t for h in hs):
                pos = t.find("#include"); pos = t.find("\n", pos) + 1 if pos >= 0 else 0
                open(f, "w").write(t[:pos] + inc + "\n" + t[pos:])
    return n, problems
os.makedirs(f"{W}/surgery", exist_ok=True)
state_f = os.environ.get("STATE", f"{W}/notes/surgery_state.json"); state = json.load(open(state_f)) if os.path.exists(state_f) else {}
todo = [i for i in ids if state.get(str(i), {}).get("status") != "DONE"]
tasks = [{"id": f"g{i}", "system": SYS, "user": packet(i, state.get(str(i), {}).get("problem", "") if rnd > 1 else ""), "out": f"{W}/surgery/{os.environ.get('PREFIX', '')}r{rnd}_g{i}.md"} for i in todo]
print(len(tasks), "surgery ants; max est", max([est(t["system"], t["user"]) for t in tasks] or [0]), flush=True)
asyncio.run(run_all(tasks, f"{W}/surgery.log.jsonl", 5))
g = Gate(W)
for i in todo:
    p = f"{W}/surgery/{os.environ.get('PREFIX', '')}r{rnd}_g{i}.md"
    if not os.path.exists(p): state[str(i)] = {"status": "NO_OUTPUT"}; continue
    out = open(p).read()
    if "CANNOT_FORWARD" in out and "=== " not in out: state[str(i)] = {"status": "CANNOT", "problem": out[:800]}; continue
    n, probs = apply(out, i)
    r = g.check(golden_must_match=str(i) not in os.environ.get('GOLDEN_MAY_MOVE', '').split(','))
    removed = re.findall(r"=== REMOVE_TEST (\w+)\s*\|\s*(.*?) ===", out); replaced = re.findall(r"=== REPLACE_TEST (\w+) ===", out)
    if r["ok"] and not probs:
        g.commit(f"prune group {i}: keep {P[i]['keeper']['name']}; " + ", ".join(f"{l['action'].lower()} {l['name']}" for l in P[i]["losers"] if l["phase"] == 1))
        state[str(i)] = {"status": "DONE", "edits": n, "tests_removed": removed, "tests_retargeted": replaced, "suites": [r["srd"], r["engine"]], "golden_moved": r.get("golden_diffs", [])}
        print(f"group {i}: DONE ({n} edits; tests retargeted {len(replaced)}, removed {len(removed)}) srd {r['srd']} engine {r['engine']}", flush=True)
    else:
        g.revert(); state[str(i)] = {"status": "FAILED", "problem": (r.get("why", "") + "\n" + r.get("log", "") + "\n" + "\n".join(probs))[:2500]}
        print(f"group {i}: REVERTED - {r.get('why') or probs[:2]}", flush=True)
    json.dump(state, open(state_f, "w"), indent=1)
print("DONE:", sum(1 for v in state.values() if v["status"] == "DONE"), "| FAILED:", [k for k, v in state.items() if v["status"] != "DONE"], flush=True)
