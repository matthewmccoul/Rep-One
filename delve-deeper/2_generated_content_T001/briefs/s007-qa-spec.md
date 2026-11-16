# s007-qa-spec: check spec for the C++ package (owner: development-qa-tester)
Task ID T001-s007-qa. Written by the Producer (the chat session). You write the SPEC only. You do not write scripts or code: the Software Engineer writes them from your spec, then you review them on broken inputs (a later dispatch).

## What is being built (so you can specify the checks)
The user asked for the three content pieces (map, puzzle, Mysterious Figure quest) as functional code that runs inside Unreal Engine. Unreal is not available here, so the package is built like this (the user approved C++ loaders with JSON/CSV data in the starting brief):
- `work/T001/code/` is a CMake project in plain C++17 with the standard library only. No Unreal includes anywhere in `core` code. `ctest` runs its tests.
  - `common/`: a small strict JSON reader and CSV reader.
  - `map/`: loads `work/T001/map/floor1_map.json`; validates it (unique IDs, legal door types, door rules, reachability, the route to the descent only through the boss room, the puzzle's clue and controls reachable while e08 is closed); answers queries (neighbours, door type and ways through, reachability with chosen edges blocked).
  - `puzzle/`: the storeroom puzzle state machine as in `work/T001/puzzle/storeroom_puzzle_spec.md` and `puzzle_state_machine.py` (the Python file is the reference): Use on the three controls, wrong order resets, solve opens e08, the magic key, pick and break routes bypass it, save and load of the state.
  - `quest/`: the Figure quest as in `work/T001/quest/figure_ore_lift_quest.md` and `.json`: stage bands, offer, accept or decline, observing the chain at the ore lift, reporting back, reward chosen through an injectable random source, dialogue chosen from `figure_ore_lift_strings.csv` with blanks filled from game state, and all IDs checked against the map.
  - `unreal/`: thin Unreal wrapper source (UCLASS / UActorComponent style classes that call the core). These cannot be compiled here; they are checked only statically.
  - `tools/`: helper scripts (for example a golden-vector generator that runs the Python reference and writes expected results, and a script that checks the core contains no Unreal includes).

## What to read
GDD sections that bind these pieces (`08_Final_GDD_Extracted.txt`): Encounter Choices and Outcomes (Doors, Traps, Puzzle, The faction), Boss Encounter, Watchers and Town Characters (Mysterious Figure), Assessment Consequences (score bands), Loot (magic key). The request: pipeline_state/tasks/T001/request.md. The content: everything under work/T001/map/, work/T001/puzzle/, work/T001/quest/. The critic's round-2 report: work/T001/critic/critic_report_r2.md. Open defects: work/T001/defects/.

## What to write: work/T001/qa/check_spec.md (only this file)
One line per check, in this fixed form:
`id | what it checks | one simple command, no cd | input | expected exit code and output`
- Group the ids by module: `COM-`, `MAP-`, `PZL-`, `QST-`, `UNR-` (static checks of the Unreal wrapper), `EQV-` (the C++ gives the same results as the Python reference on shared test vectors).
- Commands must be simple and runnable here with no cd: for example `cmake -S work/T001/code -B work/T001/build`, `cmake --build work/T001/build`, `ctest --test-dir work/T001/build --output-on-failure -R <name>`, or `python3 -I work/T001/code/tools/<script>.py`.
- For EVERY test, loader and script that will be written, include at least one broken-input check that must FAIL (or be rejected): for example a map JSON where e08 lacks break, where the descent is reachable without the boss room, with a duplicate ID, or with an illegal door type; a puzzle sequence with a wrong order; a quest CSV with a missing key or an unknown map ID. A checker that cannot fail is a defect.
- Expected results come from the content and the GDD. Where the content or GDD says TBD, specify the check as "TBD, not testable" and do not invent a number.
- Mark checks that cannot be run here (anything that needs Unreal to compile or run) as NOT RUN with the reason; never as passing.
- End the spec with a short "What this spec does not cover" list and a coverage table: each acceptance point of the Producer's packet (work/T001/packets/s001-packet.md, section 5) against the check ids that cover it.

## Acceptance for your spec
- Every check line is in the fixed form; every module has broken-input checks; the coverage table has no empty row except items marked NOT RUN with a reason.
- You wrote no scripts and edited no other file.

## Stop limit
One pass. End with STATUS, OUTPUTS, ISSUES. Evidence label: STATIC.
