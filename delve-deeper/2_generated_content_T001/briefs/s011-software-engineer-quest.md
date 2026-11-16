# s011-software-engineer-quest: C++ quest module (owner: development-software-engineer)
Task ID T001-s012-software. Written by the Producer (the chat session). Build step 3 of 4 for the C++ package. A puzzle module is being built IN PARALLEL by another run: keep to your own files (rules below).

## Goal
The Mysterious Figure quest as functional core code an Unreal component can call: the quest state machine, the dialogue selection from the strings file by the Figure's standing, blank filling from game state, the reward roll with an injectable random source, data loading with every ID checked against the map, and a command-line check tool, in the same style as `common` and `map` (read `work/T001/code/README_code.md` first: the core rules are binding).

## Inputs
- `work/T001/qa/check_spec.md` (version 2): Section 0 (conventions, contract, including the `stage=<n>` event and the `FAIL STAGE_COVERAGE` rule), Section 4 (QST) and any EQV lines that concern the quest. Implement every QST line as written. If one cannot be implemented as written, say which and why: never drop one silently. You do not edit the spec. You do not mark checks as passing: QA reviews them on broken inputs later.
- The content: `work/T001/quest/figure_ore_lift_quest.md`, `figure_ore_lift_quest.json`, `figure_ore_lift_strings.csv` (version 3: a hostile Figure has lines mid-quest), `validate_quest.py` (Python reference for the validation).
- `work/T001/map/floor1_map.json` for the ID cross-check (giver node, destination node, route nodes and edges).
- Producer decisions you must follow: a NEW offer at standing -2 is refused (`fig_offer_refused`, accept impossible); if the quest was offered at a higher standing and the standing then falls to -2, accept, decline, report and reward lines still work (the CSV has lines for them at -2). `fig_reward_item` may occur at -2; the chance and item list are TBD in the content: take them as injectable data, invent nothing. The Figure never states his reason.

## What to build
- `work/T001/code/quest/` : core library `dd_quest`, created with `add_core_library` (no exceptions, no RTTI, no Unreal includes; standard library only; it takes text, never file paths). Public header `quest/include/dd/quest.h`. API a wrapper can call: load from the quest JSON text and the strings CSV text; `stageFromScore(0..100)`; offer, accept, decline, observe the chain at the destination (present or gone), report; the line key and filled text to show; reward roll through an interface the caller supplies (a fixed roll in tests); serialise and restore quest state as JSON text (rejecting bad state with an error).
- `quest_check` executable per the spec's contract (`validate`, `stage`, `line`, `fill`, `run`, `reward`). Put its source in a NEW file `work/T001/code/cli/src/quest_check_main.cpp`. Do NOT edit `cli/CMakeLists.txt`: define the executable in `quest/CMakeLists.txt` and link `dd_cli` (file reading), `dd_quest`, `dd_map` and `dd_common`.
- Tests: `work/T001/code/tests/quest/` (new files), ctest tests named `qst_*`, defined in `quest/CMakeLists.txt`. They must cover every offer, accept, decline and report variant, all five standing bands, blank filling (all rows fillable, a missing value rejected), the mid-quest drop to hostile (offer at 0, accept, standing -2, report completes), the refused offer at -2, the reward roll with fixed rolls, save and load, ID cross-checks against the map (including broken ones: an unknown node, an unknown key), and the coverage rule (a CSV copy where one mid-quest key has `stage_min` -1 is rejected). Add your own small fixtures under `work/T001/code/tests/quest/fixtures/`.
- `work/T001/code/quest/README_quest.md`: the API, the contract as built, contract changes with reasons, what is NOT RUN. Do NOT edit `README_code.md`.

## Files you must NOT edit (shared with the parallel puzzle step)
Top-level `CMakeLists.txt`, `cli/CMakeLists.txt`, `common/`, `map/`, `tests/testing.h`, `tools/`, `README_code.md`. If you need a change in any of them, list it in ISSUES instead of making it.

## How to build and run (use your OWN build directory)
- Configure: `cmake -S work/T001/code -B work/T001/build_quest -DDD_MODULES="common;map;quest;cli"` (this builds only your modules plus what they need, so the other run's unfinished files cannot break you).
- Build: `cmake --build work/T001/build_quest`.
- Test: `ctest --test-dir work/T001/build_quest --output-on-failure -R "qst_|com_|map_" --output-log work/T001/code/logs/ctest_step3.log` (ctest writes the log; no shell redirects; nothing typed by hand). To exercise the command line, add ctest tests that run `quest_check`; you cannot run executables from your build directory by hand.
- One simple command at a time: no `cd`, `;`, `&&`, pipes.

## Acceptance
- Builds clean with no warnings; the `qst_*` tests and the existing `com_*` and `map_*` tests pass in your build directory (report the real counts).
- Dialogue selection and blank filling agree with the CSV on every row; the validation agrees with `validate_quest.py` on the real files.
- `quest_check` follows the contract; on bad input it exits 1 with a line starting `FAIL`.
- No Unreal includes, no exceptions, no RTTI in `quest/` (the purity checker: `python3 -I work/T001/code/tools/check_no_unreal.py work/T001/code` must report no violation in `quest/`; violations in `puzzle/` may be the other run's unfinished work).

## Stop limit
One pass, with a self-check loop on your own test failures. Write only under `work/T001/code/` (the paths named above) and your build directory. End with STATUS, OUTPUTS, ISSUES. Evidence: EXECUTED (with log path); NOT RUN for anything needing Unreal.
