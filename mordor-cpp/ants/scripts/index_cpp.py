"""Index the C++ demo: every function declaration in include/ (name, file, signature, comment lines just above) and every definition name in src/."""
import glob, json, os, re
C = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/dl/cpp"; out = []
decl = re.compile(r"^\s*(?:inline\s+|static\s+|constexpr\s+)*[A-Za-z_][\w:<>,\s\*&]*?\b([A-Za-z_]\w*)\s*\(([^;{]*)\)\s*(?:const)?\s*;", re.M)
for h in sorted(glob.glob(f"{C}/include/mordor/*.h")):
    t = open(h, errors="replace").read(); lines = t.split("\n")
    for m in decl.finditer(t):
        name = m.group(1)
        if name in ("if", "for", "while", "return", "sizeof", "static_cast"): continue
        ln = t[:m.start()].count("\n"); k = ln; com = []
        while k > 0 and lines[k - 1].strip().startswith("//") and len(com) < 3: com.insert(0, lines[k - 1].strip()); k -= 1
        sig = " ".join(m.group(0).split())[:220]
        out.append({"name": name, "file": os.path.relpath(h, C), "sig": sig, "comment": " ".join(com)[:300]})
src = "\n".join(open(f, errors="replace").read() for f in glob.glob(f"{C}/src/*.cpp"))
unw = open(f"{C}/UNWIRED.md").read()
unwired = set(re.findall(r"[A-Za-z_]\w+", "\n".join(l.split(":", 1)[1] for l in unw.splitlines() if l.startswith("- `") and ":" in l)))
for d in out: d["unwired"] = d["name"] in unwired
json.dump(out, open("/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/ants2/notes/cpp_index.json", "w"), indent=0)
json.dump(sorted(unwired), open("/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/ants2/notes/unwired.json", "w"))
print(len(out), "declarations;", len({d['name'] for d in out}), "names;", len(unwired), "unwired names;", sum(d['unwired'] for d in out), "decls unwired")
