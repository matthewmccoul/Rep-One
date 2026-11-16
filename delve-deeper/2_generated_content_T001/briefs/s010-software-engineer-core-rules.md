# s010-software-engineer-core-rules: make the core Unreal-compatible (owner: development-software-engineer)
Task ID T001-s010-software. Written by the Producer (the chat session). This is step 1b of the C++ package. Step 1 (your earlier work: common + map) passed my own build and its 35 ctest tests. Do this rework BEFORE the puzzle and quest modules are built on top of it.

## Why
Unreal modules compile with C++ exceptions and RTTI turned OFF by default. Your step-1 core uses exceptions internally (`throw`/`try`/`catch` in `common/src/json.cpp` and `map/src/map.cpp`). Code like that would not compile in a default Unreal module. You flagged this yourself in your ISSUES; the Producer's decision is: the core must not use them.

## What to change (core = `work/T001/code/common/` and `work/T001/code/map/`)
1. **No exceptions, no RTTI in core code.** No `throw`, `try`, `catch`, `dynamic_cast`, `typeid`. Report errors as return values: for example a result type with `ok`, an error code and message (and the line number where the CSV or JSON position matters). The `FAIL <CODE>: <detail>` lines of the command-line contract stay exactly as they are.
2. **Compile the core with `-fno-exceptions -fno-rtti`.** Put this in one CMake helper in the top-level `CMakeLists.txt` (for example `add_core_library(name sources...)`) that applies the flags and the include rules to a core library. Use it for `dd_common` and `dd_map`, and document in `README_code.md` that every later core module (puzzle, quest) must be created with it. Test programs and the command-line executables may compile normally, but must link against the core as it is.
3. **The core takes in-memory text, not file paths.** The parse and load functions take the file's contents (a `std::string`) and return a result; reading a file from disk happens only in the small `fileio` helper used by the command-line tools and tests. (Reason: Unreal will read files with its own APIs and hand the text to the core.) If your step-1 code already does this, keep it and say so; if not, change it.
4. **A purity checker.** Write `work/T001/code/tools/check_no_unreal.py` as the QA spec describes under UNR (read `work/T001/qa/check_spec.md`, UNR section and its Section 0 notes): it scans the core directories and exits 1 with a list of violations if it finds an Unreal include or Unreal macro. Also make it reject the forbidden constructs from item 1 in core code. Run it on the core and report its real output. Write its log with `--log` from inside the script, or have ctest capture it (a ctest test named `unr_core_purity` that runs it); no shell redirects.
5. **Everything else stays**: the command-line contract and the 35 existing tests (rewrite the ones that expected an exception to expect the returned error). Add a test for each error path you converted.

## Run and report
Configure and build from scratch with `cmake -S work/T001/code -B work/T001/build` and `cmake --build work/T001/build`, then `ctest --test-dir work/T001/build --output-on-failure --output-log work/T001/code/logs/ctest_step1b.log` (ctest writes the log; nothing typed by hand). Also prove the flags are really on: show that the core library sources compile with `-fno-exceptions -fno-rtti` (for example by reading `compile_commands`-style output from `cmake --build ... --verbose` for one core file, in your report).
You may run the built executables directly, for example `work/T001/build/map_check validate work/T001/map/floor1_map.json`; they are allowed.

## Acceptance
- Builds clean; every ctest test passes; the checker reports no violations on the core; the CMake helper exists and is used.
- No `throw`, `try`, `catch`, `dynamic_cast` or `typeid` in `common/` or `map/` core code (the checker proves it).
- Behaviour on the real files is unchanged (`map_check validate` still prints `OK map nodes=17 edges=15`).
- `README_code.md` updated: the new rules, the helper, the purity checker, the contract changes still standing from step 1.

## Stop limit
One pass, with a self-check loop on your own failures. Write only under `work/T001/code/`. End with STATUS, OUTPUTS, ISSUES. Evidence: EXECUTED (with the log path); NOT RUN for anything needing Unreal.
