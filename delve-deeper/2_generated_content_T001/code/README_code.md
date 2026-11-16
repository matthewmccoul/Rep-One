Version: 1b | Task: T001-s010-software (step 1b: make the core Unreal-compatible) | Author: development-software-engineer | Status: done, awaiting QA review (not marked passed by me) | Evidence: EXECUTED for build/ctest/checker (logs below); STATIC for everything else; Unreal parts NOT RUN - Unreal missing (none written yet)
Inputs read: brief s010; work/T001/qa/check_spec.md (UNR section); step-1 code; GDD (CLAUDE.md import).

# C++ package, step 1b: exception-free core (common + map)

PROPOSAL until the user approves it: plain C++17 core (standard library only, no Unreal includes), tested with ctest here; thin Unreal wrappers come later. Nothing here was compiled or run inside Unreal.

## Core rules (new in 1b)
1. **No exceptions, no RTTI in core code**: no `throw`, `try`, `catch`, `dynamic_cast`, `typeid`. Errors are return values (`bool` plus an `Error`/`Issue` struct with code, message, and line/column for JSON and CSV). The JSON parser sets a `failed_` flag (first error wins) and every function returns at once; the map loader (`Loader` in `map.cpp`) records the first schema problem and later steps run on harmlessly with empty defaults.
2. **Every core module is built with `add_core_library(name sources...)`** (top-level `CMakeLists.txt`). It creates a static library, sets `-fno-exceptions -fno-rtti` (MSVC: `/EHs-c- /GR-`) as PRIVATE compile options, and sets the include rule (`<module>/include`, headers included as `"dd/<header>.h"`). **The later core modules (`puzzle`, `quest`, and any other) must be created with it** and live in a folder named in `check_no_unreal.py` (`CORE_DIRS`: common, map, puzzle, quest). Used by `dd_common` and `dd_map`.
3. **The core takes text, not paths.** `json::parse(text, ...)`, `csv::parse(text, ...)`, `map::fromText(text, ...)`, `map::fromJson(value, ...)` already did this in step 1 and are unchanged. Reading files is only in `cli/` (`readFile`), used by the command-line tools and the tests. In Unreal the engine reads the file and hands the text over.
4. **Not-core code** (tests, `cli/`) compiles with ordinary flags and links the core libraries as built.

## Layout
- `CMakeLists.txt`: C++17, `-Wall -Wextra`, `add_core_library`, `enable_testing()`, executables land in `work/T001/build/`. Adds `common`, `map`, `puzzle`, `quest`, `eqv`, `cli` (in that order) if the folder has its own `CMakeLists.txt`. Also registers ctest tests `unr_core_purity` and `unr_cmake_no_unreal`.
- `common/` (`dd_common`, core): `json.h/.cpp`, `csv.h/.cpp`.
- `map/` (`dd_map`, core): `map.h`, `map.cpp`.
- `cli/` (`dd_cli`, NOT core): `fileio.h/.cpp` (`readFile`, `comCheckMain`), `map_cli.h`, `map_cli.cpp` (`mapCheckMain`), `com_check` and `map_check` executables. **Moved here in 1b** so the core folders hold no file I/O or iostream code; `mapCheckMain` is now declared in `dd/map_cli.h` (was in `dd/map.h`).
- `tools/check_no_unreal.py`: purity checker (below).
- `tests/`: `testing.h`, `common/test_common.cpp`, `map/test_map.cpp`.
- `logs/`: `ctest_step1.log` (step 1), `ctest_step1b.log` (written by ctest), `check_no_unreal.log` (written by the checker's `--log`).

## Purity checker
`python3 -I work/T001/code/tools/check_no_unreal.py <code dir> [--log FILE]` scans `common/ map/ puzzle/ quest/` (those that exist) for: Unreal includes (CoreMinimal.h, Engine.h, GameFramework/..., UObject/..., Runtime/..., `U/A/F/T`-style headers), Unreal markers (`UCLASS(`, `USTRUCT(`, `UPROPERTY(`, `UFUNCTION(`, `GENERATED_BODY`, `FString`, `FName`, `TArray`, `TMap`, `TSet`, `UE_LOG`, `TEXT(`, ...), and the forbidden constructs `throw`, `try`, `catch`, `dynamic_cast`, `typeid` (comments and string literals are blanked first). Exit 0 clean; 1 violations or 0 files scanned; 2 usage. `--cmake <code dir>` fails if a CMake file names an `unreal/` source (UNR-06, UNR-07). Not built yet: `check_unreal_wrapper.py` (UNR-08 to UNR-10; wrappers do not exist).
Checker self-test (EXECUTED in a scratch dir, no log kept): a file with `CoreMinimal.h`, `<GameFramework/Actor.h>`, `UPROPERTY()`, `FString`, and `throw/try/catch/dynamic_cast/typeid` gave 9 violations, exit 1; words inside a comment and a string were not flagged; a nonexistent directory gave `0 files scanned`, exit 1.

## Build and test (from the repo root, one command each)
1. `cmake -S work/T001/code -B work/T001/build`
2. `cmake --build work/T001/build`
3. `ctest --test-dir work/T001/build --output-on-failure --output-log work/T001/code/logs/ctest_step1b.log`

Result (EXECUTED, g++ 13.3): no errors, no warnings; ctest 100% passed, 39 of 39 (35 old + `com_json_error_paths`, `map_schema_errors`, `unr_core_purity`, `unr_cmake_no_unreal`). The same source also built clean with clang++ 18.1 in a scratch build dir (build only; tests not re-run there).
Flags proof: `flags.make` for `dd_common` and `dd_map` shows `-std=c++17 -Wall -Wextra -fno-exceptions -fno-rtti`; `dd_cli` shows no such flags. Verbose compile of a core file (`cmake --build work/T001/build --verbose --target dd_map -- -B`): `/usr/bin/c++ -I.../map/include -I.../common/include -std=c++17 -Wall -Wextra -fno-exceptions -fno-rtti ... -c .../map/src/map.cpp`.
`work/T001/build/map_check validate work/T001/map/floor1_map.json` prints `OK map nodes=17 edges=15`.

## Tests for converted error paths
- `com_json_error_paths`: every JSON failure code (EMPTY, TRAILING_CONTENT, TRUNCATED in several places, TOO_DEEP, SYNTAX in several places, NUMBER_RANGE, CONTROL_CHAR, BAD_ESCAPE x4, TRAILING_COMMA x2, DUP_KEY, BAD_UTF8) with the line where it matters, and a check that a failed parse leaves the caller's value untouched.
- `map_schema_errors`: missing fields, wrong types, wrong top-level type, parse error code passed through `fromText`.
- The old tests never expected an exception (they used the returned codes), so none needed rewriting.

## Command-line contract (unchanged from step 1)
Exit 0 accepted, 1 rejected (first stdout line `FAIL <CODE>: <detail>`), 2 usage or I/O error.
- `com_check json <file>` prints `OK json`; `com_check csv <file> [--require a,b]` prints `OK csv columns=<n> rows=<n>` (real strings file: `columns=6 rows=25`).
- `map_check validate <map.json>` prints `OK map nodes=17 edges=15`; failure: first problem as `FAIL`, others as `  also <CODE>: <detail>`.
- `map_check neighbours <map> <node>`, `door <map> <edge>`, `ways <map> <edge>`, `reach <map> <from> <to> [blocked_edge ...]` as in step 1.

### Contract changes still standing from step 1 (QA decides)
- C1. `com_check csv` requires header column `text` by default; `--require ""` turns it off.
- C2. Output shapes are my choice (spec left them open).
- C3. Malformed JSON or wrong-shape JSON given to `map_check` exits 1 with `FAIL JSON_*` / `FAIL SCHEMA`.
- Extra codes: `JSON_*`, `CSV_*`, `SCHEMA`, `LEGAL_DOOR_LIST`, `MONSTER_UNUSED`, `NO_BANDIT_ENCOUNTER`, `NO_BOSS_ENCOUNTER`, `NODE_TYPE_COUNT`, `NO_OPEN_ROUTE`, `DOOR_TYPES_USED`.
- New in 1b: the `mapCheckMain` header moved (see Layout). The CLI behaviour is unchanged.

## Spec lines (state as in step 1, plus)
- UNR-01 and UNR-06 implemented (checker and `--cmake` mode, run on the real tree: pass). UNR-02 to UNR-05 and UNR-07 are QA's fixture runs; the checker is built to the spec for them (include in header/cpp/inl, `<...>` and spaced forms, markers, empty dir fails, CMake leak). UNR-13 (both compilers, warnings on) built clean; QA repeats it. UNR-08 to UNR-10 need the wrapper checker (later step). UNR-11, UNR-12: NOT RUN - Unreal missing.
- COM, MAP lines: as in step 1 (COM-01..13, MAP-01, 03..10, 12..39 implemented).

## Limits and known gaps
- Same as step 1 (numbers, BOM, hand-written ctest case lists: a new test case must be added to `common/CMakeLists.txt` or `map/CMakeLists.txt`).
- The ctest case `unr_core_purity` writes `logs/check_no_unreal.log` into the source tree.
- The checker is text-based (regex), not a parser: a construct split across lines or hidden by macros could slip past it.

## NOT RUN - Unreal missing
No wrapper exists; nothing compiled against Unreal headers or run in the editor. Proposal for later: put `common/` and `map/` sources into the Unreal module via `Build.cs`; the wrapper reads files with Unreal APIs and calls `fromText`; convert `std::string`/`std::vector` to `FString`/`TArray` only at the boundary.

STATUS: done
OUTPUTS: work/T001/code/CMakeLists.txt, work/T001/code/common/, work/T001/code/map/, work/T001/code/cli/, work/T001/code/tools/check_no_unreal.py, work/T001/code/tests/, work/T001/code/README_code.md, work/T001/code/logs/ctest_step1b.log, work/T001/code/logs/check_no_unreal.log
ISSUES:
1. Producer: five files moved to `cli/` could not be deleted by me (mv/rm need approval). Old paths are now one-line comment stubs, not in any build: `common/src/fileio.cpp`, `common/src/com_check_main.cpp`, `common/include/dd/fileio.h`, `map/src/map_check_main.cpp`, `map/src/map_cli.cpp`. Please delete them. The stale `common/include/dd/fileio.h` stub is on the core include path; `dd_cli` is listed first in the tests' link lines so the real header wins.
2. QA: review UNR-01..07 on your fixtures; decide contract changes C1-C3.
3. Producer: `check_unreal_wrapper.py` and the puzzle/quest modules are later steps (they must use `add_core_library`).
4. Producer: Unreal Engine missing; wrappers cannot be compiled (user supplies).
