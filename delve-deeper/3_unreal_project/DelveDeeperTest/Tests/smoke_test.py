# Headless smoke test for the DelveDeeperContent plugin (UE 5.8).
# Exercises each wrapper class once and writes results to Tests/smoke_result.txt.
import os
import traceback
import unreal

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "smoke_result.txt")
lines = []
fails = 0


def log(msg):
    lines.append(str(msg))
    unreal.log("[DDSMOKE] " + str(msg))


def check(name, ok, detail=""):
    global fails
    if not ok:
        fails += 1
    log(("PASS " if ok else "FAIL ") + name + (("  | " + str(detail)) if detail != "" else ""))


def section(name, fn):
    try:
        fn()
    except Exception:
        check(name + " raised", False, traceback.format_exc())


MAP = "floor1_map.json"


def test_map():
    r = unreal.DDMapLibrary.validate_floor(MAP)
    check("ValidateFloor", r is not None, "problems=%s" % (list(r) if r is not None else "returned false"))
    r = unreal.DDMapLibrary.load_floor(MAP)
    nodes, edges, err = r if r is not None else ([], [], "returned false")
    check("LoadFloor", r is not None and len(nodes) > 0 and len(edges) > 0, "nodes=%d edges=%d err=%s" % (len(nodes), len(edges), err))
    r = unreal.DDMapLibrary.find_reachable(MAP, "town_entrance", [], [], False)
    reach = list(r[0]) if r is not None else []
    check("FindReachable from town_entrance", r is not None and len(reach) > 0, "%d reachable: %s" % (len(reach), reach))
    r2 = unreal.DDMapLibrary.find_reachable(MAP, "town_entrance", ["e08"], [], False)
    reach2 = list(r2[0]) if r2 is not None else []
    check("FindReachable with e08 blocked excludes storeroom", r2 is not None and "storeroom" not in reach2 and "storeroom" in reach,
          "%d reachable" % len(reach2))


def test_floor_actor():
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    actor = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(
        unreal.DDFloorMapActor, unreal.Vector(0, 0, 0))
    check("Spawn DDFloorMapActor", actor is not None)
    log("  MapFile default = '%s', CellSize = %s" % (actor.get_editor_property("map_file"), actor.get_editor_property("cell_size")))
    actor.build_floor()
    pieces = actor.get_attached_actors()
    tags = {}
    for p in pieces:
        for t in p.tags:
            k = str(t).split(":")[0]
            tags[k] = tags.get(k, 0) + 1
    check("BuildFloor spawned pieces", len(pieces) > 0, "%d pieces, tag counts %s" % (len(pieces), tags))
    actor.clear_floor()
    check("ClearFloor removed pieces", len(actor.get_attached_actors()) == 0, len(actor.get_attached_actors()))


def test_puzzle():
    outer = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    pz = unreal.new_object(unreal.DDPuzzleComponent, outer)
    log("  PuzzleFile='%s' MapFile='%s'" % (pz.get_editor_property("puzzle_file"), pz.get_editor_property("map_file")))
    r = pz.load_puzzle()
    check("LoadPuzzle", r is not None and pz.is_loaded(), "out=%s" % (r,))
    log("  state=%s summary=%s" % (pz.get_state_name(), pz.get_summary()))
    # Wrong order first: should reset progress, no door.
    o1 = pz.use_control("ctl_cart")
    log("  use ctl_cart -> %s" % o1)
    check("Wrong control does not open door", not pz.is_door_open(), pz.get_state_name())
    for c in ["ctl_pick", "ctl_cart", "ctl_furnace"]:
        o = pz.use_control(c)
        log("  use %s -> %s" % (c, o))
    check("Correct order opens door", pz.is_door_open() and pz.get_opened_via() == "puzzle", "via=%s state=%s" % (pz.get_opened_via(), pz.get_state_name()))
    xp1 = pz.claim_xp()
    xp2 = pz.claim_xp()
    check("ClaimXp once only", xp1 != "" and xp2 == "", "first='%s' second='%s'" % (xp1, xp2))
    saved = pz.save_state_text()
    log("  saved state: %s" % saved)
    pz.reset_progress()
    r = pz.load_state_text(saved)
    check("Save/LoadStateText round trip", r is not None and pz.is_door_open(), "out=%s" % (r,))

    pz2 = unreal.new_object(unreal.DDPuzzleComponent, outer)
    pz2.load_puzzle()
    r = pz2.open_other("pick_lock")
    check("Bypass pick_lock opens door, no XP", pz2.is_door_open() and pz2.get_opened_via() == "pick_lock" and pz2.claim_xp() == "",
          "result=%s via=%s" % (r, pz2.get_opened_via()))


def test_quest():
    outer = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).spawn_actor_from_class(unreal.Actor, unreal.Vector(0, 0, 0))
    q = unreal.new_object(unreal.DDQuestComponent, outer)
    r = q.load_quest()
    check("LoadQuest", r is not None and q.is_loaded(), "out=%s" % (r,))
    stage = unreal.DDQuestComponent.stage_from_score(50)
    check("StageFromScore(50)", stage is not None, stage)
    for label, step in [
        ("Offer", lambda: q.offer("hero", 0, 1)),
        ("Accept", lambda: q.accept("hero", 0, 1)),
        ("Observe ore_lift", lambda: q.observe("hero", "ore_lift", "", 2)),
        ("Report sewer_grate", lambda: q.report("hero", "sewer_grate", 0, 3, 0.5)),
    ]:
        s = step()
        log("  %s -> %s" % (label, s))
    rec = q.get_record("hero")
    log("  record: %s" % rec)
    check("Quest reached completed", "completed" in str(rec).lower(), "")
    saved = q.save_state_text()
    q2 = unreal.new_object(unreal.DDQuestComponent, outer)
    q2.load_quest()
    r = q2.restore_state_text(saved)
    check("Quest Save/Restore round trip", r is not None and [getattr(q2.get_record("hero"), k) for k in ("state","offered","refused","chain","reward_rolled","reward_item")] == [getattr(rec, k) for k in ("state","offered","refused","chain","reward_rolled","reward_item")], "out=%s" % (r,))


section("map", test_map)
section("floor actor", test_floor_actor)
section("puzzle", test_puzzle)
section("quest", test_quest)
log("DONE: %d failure(s)" % fails)
with open(OUT, "w", encoding="utf-8") as f:
    f.write("\n".join(lines) + "\n")


