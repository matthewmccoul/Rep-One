"""Root-cause investigation of failing acceptance tests with EVIDENCE PACKETS.
usage: phase_investigate.py <engine log> [round] [cluster ids...]
For each cluster of failing tests the script gathers: the failing C++ tests, our code, the reference C# test and engine code, the SRD paragraphs that mention the rule, and facts about the data file.
One ant per cluster returns a structured verdict (CODE_BUG | SRD_WRONG | SRD_SILENT | TEST_WRONG | DATA_SCOPE | UNRESOLVABLE) plus patches. Patches are applied by this script; SRD errata are collected."""
import asyncio, collections, glob, json, os, re, subprocess, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
from triage import find_function
from phase_fixtests import parse_failures, test_defs, test_body, apply as apply_fix

M = os.path.dirname(ROOT)
ENG = f"{M}/src/engine-source/OpenDejenol-v0.1.3"
SRD = f"{M}/out/srd"

# ---- clusters: failing-test name substrings, reference identifiers, SRD words, our identifiers -------------------------------------------
CLUSTERS = {
 "gold_factor": dict(tests=["_gold_factor"], cs=["GoldFactor"], srd=["gold factor", "GoldFactor", "goldFactor"], cpp=["kGoldFactor", "goldFactor"],
     note="Reference asserts GuildData.ByName(guild).GoldFactor (an integer from the data file MDATA1). Our test reads kGoldFactor, a table in our code. Decide whether our code needs a different representation or the test should read GameData."),
 "max_ad": dict(tests=["_max_ad", "best_independent", "guild_att_def_takes_max", "guild_att_def_zero"], cs=["GuildAD", "MaxAttack", "AdMaxLevel"], srd=["max A/D", "AdMaxLevel", "highest guild attack", "A/D cap", "MaxAttackDefence"], cpp=["MaxAttackDefence", "guildAD", "GuildAD", "bestGuildAD", "adInterval"],
     note="Reference: Rules.GuildAD at a huge guild level gives the guild's highest attack/defence (the wiki page value). Compare the formula, the per-guild inputs (AdMaxLevel, AdEvery, AttPerLevel, DefPerLevel in the data file) and the rounding."),
 "join_cost": dict(tests=["join_cost"], cs=["GuildJoinCost"], srd=["join cost", "GuildJoinCost", "5356"], cpp=["guildJoinCost", "GuildJoinCost", "JoinCost"],
     note="Reference: Rules.GuildJoinCost(GuildData) for each guild; the test expects a specific number. Our functions return something else (0?). Several modules declare join-cost functions; find which one the test calls and what the reference formula is."),
 "guildmasters": dict(tests=["named_guildmasters_on_new_game"], cs=["Guildmasters", "Seeded"], srd=["guildmaster", "Seeded", "new game"], cpp=["newGame", "claimMastership", "guildMasters"],
     note="Reference: a new game installs 12 named seeded guildmasters (Guildmasters.Seeded.Length == 12)."),
 "nomad_ad": dict(tests=["nomad_all_stats_15_guild_ad"], cs=["Paint", "GuildAD"], srd=["Char page", "guild A/D", "Nomad"], cpp=["guildAD", "bestGuildAD", "nomadValue", "GuildAttDef"],
     note="Reference: character with all stats 15 and a Nomad guild shows guild A/D 10/10 on the Char page."),
 "chest_award": dict(tests=["chest_award"], cs=["ChestExp"], srd=["chest experience", "ChestExp", "chest award", "disarm"], cpp=["chestAward"],
     note="Reference: Rules.ChestExp(partySize, disarmChance, monsterGuildLevel, characterGuildLevel). Cases: disarm 1000, monster level 2, character level 1/333/999/0 in a party of one give 997/500/250/1000; party of 2/3/4 at level 0 give 960/920/880."),
 "acid": dict(tests=["acid_count", "acid_reachable"], cs=["AcidTarget"], srd=["acid"], cpp=["rollAcidSlot", "monsterAcid", "pickCompanionSlot"],
     note="Reference: CombatEngine.AcidTarget over 60000 draws (Random seeded 11, four occupied companion slots): about 20% (10800..13200) return 0 (the character) and each other slot is similar. Compare how the target is drawn."),
 "readout": dict(tests=["readout"], cs=["Readout", "LookAround"], srd=["readout", "(Lost)", "living and dead"], cpp=["readout", "Readout", "lookReadout"],
     note="Reference: LookAround.Readout(lead, lead, x, y, z, facing, living, dead) returns text like '26,6,6 West [ 1 ]', '26,6,6 West [ 2+1 ]', '(Lost) West [ 1 ]'."),
 "data_scope": dict(tests=["total_squares", "teleporter_square_total"], cs=["LoadDungeon", "Teleporters"], srd=["teleporter", "15 levels", "fifteen"], cpp=[],
     note="Reference counts squares/teleporters over the 15 levels of MDATA11.MDR. Our data file may hold fewer levels. Use the DATA FACTS to decide whether this is unresolvable with the shipped data."),
 "spell_max": dict(tests=["cold_blast_max_damage", "acidic_spray_max_damage"], cs=["SpellDamage"], srd=["damage formula", "log term", "power term", "SpellDamage", "random cut"], cpp=["spellDamage", "damageFor", "magnitude"],
     note="Reference: Rules.SpellDamage with a random source that always returns zero gives the MAXIMUM damage: Cold Blast (base 15, power 2) at spell levels 15/65/115/255 expects 40,59,69,90."),
 "help_cry": dict(tests=["help_cry"], cs=["HelpCry"], srd=["help cry", "HelpCry"], cpp=["helpCryNeeded"],
     note="Reference: HelpCry.Needed: needed when current hits under 10 or under a tenth of max hits. 50 of 500 hits expects true; 9 of 20 expects true."),
 "poison": dict(tests=["highest_poison"], cs=["ChestTraps", "Spring"], srd=["trap 1", "poison gas", "poisoned"], cpp=["trapPoisonGas", "resolveChestTrap"],
     note="Reference: the race with the highest poison resistance (Resistances[5]) has chest trap 1 sprung on a fresh 15-stat character 4000 times: at least one ends poisoned and the poisoned percentage is near half of the base chance."),
 "companion_balance": dict(tests=["member0_chosen", "members_0_and_2_balanced"], cs=["CompanionMishap", "Victim"], srd=["mishap", "victim"], cpp=["companionMishap", "pickMonsterTarget"],
     note="Reference: CompanionMishap.Victim(...) over 40000 breath mishap draws with owner index 1: member 0 is chosen at least once, and members 0 and 2 are balanced."),
 "library_location": dict(tests=["location_after_second_filing"], cs=["FileFind"], srd=["FileFind", "library", "location"], cpp=["fileFind", "FileFind"],
     note="Reference: Librarian.FileFind a second filing for the same item (IdLevel.Partial, monster Id -2, 5 days later, not lost, level found 6): the stored Location is '?'."),
 "confinement": dict(tests=["confinement_replenish_peak_count"], cs=["Confinement", "Replenish"], srd=["replenish", "Confinement", "pen"], cpp=["replenish", "penStock"],
     note="Reference: Confinement.Replenish called 4000 times from empty pens: the peak shelf count reaches at least ShelfCap + 1."),
 "drag_swap": dict(tests=["empty_slot_dropped_on_slot0"], cs=["Rearrange"], srd=["rearrange", "drag", "companion slot"], cpp=["rearrange", "dragSwap", "swapSlots"],
     note="Reference: CompanionSlots.Rearrange(3, 0): an empty slot 3 dropped on slot 0 with slots B,A,C,empty gives (empty,B,A,C)."),
 "fifth_slot": dict(tests=["fifth_companion_no_slot"], cs=["CompanionSlots", "Companions"], srd=["companion slot", "first hole", "four slots"], cpp=["firstFreeCompanionSlot", "addCompanion"],
     note="Reference: a character holding four companions: Companions.Add of a fifth returns -1 (no slot)."),
 "death_cleanup": dict(tests=["cleanup_clears_ailments_and_carrier"], cs=["Sub0930Head", "EndEveryAilment"], srd=["EndEveryAilment", "Sub0930", "ailments cleared"], cpp=["endEveryAilment", "sub0930Head", "cleanup"],
     note="Reference: Character.Sub0930Head clears poison, disease, spell resistances, paralysis, and the carrier."),
 "exchange_overflow": dict(tests=["exchange_overflow_keeps_a"], cs=["TransferGold"], srd=["TransferGold", "exchange", "overflow"], cpp=["transferGold"],
     note="Reference: Party.TransferGold: A (on hand MaxValue) exchanges 5 to B (on hand MaxValue): report the return text and whether A's on-hand is unchanged."),
 "enc_spread": dict(tests=["encounter_spread"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived tests (written blind from the SRD) for the encounter spread fail. Decide whether the test, our code or the SRD is wrong; the reference C# is the tie-breaker."),
 "perception": dict(tests=["perceive_ahead", "square_perception"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived tests for perception/notice/identify thresholds fail at a boundary."),
 "pit_hover": dict(tests=["pit_hover"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived tests for the pit hover chance fail (level zero boundary, level term)."),
 "rotator_flags": dict(tests=["rotator_failure", "work_out_high_draw"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived tests: a failed rotator and a high draw in the square routine must keep floor flags; our code changes flags."),
 "locate_char": dict(tests=["locateCharacter"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived test for locateCharacter (position and dead flag)."),
 "guild_item": dict(tests=["guild_item_checks"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived test: the guild item check looks at the primary guild only."),
 "ninja_titles": dict(tests=["ninja_title_ladder"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived test: Ninja title ladder by guild level."),
 "quest_tier": dict(tests=["quest_tier"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived test: guild quest tier examples and boundaries."),
 "id_chances": dict(tests=["identify_recognise_rename"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived test: identify / recognise / rename chances clamp."),
 "typed_gold": dict(tests=["typed_gold"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived test: typed gold uses CLng and the thief take caps at the purse."),
 "chest_lock": dict(tests=["loot2_lock"], cs="AUTO", srd="AUTO", cpp="AUTO", note="SRD-derived tests: chest lock roll (magic locked, boundary at the locked chance, zero and full chance, container class never magic locked)."),
 "rt_stat": dict(tests=["engine_rtt020"], cs="AUTO", srd="AUTO", cpp="AUTO", note="Reference assertions (re-translated, FIXED, expected values verbatim): a strength +2 potion caps at the race ceiling plus 5; a reversed negative modifier on intelligence gives 13. Our stat-modification code disagrees; fix the CODE (or prove the test setup wrong)."),
 "rt_wear": dict(tests=["engine_rtt140"], cs="AUTO", srd="AUTO", cpp="AUTO", note="Reference assertion (FIXED): an item whose wear class has a maximum of zero is not wearable. Our isWearable says it is."),
 "rt_days": dict(tests=["engine_rtt195"], cs="AUTO", srd="AUTO", cpp="AUTO", note="Reference assertion (FIXED): days until found with a zero roll on level 1, area 5 is 1. Ours differs."),
}

# ---- evidence ------------------------------------------------------------------------------------------------------------------------
def cs_members(words, folder, maxchars):
    out, seen = [], set()
    files = sorted(glob.glob(f"{ENG}/{folder}/*.cs"))
    scored = []
    for f in files:
        try: t = open(f, encoding="utf-8", errors="replace").read()
        except Exception: continue
        lines = t.split("\n")
        for w in words:
            for i, l in enumerate(lines):
                if re.search(rf"\b{re.escape(w)}\b", l):
                    # member start: nearest previous line that looks like a declaration with '('
                    s = i
                    while s > 0 and not re.match(r"^\s{4,12}(public|internal|private|protected|static)[^;=]*\(", lines[s]): s -= 1
                    if s == 0 and not re.match(r"^\s{4,12}(public|internal|private|protected|static)", lines[0]): s = i
                    key = (f, s)
                    if key in seen: continue
                    seen.add(key)
                    depth, j, started = 0, s, False
                    while j < len(lines) and j < s + 160:
                        depth += lines[j].count("{") - lines[j].count("}"); started |= "{" in lines[j]
                        if started and depth <= 0: break
                        j += 1
                    body = "\n".join(lines[max(0, s - 4):j + 1])
                    isdef = bool(re.search(rf"\b{re.escape(w)}\s*\(", lines[s])) and w in lines[s]
                    scored.append((0 if isdef else 1, len(body), f"// ---- {os.path.relpath(f, ENG)}:{s+1}\n{body}"))
    scored.sort(key=lambda x: (x[0], x[1])); used = 0
    for _, n, b in scored:
        if used + len(b) > maxchars: continue
        out.append(b); used += len(b)
    return "\n\n".join(out)
def cs_test_context(words, maxchars):
    out, used, seen = [], 0, set()
    for f in sorted(glob.glob(f"{ENG}/tests/Mordor.Tests/*.cs")):
        t = open(f, encoding="utf-8", errors="replace").read().split("\n")
        for i, l in enumerate(t):
            if any(re.search(rf"\b{re.escape(w)}\b", l) for w in words):
                a, b = max(0, i - 10), min(len(t), i + 12)
                key = (f, a // 12)
                if key in seen: continue
                seen.add(key); blk = f"// ---- {os.path.basename(f)}:{i+1}\n" + "\n".join(t[a:b])
                if used + len(blk) <= maxchars: out.append(blk); used += len(blk)
    return "\n\n".join(out)
def srd_hits(words, maxchars):
    out, used, seen = [], 0, set()
    for p in sorted(glob.glob(f"{SRD}/[0-2][0-9]_*.md")):
        n = os.path.basename(p)[:2]
        for para in re.split(r"\n\s*\n", open(p, encoding="utf-8").read()):
            low = para.lower()
            if any(w.lower() in low for w in words) and len(para) < 2500:
                k = para[:80]
                if k in seen: continue
                seen.add(k); blk = f"[ch{n}] {para.strip()}"
                if used + len(blk) <= maxchars: out.append(blk); used += len(blk)
    return "\n\n".join(out)
def cpp_code(names, maxchars):
    out, used, seen = [], 0, set()
    idx = {}
    for f in glob.glob(f"{ROOT}/src/*.cpp"):
        t = open(f).read()
        for n in names:
            if n in t:
                loc = find_function(t, n)
                if loc and (n, f) not in seen:
                    seen.add((n, f)); blk = f"// ---- {os.path.relpath(f, ROOT)} :: {n}\n{t[loc[0]:loc[1]]}"
                    if used + len(blk) <= maxchars: out.append(blk); used += len(blk)
    # one-line grep hits for non-function identifiers (tables, constants)
    hits = []
    for h in glob.glob(f"{ROOT}/include/mordor/*.h") + glob.glob(f"{ROOT}/src/*.cpp"):
        for i, l in enumerate(open(h).read().split("\n")):
            if any(re.search(rf"\b{re.escape(n)}\b", l) for n in names) and not l.strip().startswith("//") and len(l) < 220:
                hits.append(f"{os.path.relpath(h, ROOT)}:{i+1}: {l.strip()}")
    return "\n\n".join(out) + ("\n\n// ---- other uses (grep)\n" + "\n".join(hits[:30]) if hits else "")
def case_records(names):
    out = []
    for n in names:
        m = re.match(r"engine_(B_\d+)_(\d+)_", n)
        if not m: continue
        f = f"{M}/testcheck/cases/{m.group(1)}.jsonl"
        if not os.path.exists(f): continue
        for l in open(f):
            r = json.loads(l)
            if r["id"] == f"{m.group(1)}#{m.group(2)}":
                out.append(f"[{r['id']}] ({r.get('api')}) {r['question']}\n   expected: {r['expected']}")
    return "\n".join(out)
def decl_blocks(names, maxchars):
    """Full definitions of structs/enums, constexpr constants and typedefs from our headers, and helper functions from the tests."""
    out, used, seen = [], 0, set()
    hdrs = sorted(glob.glob(f"{ROOT}/include/mordor/*.h"))
    for n in names:
        for h in hdrs:
            t = open(h).read()
            for m in re.finditer(rf"^\s*(?:struct|class|enum class|enum)\s+{re.escape(n)}\b[^;{{]*\{{", t, re.M):
                depth, j = 0, m.end() - 1
                while j < len(t):
                    if t[j] == "{": depth += 1
                    elif t[j] == "}":
                        depth -= 1
                        if depth == 0: break
                    j += 1
                blk = f"// ---- {os.path.relpath(h, ROOT)}\n" + t[m.start():j + 2]
                if (n, h) not in seen and used + len(blk) <= maxchars: seen.add((n, h)); out.append(blk); used += len(blk)
            for m in re.finditer(rf"^[^\n]*\b(?:constexpr|const)\b[^\n;]*\b{re.escape(n)}\b[^\n;]*(?:;|\n)", t, re.M):
                blk = f"// ---- {os.path.relpath(h, ROOT)}\n" + m.group(0).strip()
                if (m.group(0), h) not in seen and used + len(blk) <= maxchars: seen.add((m.group(0), h)); out.append(blk); used += len(blk)
        for f in glob.glob(f"{ROOT}/tests/test_engine_*.cpp") + glob.glob(f"{ROOT}/tests/test_support.h"):
            t = open(f).read()
            if n in t and (n, f) not in seen:
                loc = find_function(t, n)
                if loc and not t[loc[0]:loc[1]].startswith("TEST"):
                    blk = f"// ---- {os.path.relpath(f, ROOT)} (test helper)\n" + t[loc[0]:loc[1]]
                    if used + len(blk) <= maxchars: seen.add((n, f)); out.append(blk); used += len(blk)
    return "\n\n".join(out)
def need_tokens(ans):
    want = " ".join(re.findall(r"NEED:\s*(.*)", ans))
    return list(dict.fromkeys(re.findall(r"[A-Za-z_]\w{3,}", want) + re.findall(r"`(\w{4,})`", ans)))
def guild_dump():
    d = json.load(open(f"{ROOT}/data/combat-data.json"))
    rows = []
    for g in d["guilds"]:
        r = {k: v for k, v in g.items() if k not in ("reqStats", "spellRate", "tableIndex", "hasSpellTable")}
        r["spellMax"] = [x for x in g["spellMax"]]
        rows.append(json.dumps(r, separators=(",", ":")))
    return "GUILD ENTRIES (data file, order = guild index 0..11)\n" + "\n".join(rows)
STOP = set("test tests that this with from have when then than only never always keeps keep uses used returns return expected check checks case cases boundary boundaries examples example level levels zero full chance roll value values result results party character should must into does also each over under being their there where which while about after before above below equal least most more less".split())
_MEM = None
def _members():
    global _MEM
    if _MEM is None:
        _MEM = []
        for f in sorted(glob.glob(f"{ENG}/src/Mordor.Core/*.cs")):
            lines = open(f, encoding="utf-8", errors="replace").read().split("\n"); i = 0
            while i < len(lines):
                if re.match(r"^\s{4,12}(public|internal|private|protected|static)[^;=]*\(", lines[i]):
                    s = i
                    while s > 0 and lines[s - 1].strip().startswith("///"): s -= 1
                    depth, j, started = 0, i, False
                    while j < len(lines) and j < i + 140:
                        depth += lines[j].count("{") - lines[j].count("}"); started |= "{" in lines[j]
                        if started and depth <= 0: break
                        j += 1
                    _MEM.append((f, s, "\n".join(lines[s:j + 1]))); i = j + 1
                else: i += 1
    return _MEM
def tokens_of(*texts):
    toks = []
    for tx in texts:
        for w in re.findall(r"[A-Za-z]+", re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", tx)):
            w = w.lower()
            if len(w) >= 4 and w not in STOP: toks.append(w)
    return list(dict.fromkeys(toks))
def cs_rank(tokens, maxchars, k=4):
    import math
    mem = _members(); low = [m[2].lower() for m in mem]
    df = {w: sum(1 for x in low if w in x) for w in tokens}
    scored = []
    for (f, s, body), lw in zip(mem, low):
        sc = sum(math.log(1 + len(mem) / (1 + df[w])) for w in tokens if w in lw) / (1 + len(body) / 6000.0)
        if sc > 0: scored.append((sc, f, s, body))
    scored.sort(key=lambda x: -x[0]); out, used = [], 0
    for sc, f, s, body in scored[:k * 3]:
        if used + len(body) > maxchars: continue
        out.append(f"// ---- {os.path.relpath(f, ENG)}:{s+1} (match score {sc:.1f})\n{body}"); used += len(body)
        if len(out) >= k: break
    return "\n\n".join(out)
def srd_rank(tokens, maxchars, k=6):
    import math
    paras = []
    for p in sorted(glob.glob(f"{SRD}/[0-2][0-9]_*.md")):
        n = os.path.basename(p)[:2]
        for para in re.split(r"\n\s*\n", open(p, encoding="utf-8").read()):
            if 40 < len(para) < 2500: paras.append((n, para.strip(), para.lower()))
    df = {w: sum(1 for x in paras if w in x[2]) for w in tokens}
    scored = sorted(((sum(math.log(1 + len(paras) / (1 + df[w])) for w in tokens if w in lw), n, para) for n, para, lw in paras), key=lambda x: -x[0])
    out, used = [], 0
    for sc, n, para in scored[:k * 3]:
        if sc <= 0 or used + len(para) > maxchars: continue
        out.append(f"[ch{n}] {para}"); used += len(para)
        if len(out) >= k: break
    return "\n\n".join(out)

def data_facts():
    d = json.load(open(f"{ROOT}/data/combat-data.json"))
    inv = {v: k for k, v in d["floorFlagBits"].items()}
    tele = sum(1 for fl in d["floors"] for f in fl["fields"] if (f["flags"] >> d["floorFlagBits"]["TELEPORTER"]) & 1)
    sq = sum(len(fl["fields"]) for fl in d["floors"])
    return (f"The data file holds {len(d['floors'])} floors ({', '.join(str(fl['levelNum']) for fl in d['floors'])}), {sq} squares in total, {tele} squares with the TELEPORTER flag, "
            f"{len(d['monsters'])} monsters, {len(d['guilds'])} guilds. Source: {d['source']}. Guild goldFactor values: " + str({g['name']: g['goldFactor'] for g in d['guilds']}))

CONV = open(f"{ROOT}/spec/CONVENTIONS.md").read()
SYS = ("You are a root-cause investigator for failing acceptance tests in a C++ re-implementation of a game's rules. The tests were translated from the REFERENCE implementation's own assertions (C#); the reference values are ground truth. "
       "You get an EVIDENCE PACKET: the failing C++ tests, our code, the reference C# test and engine code, the SRD paragraphs that mention the rule, and facts about the data file. "
       "Decide the cause and fix it. Verdicts: CODE_BUG (our code deviates from the reference and the SRD states the rule: fix the code); SRD_WRONG (the SRD states something different from the reference: fix the code to match the reference AND give an erratum for the SRD); "
       "SRD_SILENT (the SRD does not contain the rule; implement it from the reference and give the SRD an addition); TEST_WRONG (the translation is wrong: units are Currency = gold*10000, indexes 1-based for members and Party.x/y, wrong function or argument meaning; fix the test but NEVER change an expected value); "
       "DATA_SCOPE (the reference result depends on data our data file does not contain; prove it from the data facts); UNRESOLVABLE (the reference mechanic is incomplete, a placeholder, or cannot be expressed in this API; quote the reference code that shows it). "
       "Quote evidence briefly. Write patches as complete definitions:\n"
       "=== REPLACE <file> :: <function> ===\n<new function>\n=== END REPLACE ===\n"
       "=== REPLACE_TEST <name> ===\n<new TEST(<name>) { ... }>\n=== END ===\n"
       "=== EDIT <file> ===\n<exact old text, must occur once>\n=== TO ===\n<new text>\n=== END EDIT ===\n"
       "=== ERRATUM ch<NN> ===\n<one or two sentences to add to that SRD chapter>\n=== END ERRATUM ===\n"
       "If you cannot patch because code is missing from the packet, write the verdict you believe is most likely, give no patch, and end with one line `NEED: name1, name2` naming the exact functions or tables of OUR code you need (the next round will include them). Never guess a body you cannot see. "
       "Begin your answer with `VERDICT: <one of the six>` then `REFERENCE:`, `SRD:`, `OURS:` and `REASON:` lines, then the patches. Keep signatures; no I/O, globals or exceptions. Do not invent members or functions that are not shown.\n\n" + CONV)

def normalise(out):
    """Ants often close a block with '=== END ===' instead of the exact marker: accept both."""
    out = re.sub(r"(=== REPLACE \S+ :: \w+ ===\n.*?)\n=== END ===", r"\1\n=== END REPLACE ===", out, flags=re.S)
    out = re.sub(r"(=== REPLACE_TEST \w+ ===\n.*?)\n=== END (?:TEST|REPLACE_TEST|REPLACE) ===", r"\1\n=== END ===", out, flags=re.S)
    out = re.sub(r"(=== EDIT \S+ ===\n.*?\n=== TO ===\n.*?)\n=== END(?: EDIT)? ===", r"\1\n=== END EDIT ===", out, flags=re.S)
    return out
def fix_paths(out):
    """If the named file does not define the function, point the block at the unique src file that does."""
    from triage import find_function as ff
    def sub(m):
        f, fn = m.group(1), m.group(2)
        p = f"{ROOT}/{f}"
        if os.path.exists(p) and ff(open(p).read(), fn): return m.group(0)
        hits = [g for g in glob.glob(f"{ROOT}/src/*.cpp") if ff(open(g).read(), fn)]
        if len(hits) == 1: return f"=== REPLACE {os.path.relpath(hits[0], ROOT)} :: {fn} ==="
        return m.group(0)
    return re.sub(r"=== REPLACE (\S+) :: (\w+) ===", sub, out)
def apply_patches(out):
    out = fix_paths(normalise(out))
    n = apply_fix(out)   # REPLACE_TEST and REPLACE blocks
    for m in re.finditer(r"=== EDIT (\S+) ===\n(.*?)\n=== TO ===\n(.*?)\n=== END EDIT ===", out, re.S):
        f, old, new = m.groups(); p = f"{ROOT}/{f}"
        if os.path.exists(p):
            t = open(p).read()
            if t.count(old) == 1: open(p, "w").write(t.replace(old, new)); n += 1
    return n
def collect_errata(out, cid):
    return [(m.group(1), m.group(2).strip(), cid) for m in re.finditer(r"=== ERRATUM ch(\d\d) ===\n(.*?)\n=== END ERRATUM ===", out, re.S)]

if __name__ == "__main__":
    log = sys.argv[1]; rnd = sys.argv[2] if len(sys.argv) > 2 else "1"; only = sys.argv[3:]
    fails = parse_failures(log)
    for extra in os.environ.get("ALSO", "").split(","):
        if extra: fails.update(parse_failures(extra))
    defs = test_defs(); fidx = None
    prev = {}
    CF = f"{ROOT}/logs/gate_conflicts.json"
    conflicts = json.load(open(CF)) if os.path.exists(CF) else {}
    pf = f"{ROOT}/logs/investigate_round{int(rnd)-1}.json" if int(rnd) > 1 else None
    if pf and os.path.exists(pf): prev = json.load(open(pf))
    tasks, meta = [], {}
    for cid, c in CLUSTERS.items():
        if only and cid not in only: continue
        names = [n for n in fails if any(s in n for s in c["tests"])]
        if not names: continue
        c = dict(c)
        if "AUTO" in (c["cpp"], c["cs"], c["srd"]):
            import phase_fixtests as pf_auto
            fi = pf_auto.src_function_index(); called = []
            for n in names:
                lo = test_body(defs[n], n) if n in defs else None
                if lo: called += re.findall(r"\b([a-zA-Z]\w{4,})\s*\(", open(defs[n]).read()[lo[0]:lo[1]])
            called = [x for x in dict.fromkeys(called) if x in fi][:10]
            spaced = [re.sub(r"(?<=[a-z0-9])(?=[A-Z])", " ", x).lower() for x in called]
            c["auto"] = True
            c["tokens"] = tokens_of(" ".join(names), c["note"], " ".join(called), " ".join(re.findall(r"//\s*(.*)", " ".join(open(defs[n]).read()[test_body(defs[n], n)[0]:test_body(defs[n], n)[1]] for n in names if n in defs and test_body(defs[n], n)))))
            if c["cpp"] == "AUTO": c["cpp"] = called
            if c["cs"] == "AUTO": c["cs"] = [x[0].upper() + x[1:] for x in called]
            if c["srd"] == "AUTO": c["srd"] = spaced + called
        bodies = []
        for n in names:
            loc = test_body(defs[n], n) if n in defs else None
            if loc: bodies.append(open(defs[n]).read()[loc[0]:loc[1]])
        cpp_names = list(c["cpp"]); need_decls = ""
        if cid in prev:
            if fidx is None:
                import phase_fixtests as pf_mod; fidx = pf_mod.src_function_index()
            ans = prev[cid].get("answer", "")
            want = re.findall(r"NEED:\s*(.*)", ans)
            toks = need_tokens(ans)
            cpp_names = [x for x in toks if x in fidx][:14] + [x for x in cpp_names if x not in toks]   # requested code first
            # one hop down the call graph: functions called by the ones we already show (and by the failing tests' helpers)
            callees = []
            for nm in list(cpp_names) + [w for w in re.findall(r"\b([a-z]\w{3,})\s*\(", " ".join(bodies)) if w in fidx]:
                for f in fidx.get(nm, [])[:1]:
                    loc = find_function(open(f).read(), nm)
                    if loc:
                        for cal in re.findall(r"\b([A-Za-z_]\w{3,})\s*\(", open(f).read()[loc[0]:loc[1]]):
                            if cal in fidx and cal not in cpp_names and cal not in callees and cal not in ("static_cast", "sizeof"): callees.append(cal)
            cpp_names += callees[:10]
            need_decls = decl_blocks(toks, 9000)
        preamble = ""
        for tf in sorted({defs[n] for n in names if n in defs}):
            src_t = open(tf).read(); k = src_t.find("\nTEST(")
            preamble += f"// ---- {os.path.relpath(tf, ROOT)} (helpers before the first test)\n" + src_t[:k][:2500] + "\n"
        flines = "\n".join(f"{n}: line {ln}: {ex}" for n in names for (_, ln, ex) in fails[n][:3])
        budget = dict(tests=6000, cstest=8000, cs=15000, cpp=12000, srd=9000, decl=3000, need=9000) if cid not in prev else dict(tests=4000, cstest=4000, cs=7000, cpp=26000, srd=4000, decl=2500, need=9000)
        def packet(b):
            parts = [f"CLUSTER {cid}: {len(names)} failing tests\n{c['note']}", "FAILING C++ TESTS\n" + "\n\n".join(bodies)[:b["tests"]], "TEST FILE HELPERS\n" + preamble[:b["tests"]//2], "FAILURE LINES\n" + flines[:2500],
                     "REFERENCE CASE RECORDS (question text and expected answer as extracted from the reference)\n" + case_records(names), "REFERENCE C# TESTS\n" + cs_test_context(c["cs"], b["cstest"]), "REFERENCE C# ENGINE CODE\n" + (cs_rank(c["tokens"], b["cs"]) if c.get("auto") else cs_members(c["cs"], "src/Mordor.Core", b["cs"])),
                     "OUR C++ CODE\n" + (cpp_code(cpp_names, b["cpp"]) if cpp_names else "(none named)"), "REQUESTED DEFINITIONS (structs, constants, test helpers you asked for)\n" + need_decls[:b["need"]], "DECLARATIONS (relevant API)\n" + api_index.relevant_index(" ".join(bodies) + " " + " ".join(cpp_names), cap=b["decl"]), "SRD PARAGRAPHS\n" + ((srd_rank(c["tokens"], b["srd"]) if c.get("auto") else srd_hits(c["srd"], b["srd"])) or "(no paragraph mentions it)"), "DATA FACTS\n" + data_facts() + ("\n\n" + guild_dump() if cid in ("join_cost", "max_ad", "nomad_ad", "guildmasters") else "")]
            if cid in conflicts: parts.append("CONFLICTING TESTS: your previous patch for this cluster was REVERTED because it made these passing tests fail. Your next patch must satisfy BOTH these tests and the failing ones (split a function in two if one function serves two readings):\n" + "\n".join(conflicts[cid])[:2500])
            if cid in prev: parts.append("PREVIOUS ATTEMPT (it did not resolve the failures)\n" + prev[cid].get("answer", "")[:2200])
            return "\n\n".join(parts)
        user = packet(budget)
        shrink = 0
        while est_tokens(SYS, user) > 31_000 and shrink < 12:
            budget = {k: int(v * 0.85) for k, v in budget.items()}; user = packet(budget); shrink += 1
        print(cid, len(names), "tests; est", est_tokens(SYS, user), flush=True)
        if os.environ.get("DRY"):
            os.makedirs(f"{ROOT}/spec/investigate", exist_ok=True); open(f"{ROOT}/spec/investigate/packet_{cid}.txt", "w").write(user)
        tasks.append({"id": cid, "system": SYS, "user": user, "allow": [], "save": f"spec/investigate/r{rnd}_{cid}.out"}); meta[cid] = names
    if os.environ.get("DRY"): sys.exit(0)
    asyncio.run(run_all(tasks, f"investigate{rnd}", 4))
    results, errata, verdicts = {}, [], collections.Counter(); gate = None; gbase = None
    for t in tasks:
        p = f"{ROOT}/{t['save']}"
        if not os.path.exists(p): continue
        out = open(p).read(); cid = t["id"]
        v = re.search(r"VERDICT:\s*(CODE_BUG|SRD_WRONG|SRD_SILENT|TEST_WRONG|DATA_SCOPE|UNRESOLVABLE)", out)
        if os.environ.get("GATE"):
            from gate import Gate
            if gate is None: gate = Gate(); gbase = gate.run()
            gate.backup(); n = apply_patches(out)
            if n:
                after = gate.build_and_run(); new = gate.new_failures(gbase, after)
                if new:
                    gate.restore(); conflicts[cid] = [f"{k}: " + "; ".join(v[:2]) for k, v in list(new.items())[:12]]
                    print(f"  GATE REVERTED {cid}: {len(new)} newly failing: {list(new)[:5]}", flush=True); n = 0
                    json.dump(conflicts, open(CF, "w"), indent=1)
                else:
                    gbase = after; conflicts.pop(cid, None); json.dump(conflicts, open(CF, "w"), indent=1)
                    print(f"  gate ok {cid}: engine failing {len(after['engine'])}, srd failing {len(after['srd'])}", flush=True)
        else:
            n = apply_patches(out)
        errata += collect_errata(out, cid)
        verdicts[v.group(1) if v else "NONE"] += len(meta[cid])
        results[cid] = {"verdict": v.group(1) if v else None, "tests": meta[cid], "patches": n, "answer": out}
        print(f"{cid:18} {v.group(1) if v else 'NONE':13} patches {n}  tests {len(meta[cid])}")
    json.dump(results, open(f"{ROOT}/logs/investigate_round{rnd}.json", "w"), indent=1)
    old = json.load(open(f"{ROOT}/logs/errata.json")) if os.path.exists(f"{ROOT}/logs/errata.json") else []
    json.dump(old + errata, open(f"{ROOT}/logs/errata.json", "w"), indent=1)
    print("verdicts (by test):", dict(verdicts), "| errata added:", len(errata))
