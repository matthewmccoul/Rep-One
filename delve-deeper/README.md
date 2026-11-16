# Delve Deeper: Assignment #4, Dynamic Content Pipeline (post-feedback revamp)

Capstone game: **Delve Deeper**, a dark pulp fantasy dungeon crawler RPG for PC/Unreal. It is set in an ancient dwarven mine under a frontier town (GDD, `1_pipeline/08_Final_GDD_Extracted.txt`).

This resubmission answers the feedback "Get this into Unreal". The three generated content pieces now run as code in **Unreal Engine 5.8**. You can walk the floor, solve the puzzle and complete the quest in a playable level. The pipeline that generated them is included and runs.

## What is in this folder

| Folder | What it is |
|---|---|
| `1_pipeline/` | The content pipeline. `run_pipeline.py` walks the workflow diagram (`03_DEVELOPMENT_WORKFLOW.mmd`), and a Producer agent dispatches role agents (World, Systems and Narrative Designer, Critic, Software Engineer, QA) through `orchestration/invoke_agent.sh`. It covers budget, critic and QA loops, and quote checking. Every agent is given the GDD (`08_Final_GDD_Extracted.txt`) as its source. |
| `2_generated_content_T001/` | Everything pipeline task T001 produced: the 3 content pieces (`map/`, `puzzle/`, `quest/`), their validators, the C++ game-code core (`code/`), 3 critic reports (`critic/`), 4 defect reports (`defects/`), the QA spec and results with tool-made logs (`qa/`), each agent's brief (`briefs/`), and the Producer's decision log (`producer_notes.md`). |
| `3_unreal_project/DelveDeeperTest/` | A UE 5.8 C++ project. It has the `DelveDeeperContent` plugin (Unreal wrappers around the T001 C++ core and data) and a playable demo level (`Content/Maps/DemoLevel`). `TEST_REPORT.md` lists the 5 fixes the plugin needed to compile in UE 5.8. |
| `4_evidence/` | Proof that it runs: the pipeline test output (49/49), the Unreal build log, the Unreal smoke-test result (17/17 checks), and a log of the demo level starting. |

## The three generated outputs

1. **Floor 1 map**: `2_generated_content_T001/map/floor1_map.json` plus `floor1_layout.md`. It has 17 places and 15 doors: the town entrance, the sewer grate, entry hall, mess hall, foreman's office, slag passage, smelter hall, storeroom, hidden cache, collapsed gallery, old ore lift, barracks, toll hall, the boss room and the descent. Doors are plain, locked, secret or stuck, as the GDD describes. There are encounter, trap and loot slots.
2. **Storeroom puzzle**: `2_generated_content_T001/puzzle/storeroom_puzzle.json` plus its spec. A carving in the foreman's office reads "Three tools keep the mine alive…". The pick rack, ore cart and furnace in the smelter hall must be used in that order to open the locked storeroom door (e08). A wrong order resets progress without harm. Following the GDD's locked-door rule, the door can also be picked, opened by magic or broken.
3. **Mysterious Figure quest**: `2_generated_content_T001/quest/figure_ore_lift_quest.json`, the dialogue `figure_ore_lift_strings.csv` and `figure_ore_lift_quest.md`. The GDD's Mysterious Figure, glowing eyes behind a sewer grate, sends you to check whether the chain is still on the drum of the old ore lift. You observe it on arrival and report back. The Figure's lines change with your standing with them.

## Does it sound like our game?

Yes, because the pipeline makes it source everything from the GDD and then checks that:
- Every agent brief gives the GDD as the source of truth, and the Critic must quote it. The final critic round had **37 of 37 quotes verified** against the GDD text by `orchestration/verify_quotes.py` (`critic/critic_report_r3.md`).
- The content uses the GDD's own setting and rules. It's a dwarven mine (GDD line 36). The Mysterious Figure is "a pair of glowing eyes behind the bars of a sewer drain" (GDD line 355). Locked doors "must be picked, unlocked with magic, or broken" (GDD line 292). The bandit boss demands a toll at the descent (GDD lines 385–386).
- The tone is the GDD's terse dark pulp. A sample Figure line: *"Eyes flare behind the bars. A green finger points. You. The old ore lift. Is the chain still on the drum? Go. Look. Come back and say."*
- Where the GDD leaves things undecided, the content says "TBD" instead of making things up: XP amounts, toll amounts, trap effects and reward chance.

## What the critic agent caught

**Round 1: verdict CONTRADICTIONS** (`critic/critic_report.md`)
- **BLOCKING:** locked door e08 had no "break" option, although the GDD gives one to every locked door. Filed as DEF-T001-2, fixed in the map and puzzle, and verified by QA (MAP-27a, MAP-07).
- **MAJOR:** the puzzle needed a "Use" action the GDD doesn't list. Filed as DEF-T001-1 and decided by me as designer ("Approve Use"), for the three puzzle controls only.
- **MINOR:**
  - the sewer grate's location was described two ways
  - a Figure line asserted a cause the quest data leaves open
  - the toll was placed in a different room from the boss
  - puzzle XP was proposed although the GDD doesn't name puzzles as an XP source

**Round 2: verdict CONSISTENT**, with 22 of 22 quotes verified and 4 minor wording or status items, which were cleaned up.

**Round 3: verdict CONSISTENT**, with 37 of 37 quotes verified. Three minor items are left and listed in `producer_notes.md`:
- a declined player hears "I asked nothing of you"
- "magic" on locked doors is carried as a key item only
- some stale labels in file headers

**The QA agent** also caught two bugs in its own checking tools (DEF-T001-3, DEF-T001-4). Both were fixed and re-checked, with 17 of 17 re-check rows passing.
- The text-guard couldn't read the quest CSV.
- A transition test compared the table with itself, so it could never fail.

The QA spec has 215 result rows: 207 pass, the 3 failures were fixed and re-checked, and 5 rows weren't run. Of those 5, 2 needed Unreal and are now covered by section 4 below, and 3 are TBD in the GDD.

**Unreal integration (this revamp):** compiling the plugin in UE 5.8 found 5 problems the pipeline couldn't see without Unreal, all now fixed (see `3_unreal_project/DelveDeeperTest/TEST_REPORT.md`):
1. C++17 is no longer allowed in UE 5.8.
2. An engine-version pin made UE 5.8 skip the plugin.
3. A local variable name clashed with an engine member.
4. A memory-ownership pattern in the components wouldn't compile.
5. Build Floor gave no visible feedback.

## How to run it

### 1. Pipeline: `1_pipeline/`
- **Offline test, no cost, no Claude account.** Needs Linux or WSL with Python 3.8+ and PyYAML.
  ```
  python3 tests/test_pipeline.py
  ```
  It runs 49 checks of the pipeline logic, using a bundled stand-in for Claude Code (`tests/fake_claude/claude`). Result: `4_evidence/pipeline_tests_49_of_49.txt`.
- **Real run.** Open the folder in Claude Code and run:
  ```
  python3 run_pipeline.py
  ```
  This needs the `claude` CLI on PATH. `python3 run_pipeline.py --check` checks the setup first. Task T001 was run with the chat session acting as the Producer, as recorded in `2_generated_content_T001/producer_notes.md`.

### 2. Generated content and C++ core: `2_generated_content_T001/`
- Python validators and tests: `map/validate_map.py`, `puzzle/test_puzzle.py` and `quest/validate_quest.py`, with their run logs next to them.
- To build and test the C++ core:
  ```
  cmake -S code -B build
  cmake --build build
  ctest --test-dir build
  ```
  The cloud run built it clean with g++ and clang++, and the final fresh build passed 123 of 123 tests; see `code/README_code.md` and `qa/`. The command-line tools `puzzle_check` and `quest_check` play the puzzle and quest in a terminal.

### 3. Unreal: `3_unreal_project/DelveDeeperTest/`
- Needs **Unreal Engine 5.8** on Windows. Prebuilt Win64 editor binaries are included. If UE asks to rebuild, you'll need Visual Studio 2022 C++ tools.
- **To play:**
  1. Open `DelveDeeperTest.uproject`. It opens on `DemoLevel`.
  2. Press **Play** (Alt+P) and click in the view.
- **Controls:** WASD to move, mouse to look, Space to jump, **E** to interact, **Q** for the alternate action.
- **Route:**
  1. Talk to the Mysterious Figure at the sewer grate, next to the start, and accept the task.
  2. Walk through the entry hall and collapsed gallery to the old ore lift (grey corner pillars and a rust-brown drum).
  3. Go back to the Figure and report.
  4. Read the carving in the foreman's office.
  5. In the smelter hall, use the pick rack, then the ore cart, then the furnace. The red block sealing the storeroom disappears.
  6. Alternative: press Q at the storeroom door to pick the lock (the bypass route, which gives no XP).
- **What's on screen:** the top left shows the live puzzle and quest state. The bottom left shows the content's own lines, straight from the T001 data files.
- **Headless test:**
  ```
  UnrealEditor-Cmd.exe DelveDeeperTest.uproject -run=pythonscript -script=<full path>/Tests/smoke_test.py -nullrhi -unattended
  ```
  It writes `Tests/smoke_result.txt`.

## What runs where (honest status)

| Item | Status |
|---|---|
| Pipeline logic | **Ran here:** 49/49 offline checks pass (`4_evidence/`). Real agent runs happened in the cloud for task T001 (30 agent runs, 17.99 USD; `producer_notes.md`). |
| Map, puzzle, quest data and the C++ core | Validated and tested by the pipeline's QA in the cloud; see `qa/` results and logs. |
| Unreal plugin and demo level | **Ran here:** builds clean in UE 5.8.3, and the 17/17 smoke checks pass. The demo level was played through by hand: the floor, the puzzle solve and the quest (offer, accept, observe, report) all work. The smoke test also covers the bypass. |
| Not covered | Packaging a standalone build, and the GDD's TBD numbers (XP, toll amounts, trap effects, reward chance). |
