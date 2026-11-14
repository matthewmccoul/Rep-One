"""From outline.json: fwd.h, enums.h, tuples.h, and a C++ signature catalog per type (catalog.json)."""
import json, re, sys, collections
sys.argv = [sys.argv[0], sys.argv[1]]; X = sys.argv[1]
exec(open(f"{X}/scripts/typemap.py").read().split('if __name__ == "__main__":')[0])
MODS = r"(?:public|private|internal|protected|static|readonly|const|required|override|virtual|abstract|sealed|new|volatile|extern|unsafe|partial|async|explicit|implicit)"
def enum_expr(v):
    v = re.sub(r"(\b[A-Z]\w*)\.(\w+)", r"\1::\2", v); return v
def default_expr(d, ptype):
    d = d.strip()
    if d == "null": return "{}"
    if d == "default": return "{}"
    if re.match(r"^-?\d+(\.\d+)?[fFdDmMlL]?$", d): return re.sub(r"[mMdD]$", "", d)
    if d in ("true", "false"): return d
    if d.startswith('"'): return f"cs::String({d})"
    m = re.match(r"^([A-Z]\w*)\.(\w+)$", d)
    if m:
        r = resolve(m.group(1)); return f"{r[0] if r else m.group(1)}::{m.group(2)}"
    return "/*default:*/{" + d + "}"
cat = {}; fwd = []; enums = []
for t in T:
    q = (t["outer"] + "." if t["outer"] else "") + t["name"]; name, kind = reg[q]
    lines = []
    if t["kind"] == "enum":
        base = t["baseList"].replace(":", "").strip() or "int"
        base = PRIM.get(base, "int32_t")
        enums.append(f"enum class {name} : {base} {{ " + ", ".join(enum_expr(m) for m in t["enumMembers"]) + " };")
        cat[q] = {"cpp": name, "kind": "enum", "file": t["file"], "decl": [f"enum class {name} : {base} {{ {', '.join(enum_expr(m) for m in t['enumMembers'])} }};"]}
        continue
    if t["kind"] == "interface" or t["isStatic"] or t["kind"] == "class": fwd.append(f"struct {name};")
    else: fwd.append(f"struct {name};")
    for s in t["fields"]:
        m = re.match(rf"^((?:{MODS}\s+)*)(.+?)\s+(\w+)$", s)
        if not m: lines.append("// ? " + s); continue
        mods = m.group(1); st = "static " if ("static" in mods or "const" in mods) else ""
        lines.append(f"{st}{cpp(m.group(2), q)} {m.group(3)};   // field{' (const)' if 'const' in mods else ''}")
    for s in t["autoProps"]:
        m = re.match(rf"^((?:{MODS}\s+)*)(.+?)\s+(\w+)$", s)
        if not m: lines.append("// ? " + s); continue
        st = "static " if "static" in m.group(1) else ""
        lines.append(f"{st}{cpp(m.group(2), q)} {m.group(3)};   // auto-property -> plain field")
    for s in t["compProps"]:
        m = re.match(rf"^((?:{MODS}\s+)*)(.+?)\s+(\w+|this\[.*\])\s*\[(.*)\]$", s)
        if not m: lines.append("// ? " + s); continue
        st = "static " if "static" in m.group(1) else ""; ty = cpp(m.group(2), q)
        if m.group(3).startswith("this"):
            mi = re.search(r"this\[(\S+)", s); lines.append(f"{ty} Index({cpp(mi.group(1), q) if mi else 'int32_t'} i);   // indexer this[...] -> Index(i) / SetIndex(i, v)"); continue
        lines.append(f"{st}{ty} {m.group(3)}();   // computed property -> getter method")
        if "set" in m.group(4): lines.append(f"{st}void set_{m.group(3)}({ty} value);   // its setter")
    for s in t["methods"]:
        m = re.match(rf"^((?:{MODS}\s+)*)(.*?)\s*(ctor \w+|operator \S+|\w+)(<[^>]*>)?\((.*)\)$", s)
        if not m: lines.append("// ? " + s); continue
        mods, ret, nm, gen, params = m.groups(); gp = tuple(re.findall(r"\w+", gen or ""))
        ps = []
        for p in split_args(params):
            mod = re.match(r"^(this|ref|out|in|params)\s+", p); p2 = re.sub(r"^(this|ref|out|in|params)\s+", "", p)
            dflt = None
            if "=" in p2: p2, dflt = p2.split("=", 1)
            mm = re.match(r"^(.*\S)\s+(\w+)$", p2.strip())
            if not mm: ps.append("/*?*/" + p); continue
            ty = cpp(mm.group(1), q, gp)
            if mod and mod.group(1) in ("ref", "out"): ty += "&"
            ps.append(f"{ty} {mm.group(2)}" + (f" = {default_expr(dflt, ty)}" if dflt is not None else "") + (" /*this: extension method*/" if mod and mod.group(1) == "this" else ""))
        tmpl = f"template <class {', class '.join(gp)}> " if gp else ""
        st = "static " if "static" in mods else ""
        if nm.startswith("ctor"): lines.append(f"{name}({', '.join(ps)});   // constructor"); continue
        if nm.startswith("operator"): lines.append(f"// operator: {s}"); continue
        lines.append(f"{tmpl}{st}{cpp(ret, q, gp)} {nm}({', '.join(ps)});")
    if q in cat:  # partial class: one C++ class, members from every part
        cat[q]["decl"] += lines; cat[q]["files"].append(t["file"]); cat[q]["base"] = cat[q]["base"] or t["baseList"]
    else:
        cat[q] = {"cpp": name, "kind": "static class" if t["isStatic"] else t["kind"], "file": t["file"], "files": [t["file"]], "base": t["baseList"], "decl": lines}
open(f"{X}/cpp/fwd.h", "w").write("#pragma once\n#include \"cs.h\"\nnamespace mc {\n" + "\n".join(sorted(set(fwd))) + "\n}  // namespace mc\n")
helpers = []
for t2 in T:
    if t2["kind"] != "enum": continue
    q2 = (t2["outer"] + "." if t2["outer"] else "") + t2["name"]; n2 = reg[q2][0]
    names = [m.split("=")[0].strip() for m in t2["enumMembers"]]
    helpers.append(f"inline cs::String ToStr({n2} e) {{ switch (e) {{ " + " ".join(f"case {n2}::{nm}: return \"{nm}\";" for nm in dict.fromkeys(names)) + f" }} return cs::ToStr((long long)e); }}")
    helpers.append(f"inline cs::List<{n2}> EnumValues_{n2}() {{ return cs::List<{n2}>::Of({{ " + ", ".join(f"{n2}::{nm}" for nm in dict.fromkeys(names)) + " }); }")
    helpers.append(f"inline {n2} operator|({n2} a, {n2} b) {{ return ({n2})((long long)a | (long long)b); }} inline {n2} operator&({n2} a, {n2} b) {{ return ({n2})((long long)a & (long long)b); }} inline {n2} operator^({n2} a, {n2} b) {{ return ({n2})((long long)a ^ (long long)b); }} inline {n2} operator~({n2} a) {{ return ({n2})(~(long long)a); }} inline {n2}& operator|=({n2}& a, {n2} b) {{ a = a | b; return a; }} inline {n2}& operator&=({n2}& a, {n2} b) {{ a = a & b; return a; }} inline bool HasFlag({n2} a, {n2} b) {{ return (a & b) == b; }}")
open(f"{X}/cpp/enums.h", "w").write("#pragma once\n#include \"cs.h\"\nnamespace mc {\n" + "\n".join(enums) + "\n\n// names, values and flag operators, as C# gives enums\n" + "\n".join(helpers) + "\n}  // namespace mc\n")
tl = []
for n, fs in tuples.items(): tl.append(f"struct {n} {{ " + " ".join(f"{t} {nm}{{}};" for t, nm in fs) + " };")
open(f"{X}/cpp/tuples_list.txt", "w").write("\n".join(tl))
json.dump(cat, open(f"{X}/catalog.json", "w"), indent=1)
print(len(cat), "types in catalog;", len(enums), "enums;", len(tl), "tuple structs; unknown:", unknown.most_common(8))
