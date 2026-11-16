# Instructions for a Claude Code agent on the user's Windows computer

You are picking up Assignment 5 for **Delve Deeper** (Unreal Engine 5.8 C++ dungeon crawler, ELVTR course "Multi-Agent AI for
Game Development"). Read `README_ASSIGNMENT5.md` first. The assignment brief is `delve-deeper/docs/Assignment_5_Claude_Code_Handoff.md`
and the game design document is `delve-deeper/docs/07_Final_GDD.pdf` (text: `delve-deeper/1_pipeline/08_Final_GDD_Extracted.txt`).

## Where things stand

- The Assignment 5 agent (`delve-deeper/tools/assignment5/run_agent.py`) is built and tested (21 tests).
- It already ran in a Linux sandbox and **wrote the feature** (gold, the bandit boss's toll, the descent that wins the demo) in
  `delve-deeper/3_unreal_project/DelveDeeperTest/Source/DelveDeeperTest/`. Run logs: `delve-deeper/assignment5_runs/`.
- That sandbox had no Unreal Engine, so the **Unreal build and the playtest have never been run**. That is your job.

## What to do, in order

1. **Check the tools.** Run `git --version`, `py -3 --version`, `claude --version`, and check that
   `C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat` exists (set `UE_ROOT` if Unreal is elsewhere).
   Visual Studio 2022 with the C++ tools is also needed. Report anything missing to the user and stop; do not install programs
   without asking.
2. **Build.** From the repository root run `delve-deeper\tools\assignment5\build_ue.bat`. Save the output.
3. **If the build fails,** let the agent fix it. From the repository root run
   `py -3 delve-deeper/tools/assignment5/run_agent.py --build-cmd "tools\assignment5\build_ue.bat"`.
   It feeds the compile errors back to the model and stops after 2 attempts. Do not hand-edit the agent's code first.
4. **If the agent cannot fix it,** make the smallest manual fix yourself, in its own commit, and add one line to the
   "Manual changes" list in `README_ASSIGNMENT5.md` saying which file, what and why. Keep the agent's original change in git history.
5. **When it builds,** run the existing headless smoke test (it checks the earlier puzzle and quest still work):
   `"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<full path>\delve-deeper\3_unreal_project\DelveDeeperTest\DelveDeeperTest.uproject" -run=pythonscript -script="<full path>\delve-deeper\3_unreal_project\DelveDeeperTest\Tests\smoke_test.py" -nullrhi -unattended`
   then read `Tests\smoke_result.txt`. Its 17 checks do not cover the new boss, gold or descent behavior.
6. **Run the agent once more with the build,** to show it stops without writing and the build is verified (exit code 0):
   `py -3 delve-deeper/tools/assignment5/run_agent.py --build-cmd "tools\assignment5\build_ue.bat"`.
7. **Tell the user to play it** using the steps in `README_ASSIGNMENT5.md` ("See the feature in the game"). You cannot press keys
   in the game unless computer use is enabled. Say plainly that the playtest is not done until the user has done it.
8. **Update `README_ASSIGNMENT5.md`** "What is verified" table with the real results (build passed or failed, smoke test, second run
   exit code). Commit and push to the branch `claude/new-session-t9t87a`. If the push is refused for lack of GitHub sign-in, say so.

## Rules

- Report outcomes as they are. Never write that something built, passed or was played unless you ran it.
- Do not edit or commit anything under `Binaries/`, `Intermediate/` or `Saved/` (gitignored build output).
- The GDD is the source of truth. Where it says TBD, do not invent a design. The 100 gold toll is the Assignment 3 crew's
  EXPERIMENTAL value, not approved design.
- The user is literal and wants exactly what they asked. If a step is unclear or a program is missing, ask once and wait.
