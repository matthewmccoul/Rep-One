# Mordor rules core and console demo (C++17)

A playable demo of the Mordor: The Depths of Dejenol rules, built from the SRD by an ant army (see CODING_REPORT.md). Requires g++ or clang++ 13+, CMake 3.16+, Ninja (or make).

    cmake -S . -B build -G Ninja && ninja -C build           # about 90 s
    build/mordor_console --quick --seed 3                    # four ready-made characters; type `help`
    build/mordor_console                                     # create your own party
    build/mordor_console --bot 400 --seed 5                  # autopilot (integration test)
    build/mordor_tests                                       # SRD-derived tests + game flow tests (1,655 tests)
    build/mordor_engine_tests                                # acceptance tests translated from the reference implementation (314)

Quick tour: `descend`, `forward`, `left`, `right`, `map`, `fight` (when monsters appear), `status`, `char 1`, `stairs` (back up), `rest`, `shop`, `buy <row> <member>`, `bank deposit <gold>`, `guild`, `levelup <member>`, `save`, `load`. Test aid: `cheat xp <member> <amount>`. Members are numbered 1 to 4.

Layout: `include/mordor/` public headers (foundation, `state_*.h`, one header per rules module, `game.h`), `src/` implementations, `tests/`, `apps/console/`, `data/combat-data.json`, `tools/` (the ant pipeline scripts), `logs/` (every ant call and every test run), `NOTES.md` (working state), `UNREAL_PORTING.md`.
