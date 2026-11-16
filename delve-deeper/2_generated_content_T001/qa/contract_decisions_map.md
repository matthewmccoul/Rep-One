Version: 1 | Task: T001-s015-qa2 | Author: development-qa-tester | Status: done | Inputs read: brief s015-qa-map.md, check_spec.md (0, 2, 6, 7, 8), code/README_code.md, code/map/src/map.cpp (validate), validate_map.py, floor1_map.json | Evidence: EXECUTED (logs in work/T001/qa/out/) and STATIC

# Contract decisions: MAP

| # | Choice (source) | Decision | Reason |
|---|---|---|---|
| M1 | Output shapes `OK map nodes=17 edges=15`, `FAIL <CODE>: <detail>` then `  also <CODE>: ...` lines (README C2) | Accept | Spec lines allow any stable shape starting `FAIL`; the first line carried the expected code on all 20 broken fixtures. |
| M2 | Malformed or wrong-shape JSON to `map_check` exits 1 with `FAIL JSON_*` / `FAIL SCHEMA` (README C3) | Accept | Matches the exit-code contract (1 = input rejected). Covered by ctest `map_cli_failures` and `map_schema_errors`, which passed (MAP-36). |
| M3 | Extra codes `LEGAL_DOOR_LIST`, `MONSTER_UNUSED`, `NO_BANDIT_ENCOUNTER`, `NO_BOSS_ENCOUNTER`, `NODE_TYPE_COUNT`, `NO_OPEN_ROUTE`, `DOOR_TYPES_USED` | Accept | Each rests on a GDD count or role (three monster types, four door types, one boss, bandits, one entrance/descent) or mirrors `validate_map.py`. None contradicts the spec. Not individually broken-tested here (no spec line); only MONSTER_UNUSED, NO_OPEN_ROUTE and DOOR_TYPES_USED appeared as side effects in MAP-14, 20, 21. |
| M4 | A plain-door-only route to the boss, descent and quest/puzzle sites is a hard validation rule (as in `validate_map.py`; spec MAP-13) | Accept, flagged | The spec requires it. GDD lets a magic key open a lock, so a plain-only route is a design choice of the reference validator, not a GDD rule. Design finding for the Systems Designer or World Designer if it should be relaxed; not a defect. |
| M5 | `reach` and `neighbours` treat every door type as passable and ignore secret doors (spec MAP-32 expects e09 on the long route) | Accept | Matches the spec and the Python reference. |
| M6 | MAP-11 (plain door carrying a solutions list) has no check | Accept | TBD in GDD/content; not testable. Recorded as not-run so it is not dropped. |
| M7 | `map_check validate` is stricter than `validate_map.py` on SECRET_BASE and CELL_CLASH | Accept | The spec (MAP-10, MAP-39) requires both. The Python reference has no such rule, so EQV-10 shows two disagreements (listed in results_map.csv). Not a C++ defect; the reference lacks them. |

No choice rejected. No defect filed from the MAP part. DEF-T001-2 status updated to verified (MAP-07, MAP-27a).

## Review notes (STATIC)
- `validate()` in map.cpp checks each rule on its own: door rules per edge, puzzle edge, counts, reachability via `reachable`/`canReach` with block options, cell clash with the `inside_room` exemption. It does what each spec line says. Each rule failed on its broken fixture, so none is lenient. The mutation run (MAP-37) shows the tests detect a broken reachability.
- Stale stub files listed in README ISSUES 1 (old paths under common/ and map/) are not part of this build; not tested.
- Fixture edits are one each, listed in results_map.csv notes; MAP-15 also trips CELL_CLASH and NO_OPEN_ROUTE as secondary codes (side effects of the single edit; the first code is the expected one).
