"""Stage A: one ant per C++ function without a Mordor.Core mapping. One question: what kind of thing is it?
usage: stage_a.py <S> [limit]"""
import asyncio, json, os, re, sys, collections
S = sys.argv[1]; F = f"{S}/feat"; sys.path.insert(0, f"{S}/ants2/scripts"); from army import run_all, est
C = f"{S}/prune/code"; lim = int(sys.argv[2]) if len(sys.argv) > 2 else None
defs = [d for d in json.load(open(f"{F}/defs.json")) if not d["mapped_to"]]
heads = {}
def head(f):
    if f not in heads:
        lines = open(f"{C}/{f}", errors="replace").read().splitlines()[:12]
        heads[f] = "\n".join(l for l in lines if l.startswith("//"))[:800]
    return heads[f]
def body(d):
    lines = open(f"{C}/{d['file']}", errors="replace").read().splitlines()
    a, b = int(d["line"]) - 1, int(d["end"])
    pre = []
    k = a - 1
    while k >= 0 and lines[k].strip().startswith("//") and len(pre) < 12: pre.insert(0, lines[k]); k -= 1
    return "\n".join(pre + lines[a:b])[:3500]
SYS = """You classify ONE function from a C++ re-implementation of the rules of the 1995 dungeon RPG "Mordor: The Depths of Dejenol". You get the function, the comment at the top of its file, and the names of its callers. Answer one question: what kind of thing is this function?
Kinds:
PLUMBING = general programming utility with no game meaning (number/text formatting, string helpers, math helpers, index conversion, container access, table lookup by id).
RULE = computes or applies a game mechanic (a chance, a formula, damage, cost, a state change the game's rules define).
FLOW = game flow or player-command handling (menus, town/dungeon screens, parsing a typed command, producing messages for the player).
DATA = reading, writing or validating data files, saves, or formats.
MOD = mod or plugin support, or engine extension hooks.
STUB = placeholder: empty body, always returns a constant, or marked as not implemented.
Reply with exactly two lines and nothing else:
KIND: <one kind>
WHAT: <one sentence saying what it does, in game terms where it has game meaning>"""
tasks = []
for i, d in enumerate(defs[:lim] if lim else defs):
    tid = f"a{i:04d}"; out = f"{F}/a/{tid}.md"
    u = f"FILE: {d['file']}\nFILE HEADER:\n{head(d['file'])}\n\nCALLERS: {', '.join(c.split('@')[0] for c in (d['callers'] if isinstance(d['callers'],list) else eval(d['callers']))[:12]) or 'none'}\n\nFUNCTION:\n{body(d)}"
    tasks.append({"id": tid, "system": SYS, "user": u, "out": out, "def": f"{d['name']}@{d['file']}:{d['line']}"})
json.dump({t["id"]: t["def"] for t in tasks}, open(f"{F}/a_index.json", "w"))
print(len(tasks), "ants; max est", max(est(t["system"], t["user"]) for t in tasks), flush=True)
asyncio.run(run_all(tasks, f"{F}/a.log.jsonl", int(os.environ.get("CONC", "8"))))
