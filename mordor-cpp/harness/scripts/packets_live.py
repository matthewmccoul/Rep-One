"""Harness packets: one ant per copy group writes cases.json + Case.cs (Mordor.Core) + case.cpp (each C++ copy)."""
import glob, json, math, os, re, sys, collections
S, H, W = sys.argv[1:4]; sys.path.insert(0, f"{S}/ants2/scripts"); from army import est
CORE = f"{S}/build/od/src/Mordor.Core"; C = f"{W}/code"
inv = json.load(open(f"{W}/notes/inventory.json")); G = json.load(open(f"{H}/live_groups.json")); P = [{"pile": i, "rule": g["rule"], "keeper": g["copies"][0], "losers": [dict(c, phase=2) for c in g["copies"][1:]]} for i, g in enumerate(G)]; meta = {i: {"refs": [tuple(r) for r in g["refs"]]} for i, g in enumerate(G)}
def strip(t):
    t = re.sub(r"/\*.*?\*/", lambda m: " " * len(m.group(0)), t, flags=re.S); return re.sub(r"//[^\n]*", lambda m: " " * len(m.group(0)), t)
def cs_member(file, method):
    meth = re.sub(r"\(.*", "", method.split(".", 1)[-1]); t = open(f"{CORE}/{file}", encoding="utf-8", errors="replace").read(); out = []
    for m in re.finditer(rf"^\s{{4,8}}(?:public|internal|private)[^\n;=]*\b{re.escape(meth)}\s*(?:<[^>\n]*>)?\s*\(", t, re.M):
        i = t.find("{", m.end()); arrow = t.find("=>", m.end()); semi = t.find(";", m.end())
        if 0 <= arrow < (i if i >= 0 else 10**9): out.append(t[m.start():semi + 1]); continue
        if i < 0: continue
        d, j = 0, i
        while j < len(t):
            if t[j] == "{": d += 1
            elif t[j] == "}":
                d -= 1
                if d == 0: break
            j += 1
        out.append(t[m.start():j + 1])
    return "\n".join(out)
# C# type outlines: public members of Core classes named in the reference code
cs_classes = {}
for f in glob.glob(f"{CORE}/**/*.cs", recursive=True):
    t = open(f, encoding="utf-8", errors="replace").read()
    for m in re.finditer(r"^(?:public|internal)\s+(?:sealed\s+|static\s+|abstract\s+|partial\s+)*(?:class|struct|record|enum)\s+(\w+)", t, re.M):
        cs_classes.setdefault(m.group(1), (f, m.start()))
def cs_outline(name, cap=1800):
    if name not in cs_classes: return ""
    f, s = cs_classes[name]; t = open(f, encoding="utf-8", errors="replace").read()[s:]
    lines = [l.strip() for l in t.split("\n")[:400] if re.match(r"\s{4}public\s", l)]
    return f"// C# {name} ({os.path.relpath(f, CORE)})\n" + "\n".join(l[:160] for l in lines)[:cap]
# C++ struct definitions from headers
hdr = {h: open(h, errors="replace").read() for h in glob.glob(f"{C}/include/mordor/*.h")}
def cpp_struct(name, cap=2500):
    for h, t in hdr.items():
        m = re.search(rf"^(?:struct|class|enum class|enum)\s+{re.escape(name)}\b[^;{{]*\{{", t, re.M)
        if m:
            d, j = 0, m.end() - 1
            while j < len(t):
                if t[j] == "{": d += 1
                elif t[j] == "}":
                    d -= 1
                    if d == 0: break
                j += 1
            return f"// C++ {name} ({os.path.basename(h)})\n" + t[m.start():j + 2][:cap]
    return ""
def cpp_decls(name):
    out = []
    for h, t in hdr.items():
        for l in t.split("\n"):
            if re.search(rf"\b{re.escape(name)}\s*\(", l) and l.rstrip().endswith(";"): out.append(f"{os.path.basename(h)}: {l.strip()}")
    return out
# Core members for retrieval on unlinked groups
members = []
for f in glob.glob(f"{CORE}/**/*.cs", recursive=True):
    t = open(f, encoding="utf-8", errors="replace").read(); cls = os.path.splitext(os.path.basename(f))[0]
    for m in re.finditer(r"^\s{4,8}(?:public|internal)\s+[^=;{\n]*\b(\w+)\s*(?:<[^>\n]*>)?\s*\(", t, re.M):
        members.append((os.path.relpath(f, CORE), f"{cls}.{m.group(1)}"))
def words(t):
    out = []
    for w in re.findall(r"[A-Za-z][A-Za-z0-9]*", t):
        for p in re.findall(r"[A-Z]?[a-z]+|[A-Z]+(?![a-z])|\d+", w):
            if len(p) >= 3: out.append(p.lower())
    return out
memtxt = {}
for f, m in members: memtxt[(f, m)] = None
def rank_core(text, k=4):
    q = collections.Counter(words(text)); sc = []
    for f, m in members:
        if memtxt[(f, m)] is None: memtxt[(f, m)] = set(words(m + " " + cs_member(f, m)[:3000]))
        ws = memtxt[(f, m)]; s = sum(min(c, 3) for w, c in q.items() if w in ws) / (1 + len(ws) / 300)
        sc.append((s, f, m))
    sc.sort(reverse=True); return [(f, m) for s, f, m in sc[:k]]
def body(f):
    raw = open(f"{C}/{f['file']}", errors="replace").read(); t = strip(raw); best = None
    for m in re.finditer(rf"^(?:static\s+|inline\s+|constexpr\s+)*[A-Za-z_][\w:<>,&\*\s]*?\b{re.escape(f['name'])}\s*\(", t, re.M):
        j = t.find("{", m.end()); semi = t.find(";", m.end())
        if j < 0 or (0 <= semi < j): continue
        d, k = 0, j
        while k < len(t):
            if t[k] == "{": d += 1
            elif t[k] == "}":
                d -= 1
                if d == 0: break
            k += 1
        line = raw[:m.start()].count("\n") + 1
        if best is None or abs(line - f["line"]) < abs(best[2] - f["line"]): best = (m.start(), k + 1, line)
    return raw[best[0]:best[1]] if best else None
def file_local(f):
    """True when the copy is static or sits inside an anonymous namespace in its .cpp."""
    raw = open(f"{C}/{f['file']}", errors="replace").read(); t = strip(raw)
    lines = raw.split("\n"); pos = sum(len(l) + 1 for l in lines[:max(0, f["line"] - 1)])
    if re.match(r"\s*static\b", lines[f["line"] - 1] if f["line"] - 1 < len(lines) else ""): return True
    depth_stack = []; i = 0
    for m in re.finditer(r"namespace\s*\{|\{|\}", t[:pos]):
        tok = m.group(0)
        if tok.startswith("namespace"): depth_stack.append("anon")
        elif tok == "{": depth_stack.append("x")
        else:
            if depth_stack: depth_stack.pop()
    return "anon" in depth_stack
CONTRACT = open(f"{H}/cpp/case_api.h").read()
SYS = """You build a differential test for ONE rule of the 1995 RPG Mordor. The reference is Mordor.Core (C#). A C++ re-implementation has several COPIES of this rule; the test must run the same inputs and the same scripted random draws through the reference and through every copy, and print the results in the same canonical form, so a script can see which copies match the reference.
Write exactly three files:
=== FILE: cases.json ===
A JSON array of 20 to 50 cases: {"args": {...}, "rng": [doubles in [0,1), the draws consumed in order; the last repeats]}. Cover normal values, every boundary in the reference (thresholds, clamps, zero, negative, maximum), and every branch.
=== END FILE ===
=== FILE: Case.cs ===
public static class Case { public static string Run(System.Text.Json.JsonElement a, System.Random rng) { ... } }
Build the reference inputs from a, call the REFERENCE method (Mordor.Core, namespace Mordor.Core; game tables are loaded: H.Items, H.ItemsById, H.Monsters, H.Spells, H.Dungeon, RaceData.All, GuildData), pass rng wherever the reference takes a Random, and return one canonical string: the return value and every state field the rule changes, joined with '|'. Integers as plain numbers, doubles through H.F(x), bools as 0/1. If the reference needs a Character, use Character.Create(name, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, stats) and set fields.
=== END FILE ===
=== FILE: case.cpp ===
#include "case_api.h" plus the needed mordor/*.h headers; inside namespace mordor define one function per C++ COPY: static Str run_<copyname>(const JsonValue& a, Ctx& ctx) { ... } building the SAME inputs from a (A(a,"key").asInt() etc.), calling that copy (ctx.rng is the scripted source; ctx.data the game tables), and returning the SAME canonical string (I(), F(), "|"). Then: const CopyEntry kCopies[] = {{"<copyname>", &run_<copyname>}, ...}; const std::size_t kNumCopies = <n>;
If two copies have the same name, register them as name@file. If a copy is file-local (static, or inside an anonymous namespace in its .cpp), add a fourth file `=== FILE: shim_<stem>.cpp ===` (<stem> = that copy's source file name without .cpp); the harness appends it to the end of that source file and compiles them together, so it can call the file-local copy. In it write `namespace mordor { <ret> <exportname>(<params>) { return <copy>(<args>); } }` with a new unique exportname, and in case.cpp declare and call that exportname. Leave a copy out only if it truly cannot be fed the inputs; then write `UNREACHABLE: name - reason` after the files.
Test data: both sides load the same shareware tables (366 items, 401 monsters, 105 spells); pick real records by id in args (C#: H.ItemsById[id], H.Monsters, H.Spells; C++: search ctx.data.items for the record whose .id matches, ctx.data.monsters, ctx.data.spells) instead of inventing records. Class instances in the reference may be created with their public constructors shown in the C# outlines.
=== END FILE ===
Starting state: Mordor.Core's Character.Create(name, race, alignment, sex, stats) gives the starting guild (Nomad) at level 1, GoldOnHand = 1500, MaxHits/Hits 15, and the race's StatMax; a default-constructed C++ Character has none of these. Set EVERY field the rule reads explicitly and identically on both sides (stats, guild memberships and levels, gold, hits, items, alignment), overwriting defaults rather than adding to them. Rules: identical inputs and identical output format on both sides; convert units where the two sides differ (C++ Currency is gold x 10000) so equal behaviour gives equal strings; never adapt the output to make a copy agree; do not invent methods or fields - use only those shown. If none of the reference methods shown implements this rule, write only `NO_REFERENCE: reason` and no files. End with `REFERENCE: <C# method used>`.
HARNESS CONTRACT (case_api.h):
""" + CONTRACT
groups = list(range(len(P)))
os.makedirs(f"{H}/groups", exist_ok=True); tasks = []; manifest = {}
for gi in groups:
    p = P[gi]; refs = meta[p["pile"]]["refs"]
    copies = [p["keeper"]] + p["losers"]
    fs = []
    for cp in copies:
        f = next((x for x in inv if x["name"] == cp["name"] and x["file"] == cp["file"] and abs(x["line"] - cp["line"]) < 3), None) or next((x for x in inv if x["name"] == cp["name"] and x["file"] == cp["file"]), None)
        if f: fs.append(f)
    bodies = [(f, body(f)) for f in fs]
    if not refs: refs = rank_core(p["rule"] + " " + " ".join((b or "")[:1500] for _, b in bodies))
    def mk(bc, rc, oc):
        parts = [f"RULE: {p['rule']}", "C++ COPIES:"]
        types = set()
        for f, b in bodies:
            loc = file_local(f)
            parts.append(f"--- copy {f['name']} ({f['file']}){' FILE-LOCAL: not callable from outside its file, write a shim_' + os.path.splitext(os.path.basename(f['file']))[0] + '.cpp export wrapper' if loc else ''} declarations: {'; '.join(cpp_decls(f['name'])[:3]) if not loc else 'n/a (file-local)'}\n{(b or '(not found)')[:bc]}")
            types |= set(re.findall(r"\b([A-Z][A-Za-z0-9]+)\s*[&*]", (b or "")[:600]))
        st = [cpp_struct(t) for t in sorted(types)][:5]
        parts.append("C++ TYPES USED:\n" + "\n\n".join(x for x in st if x)[:oc])
        parts.append("REFERENCE CANDIDATES (Mordor.Core):")
        cst = set()
        for fl, m in refs[:4]:
            b = cs_member(fl, m)
            if b: parts.append(f"=== {m} ({fl})\n{b[:rc]}"); cst |= set(re.findall(r"\b([A-Z][A-Za-z0-9]+)\s+\w+[,)]", b[:400])); cst.add(m.split(".")[0])
        parts.append("C# TYPES USED:\n" + "\n\n".join(cs_outline(t) for t in sorted(cst)[:4] if cs_outline(t))[:oc])
        return "\n\n".join(parts)
    cnt = collections.Counter(f["name"] for f in fs); ren = [(f["file"], f["name"], f"{f['name']}__{os.path.splitext(os.path.basename(f['file']))[0]}") for f in fs if cnt[f["name"]] > 1]
    note = ("SAME-NAME COPIES: these copies share a name, so the harness compiles each one's source file with the function renamed. Call them by the new name, declaring each yourself inside namespace mordor with the same signature as the original: " + "; ".join(f"{n} in {fl} -> {nn}" for fl, n, nn in ren)) if ren else ""
    bc, rc, oc = 3500, 3500, 5000; u = mk(bc, rc, oc) + ("\n\n" + note if note else "")
    while est(SYS, u) > 31000 and bc > 800: bc, rc, oc = int(bc * .85), int(rc * .85), int(oc * .85); u = mk(bc, rc, oc) + ("\n\n" + note if note else "")
    gid = f"L{gi:03d}"; os.makedirs(f"{H}/groups/{gid}", exist_ok=True)
    open(f"{H}/groups/{gid}/renames.txt", "w").write("".join(f"{fl} {n} {nn}\n" for fl, n, nn in ren))
    manifest[gid] = {"plan_index": gi, "rule": p["rule"], "copies": [f"{f['name']}@{f['file']}" for f in fs], "refs": refs, "linked": bool(meta[p["pile"]]["refs"])}
    tasks.append({"id": gid, "system": SYS, "user": u, "out": f"{H}/groups/{gid}/ant_r1.md"})
json.dump(manifest, open(f"{H}/manifest_live.json", "w"), indent=1)
json.dump(tasks, open(f"{H}/tasks_live.json", "w"))
print(len(tasks), "harness ants; max est", max(est(t["system"], t["user"]) for t in tasks))
