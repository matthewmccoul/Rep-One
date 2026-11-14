"""Extract translated files into cpp/gen and write core.h with headers in dependency order."""
import glob, json, os, re, sys
X = sys.argv[1]; G = f"{X}/cpp/gen"; os.makedirs(G, exist_ok=True)
cat = json.load(open(f"{X}/catalog.json"))
FILE = re.compile(r"=== FILE: (\S+) ===\n(.*?)\n=== END FILE ===", re.S)
def unfence(b): b = re.sub(r"^```[a-zA-Z+]*\n", "", b); return re.sub(r"\n```\s*$", "", b)
missing = []
for md in sorted(glob.glob(f"{X}/out/*.md")):
    stem = os.path.basename(md)[:-3]; t = open(md).read(); fs = dict(FILE.findall(t))
    for want in (stem + ".h", stem + ".cpp"):
        if want not in fs: missing.append(want); continue
        p = f"{G}/{want}"
        if not os.path.exists(p) or os.environ.get("OVERWRITE"): open(p, "w").write(unfence(fs[want]) + "\n")
structs = {c["cpp"] for c in cat.values() if c["kind"] == "struct"}
defs = {}; uses = {}
def strip(t): t = re.sub(r"/\*.*?\*/", "", t, flags=re.S); return re.sub(r"//[^\n]*", "", t)
for h in glob.glob(f"{G}/*.h"):
    t = strip(open(h).read()); n = os.path.basename(h)
    defs[n] = set(re.findall(r"^\s*struct\s+(\w+)\s*(?::[^{;]*)?\{", t, re.M))
    noref = re.sub(r"cs::Ref<[^<>]*(<[^<>]*>)?[^<>]*>", "", t)
    uses[n] = {w for w in re.findall(r"\b\w+\b", noref) if w in structs}
order, done = [], set()
def visit(n, stack=()):
    if n in done or n in stack: return
    for dep in sorted(uses[n]):
        for m, d in defs.items():
            if dep in d and m != n: visit(m, stack + (n,))
    done.add(n); order.append(n)
for n in sorted(defs, key=lambda n: (not (defs[n] & structs), n)): visit(n)
# tuples whose members are Core structs go after the struct headers
tl = []
for l in open(f"{X}/cpp/tuples_list.txt").read().splitlines():
    nm = re.search(r"struct (\w+)", l).group(1); mem = re.findall(r"(\w+)\{\};", l)
    eq = " && ".join(f"cs::Eq({m}, o.{m})" for m in mem); lt = "".join(f"if (cs::Less({m}, o.{m})) return true; if (cs::Less(o.{m}, {m})) return false; " for m in mem)
    tl.append(l[:-2] + f" bool operator==(const {nm}& o) const {{ return {eq}; }} bool operator!=(const {nm}& o) const {{ return !(*this == o); }} bool operator<(const {nm}& o) const {{ {lt}return false; }} }};")
early = [l for l in tl if not any(re.search(rf"\b{s}\b", l) for s in structs)]; late = [l for l in tl if l not in early]
open(f"{X}/cpp/tuples.h", "w").write('#pragma once\n#include "fwd.h"\n#include "enums.h"\nnamespace mc {\n' + "\n".join(early) + '\n}  // namespace mc\n')
struct_hdrs = [n for n in order if defs[n] & structs]; rest = [n for n in order if n not in struct_hdrs]
body = "".join(f'#include "gen/{n}"\n' for n in struct_hdrs)
body += '\nnamespace mc {\n' + "\n".join(late) + '\n}  // namespace mc\n\n' + "".join(f'#include "gen/{n}"\n' for n in rest)
open(f"{X}/cpp/core.h", "w").write('#pragma once\n// Umbrella: every translated header, struct (value) types first, in dependency order.\n#include "cs.h"\n#include "fwd.h"\n#include "enums.h"\n#include "tuples.h"\n' + body + "#include \"gen/JsonContracts.h\"\n")
print(len(defs), "headers;", len(struct_hdrs), "with structs; late tuples", len(late), "; missing blocks:", missing)
