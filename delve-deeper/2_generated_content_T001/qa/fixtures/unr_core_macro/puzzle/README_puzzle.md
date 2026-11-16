Version: 2a | Task: T001-s011-software (step 2: puzzle module) | Author: development-software-engineer | Status: done, awaiting QA review (no check marked passed by me) | Evidence: EXECUTED for build, ctest and purity checker (log paths below); everything Unreal NOT RUN - Unreal missing
Inputs read: brief s011; README_code.md; check_spec.md (Section 0, PZL-01..34, EQV); puzzle spec JSON, `puzzle_state_machine.py`, `test_puzzle.py`; `floor1_map.json`.

# Puzzle module (`dd_puzzle`) and `puzzle_check`

PROPOSAL until the user approves the C++ architecture. Only Use on the three controls is user-approved; everything else in the puzzle is the Systems Designer's PROPOSAL.

## Files
- `puzzle/include/dd/puzzle.h`, `puzzle/src/puzzle.cpp`: core library `dd_puzzle` (`add_core_library`, `-fno-exceptions -fno-rtti`, standard library only, text in and out). Links `dd_common` and `dd_map`.
- `cli/include/dd/puzzle_cli.h`, `cli/src/puzzle_cli.cpp` (`puzzleCheckMain`), `cli/src/puzzle_check_main.cpp` (`main`): NOT core. Built in `puzzle/CMakeLists.txt` (`dd_puzzle_cli`, `puzzle_check`); links `dd_cli`. `cli/CMakeLists.txt` untouched.
- `tests/puzzle/test_puzzle.cpp`, `tests/puzzle/expect_fail.cmake`, `tests/puzzle/fixtures/*.json`.

## API (namespace `dd::puzzle`)
- `Data` + `dataFromText(text, Data&, Issue&)`: parse and check the puzzle JSON. Codes: `JSON_*`, `SCHEMA`, `NO_CONTROLS`, `DUP_CONTROL`, `BAD_SOLUTION` (empty, or names a control not in `controls`), `MISSING_STRING` (a `str.use.*` key the machine can return is missing; step keys needed = solution length - 1, so length is data-driven).
- `validateAgainstMap(Data, map::FloorMap)` returns all issues, primary first: `UNKNOWN_NODE` (clue/controls site or room, opens_room), `PUZZLE_EDGE` (edge absent, not `locked`, no `puzzle` in its solutions, or not touching opens_room), `ROUTES_MISMATCH` (`other_routes` as a set differs from the e08 solutions minus `puzzle`), `PUZZLE_REF` (map `puzzle` block differs), `CONTROLS_MISMATCH` (map controls site list differs).
- `Puzzle::fromText(text, Puzzle&, Issue&)`; `Puzzle(Data)`.
- `UseOutcome use(control)`: `result` is `StepOk | WrongReset | Solved | Inert | UnknownControl`; `resultKey` is `step_ok | wrong_reset | solved | inert` (empty for unknown); `stringKey` is `str.use.step_<n> | str.use.wrong | str.use.solved | str.use.inert`. Unknown control returns `UnknownControl` and changes nothing (the reference raised ValueError; no exceptions here). Unknown is checked before inert, as in the reference.
- `BypassResult openOther(route)`: `Applied`, `NotApplied` (door already open: nothing changes), `UnknownRoute` (also for `puzzle`). A bypass clears progress and keeps `wrong_count`.
- `claimXp()` returns `xp_class` once after a puzzle solve, else empty; `reset()` clears partial progress only.
- Queries: `state()`, `stateName()`, `progress()`, `solved()`, `doorOpen()`, `openedVia()` ("" = none), `wrongCount()`, `xpAwarded()`, `summary()`.
- Save: `toJson()` gives compact JSON with the six reference fields (`progress, solved, door_open, opened_via, wrong_count, xp_awarded`). `loadState(text, Issue&)` replaces the state only if valid; on any error `*this` is unchanged. Codes: `JSON_*`; `SAVE_FIELD` (not an object, missing, unknown or wrong-type field; `wrong_count` must be an integer); `SAVE_VALUE` (unknown `opened_via`, unknown control, progress longer than the solution, negative `wrong_count`); `SAVE_STATE` (contradictions: `door_open` vs `opened_via`; `solved` vs `opened_via=puzzle`; solved without the full solution; bypassed with progress; unsolved progress that is not a prefix of the solution or is full; `xp_awarded` without a solve). Checks run in that order.
- The puzzle has no calendar input, so a day passing cannot change it (PZL-24 argued STATIC by the API plus EXECUTED save/load/reset loops).

## `puzzle_check` contract as built
Exit 0 accepted, 1 rejected (first stdout line `FAIL <CODE>: <detail>`; further validate problems as `  also ...`), 2 usage or unreadable file.
- `validate <puzzle> <map>`: prints `OK puzzle controls=3 solution=3`.
- `seq <puzzle> <control>...`: one line per press `<control> <result_key> <string_key>`, then the last line `state=... door_open=.. opened_via=.. wrong=.. progress=..`.
- `bypass <puzzle> <route> [<control>...]`: controls, then the route (`route <r> applied|not_applied`), then one press of the puzzle's first control (shows it is inert), then the state line (PZL-19 note).
- `roundtrip <puzzle> <control>...`: presses, saves, loads into a fresh puzzle, compares; prints `saved <json>` and the state line of the loaded puzzle; mismatch gives `FAIL ROUNDTRIP`.
- `load <puzzle> <state.json>`: prints `OK load` and the state line.

### Contract changes and choices (QA decides)
- P1. Test names: the brief asks for `pzl_*`; the spec names `puzzle_no_lockout`, `puzzle_second_open`, `puzzle_roundtrip_bypass`, `puzzle_persist_open`, `puzzle_xp_claim`. Both exist (the five `puzzle_*` names are aliases of the same cases). `-R puzzle_` therefore runs only those five, not a full group (PZL-32 wants "puzzle ctest group"; use `-R "pzl_|puzzle_"`).
- P2. Extra data codes (`NO_CONTROLS`, `DUP_CONTROL`, `MISSING_STRING`, `PUZZLE_REF`, `CONTROLS_MISMATCH`, `SCHEMA`) and a save rule for unknown extra fields (`SAVE_FIELD`): my choice, beyond the reference.
- P3. Save consistency rules (PZL-26, 27) go beyond the Python reference, which does not validate saves (spec says so).
- P4. PZL-19: the final press is the puzzle's first control, not hard-coded `ctl_pick`.
- P5. `bypass` with an unknown route fails before any control is pressed.
- Not implemented: PZL-29 (TBD, no check), PZL-30/31 (`check_text_guard.py`, a tool, not this step), PZL-34 and EQV-01..08 (Python and `eqv_check`, later steps), PZL-33 (QA mutation build).

## Spec lines (my reading; QA reviews)
PZL-01..05 `validate` plus fixtures in tests (`pzl_map_cross_*`, `pzl_exe_validate`, `pzl_exe_bad_edge`; the spec's FX files do not exist yet, mutated in memory instead). PZL-06..13, 15, 16: `pzl_cli_functions`, `pzl_oracle_sequences`, `pzl_exe_*`. PZL-14, 20, 23, 24, 28: the five named tests. PZL-17..19, 21: `pzl_cli_functions`, `pzl_bypass_*`. PZL-22: `pzl_roundtrip_prefixes` (all 121 sequences of length 0..4) and `pzl_exe_roundtrip`. PZL-25..27: `pzl_bad_state_text`, `pzl_state_fixtures`, `pzl_exe_bad_save_*`. EQV: the C++ agrees with the Python reference on the ported Python cases (`pzl_ref_python_cases`, `pzl_all_orderings`) and on a small oracle for all 121 sequences; this is not the golden-file comparison (EQV-05, later `eqv_check`).

## Build and result (EXECUTED, g++ 13.3, own build dir `work/T001/build_puzzle`)
1. `cmake -S work/T001/code -B work/T001/build_puzzle -DDD_MODULES="common;map;puzzle;cli"`
2. `cmake --build work/T001/build_puzzle`: no errors, no warnings.
3. `ctest --test-dir work/T001/build_puzzle --output-on-failure -R "pzl_|puzzle_|com_|map_" --output-log work/T001/code/logs/ctest_step2.log`: 100%, 78 of 78 passed (36 `pzl_*` of which 12 run the real `puzzle_check`, 5 `puzzle_*` aliases, 37 `com_*`/`map_*`). A first run failed `pzl_ref_python_cases` (a dangling temporary in my test); fixed and rerun.
4. `python3 -I work/T001/code/tools/check_no_unreal.py work/T001/code`: `10 core files scanned, 0 violation(s)`, PASS (no `quest/` yet at that time).
Not run: clang++; the test-sensitivity (mutation) check; the full top-level build with `quest`/`eqv`.

## NOT RUN - Unreal missing
No wrapper exists. Proposal: an Unreal component reads `storeroom_puzzle.json` and the save text, calls `Puzzle::fromText`, `use`, `openOther`, `toJson`/`loadState`; converts strings only at the boundary; the door e08 bypass hooks call `openOther("magic_key"|"pick_lock"|"break")`.

STATUS: done
OUTPUTS: work/T001/code/puzzle/ (CMakeLists.txt, include/dd/puzzle.h, src/puzzle.cpp, README_puzzle.md), work/T001/code/cli/src/puzzle_check_main.cpp, work/T001/code/cli/src/puzzle_cli.cpp, work/T001/code/cli/include/dd/puzzle_cli.h, work/T001/code/tests/puzzle/ (test_puzzle.cpp, expect_fail.cmake, fixtures/), work/T001/code/logs/ctest_step2.log
ISSUES:
1. QA: decide P1-P5; the spec's `pzl_*` vs `puzzle_*` naming (P1). `check_text_guard.py` (PZL-30/31) and the golden-file tools are not in this brief.
2. Producer: top-level full build (with `quest`, `eqv`) not tried; the quest run is parallel. `puzzle/CMakeLists.txt` references `${CMAKE_SOURCE_DIR}/cli/src/...`, so it assumes the top-level source dir is `work/T001/code`.
3. Producer: Unreal Engine missing; wrappers cannot be compiled (user supplies).
