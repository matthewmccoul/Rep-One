"""Combat.cs in parts: other types (A), the CombatEngine declaration (H), CombatEngine bodies in chunks (B1..Bn)."""
import asyncio, json, os, re, sys
X = sys.argv[1]; sys.path.insert(0, f"{X}/scripts"); from army import run_all, est
CORE = "/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/dl/src/OpenDejenol-v0.1.3/src/Mordor.Core"
src = open(f"{CORE}/Combat.cs").read().splitlines()
def nodoc(lines): return "\n".join(l for l in lines if not l.strip().startswith("///"))
head = src[:3]; eng = src[225:2311]; rest = src[3:225] + src[2311:]
cat = json.load(open(f"{X}/catalog.json")); RULES = open(f"{X}/scripts/RULES.md").read(); CSH = open(f"{X}/cpp/cs.h").read()
TUP = open(f"{X}/cpp/tuples_list.txt").read().splitlines()
simple = {}
for q, c in cat.items(): simple.setdefault(q.split(".")[-1], []).append(q)
def entry(q, c):
    h = f"### C# {q} -> C++ `{c['cpp']}` : {c['kind'].upper()}" + (" (STRUCT, by value)" if c["kind"] == "struct" else (" (use cs::Ref<" + c["cpp"] + ">)" if c["kind"] in ("class", "interface") else ""))
    if len(c.get("files", [])) > 1: h += f"  PARTIAL across {c['files']}"
    if c.get("base"): h += f"  bases: {c['base']}"
    return h + "\n" + "\n".join("  " + d for d in c["decl"])
def packet(stem, text, own, note):
    words = set(re.findall(r"\b[A-Z]\w+\b", text)); used = sorted({q for w in words for q in simple.get(w, []) if q not in own})
    parts = [f"STEM: {stem}", note, "TYPES DECLARED IN THIS PART:"] + [entry(q, cat[q]) for q in own] + ["TYPES THIS PART USES:"] + [entry(q, cat[q]) for q in used]
    tn = set(re.findall(r"\bTup_\w+", "\n".join(parts))); parts += ["TUPLE STRUCTS (tuples.h):"] + [l for l in TUP if re.search(r"struct (\w+)", l).group(1) in tn]
    parts += ["cs.h (support runtime):", CSH, "C# SOURCE (part of Combat.cs):", text]
    return {"id": stem, "system": RULES, "user": "\n\n".join(parts), "out": f"{X}/out/{stem}.md"}
own_rest = ["CombatMonster", "MonsterProperty", "SpecialAttack", "FoeSlot", "RoomRecord", "Encounter", "EncounterGenerator"]
own_rest = [q for q in cat if q.split(".")[0] in own_rest]
tasks = [packet("Combat_A", nodoc(head + rest), own_rest, "This part holds every type of Combat.cs except CombatEngine. Write its .h and .cpp normally.")]
eng_q = [q for q in cat if q == "CombatEngine" or q.startswith("CombatEngine.")]
tasks.append(packet("Combat_H", nodoc(head + eng), eng_q, "This part is ONLY the class declaration of the partial class CombatEngine (HEADER_OWNER) and its nested types. Write the .h with the complete CombatEngine declaration: every member in the catalog entry (from both Combat.cs and MonsterAttack.cs), data members with their C# initializers, static fields, nested-type aliases. Write the .cpp as just `#include \"core.h\"` and an empty namespace mc: method bodies are written by other parts."))
# chunks of CombatEngine bodies at member boundaries (a line at 4-space indent starting a member, after a blank line)
cuts = [i for i in range(1, len(eng)) if eng[i - 1].strip() == "" and re.match(r"^    (public|private|internal|protected|static|\[)", eng[i])]
chunks, start, size = [], 1, 520
for c in cuts:
    if c - start >= size: chunks.append((start, c)); start = c
chunks.append((start, len(eng) - 1))
for k, (a, b) in enumerate(chunks):
    text = nodoc(head + [eng[0], "{"] + eng[a:b] + ["}"])
    tasks.append(packet(f"Combat_B{k+1}", text, eng_q, f"This part is lines of the partial class CombatEngine (its declaration is written elsewhere: do NOT declare the class). Write the .h as only `#pragma once` plus the three includes and an empty namespace mc. Write the .cpp with the out-of-class definitions `... CombatEngine::Method(...) { ... }` of every method, constructor and computed property in this part, exactly matching the catalog signatures; static field definitions are not needed (they are inline in the header)."))
print(len(tasks), "parts", [ (t["id"], est(t["system"], t["user"])) for t in tasks], flush=True)
asyncio.run(run_all(tasks, f"{X}/translate_combat.log.jsonl", 8))
