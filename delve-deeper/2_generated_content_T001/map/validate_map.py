#!/usr/bin/env python3
"""Version 3 (revision 2 of 2). Validate floor1_map.json. Exit 0 = pass.
Usage: python3 validate_map.py            (real run: validates the default map, writes work/T001/map/validate_map.log)
       python3 validate_map.py --log PATH (validates the default map, writes the log to PATH)
       python3 validate_map.py MAP_PATH   (validates that file, prints only, writes NO log)"""
import json, sys
from collections import deque

DEFAULT_MAP = "work/T001/map/floor1_map.json"
DEFAULT_LOG = "work/T001/map/validate_map.log"
args = sys.argv[1:]
log_path = None
path = DEFAULT_MAP
if not args:
    log_path = DEFAULT_LOG
elif args[0] == "--log" and len(args) == 2:
    log_path = args[1]
elif len(args) == 1 and not args[0].startswith("--"):
    path = args[0]
else:
    print("usage: validate_map.py [MAP_PATH | --log LOG_PATH]")
    sys.exit(2)

LOG = open(log_path, "w") if log_path else None
def out(line):
    print(line)
    if LOG:
        LOG.write(line + "\n")
        LOG.flush()

d = json.load(open(path))
errs = []
nodes = {n["id"]: n for n in d["nodes"]}
if len(nodes) != len(d["nodes"]):
    errs.append("duplicate node ids")
eids = [e["id"] for e in d["edges"]]
if len(set(eids)) != len(eids):
    errs.append("duplicate edge ids")
for n in d["nodes"]:
    if not all(k in n for k in ("x", "y", "type", "area")):
        errs.append(f"node {n['id']} missing x/y/type/area")
    if n["area"] not in d["areas"]:
        errs.append(f"node {n['id']} unknown area")
    if "(" in n.get("name", "") or ")" in n.get("name", ""):
        errs.append(f"node {n['id']} name contains parentheses")
for r in d["reserved"]:
    if r not in nodes:
        errs.append(f"reserved node missing: {r}")
for e in d["edges"]:
    for k in ("from", "to"):
        if e[k] not in nodes:
            errs.append(f"edge {e['id']} endpoint missing: {e[k]}")
    if e["door"] not in d["legal_door_types"]:
        errs.append(f"edge {e['id']} illegal door {e['door']}")
    if e["door"] == "locked":
        for k in ("magic_key", "pick_lock", "break"):
            if k not in e.get("solutions", []):
                errs.append(f"locked edge {e['id']} missing solution {k}")
    if e["door"] == "stuck" and e.get("solutions") != ["break"]:
        errs.append(f"stuck edge {e['id']} solutions must be exactly [break]")

def reach(start, blocked_edges=(), blocked_nodes=(), allowed_doors=None):
    adj = {}
    # sites inside a room are reachable exactly when the room is
    for n in d["nodes"]:
        if "inside_room" in n:
            adj.setdefault(n["id"], []).append(n["inside_room"])
            adj.setdefault(n["inside_room"], []).append(n["id"])
    for e in d["edges"]:
        if e["id"] in blocked_edges: continue
        if allowed_doors and e["door"] not in allowed_doors: continue
        adj.setdefault(e["from"], []).append(e["to"])
        adj.setdefault(e["to"], []).append(e["from"])
    seen, q = {start}, deque([start])
    while q:
        u = q.popleft()
        for v in adj.get(u, []):
            if v not in seen and v not in blocked_nodes:
                seen.add(v); q.append(v)
    return seen

if not errs:
    full = reach("town_entrance")
    for t in ("boss_room", "descent", "ore_lift", "sewer_grate", "storeroom"):
        if t not in full: errs.append(f"{t} not reachable from town_entrance")
    # storeroom door closed: clue and controls still reachable
    closed = reach("town_entrance", blocked_edges={e["id"] for e in d["edges"] if "storeroom" in (e["from"], e["to"])})
    for t in ("puzzle_clue_site", "puzzle_controls_site"):
        if t not in closed: errs.append(f"{t} not reachable with storeroom closed")
    if "storeroom" in closed: errs.append("storeroom reachable while closed (bug)")
    # a route to the descent needing no lockpicking, search, or breaking
    plain = reach("town_entrance", allowed_doors={"plain"})
    for t in ("boss_room", "descent", "ore_lift", "puzzle_clue_site", "puzzle_controls_site"):
        if t not in plain: errs.append(f"{t} needs non-plain door")
    # descent only past boss_room
    if "descent" in reach("town_entrance", blocked_nodes={"boss_room"}):
        errs.append("descent reachable without boss_room")
    # the storeroom door must be the locked/stuck kind and puzzle opens an existing edge
    pe = {e["id"]: e for e in d["edges"]}.get(d["puzzle"]["opens_edge"])
    if not pe or pe["door"] not in ("locked", "stuck"):
        errs.append("puzzle opens_edge is not a locked/stuck edge")
    if pe and pe["id"] == "e08" and pe.get("solutions") != ["puzzle", "magic_key", "pick_lock", "break"]:
        errs.append("e08 solutions must be exactly [puzzle, magic_key, pick_lock, break]")
    if "magic_key" not in pe.get("solutions", []) if pe else True:
        errs.append("storeroom door has no magic_key solution")
    if d["quest_destinations"]["figure_task_destination"] != "ore_lift":
        errs.append("figure destination must be ore_lift")
    # GDD counts
    mt = {x.get("monster_type") for n in d["nodes"] for x in n.get("encounters", []) if x["kind"] == "monster"}
    if mt != set(d["monster_types"]) or len(mt) != 3: errs.append(f"monster types != 3: {mt}")
    traps = [t for n in d["nodes"] for t in n.get("traps", [])]
    if len(traps) != 3: errs.append(f"traps != 3: {len(traps)}")
    kinds = {x["kind"] for n in d["nodes"] for x in n.get("encounters", [])}
    for k in ("bandits", "boss"):
        if k not in kinds: errs.append(f"no {k} encounter slot")
    if sum(1 for n in d["nodes"] if n["type"] == "puzzle_clue") != 1: errs.append("puzzle clue count != 1")
    used = {e["door"] for e in d["edges"]}
    if used != set(d["legal_door_types"]): errs.append(f"door types used {sorted(used)}")
    # reachability notes
    for t in ("hidden_cache",):
        if t not in full: errs.append(f"{t} unreachable")

if errs:
    out("FAIL")
    for x in errs: out(" - " + x)
    sys.exit(1)
out(f"PASS: {len(nodes)} nodes, {len(d['edges'])} edges, 3 monster types, 3 traps, 1 puzzle, 4 door types, plain-only route to descent exists")
sys.exit(0)
