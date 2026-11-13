"""Implement functions that headers declare but no source defines. One ant per header. usage: phase_missing.py"""
import asyncio, json, os, sys
SUF = os.environ.get('SUF', '')
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
from partsplit import split_half, condense, header_view
import phase_impl_common as pic
CONV = open(f"{ROOT}/spec/CONVENTIONS.md").read()
SYS = ("You are an implementer ant finishing a module. Some functions declared in the header were never implemented. Implement EXACTLY the listed functions (same signatures) following the SRD text; "
       "do not define anything else, do not redefine functions that other headers declare. Randomness only through ctx.rng, output only through ctx.say. Cite rule numbers; mark `// UNKNOWN:`/`// CONFLICT:` where the SRD does. "
       "If the SRD does not say how a function should behave, implement the most reasonable behaviour from its name, signature and neighbouring rules and mark `// ASSUMED:`.\n\n" + CONV +
       "\nOutput ONLY:\n=== FILE: src/<module>_missing{SUF}.cpp ===\n<code>\n=== END FILE ===")
missing = json.load(open(f"{ROOT}/logs/missing_defs.json")); mods = pic.modules(); core = api_index.core_digest(); tasks = []
for hb, decls in missing.items():
    mod = hb[:-2]
    if mod not in mods or os.path.exists(f"{ROOT}/src/{mod}_missing{SUF}.cpp"): continue
    pid, half = mods[mod]; part = open(f"{ROOT}/spec/parts/{pid}.md").read()
    if half: part = split_half(part)[0 if half == "a" else 1]
    part = condense(part); header = open(f"{ROOT}/include/mordor/{mod}.h").read(); hv = header_view(header)
    spine = api_index.spine_for_header(header); idx = api_index.relevant_index(part + hv, exclude=(hb,), cap=7000)
    def build(txt, note=""):
        return (f"MODULE: {mod}   write src/{mod}_missing{SUF}.cpp\n\nFUNCTIONS TO IMPLEMENT{note}:\n" + "\n".join(decls) + f"\n\nFOUNDATION AND SHARED STATE\n{core}\n{spine}\n\nAPI INDEX\n{idx}\n\nHEADER include/mordor/{mod}.h\n{hv}\n\nSRD PART {pid}\n{txt}")
    user = build(part)
    if est_tokens(SYS, user) > 31_000:
        a, b = split_half(part); user = build(a, " (implement those whose rules appear in the SRD text below; others are handled elsewhere)")
        user2 = build(b, " (implement those whose rules appear in the SRD text below; others are handled elsewhere)")
        tasks.append({"id": mod + "_m2", "system": SYS.replace("<module>", mod + "2").replace("{SUF}", SUF), "user": user2, "allow": [f"src/{mod}2_missing{SUF}.cpp"], "save": f"spec/missing/{mod}2.out"})
    print(mod, len(decls), "functions, est", est_tokens(SYS, user))
    tasks.append({"id": mod + "_m", "system": SYS.replace("<module>", mod).replace("{SUF}", SUF), "user": user, "allow": [f"src/{mod}_missing{SUF}.cpp"], "save": f"spec/missing/{mod}.out"})
asyncio.run(run_all(tasks, "missing", 4))
