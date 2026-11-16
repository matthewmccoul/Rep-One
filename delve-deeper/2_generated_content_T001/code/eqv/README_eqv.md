Version: 4a | Task: T001-s013-software (step 4a: equivalence tools and text guard) | Author: development-software-engineer | Status: done, awaiting QA review (no check marked passed by me) | Evidence: EXECUTED for build and ctest (log: work/T001/code/logs/ctest_step4a.log); Unreal parts NOT RUN - Unreal missing (none here)
Inputs read: brief s013; README_code.md, puzzle/README_puzzle.md, quest/README_quest.md; check_spec.md v2 (Section 0, EQV-01..14, PZL-30/31); `puzzle_state_machine.py`; `storeroom_puzzle.json`; puzzle/quest headers and CMake files; GDD (CLAUDE.md import).

# Equivalence tooling (`eqv/`, `tools/`)

PROPOSAL until the user approves the C++ architecture. All of this is NOT core (ordinary flags, exceptions allowed); the core is untouched.

## Files
- `tools/gen_golden.py`: golden-vector generator (runs the Python reference).
- `eqv/src/eqv_check.cpp`, `eqv/CMakeLists.txt`: `eqv_check` executable (links `dd_cli`, `dd_puzzle`, `dd_map`, `dd_common`).
- `tools/check_text_guard.py`: carving-text guard (PZL-30, PZL-31).
- `tests/eqv/test_eqv_tools.py`: my own tests of all three, incl. broken inputs; run by ctest.
- EQV-13 (`quest_transition_table`): **already provided by the quest step**: `quest/CMakeLists.txt`, source `tests/quest/test_quest.cpp` (`TEST(quest_transition_table)`), ctest name `qst_quest_transition_table` (`-R quest_transition_table` matches it). Not duplicated.

## How to run (one command each, repo root)
1. `cmake -S work/T001/code -B work/T001/build_eqv -DDD_MODULES="common;map;puzzle;quest;eqv;cli"`
2. `cmake --build work/T001/build_eqv`
3. `ctest --test-dir work/T001/build_eqv --output-on-failure --output-log work/T001/code/logs/ctest_step4a.log`
Spec command lines (with the default build dir `work/T001/build` the same targets land there):
- EQV-02: `python3 -I work/T001/code/tools/gen_golden.py work/T001/qa/out/golden.json`
- EQV-03/04: add `--reference <path>`. Comparison of two files: `python3 -I work/T001/code/tools/gen_golden.py --diff A B` (exit 0 identical, 1 different, 2 unreadable).
- EQV-05..08: `work/T001/build/eqv_check <golden.json>`
- PZL-30/31: `python3 -I work/T001/code/tools/check_text_guard.py <puzzle.json>`

## Contract as built
**gen_golden.py** `<out.json> [--reference <py>]`. Exit 0 written (prints `OK <N> vectors (sequences=120 bypass=.. save_load=.. edge=..) written to <path>`); 1 reference unusable or generation failed; 2 usage or I/O error. On 1 or 2 no output file is written (written via `.part` then renamed, only after success).
Default reference: `work/T001/puzzle/puzzle_state_machine.py`.
Vectors (real run: 331): 120 `seq-NNN` (all control sequences of length 1..4 over the 3 controls, then `claim_xp` twice); `bypass-<state>-<route>` (each route from each distinct reachable state: 14 states x 3 routes = 42; then every route again, one more press, `claim_xp`); `saveload-NNN` (a save/load after every press of each of the 120 sequences) and `saveload-bypass-*`, `saveload-solved` (163 total); 6 `edge-*` (unknown control, unknown route, `puzzle` as route, resets). Each vector: `id`, `kind`, `ops` (inputs), `results` (per-op result and string key; `saved` snapshot for save_load), `expected` (the reference's final `to_dict()`). File header: `format`, `reference_sha256`, `controls`, `solution`, `routes`, `counts`. Output is deterministic (sorted keys).
**eqv_check** `<golden.json> [--puzzle <puzzle.json>]`. Replays every vector on a fresh C++ `Puzzle` built from the REAL `storeroom_puzzle.json` (path compiled in; not taken from the golden file, so a golden file from a changed reference fails). Compares each per-op result, the string key, the saved snapshot (C++ saves, loads into a fresh puzzle and continues), and the final state. Prints `MISMATCH <vector id>: <detail>` (first 25), then `N vectors, M mismatches`. Exit 0 if M=0; 1 on mismatch or bad file; 2 usage/I/O. Zero vectors: `FAIL NO_VECTORS`. Other: `FAIL JSON_*`, `FAIL GOLDEN_SHAPE`; a vector without ops/results/expected or id counts as a mismatch.
**check_text_guard.py** `<puzzle.json> [--words a,b,c]`. Scans every value under any key starting `str.` (anywhere in the document). Default words: dwarf, dwarves, builder, built, maker, makers, ancient, colonist, newcomer; case-insensitive, matched at a word start (so plurals such as `makers`, `dwarfs`, `newcomers` are caught; `Rebuilt` is not), after NFKC and removal of invisible format characters (soft hyphen, zero-width). Exit 0 clean; 1 `FAIL TEXT_LEAK: <key>: contains '<word>'`, `FAIL JSON_SYNTAX`, `FAIL JSON_DUP_KEY` (a repeated key could hide a leak), `FAIL BAD_STRING`, `FAIL NO_STRINGS` (nothing scanned); 2 usage or unreadable file.

**CSV mode (added in T001-s017, DEF-T001-3; QST-17, QST-18).** `check_text_guard.py <strings.csv>` (file name ends in `.csv`) scans the `text` column of every row, keyed by `key`, with the list above plus the reason phrases `because`, `in order to`, `so that`, `the reason`, `my master`, `my plan` (the spec's QST-16 list; the QST-18 fixture's leak is "because"). Exit 0 on the real `work/T001/quest/figure_ore_lift_strings.csv`; a leak exits 1 with `FAIL TEXT_LEAK: <key>: contains '<word>'` (the word is the list entry, so "makers" reports `maker`). A malformed file exits 1 with a different code, never TEXT_LEAK: `CSV_SYNTAX` (bad quoting, not UTF-8), `CSV_EMPTY`, `CSV_NO_COLUMN` (no `key` or `text`), `CSV_RAGGED`, `CSV_DUP_KEY`, `CSV_NO_ROWS`. JSON behaviour is unchanged. Tests: `eqv_guard_csv_real`, `eqv_guard_csv_leaks` (clean, leak in `fig_offer_cold`, each listed word, leak in a later row, key column not scanned, and each malformed case), `eqv_qst_text_guard_real` (QST-17 command line), `eqv_qst_text_guard_leak_fixture` (fixture `tests/quest/fixtures/q_leak_fig_offer_cold.csv`; passes when the guard fails naming `fig_offer_cold`).

## Contract changes and choices (QA decides)
- E1. `eqv_check` takes an optional `--puzzle` and otherwise reads the real puzzle file (spec gave one argument). Reason: EQV-07 needs the C++ to keep the real order while the golden file comes from a changed reference.
- E2. Golden format and vector ids are my choice (spec: inputs plus expected `to_dict()`, per-step result, string keys: all present).
- E3. If a reference copy has no `storeroom_puzzle.json` beside it (a QA copy in `fixtures/` may not), the generator passes the real data file in; the header records `data_source`. If the changed order is in the data file beside the copy, that file is used.
- E4. The Python reference raises `ValueError` on an unknown control or route; the C++ returns `UnknownControl`/`UnknownRoute`. The generator records `error` for those; `edge-*` vectors cover them (not a mismatch).
- E5. `gen_golden.py --diff` is mine (the spec allowed "or a QA diff").
- E6. Text guard also fails on duplicate keys, bad UTF-8, no `str.*` strings and non-string values (my choice, the spec only names the leak case). Word matching is prefix-at-word-start, a superset of the exact list.
- E7. ctest names: `eqv_gen_and_match`, `eqv_gen_missing_reference`, `eqv_gen_depends_on_reference`, `eqv_tampered`, `eqv_empty`, `eqv_changed_reference`, `eqv_bad_files`, `eqv_guard_real`, `eqv_guard_leaks` (my broken-input tests); `eqv_gen_golden` then `eqv_check_golden` (EQV-02, 05 as a user runs them); `eqv_pzl_text_guard_real` (PZL-30).

## Spec lines (my reading; QA reviews, none marked passed)
- EQV-01: unchanged reference test, QA runs it. EQV-02, 05: `eqv_gen_golden`, `eqv_check_golden`, `eqv_gen_and_match`. EQV-03: `eqv_gen_missing_reference` (also empty, syntax-error, no-class references). EQV-04: `eqv_gen_depends_on_reference` (two variants: changed data file; changed code with no data file beside it). EQV-06: `eqv_tampered` (door_open flipped on one vector; also a string key and a save snapshot). EQV-07: `eqv_changed_reference`. EQV-08: `eqv_empty` (`{"vectors":[]}` gives `FAIL NO_VECTORS`).
- EQV-09 to EQV-12: no new code; they use `map_check`, `quest_check` and the Python validators as built (not run by me in this step, QA's fixtures do not exist yet). EQV-13: existing `qst_quest_transition_table` (passed in my full run, 118/118). EQV-14: QA's mutation build.
- PZL-30: `eqv_pzl_text_guard_real`, `eqv_guard_real` (exit 0 on the real file). PZL-31: `eqv_guard_leaks` (builds the spec's one-edit copy, `str.carving.text` = "The dwarves built this mine.", expects exit 1 naming the key; plus each listed word, case, plural, invisible/full-width characters, nested key, non-`str.` key not scanned, missing/duplicate/broken input).

## Results (EXECUTED, g++ 13.3)
- Configure and build with the commands above: no errors, no warnings.
- Full ctest: 100% passed, 118 of 118 (103 earlier + 15 new `eqv_*`, incl. `unr_core_purity` and `unr_cmake_no_unreal` over the unchanged core). Log: `work/T001/code/logs/ctest_step4a.log`. Earlier first run: `eqv_tampered` failed because of a wrong selector in my test; fixed and rerun.
- Self-test of the generator: `python3 -I work/T001/code/tools/gen_golden.py work/T001/code/logs/golden_selftest.json` printed `OK 331 vectors (sequences=120 ...)`, exit 0.
- Not run: clang++ for this step; the QA fixture files (`work/T001/qa/fixtures/*`, do not exist yet); mutation checks.

## Limits
- Equivalence covers the puzzle state machine and its save/load only. Map and quest agreement (EQV-09..12) is by the existing CLI/validators, not golden vectors.
- `eqv_check` compares the C++ `toJson()` save only through its own reload plus the reference snapshot; it does not compare JSON text byte for byte (key order and spacing differ by design).
- The text guard is a word list: paraphrases ("those who dug the mine") pass; QA's list is applied as given.
- The eqv tests write under `<build>/tmp_tests/eqv/`.

## NOT RUN - Unreal missing
Nothing in this step touches Unreal.

STATUS: done
OUTPUTS: work/T001/code/eqv/ (CMakeLists.txt, src/eqv_check.cpp, README_eqv.md), work/T001/code/tools/gen_golden.py, work/T001/code/tools/check_text_guard.py, work/T001/code/tests/eqv/test_eqv_tools.py, work/T001/code/logs/ctest_step4a.log, work/T001/code/logs/golden_selftest.json
ISSUES:
1. QA: decide E1-E7; review EQV-02..08 and PZL-30/31 on your fixtures (they do not exist yet; EQV-03 names `eqv_no_reference.py`, EQV-04 a changed reference copy, which E3 handles in both forms).
2. QA/Producer: EQV-09..12 need no new code but are agreement checks across existing tools; EQV-10's Python-vs-C++ disagreements are for QA to record.
3. Producer: no shared file was changed. `eqv_check` links `dd_cli` through module order `...;eqv;cli`, which works because CMake resolves link names at generate time (verified by the build).
