Version: 3a | Task: T001-s012-software (build step 3: quest module) | Author: development-software-engineer | Status: done, awaiting QA review (no check marked passed by me) | Evidence: EXECUTED for build, ctest and purity checker (logs below); STATIC for the rest; Unreal parts NOT RUN - Unreal missing (none written)
Inputs read: brief s011-software-engineer-quest.md; qa/check_spec.md v2 (Section 0, 4, 6 quest lines); quest/figure_ore_lift_quest.json, figure_ore_lift_strings.csv (v3), validate_quest.py; map/floor1_map.json; code/README_code.md; common, map and cli headers; GDD (CLAUDE.md import).

# Quest module (`dd_quest`) for the Mysterious Figure ore-lift quest

PROPOSAL until the user approves the C++ architecture. Plain C++17, standard library only, built with `add_core_library` (`-fno-exceptions -fno-rtti`), takes text never paths. Nothing was compiled in Unreal.

## Files
- `quest/include/dd/quest.h`, `quest/src/quest.cpp`, `quest/CMakeLists.txt` (library, `quest_check`, `quest_tests`, ctest list).
- `cli/src/quest_check_main.cpp` (NEW file; `cli/CMakeLists.txt` untouched; the executable is defined in `quest/CMakeLists.txt` and links `dd_cli dd_quest dd_map dd_common`).
- `tests/quest/test_quest.cpp`, `tests/quest/fixtures/` (`q_bad_dest.json`, `q_cov_report_nothing.csv`, `save_good.json`, `save_bad_state.json`, `save_completed_no_chain.json`).

## API (`namespace dd::quest`)
- `load(questJson, stringsCsv, QuestData&, Issue&)`: parse and shape only (`JSON_*`, `CSV_*`, `SCHEMA`). `loadStrings(csv, Strings&, Issue&)` for the CSV alone.
- `validate(const QuestData&, const map::FloorMap&) -> vector<Issue>`: all violations, first is primary. Empty = accepted.
- `stageFromScore(score, stage&)`: bands of 20; false outside 0..100.
- `Strings::forStage(key, stage)`: row whose `stage_min..stage_max` holds the stage (null = no line). `fillLine(strings, key, stage, values, out, err)`: one-pass blank filling; a value `@key` inserts that key's own filled line (nested `{reward_line}`); values are never re-expanded; a missing value fails `UNFILLED_BLANK <name>`.
- `Engine(QuestData, FloorMap)`: per-character records. `offer`, `accept`, `decline`, `arrive(character, node, chain|"", ctx)`, `report(character, node, stage, ctx, RewardSource&)` each return a `Step` (`ok`, `fatal`, `refused`, `code`, `before/after` state, `lines` = key + filled text, `rewardRolled`). The caller passes the character's current standing as `stage` each time (standing is the caller's, not stored here). The transitions are read from the table in the quest JSON (`QuestData::next`).
- `RewardSource` (virtual `roll()`, 0 <= x < 1) and `RewardConfig {itemChance, items}`: injected. Default is no item. One roll per completion; the item index comes from the same roll (`x / chance * items`), so call count is exactly 1.
- `Engine::save()` / `restore(text, Issue&)`: JSON text; restore is all-or-nothing and rejects bad state (`SAVE_*`).

## Rules as built (Producer decisions)
- A NEW offer at -2 gives `fig_offer_refused`; the character is marked refused; `accept`/`decline` then return `REFUSED` (state stays `not_offered`). A fresh `offer` at a higher standing clears it.
- Offered at a higher standing, then standing -2: accept, decline, report and reward work (CSV has -2 lines).
- `accept` with no offer at all is allowed (spec QST-29) except at -2 (`REFUSED`).
- Report variant: cold -2..0, warm 1..2, read from the CSV ranges. `fig_report_nothing` is never chosen: its threshold is TBD (Systems), so an active quest always gives `fig_report_not_yet`.
- `{day}` comes from `Context.day`; `{node_name}` is the map's `name` of the destination node.
- The Figure never states a reason: banned-phrase check in `validate`.

## Validation codes (`validate`, order = check order)
`UNKNOWN_NODE`, `UNKNOWN_EDGE`, `DEST_MISMATCH`, `ROUTE_ENDS`, `ROUTE_EDGE`, `ROUTE_NOT_PLAIN`, `DUP_KEY`, `MISSING_KEY`, `ORPHAN_KEY`, `STAGE_COVERAGE`, `STAGE_RANGE`, `STAGE_OVERLAP`, `BAD_BLANK`, `BLANK_MISMATCH`, `BANNED_PHRASE`, `BAD_TRANSITION`, `STATE_TABLE`, `CAUSE_WORD`. Beyond `validate_quest.py`: route edge IDs must join consecutive route nodes and be plain, route ends, coverage and overlap rules, stray braces, banned words dwarf/dwarves/built/makers (QA list). Same on the real files: both accept (`qst_quest_python_reference` runs the Python script on them).

## Command line (`work/T001/build_quest/quest_check`; in the normal build `work/T001/build/`)
Exit 0 accepted, 1 rejected (stdout line `FAIL <CODE> <detail>`), 2 usage or I/O.
- `validate <quest.json> <strings.csv> <map.json>` -> `OK quest rows=25 route=5`; failure: first issue as `FAIL`, others `  also ...`.
- `stage <score>` -> the stage number; bad score `FAIL SCORE_RANGE`.
- `line <csv> <key> <stage>` -> row text; `FAIL NO_LINE` / `UNKNOWN_KEY` / `STAGE_RANGE`.
- `fill <csv> <key> <stage> name=value...` -> filled text; `reward_line=@fig_reward_item` nests.
- `run <quest> <csv> <map> <event>...`; events `offer@<s>`, `accept`, `decline`, `arrive@<node>[:present|gone]`, `report@<node>`, `char=<id>`, `stage=<n>`. One line per event: `<event> ok key=.. text=.. log=.. state=<s>` (then `  <key>: <filled text>` lines) or `<event> rejected <code> state=<s>`. `stage=` prints nothing and changes no quest state. Last line: `state=<s> chain=<present|gone|none> reward_rolled=<0|1> rolls=<n>`.
- `reward <seed|roll=x> [--chance c] [--items a,b]` -> `reward roll=.. key=fig_reward_item|none log=log_reward_..`; `FAIL ROLL_RANGE` outside [0,1).

## Contract changes (QA decides)
- Q1. Failure line is `FAIL <CODE> <detail>` with no colon (the spec writes `FAIL MISSING_KEY fig_reward_none`); `map_check` and `com_check` keep the colon.
- Q2. Extra `run` events: `day=<n>` (default 1), `roll=<x>` (next reward roll, default 0.999999 = no item). Needed to test `{day}` and the reward in `run`. `char=<id>` prints `char=<id>`.
- Q3. `quest_check reward` has no strings path, so it prints keys and the item, not text. Its default item chance 0.5 and item `Test_Item` are a COMMAND-LINE TEST PLACEHOLDER only; the real chance and list are TBD (Systems) and are injected by the caller. The same default is used by `run`.
- Q4. Rejections by rule (`refused`, `not_observing`, `wrong_node`, `no_transition`) are printed and exit 0; only bad input exits 1.
- Q5. `validate` checks more than the Python script (list above); `STAGE_COVERAGE` is checked before `STAGE_RANGE`.
- Q6. Test names are `qst_quest_<case>` so both `-R qst_` (brief) and the spec's `-R quest_roundtrip`, `quest_hostile`, `quest_fill_all`, `quest_reward_once`, `quest_load_invalid`, `quest_transition_table`, `quest_hostile_reward` match. The spec's `-R quest_` also matches all of them.

## Spec lines
QST-01, 03..16, 19..39, 41..50, 52, 54..70 implemented (command-line lines by `quest_check`; QST-44, 50, 54, 55, 65, 70 by ctest). Not dropped: QST-02, 17, 18 (Python and `check_text_guard.py`, not mine; `tools/` untouched), QST-40 (TBD, not testable), QST-51 and 56, 57 via `quest_check line` (QA runs each key), QST-53, 71 and EQV-14 (QA mutation builds), EQV-12 (QA runs both validators on fixtures; I did only the real files). QST-66's NOT TESTED case (offer at 0, drop to -2, accept) is built as the Producer decided and tested in `qst_quest_hostile_midquest`.

## Build and test (EXECUTED, g++ 13.3)
- `cmake -S work/T001/code -B work/T001/build_quest -DDD_MODULES="common;map;quest;cli"`; `cmake --build work/T001/build_quest`: no errors, no warnings.
- `ctest --test-dir work/T001/build_quest --output-on-failure -R "qst_|com_|map_" --output-log work/T001/code/logs/ctest_step3.log`: 100% passed, 63 of 63 (26 `qst_*` including `qst_quest_python_reference`, 18 `com_*`, 19 `map_*`).
- Two first-run failures were test mistakes (a shared counting source; a "bad" save that was valid); fixed, not library changes.
- `python3 -I work/T001/code/tools/check_no_unreal.py work/T001/code`: `10 core files scanned, 0 violation(s)` (RESULT PASS; includes `quest/`; no log kept).
- Not run: clang++ build, the full-tree build (other modules belong to the parallel step).

## Limits
- `quest_tests` uses `popen`/`WEXITSTATUS` (POSIX) to run `quest_check`; not for MSVC.
- Content notes: `log_quest_accepted` ends "({node_name})" and the map name is "Old ore lift (chain and drum)", so the log shows nested parentheses; `log_quest_done` reads "at the Old ore lift (chain and drum)". Narrative's call.
- The quest state `declined` is reachable without an offer; the `offered` flag is a record field, not a state.

## NOT RUN - Unreal missing
No wrapper exists. Proposal: the Unreal component reads the two files, calls `load`/`validate`, holds one `Engine`, passes the character's standing from the relationship system, supplies a `RewardSource` and `RewardConfig` from Systems data, and stores `Engine::save()` text inside the save game.

STATUS: done
OUTPUTS: see Files; logs/ctest_step3.log
ISSUES: see the brief reply.
