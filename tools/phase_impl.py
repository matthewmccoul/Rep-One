"""Phase 4: implementers. One ant per module header writes src/<module>.cpp. usage: phase_impl.py [module ...]"""
import asyncio, json, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
from partsplit import SLUG
from partsplit import split_half, split_k, condense, header_view
CONV = open(f"{ROOT}/spec/CONVENTIONS.md").read()
SYS = ("You are an implementer ant. Implement the C++ module whose public header is given, following the SRD part exactly: its formulas, constants, thresholds and order of evaluation. "
       "Define EVERY function declared in your header exactly once, with the same signature. Do not define functions that are declared in other headers (call them: they are implemented by other ants; their signatures are in the API INDEX). "
       "Cite rule numbers in comments. Randomness only through ctx.rng, output only through ctx.say. Translate the SRD's pseudo-code faithfully, including its documented quirks. "
       "Where the SRD says UNKNOWN or CONFLICT choose the stated preferred behaviour or the simplest sensible one and mark it `// UNKNOWN:` or `// CONFLICT:`. "
       "Include only headers that exist (mordor/*.h as in the API INDEX and the state headers). Keep each file under ~900 lines: if the module is long write src/<module>.cpp and src/<module>_2.cpp (each function defined once overall).\n\n"
       + CONV + "\nOutput ONLY files in this exact form (one block per file):\n=== FILE: src/<module>.cpp ===\n<code>\n=== END FILE ===")
parts = json.load(open(f"{ROOT}/spec/parts.json"))
FOUND = {"types", "rng", "events", "ctx", "math_util", "json_lite", "game_data"}
def modules():
    """module name -> (part id, half) for every non-foundation, non-state header that exists."""
    out = {}
    for f in sorted(os.listdir(f"{ROOT}/include/mordor")):
        m = f[:-2]
        if not f.endswith(".h") or m in FOUND or m.startswith("state_"): continue
        mm = re.match(r"^([a-z]+?)(\d*)([ab]?)$", m)
        if not mm: continue
        slug, k, half = mm.groups()
        ch = [c for c, v in SLUG.items() if v == slug]
        if not ch: continue
        pid = f"c{ch[0]}p{k or 1}"
        out[m] = (pid, half)
    return out
mods = modules()
want = sys.argv[1:] or list(mods)
MAXF = 20   # functions per implementer ant (keeps files and outputs small)
core = api_index.core_digest(); tasks = []
for mod in want:
    hp = f"{ROOT}/include/mordor/{mod}.h"
    if not os.path.exists(hp) or os.path.exists(f"{ROOT}/src/{mod}.cpp") or os.path.exists(f"{ROOT}/src/{mod}_a.cpp"): continue
    pid, half = mods[mod]; part = open(f"{ROOT}/spec/parts/{pid}.md").read(); header = open(hp).read()
    if half: part = split_half(part)[0 if half == 'a' else 1]
    part = condense(part); header_full = header; header = header_view(header)
    spine = api_index.spine_for_header(header)
    idx = api_index.relevant_index(part + header, exclude=(f"{mod}.h",), cap=9000)
    def build(part_text, only=None, tag=""):
        scope = (f"MODULE: {mod}   (implement the functions of include/mordor/{mod}.h in src/{mod}{tag}.cpp)\n" if not only else
                 f"MODULE: {mod}, FILE src/{mod}{tag}.cpp. Another ant implements the rest of this header from the other half of the SRD. YOU implement ONLY these functions: {', '.join(only)}. Do not define any other function.\n")
        return (f"{scope}\nFOUNDATION AND SHARED STATE\n{core}\n{spine}\n\nAPI INDEX (other modules, relevant parts)\n{idx}\n\n"
                f"YOUR HEADER include/mordor/{mod}.h (comments stripped)\n{header}\n\nSRD PART {pid}{(' (half ' + half_tag + ')') if only else ''}\n{part_text}")
    names = []
    for l in header.split("\n"):
        if l.startswith((" ", "\t")) or "{" in l or not l.rstrip().endswith(";") or "(" not in l: continue
        if re.match(r"^(struct|enum|using|constexpr|inline constexpr|static|typedef)", l.strip()): continue
        m = re.search(r"(\w+)\s*\(", l)
        if m and m.group(1) not in names: names.append(m.group(1))
    user = build(part)
    est = est_tokens(SYS, user)
    k_by_size = 2 if est > 31_000 else 1
    k_by_funcs = -(-len(names) // MAXF) if names else 1
    k = max(k_by_size, k_by_funcs)
    if k == 1:
        print(mod, pid, "est", est)
        tasks.append({"id": mod, "system": SYS.replace("<module>", mod), "user": user, "allow": [f"src/{mod}", f"src/{mod}_"], "save": f"spec/impl/{mod}.out"})
        continue
    if len(names) < 2:
        idx_saved = idx; idx = ""; u2 = build(part); e2 = est_tokens(SYS, u2); idx = idx_saved
        if e2 <= 31_000:
            print(mod, pid, "est", e2, "(API index dropped)")
            idx_saved = idx; idx = ""; u2 = build(part); idx = idx_saved
            tasks.append({"id": mod, "system": SYS.replace("<module>", mod), "user": u2, "allow": [f"src/{mod}", f"src/{mod}_"], "save": f"spec/impl/{mod}.out"}); continue
        names = ["every function whose rules appear in this part of the SRD"]
        groups = [names] * k
        idx_saved = idx; idx = ""
    else:
        size = -(-len(names) // k); groups = [names[i:i + size] for i in range(0, len(names), size)]; idx_saved = idx
    pieces = split_k(part, len(groups))
    for gi, (grp, txt) in enumerate(zip(groups, pieces)):
        tag = "_" + "abcdefgh"[gi]; half_tag = "abcdefgh"[gi]
        u = build(txt, grp, tag); e = est_tokens(SYS, u)
        print(mod + tag, pid, "est", e, f"(group {gi+1}/{len(groups)}: {len(grp)} functions)")
        tasks.append({"id": mod + tag, "system": SYS.replace("<module>", mod), "user": u, "allow": [f"src/{mod}{tag}.cpp"], "save": f"spec/impl/{mod}{tag}.out"})
    idx = idx_saved
if os.environ.get('DRY'): sys.exit(0)
asyncio.run(run_all(tasks, "impl", 4))
