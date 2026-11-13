"""Second-tier fixer: .cpp files that use struct members/enumerators that the header lacks.
The ant sees the .cpp, the errors and ONLY the struct/enum definitions involved; it returns the corrected .cpp plus APPEND blocks that this script inserts
into the defining header (append-only). game_data.h is generated and never edited. Sequential. usage: fix_members.py"""
import asyncio, glob, json, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens, parse_files
SYS = ("You fix C++ compile errors where a .cpp file uses struct members, enumerators or types that the headers do not declare. You get the .cpp, the compiler errors and the definitions of the structs/enums involved. "
       "For each error decide: if the use is sensible for the game rules, ADD the missing member (with a default initializer) or enumerator via an APPEND block; otherwise change the .cpp to use what exists. "
       "NEVER add members to SpellDef, RaceDef, GuildDef, ItemDef, MonsterDef or other GameData tables (generated from a data file): derive from the fields that exist. Never rename, remove or retype existing members. No I/O, globals or exceptions.\n"
       "Output the COMPLETE corrected .cpp as `=== FILE: <path> ===` ... `=== END FILE ===`, then zero or more APPEND blocks:\n=== APPEND struct <Name> ===\n  i32 newMember = 0;  // ADDED\n=== END APPEND ===\n=== APPEND enum <Name> ===\n  NewValue,\n=== END APPEND ===")
DATA_TYPES = {"RaceDef", "GuildDef", "SpellDef", "ItemDef", "MonsterDef", "NamedEntry", "FloorField", "FloorArea", "FloorTeleporter", "FloorChute", "FloorDef", "GameData"}
def find_def(name):
    for h in sorted(glob.glob(f"{ROOT}/include/mordor/*.h")):
        if h.endswith("game_data.h"): continue
        t = open(h).read()
        m = re.search(rf"^(?:struct|enum class|enum)\s+{name}\b[^;{{]*\{{", t, re.M)
        if m:
            depth, i = 0, m.end() - 1
            while i < len(t):
                if t[i] == "{": depth += 1
                elif t[i] == "}":
                    depth -= 1
                    if depth == 0: return h, m.start(), i
                i += 1
    return None
def apply_appends(text):
    n = 0
    for m in re.finditer(r"=== APPEND (struct|enum) (\w+) ===\n(.*?)\n=== END APPEND ===", text, re.S):
        kind, name, body = m.groups()
        if name in DATA_TYPES: continue
        d = find_def(name)
        if not d: continue
        h, s, e = d; t = open(h).read()
        inner = t[s:e].rstrip()
        add = body.rstrip("\n")
        if kind == "enum" and not inner.rstrip().endswith(("{", ",")): inner += ","
        t = t[:s] + inner + "\n" + add + "\n" + t[e:]
        open(h, "w").write(t); n += 1
    return n
if __name__ == "__main__":
    errs = json.load(open(f"{ROOT}/logs/build_errors.json")); tasks = []
    for f, e in errs.items():
        if not f.startswith("src/"): continue
        body = open(f"{ROOT}/{f}").read(); txt = "\n".join(e)
        names = set(re.findall(r"struct mordor::(\w+)", txt)) | set(re.findall(r"is not a member of ‘mordor::(\w+)’", txt)) | set(re.findall(r"to ‘(?:mordor::)?(\w+)’", txt)) | set(re.findall(r"‘(\w+)’ does not name a type", txt))
        defs = []
        for n in sorted(names):
            if n in DATA_TYPES: defs.append(f"// {n} is generated GameData (do not edit): see game_data.h"); continue
            d = find_def(n)
            if d: defs.append(open(d[0]).read()[d[1]:d[2] + 2])
        user = f"FILE: {f}\n\nCOMPILER ERRORS ({len(e)}):\n" + "\n".join(x[x.find('src/'):] if 'src/' in x else x for x in e[:40]) + "\n\nDEFINITIONS INVOLVED\n" + "\n\n".join(defs) + f"\n\nCPP FILE CONTENT\n{body}"
        est = est_tokens(SYS, user); print(f, "defs", len(defs), "est", est)
        if est > 33_000: print("  too big, skipped"); continue
        tasks.append({"id": f, "system": SYS, "user": user, "allow": [f], "save": f"spec/fixmem/{os.path.basename(f)}.out"})
    asyncio.run(run_all(tasks, "fix_members", 1))
    total = 0
    for t in tasks:
        p = f"{ROOT}/{t['save']}"
        if os.path.exists(p): total += apply_appends(open(p).read())
    print("appended", total, "member blocks to headers")
