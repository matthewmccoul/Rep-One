# s011-software-engineer-puzzle: C++ puzzle module (owner: development-software-engineer)
Task ID T001-s011-software. Written by the Producer (the chat session). Build step 2 of 4 for the C++ package. A quest module is being built IN PARALLEL by another run: keep to your own files (rules below).

## Goal
The storeroom puzzle (dig, haul, smelt) as functional core code that an Unreal component can call: a state machine, its data loading, its save and load, and a command-line check tool, in the same style as `common` and `map` (read `work/T001/code/README_code.md` first: the core rules are binding).

## Inputs
- `work/T001/qa/check_spec.md`: Section 0 (conventions, contract), Section 3 (PZL) and any EQV lines that concern the puzzle. Implement every PZL line as written. If one cannot be implemented as written, say which and why: never drop one silently. You do not edit the spec. You do not mark checks as passing: QA reviews them on broken inputs later.
- The content: `work/T001/puzzle/storeroom_puzzle_spec.md`, `storeroom_puzzle.json`, and the Python reference `puzzle_state_machine.py` with `test_puzzle.py`. The C++ must behave exactly like the Python reference on every case in its tests. The user approved the new player action Use for the three controls ("Approve Use"); breaking e08 is a GDD door option and opens it as a bypass.
- `work/T001/map/floor1_map.json` for the cross-check (the puzzle's `opens_edge`, controls, clue and controls sites must exist in the map, and e08's solutions must equal `puzzle` plus the puzzle's `other_routes`).

## What to build
- `work/T001/code/puzzle/` : core library `dd_puzzle`, created with `add_core_library` (no exceptions, no RTTI, no Unreal includes; standard library only; it takes text, never file paths). Public header `puzzle/include/dd/puzzle.h`. API a wrapper can call: create from the puzzle JSON text, Use(control id) returning what happened (step accepted, wrong, solved, inert) and the string key to show, bypass route (`magic_key`, `pick_lock`, `break`), query state, serialise state to JSON text and restore it (rejecting bad state text with an error, never accepting a state that breaks the rules).
- `puzzle_check` executable per the spec's contract (`validate`, `seq`, `bypass`, `roundtrip`, `load`). Put its source in a NEW file `work/T001/code/cli/src/puzzle_check_main.cpp` (and any helper beside it). Do NOT edit `cli/CMakeLists.txt`: define the executable in `puzzle/CMakeLists.txt` and link `dd_cli` (it provides file reading), `dd_puzzle`, `dd_map` and `dd_common`.
- Tests: `work/T001/code/tests/puzzle/` (new files), ctest tests named `pzl_*`, defined in `puzzle/CMakeLists.txt`. They must include: all orderings of the three controls, repeats, reset on wrong, solve, the three bypass routes, bypass after progress, solve after bypass does nothing, save and load mid-sequence and after solve, and rejection of bad state text. Add your own small fixtures under `work/T001/code/tests/puzzle/fixtures/`.
- `work/T001/code/puzzle/README_puzzle.md`: the API, the contract as built, any contract changes with reasons, what is NOT RUN. Do NOT edit `README_code.md`.

## Files you must NOT edit (shared with the parallel quest step)
Top-level `CMakeLists.txt`, `cli/CMakeLists.txt`, `common/`, `map/`, `tests/testing.h`, `tools/`, `README_code.md`. If you need a change in any of them, list it in ISSUES instead of making it.

## How to build and run (use your OWN build directory)
- Configure: `cmake -S work/T001/code -B work/T001/build_puzzle -DDD_MODULES="common;map;puzzle;cli"` (this builds only your modules plus what they need, so the other run's unfinished files cannot break you).
- Build: `cmake --build work/T001/build_puzzle`.
- Test: `ctest --test-dir work/T001/build_puzzle --output-on-failure -R "pzl_|com_|map_" --output-log work/T001/code/logs/ctest_step2.log` (ctest writes the log; no shell redirects; nothing typed by hand). To exercise the command line, add ctest tests that run `puzzle_check`; you cannot run executables from your build directory by hand.
- One simple command at a time: no `cd`, `;`, `&&`, pipes.

## Acceptance
- Builds clean with no warnings; the `pzl_*` tests and the existing `com_*` and `map_*` tests pass in your build directory (report the real counts).
- The C++ agrees with the Python reference on the sequences it tests.
- `puzzle_check` follows the contract; on bad input it exits 1 with a line starting `FAIL`.
- No Unreal includes, no exceptions, no RTTI in `puzzle/` (the purity checker: `python3 -I work/T001/code/tools/check_no_unreal.py work/T001/code` must report no violation in `puzzle/`; violations in `quest/` may be the other run's unfinished work).

## Stop limit
One pass, with a self-check loop on your own test failures. Write only under `work/T001/code/` (the paths named above) and your build directory. End with STATUS, OUTPUTS, ISSUES. Evidence: EXECUTED (with log path); NOT RUN for anything needing Unreal.
