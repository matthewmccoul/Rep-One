"""Triage UNKNOWN/CONFLICT/SIMPLIFIED/ASSUMED markers against the reference (ground truth), gated.
usage: phase_unknowns.py <round> [--files a.cpp,b.cpp] [--min N] [--limit K]   (GATE=1 recommended; DRY=1 prints chunk plan)
One ant per chunk (functions of one file that carry markers, <= 13k chars of code). It answers per marker RESOLVED / CONFIRMED / STILL_UNKNOWN and patches functions only where the reference clearly differs."""
import asyncio, glob, json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
args = sys.argv[1:]; sys.argv = sys.argv[:1]
import phase_investigate as pi
from cant import ROOT, run_all, est_tokens
MARK = re.compile(r"\b(UNKNOWN|CONFLICT|SIMPLIFIED|ASSUMED|UNRESOLVED|BLOCKED)\b[-\d]*:")
HEAD = re.compile(r"^[A-Za-z_][\w:<>,&\*\s]*?\b(\w+)\s*\([^;{)]*\)\s*(?:const)?\s*\{", re.M)
def functions(text):
    out = []
    for m in HEAD.finditer(text):
        if m.group(1) in ("if", "for", "while", "switch", "namespace"): continue
        depth, j = 0, m.end() - 1
        while j < len(text):
            if text[j] == "{": depth += 1
            elif text[j] == "}":
                depth -= 1
                if depth == 0: break
            j += 1
        out.append((m.group(1), m.start(), j + 1))
    return out
def plan(path, minm):
    text = open(path).read(); fns = functions(text); per = {}
    for m in MARK.finditer(text):
        ln_start = text.rfind("\n", 0, m.start()) + 1; ln_end = text.find("\n", m.end()); 
        note = text[ln_start:ln_end].strip()
        for (n, a, b) in fns:
            if a <= m.start() < b: per.setdefault((n, a, b), []).append(note[:220]); break
    items = sorted(per.items(), key=lambda kv: -len(kv[1]))
    chunks, cur, size = [], [], 0
    for (n, a, b), notes in items:
        body = text[a:b]
        if len(body) > 12000: continue
        if cur and size + len(body) > 13000: chunks.append(cur); cur, size = [], 0
        cur.append((n, body, notes)); size += len(body)
    if cur: chunks.append(cur)
    return chunks if sum(len(v) for v in per.values()) >= minm else []
SYS = ("You triage uncertainty markers in a C++ port of a game's rules. Each function below carries comments such as UNKNOWN, CONFLICT, SIMPLIFIED or ASSUMED: places where the written specification was silent, ambiguous or contradictory. "
       "You also get the REFERENCE implementation (C#, GROUND TRUTH) found by keyword search and SRD paragraphs. For EACH marker decide: RESOLVED (the reference shows what the rule is and the code differs: fix the code), CONFIRMED (the code already does what the reference does), or STILL_UNKNOWN (the reference does not cover it; leave the code). "
       "Change behaviour ONLY when the reference clearly shows different behaviour; never invent. Keep signatures, no I/O, globals or exceptions; use only types and functions shown or standard. If the keyword search did not find the relevant reference code, answer STILL_UNKNOWN. "
       "Output: first the patches, one block per CHANGED function with the complete function:\n=== REPLACE <file> :: <function> ===\n<code>\n=== END REPLACE ===\n"
       "then one line per marker: `MARKER <function>: RESOLVED|CONFIRMED|STILL_UNKNOWN - <reason, quoting the reference line if resolved>`. For a RESOLVED marker, relabel its comment `// RESOLVED-BY-REFERENCE: ...`. Optionally `=== ERRATUM ch<NN> ===` text when the reference proves the SRD wrong or silent.\n\n" + pi.CONV)
rnd = args[0]; rest = args[1:]
files = None; minm = 1; limit = 10**9
if "--files" in rest: files = rest[rest.index("--files") + 1].split(",")
if "--min" in rest: minm = int(rest[rest.index("--min") + 1])
if "--limit" in rest: limit = int(rest[rest.index("--limit") + 1])
tasks, meta = [], {}
done_f = f"{ROOT}/logs/unknowns_done.json"; done = set(json.load(open(done_f))) if os.path.exists(done_f) else set()
for path in sorted(glob.glob(f"{ROOT}/src/*.cpp")):
    base = os.path.basename(path)
    if re.search(r"game_data|json_lite|^game_", base): continue
    if files and base not in files: continue
    for k, ch in enumerate(plan(path, minm)):
        tid = f"{base}#{k}"
        if tid in done or len(tasks) >= limit: continue
        code = "\n\n".join(f"// ---- src/{base} :: {n}\n{b}" for n, b, _ in ch)
        marks = "\n".join(f"{n}: " + " | ".join(s.lstrip('/ ').strip() for s in notes[:8]) for n, _, notes in ch)
        toks = pi.tokens_of(marks, " ".join(n for n, _, _ in ch))
        def packet(b):
            return "\n\n".join([f"FILE src/{base} (chunk {k})", "MARKERS PER FUNCTION\n" + marks[:b["marks"]], "FUNCTIONS\n" + code, "REFERENCE C# (ground truth, found by keyword search)\n" + (pi.cs_rank(toks, b["cs"], 4) or "(nothing found)"),
                                "SRD PARAGRAPHS\n" + (pi.srd_rank(toks, b["srd"]) or "(none)")])
        budget = dict(marks=5000, cs=11000, srd=5000); user = packet(budget); n_try = 0
        while est_tokens(SYS, user) > 31000 and n_try < 12: budget = {a: int(v * .85) for a, v in budget.items()}; user = packet(budget); n_try += 1
        if est_tokens(SYS, user) > 33000: print("skip too big", tid); continue
        if os.environ.get("DRY"): print(tid, "functions", len(ch), "markers", sum(len(x[2]) for x in ch), "est", est_tokens(SYS, user)); continue
        tasks.append({"id": tid, "system": SYS, "user": user, "allow": [], "save": f"spec/investigate/u{rnd}_{base}_{k}.out"}); meta[tid] = base
if os.environ.get("DRY"): sys.exit(0)
print(len(tasks), "chunks", flush=True)
asyncio.run(run_all(tasks, f"unknowns{rnd}", 4))
gate = gbase = None; stats = {"RESOLVED": 0, "CONFIRMED": 0, "STILL_UNKNOWN": 0, "kept": 0, "reverted": 0, "nopatch": 0}; results = {}
for t in tasks:
    p = f"{ROOT}/{t['save']}"
    if not os.path.exists(p): continue
    out = open(p).read(); tid = t["id"]
    for st in re.findall(r"^MARKER \S+?:\s*(RESOLVED|CONFIRMED|STILL_UNKNOWN)", out, re.M): stats[st] += 1
    n = 0
    if os.environ.get("GATE"):
        from gate import Gate
        gate = gate or Gate(); gbase = gbase or gate.run()
        gate.backup(); n = pi.apply_patches(out)
        if n:
            after = gate.build_and_run(); new = gate.new_failures(gbase, after)
            if new:
                gate.restore(); stats["reverted"] += 1; print(f"  REVERTED {tid}: {list(new)[:4]}", flush=True)
                CF = f"{ROOT}/logs/gate_conflicts.json"; cf = json.load(open(CF)) if os.path.exists(CF) else {}
                cf["unknown:" + tid] = [f"{k}: " + "; ".join(v[:2]) for k, v in list(new.items())[:8]]; json.dump(cf, open(CF, "w"), indent=1)
            else: gbase = after; stats["kept"] += 1; print(f"  kept {tid} ({n} patches): engine {len(after['engine'])} fail, srd {len(after['srd'])} fail", flush=True)
        else: stats["nopatch"] += 1
    else: n = pi.apply_patches(out)
    done.add(tid); json.dump(sorted(done), open(done_f, "w"))
    results[tid] = {"patches": n}
json.dump(results, open(f"{ROOT}/logs/unknowns_round{rnd}.json", "w"), indent=1)
print("STATS", stats, flush=True)
