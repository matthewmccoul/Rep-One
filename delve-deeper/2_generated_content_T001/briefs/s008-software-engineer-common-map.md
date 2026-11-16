# s008-software-engineer-common-map: C++ skeleton, common readers, map module (owner: development-software-engineer)
Task ID T001-s008-software. Written by the Producer (the chat session). This is build step 1 of 4 for the C++ package (1: skeleton + common + map, 2: puzzle, 3: quest, 4: Unreal wrappers + tools). You build ONLY step 1.

## Goal
The user asked for the three content pieces as functional code that can run inside Unreal Engine. Unreal is not available here. So the package is a CMake project in plain C++17, standard library only, with NO Unreal includes in any core code, tested here with ctest. Thin Unreal wrapper source comes in step 4, not now.

## What QA specified (you implement it as written)
Read `work/T001/qa/check_spec.md` fully, especially Section 0 (conventions and the command-line contract), Section 1 (COM, the common readers) and Section 2 (MAP). Implement every COM and MAP line as written. If a line cannot be implemented as written, say which and why in your note: never drop one silently. If you change the command-line contract, record each change and the reason in `work/T001/code/README_code.md`. You do not edit the spec. You do not mark your checks as passing: QA reviews them later on broken inputs.

## Layout to create under work/T001/code/
- `CMakeLists.txt` (top level): C++17, `-Wall -Wextra`, `enable_testing()`, and `add_subdirectory(<module>)` for `common`, `map`, `puzzle`, `quest`, `eqv` ONLY IF that module's own `CMakeLists.txt` exists (later steps add the others). All executables must land directly in the build directory (`work/T001/build/`), so the spec's command paths such as `work/T001/build/map_check` work: set `RUNTIME_OUTPUT_DIRECTORY` to `${CMAKE_BINARY_DIR}`.
- `common/`: a strict JSON reader (reject trailing commas, duplicate keys in one object, comments, single quotes, NaN, truncated input, empty input) and a strict CSV reader (quoted commas, doubled quotes, ragged rows rejected with the line number, unterminated quote rejected, optional required-header check). Library target `dd_common`, executable `com_check`, ctest tests named `com_*`.
- `map/`: loads `work/T001/map/floor1_map.json` through `dd_common`; validates it as the spec says (unique IDs, legal door types, the door rules: locked edges list `magic_key`, `pick_lock` and `break`; stuck edges list exactly `break`; reachability; the descent reachable only through `boss_room`; the puzzle's clue and controls reachable while e08 is closed; counts); answers queries (neighbours, door, ways through, reachability with chosen edges blocked). Library `dd_map`, executable `map_check`, ctest tests named `map_*`. The Python `work/T001/map/validate_map.py` is the reference: your validator must agree with it on the real file and on every broken file in the spec.
- `tests/`: unit tests and any small fixtures YOU need, under `work/T001/code/tests/` (QA makes its own broken fixtures under `work/T001/qa/fixtures/`).
- `README_code.md`: layout, how to build and test, the command-line contract as built, what is NOT RUN (anything needing Unreal), and the Unreal-integration notes you will want in step 4.

## Rules
- Core code uses the standard library only and includes nothing from Unreal. Keep data (IDs, door rules, stages) in the JSON/CSV files, not in code, except fixed GDD rules (the four legal door types, the door rules).
- Never claim something ran that did not. Run it: `cmake -S work/T001/code -B work/T001/build`, `cmake --build work/T001/build`, then `ctest --test-dir work/T001/build --output-on-failure --output-log work/T001/code/logs/ctest_step1.log` (ctest writes that log itself; do not use shell redirects and do not type any log by hand). Report the real counts.
- No shell `cd`, no `;`, no `&&`, no pipes: one simple command at a time.
- Write only under `work/T001/code/` (the build directory is `work/T001/build/`).

## Acceptance
- The project configures and builds from scratch with no errors; `ctest` runs all `com_*` and `map_*` tests; report passes and failures honestly.
- `com_check` and `map_check` exist with the contract in the spec; on the real files they accept; on malformed input they exit 1 and print a line starting `FAIL`.
- No Unreal includes in `common/` or `map/`.
- `README_code.md` is complete.

## Stop limit
One pass, with a self-check loop on your own test failures. Stop and report a blocker if something cannot be done. End with STATUS, OUTPUTS, ISSUES. Evidence: EXECUTED (with the log path) for what ran; NOT RUN for anything needing Unreal.
