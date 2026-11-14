"""Compile-fix loop. mode=headers: compile core.h alone, fix headers with errors. mode=sources: compile each .cpp, fix failing files.
One model call per file per round, given its errors, both of its files, its C# source and its catalog entries."""
import asyncio, glob, json, os, re, subprocess, sys, concurrent.futures as cf
X, mode = sys.argv[1], sys.argv[2]; rounds = int(sys.argv[3]) if len(sys.argv) > 3 else 3
sys.path.insert(0, f"{X}/scripts"); from army import run_all
G = f"{X}/cpp/gen"; CORE = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/dl/src/OpenDejenol-v0.1.3/src/Mordor.Core"
cat = json.load(open(f"{X}/catalog.json")); RULES = open(f"{X}/scripts/RULES.md").read()
def cs_source(stem):
    if stem.startswith("Combat_"): return "(part of Combat.cs; see the catalog)"
    for f in glob.glob(f"{CORE}/**/*.cs", recursive=True):
        rel = os.path.relpath(f, CORE)
        if rel[:-3].replace("/", "_").replace(".", "_") == stem: return "\n".join(l for l in open(f).read().splitlines() if not l.strip().startswith("///"))
    return ""
def compile_one(path):
    r = subprocess.run(["g++", "-std=c++17", "-fsyntax-only", "-fmax-errors=200", "-w", f"-I{X}/cpp", f"-I{G}", path], capture_output=True, text=True)
    return r.returncode, r.stderr
def errors_by_file(err):
    by = {}
    for m in re.finditer(r"^(?:\S*/)?gen/([\w.]+\.(?:h|cpp)):(\d+):\d+: error: (.*)$", err, re.M):
        by.setdefault(m.group(1), []).append(f"{m.group(1)}:{m.group(2)}: {m.group(3)}")
    return by
SYS = RULES + """

YOU ARE NOW FIXING COMPILE ERRORS in an earlier translation of this file. Keep the translation faithful to the C#; change only what the errors require (and the same mistake elsewhere in these two files). Typical causes: in-class `inline static` members whose initializers call this class's own static functions or default arguments (move them out: declare `static T X;` in the class and define `T Class::X = init;` in the .cpp); types used by value before they are complete; wrong member access (-> vs .); catalog signature mismatches; cs.h API misuse (read cs.h for the real API). If an error is caused by ANOTHER file's header (a member missing there that the catalog says exists), do not work around it: write a line `OTHER_FILE: <header> - <what is missing>`.
Output the complete corrected files in the same two-block format."""
CSH = open(f"{X}/cpp/cs.h").read()
for rnd in range(1, rounds + 1):
    if mode == "headers":
        tu = f"{X}/cpp/_core_tu.cpp"; open(tu, "w").write('#include "core.h"\nint main() { return 0; }\n')
        rc, err = compile_one(tu); by = errors_by_file(err); bad = {k[:-2] for k in by if k.endswith(".h")}
        print(f"round {rnd}: core.h {'compiles' if rc == 0 else 'fails'}; headers with errors: {len(bad)}", flush=True)
        if rc == 0: break
        if not bad: print(err[:3000]); break
    else:
        srcs = sorted(glob.glob(f"{G}/*.cpp"))
        with cf.ThreadPoolExecutor(4) as ex: res = dict(zip(srcs, ex.map(compile_one, srcs)))
        by = {}
        for p, (rc, err) in res.items():
            if rc != 0:
                for k, v in errors_by_file(err).items(): by.setdefault(k, []).extend(v)
        bad = {k.rsplit(".", 1)[0] for k in by}
        print(f"round {rnd}: {sum(1 for rc, _ in res.values() if rc == 0)}/{len(res)} sources compile; files with errors: {len(bad)}", flush=True)
        if not bad: break
    tasks = []
    for stem in sorted(bad):
        h, c = f"{G}/{stem}.h", f"{G}/{stem}.cpp"
        errs = by.get(stem + ".h", []) + by.get(stem + ".cpp", [])
        u = "\n\n".join([f"STEM: {stem}", "COMPILER ERRORS (g++ -std=c++17), first 80:\n" + "\n".join(dict.fromkeys(errs))[:12000],
                         f"=== CURRENT {stem}.h ===\n" + (open(h).read() if os.path.exists(h) else ""), f"=== CURRENT {stem}.cpp ===\n" + (open(c).read() if os.path.exists(c) else ""),
                         "cs.h:", CSH, "C# SOURCE:", cs_source(stem)])
        tasks.append({"id": f"fix{rnd}_{stem}", "system": SYS, "user": u, "out": f"{X}/fixes/{mode}_r{rnd}_{stem}.md"})
    os.makedirs(f"{X}/fixes", exist_ok=True)
    asyncio.run(run_all(tasks, f"{X}/fix.log.jsonl", int(os.environ.get("CONC", "12"))))
    FILE = re.compile(r"=== FILE: (\S+) ===\n(.*?)\n=== END FILE ===", re.S)
    for t in tasks:
        if not os.path.exists(t["out"]): continue
        out = open(t["out"]).read()
        for name, body in FILE.findall(out):
            body = re.sub(r"^```[a-zA-Z+]*\n", "", body); body = re.sub(r"\n```\s*$", "", body)
            if "/" not in name: open(f"{G}/{name}", "w").write(body + "\n")
        for m in re.findall(r"OTHER_FILE: (.*)", out): print("  OTHER_FILE:", m[:200], flush=True)
    subprocess.run(["python3", "-I", f"{X}/scripts/assemble.py", X], capture_output=True)
