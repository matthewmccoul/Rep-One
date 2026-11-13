"""Phase 3: module headers. usage: phase_headers.py <wave 1-4>  One architect ant per SRD part writes include/mordor/<module>.h"""
import asyncio, json, os, re, sys
sys.path.insert(0, os.path.dirname(__file__))
from cant import ROOT, run_all, est_tokens
import api_index
from partsplit import split_half
WAVES = {1: ["02", "03", "04", "05", "06"], 2: ["07", "08", "09", "10"], 3: ["11", "12", "13", "14"], 4: ["15", "16", "17", "18", "19", "20", "21", "22"]}
SLUG = {"02": "attributes", "03": "races", "04": "guilds", "05": "progression", "06": "clock", "07": "combat", "08": "status", "09": "casting", "10": "spells", "11": "monsters", "12": "companions",
        "13": "items", "14": "loot", "15": "dungeon", "16": "city", "17": "party", "18": "death", "19": "options", "20": "dataformats", "21": "modding", "22": "misc"}
CONV = open(f"{ROOT}/spec/CONVENTIONS.md").read()
SYS = ("You are an architect ant. You design the PUBLIC C++ header for one part of a rules document (SRD) of a dungeon RPG, so that other ants can implement and test it in parallel. "
       "Declare every rule routine the text describes as a free function with exact parameter and return types, using the shared state structs, GameData and Ctx shown below. "
       "Group declarations by rule number with a one-line comment each (`// R18 HP gain on level-up`). Add the constants, enums and small result structs the routines need (e.g. outcome enums, tables the text gives as constants as `inline constexpr` arrays). "
       "Rules about the player's text screens are out of scope. Do NOT redeclare anything already in the API INDEX (call it instead). Declare only; no bodies except trivial inline constants/accessors. "
       "State a rule's randomness explicitly in its signature by taking Ctx& (it supplies rng and events). Mutating functions take the state by reference; pure queries take const references.\n\n"
       + CONV + "\nOutput ONLY the file in exactly this form:\n=== FILE: include/mordor/<module>.h ===\n#pragma once\n...\n=== END FILE ===")
wave = int(sys.argv[1])
parts = [p for p in json.load(open(f"{ROOT}/spec/parts.json")) if p["id"][1:3] in WAVES[wave]]
tasks = []
core = api_index.core_digest()
def variants(p):
    """(module name, part text) for this part; halved when the full prompt would not fit under the limit."""
    ch = p["id"][1:3]; k = int(p["id"].split("p")[1]); same = [q for q in json.load(open(f"{ROOT}/spec/parts.json")) if q["id"][1:3] == ch]
    mod = SLUG[ch] + (str(k) if len(same) > 1 else "")
    text = open(f"{ROOT}/spec/parts/{p['id']}.md").read()
    uses_game = text.count("Game.") + text.count("GameState") > 2
    base = est_tokens(SYS, api_index.core_digest(), api_index.spine_digest(include_game=uses_game), api_index.relevant_index(text))
    if base + len(text) / 2.0 <= 31_000: return [(mod, text)]
    a, b = split_half(text); return [(mod + "a", a), (mod + "b", b)]
for p in parts:
    for mod, part_text in variants(p):
        fname = f"include/mordor/{mod}.h"
        if os.path.exists(f"{ROOT}/{fname}"): continue
        uses_game = part_text.count("Game.") + part_text.count("GameState") > 2
        spine = api_index.spine_digest(include_game=uses_game)
        user = (f"MODULE: {mod}  (file {fname}; SRD part {p['id']})\n\nFOUNDATION AND SHARED STATE (already written)\n{core}\n{spine}\n\nAPI INDEX (already declared elsewhere; do not redeclare)\n{api_index.relevant_index(part_text)}\n\nSRD PART\n" + part_text)
        est = est_tokens(SYS, user); print(p["id"], mod, "est", est)
        tasks.append({"id": mod, "system": SYS.replace("<module>", mod), "user": user, "allow": [fname], "save": f"spec/headers/{mod}.out"})
asyncio.run(run_all(tasks, f"headers{wave}", 4))
