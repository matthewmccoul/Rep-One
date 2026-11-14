"""One model call per Core file: C# -> C++ header + source, following RULES.md and the catalog. usage: translate.py <X> [files|all]"""
import asyncio, json, os, re, sys
X = sys.argv[1]; sel = sys.argv[2] if len(sys.argv) > 2 else "all"
sys.path.insert(0, f"{X}/scripts"); from army import run_all, est
CORE = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/dl/src/OpenDejenol-v0.1.3/src/Mordor.Core"
cat = json.load(open(f"{X}/catalog.json")); RULES = open(f"{X}/scripts/RULES.md").read(); CSH = open(f"{X}/cpp/cs.h").read()
TUP = open(f"{X}/cpp/tuples_list.txt").read().splitlines()
files = sorted(os.path.relpath(os.path.join(d, f), CORE) for d, _, fs in os.walk(CORE) for f in fs if f.endswith(".cs") and "/obj/" not in d and "/bin/" not in d and not d.endswith(("/obj", "/bin")))
files = [f for f in files if not f.startswith(("obj/", "bin/"))]
if sel != "all": files = [f for f in files if f in sel.split(",")]
simple = {}
for q, c in cat.items(): simple.setdefault(q.split(".")[-1], []).append(q)
def stem(f): return f[:-3].replace("/", "_").replace(".", "_")
def strip_doc(t): return "\n".join(l for l in t.splitlines() if not l.strip().startswith("///"))
def entry(q, c, full):
    head = f"### C# {q} -> C++ `{c['cpp']}` : {c['kind'].upper()}" + (f" (STRUCT, by value)" if c["kind"] == "struct" else (" (use cs::Ref<" + c["cpp"] + ">)" if c["kind"] in ("class", "interface") else ""))
    if len(c.get("files", [])) > 1: head += f"  PARTIAL across {c['files']}; HEADER_OWNER: {c['files'][0]}"
    if c.get("base"): head += f"  bases: {c['base']}"
    return head + "\n" + "\n".join("  " + d for d in c["decl"])
tasks = []
for f in files:
    src = strip_doc(open(os.path.join(CORE, f), errors="replace").read())
    own = [q for q, c in cat.items() if f in c.get("files", [c["file"]])]
    words = set(re.findall(r"\b[A-Z]\w+\b", src))
    used = [q for w in words for q in simple.get(w, []) if q not in own]
    used = sorted(set(used))
    parts = [f"STEM: {stem(f)}", "TYPES DECLARED IN THIS FILE (write these):"] + [entry(q, cat[q], True) for q in own]
    parts += ["TYPES THIS FILE USES (already written elsewhere; call them exactly like this):"] + [entry(q, cat[q], False) for q in used]
    tnames = set(re.findall(r"\bTup_\w+", "\n".join(parts)))
    parts += ["TUPLE STRUCTS (tuples.h):"] + [l for l in TUP if re.search(r"struct (\w+)", l).group(1) in tnames]
    parts += ["cs.h (support runtime):", CSH, f"C# FILE {f}:", src]
    tasks.append({"id": stem(f), "system": RULES, "user": "\n\n".join(parts), "out": f"{X}/out/{stem(f)}.md"})
os.makedirs(f"{X}/out", exist_ok=True)
print(len(tasks), "files; max est tokens", max(est(t["system"], t["user"]) for t in tasks), flush=True)
if os.environ.get("DRY"): sys.exit(0)
asyncio.run(run_all(tasks, f"{X}/translate.log.jsonl", int(os.environ.get("CONC", "12"))))
