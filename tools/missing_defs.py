"""List functions declared in module headers but not defined in libmordor_core.a. Writes logs/missing_defs.json {header: [decl lines]}."""
import glob, json, os, re, subprocess, sys
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
out = subprocess.run(f"nm -C --defined-only {ROOT}/build/libmordor_core.a", shell=True, capture_output=True, text=True).stdout
def nargs(params):
    params = params.strip()
    if not params or params == "void": return 0
    depth = 0; n = 1
    for ch in params:
        if ch in "<([": depth += 1
        elif ch in ">)]": depth -= 1
        elif ch == "," and depth == 0: n += 1
    return n
defined = set()
for m in re.finditer(r"mordor::(\w+)(?:\[abi:cxx11\])?\((.*)\)(?: const)?$", out, re.M): defined.add((m.group(1), nargs(m.group(2))))
def strip(t):
    t = re.sub(r"/\*.*?\*/", "", t, flags=re.S)
    return "\n".join(re.sub(r"//.*", "", l).rstrip() for l in t.split("\n"))
missing, total = {}, 0
for h in sorted(glob.glob(f"{ROOT}/include/mordor/*.h")):
    b = os.path.basename(h)
    if b.startswith("state_") or b in ("types.h", "rng.h", "events.h", "ctx.h", "math_util.h", "json_lite.h", "game_data.h"): continue
    t = strip(open(h).read())
    # join multi-line declarations
    stmts = re.split(r";\s*\n", t)
    for s in stmts:
        s2 = " ".join(s.split())
        if "{" in s2 or s2.startswith(("struct", "enum", "using", "typedef", "class", "template", "#", "namespace", "inline constexpr", "constexpr", "extern")) : continue
        m = re.match(r"^(?:static\s+|inline\s+)?[\w:<>,\s\*&]+?\b(\w+)\s*\((.*)\)\s*(?:const)?$", s2)
        if not m: continue
        total += 1
        if (m.group(1), nargs(m.group(2))) not in defined: missing.setdefault(b, []).append(s2)
n = sum(len(v) for v in missing.values())
json.dump(missing, open(f"{ROOT}/logs/missing_defs.json", "w"), indent=1)
print(f"{total} declared functions; {n} without a definition in {len(missing)} headers")
for b, v in sorted(missing.items(), key=lambda kv: -len(kv[1]))[:20]: print(f"  {len(v):3} {b}")
