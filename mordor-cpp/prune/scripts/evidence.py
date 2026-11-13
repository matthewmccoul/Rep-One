"""Evidence packets per pile: members' C++ bodies + callers summary, and the Mordor.Core methods linked to the pile."""
import asyncio, json, os, re, sys, collections
sys.path.insert(0, "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/ants2/scripts"); from army import run_all, est
W = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/prune"; CORE = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/build/od/src/Mordor.Core"
inv = json.load(open(f"{W}/notes/inventory.json")); P = json.load(open(f"{W}/notes/piles.json"))
for i, f in enumerate(inv): f["id"] = i
mp = json.load(open("/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/ants2/notes/mapping_final.json"))
core_of = collections.defaultdict(set)
for r in mp:
    for n in r.get("cpp") or []: core_of[n].add((r["file"], r["method"]))
def csbody(file, method):
    meth = re.sub(r"\(.*", "", method.split(".", 1)[-1]); t = open(f"{CORE}/{file}", encoding="utf-8", errors="replace").read()
    for m in re.finditer(rf"^\s{{4,8}}(?:public|internal|private)[^\n;=]*\b{re.escape(meth)}\s*(?:<[^>\n]*>)?\s*\(", t, re.M):
        i = t.find("{", m.end()); arrow = t.find("=>", m.end()); semi = t.find(";", m.end())
        if 0 <= arrow < (i if i >= 0 else 10**9): return t[m.start():semi + 1]
        if i < 0: continue
        d, j = 0, i
        while j < len(t):
            if t[j] == "{": d += 1
            elif t[j] == "}":
                d -= 1
                if d == 0: return t[m.start():j + 1]
            j += 1
    return ""
SYS = """You are an evidence ant for a code cleanup of a C++ re-implementation of the rules of the 1995 RPG Mordor. The reference implementation is Mordor.Core (C#). A script grouped some C++ functions into a "pile" because they might be duplicate implementations of the same rule. You do NOT decide what to delete; you report facts.
For the pile, output:
1) GROUPS: one line per group of members that implement THE SAME rule (same inputs and purpose), for example: G1: m3, m7 | rule: chest trap 5 item destruction. A member that is not a copy of any other member goes in its own group. Members that merely call each other or share a helper are NOT copies.
2) For every member, one line:
mN | group Gk | vs reference: MATCHES or DIFFERS or NO_REFERENCE | short reason (name the reference method; for DIFFERS say exactly what differs: formula, rounding, order of random draws, ranges, missing cases)
Judge MATCHES only when the C++ computes the same result as the shown C# reference for the same inputs. If no reference method shown covers that rule, say NO_REFERENCE. Base every claim on the code shown. Output only these lines."""
os.makedirs(f"{W}/packets", exist_ok=True); tasks = []; meta = {}
for p in P["piles"]:
    mem = [inv[i] for i in p["members"]]
    if len(mem) < 2: continue
    refs = set()
    for f in mem: refs |= core_of.get(f["name"], set())
    refs = sorted(refs)[:8]
    def mk(bcap, rcap):
        parts = [f"PILE {p['pile']} ({len(mem)} members)"]
        for k, f in enumerate(mem, 1):
            g = [c.split("@")[0] for c in f["callers"]]
            parts.append(f"--- m{k}: {f['name']} ({f['file']}:{f['line']}, {f['lines']} lines; called by {len(g)} functions{': ' + ', '.join(g[:6]) if g else ''}; test refs {f['test_refs']}; linker-dead {f['dead']})\n" + f["body"][:bcap])
        for fl, m in refs:
            b = csbody(fl, m)
            if b: parts.append(f"=== REFERENCE Mordor.Core {m} ({fl})\n" + b[:rcap])
        return "\n\n".join(parts)
    bc, rc = 3000, 2500; u = mk(bc, rc)
    while est(SYS, u) > 31000 and bc > 600: bc, rc = int(bc * .8), int(rc * .8); u = mk(bc, rc)
    pid = f"p{p['pile']:03d}"; meta[pid] = {"pile": p["pile"], "members": p["members"], "refs": refs}
    tasks.append({"id": pid, "system": SYS, "user": u, "out": f"{W}/out/{pid}.md"})
json.dump(meta, open(f"{W}/notes/evidence_meta.json", "w"), indent=0)
print(len(tasks), "evidence ants; max est", max(est(t["system"], t["user"]) for t in tasks), "; piles with no reference", sum(1 for m in meta.values() if not m["refs"]))
if len(sys.argv) > 1 and sys.argv[1] == "RUN": asyncio.run(run_all(tasks, f"{W}/evidence.log.jsonl", 5))
