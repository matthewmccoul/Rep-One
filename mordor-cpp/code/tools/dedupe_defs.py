"""Remove duplicate strong definitions (the same function defined in two .cpp files). The non-owner file loses its definition; the owner is the module whose header declares the function.
usage: dedupe_defs.py"""
import glob, os, re, subprocess, sys
ROOT = "/tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code"
def link_dups():
    p = subprocess.run(f"cd {ROOT}/build && g++ -shared -o /tmp/mordor_dupcheck.so -Wl,--whole-archive libmordor_core.a -Wl,--no-whole-archive 2>&1", shell=True, capture_output=True, text=True)
    dups = {}
    for m in re.finditer(r"libmordor_core\.a\((\w+)\.cpp\.o\)[^\n]*\n[^\n]*?multiple definition of `([^']+)'; libmordor_core\.a\((\w+)\.cpp\.o\)", p.stdout + p.stderr):
        dups.setdefault(m.group(2), set()).update({m.group(1), m.group(3)})
    return dups
def declared_in(name):
    hs = []
    for h in glob.glob(f"{ROOT}/include/mordor/*.h"):
        t = re.sub(r"//.*", "", open(h).read())
        if re.search(rf"\b{name}\s*\([^;{{]*\)\s*(const)?\s*;", t): hs.append(os.path.basename(h)[:-2])
    return sorted(hs)
def remove_def(path, name):
    lines = open(path).read().split("\n")
    pat = re.compile(rf"^[A-Za-z_][\w:<>,&\* ]*\b{name}\s*\(")
    for i, l in enumerate(lines):
        if pat.match(l) and not l.rstrip().endswith(";"):
            j = i
            while j < len(lines) and "{" not in lines[j]: j += 1
            if j >= len(lines) or ";" in "".join(lines[i:j + 1]).split("{")[0] and "(" not in lines[i]: continue
            depth = 0; k = j
            while k < len(lines):
                depth += lines[k].count("{") - lines[k].count("}")
                if depth <= 0 and "}" in lines[k]: break
                k += 1
            s = i
            while s > 0 and lines[s - 1].strip().startswith("//"): s -= 1
            del lines[s:k + 1]
            open(path, "w").write("\n".join(lines)); return True
    return False
removed = 0
for rnd in range(6):
    dups = link_dups()
    if not dups: break
    print(f"round {rnd}: {len(dups)} duplicated symbols")
    for sym, objs in dups.items():
        name = re.sub(r"\[abi:cxx11\]", "", re.sub(r"\(.*", "", sym)).replace("mordor::", "")
        if "::" in name: continue
        owners = declared_in(name)
        keep = [o for o in objs if any(o == h or o.startswith(h + "_") or (h.startswith(o)) for h in owners)]
        late = [o for o in objs if o.endswith('_missing')]
        drop = late if (late and len(late) < len(objs)) else (sorted(objs - set(keep[:1])) if keep else sorted(objs)[1:])
        for o in drop:
            if remove_def(f"{ROOT}/src/{o}.cpp", name): removed += 1
    subprocess.run(["python3", "-I", f"{ROOT}/tools/build.py"], capture_output=True)
print("removed", removed, "duplicate definitions")
