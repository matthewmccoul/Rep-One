# Assignment 5: goal-oriented coding agent for Delve Deeper

| Deliverable | Where |
|---|---|
| Agent source | [`delve-deeper/tools/assignment5/`](delve-deeper/tools/assignment5/): `run_agent.py` (runner and `priority_score()`), `prompts.md`, `scope.md`, `tests/` |
| Feature the agent built | The first-floor ending: gold, the bandit boss's toll, and the descent that wins the demo. Code in [`delve-deeper/3_unreal_project/DelveDeeperTest/Source/DelveDeeperTest/`](delve-deeper/3_unreal_project/DelveDeeperTest/Source/DelveDeeperTest/) (commit "Add the Assignment 5 feature written by the agent") |
| Run logs | [`delve-deeper/assignment5_runs/20261116-042545/`](delve-deeper/assignment5_runs/20261116-042545/RUN_LOG.md) (the run that wrote the feature), [`20261116-043146/`](delve-deeper/assignment5_runs/20261116-043146/RUN_LOG.md) (the second run, which found nothing to do). On Windows: [`20261116-002713/`](delve-deeper/assignment5_runs/20261116-002713/RUN_LOG.md) (run with the build, exit 0), [`20261116-002643/`](delve-deeper/assignment5_runs/20261116-002643/RUN_LOG.md) (first attempt, failed on text encoding before any model call), [`windows_20261116/build_01.log`](delve-deeper/assignment5_runs/windows_20261116/build_01.log) (the standalone UE build) |

**Status, stated plainly:** the agent ran and wrote the feature. On Windows with UE 5.8.3 the feature **built** with no code
changes and the headless smoke test passed (17 of 17). The author then played `DemoLevel` and reported that it ran fine; the
individual steps were not written down. See "What is verified".

## What the agent did

The goal is milestone M1, the first-floor ending (`scope.md`). It is governed by GDD "Win" (lines 44-45), "Boss Encounter"
(385-390) and "Currency: Gold" (319). The Unreal demo already built the `toll_hall`, `boss_room` and `descent` rooms from the
map data but had no boss, gold or descent behaviour. The starting scan found all four scoped requirements **missing**.

| Step | Gap built | Score (3 x impact + 2 x urgency + unblocks) | Files |
|---|---|---|---|
| 1 | DD-GOLD-01 gold and a purse | 18, tied with DD-TOLL-01, so the lower ID won | 3 |
| 2 | DD-TOLL-01 boss and toll demand | 18 | 5 |
| 3 | DD-TOLL-01 again: the rescan found the pay prompt still missing | 18 | 1 |
| 4 | DD-TOLL-02 pay the toll | 18 (waited until gold and boss existed) | 2 |
| 5 | DD-DESC-01 descent gate and win | 15 (waited on DD-TOLL-02) | 4 |

**Why this feature.** Of four missing requirements only two were ready at the start, because the toll payment needs gold and a
boss, and the descent needs the payment. The runner never considered a blocked gap. `scores_01.json` and `RUN_LOG.md` list every
candidate, its inputs, the dependency waits and the reason for each choice.

**A weakness worth knowing.** At step 1 the model rated both ready gaps 3/3/3, so the tie was broken by requirement ID and not by
evidence. The ranking worked, but it did not discriminate between them here.

## What is verified, and what is not

| Check | Result |
|---|---|
| Runner logic tests (`tests/test_agent.py`, 21 tests): score formula, dependency order, tie-break, stop on failed build, no rewrite of finished work, evidence checks | Pass in the Linux sandbox. **On Windows 21 of 21 pass** (checked with `PYTHONUTF8` unset). As first run on Windows only 12 passed: the backslash-path bug in `run_agent.py` took it to 18, and making the tests' build commands work under `cmd.exe` took it to 21 (see "Manual changes"). Not re-run on Linux after those changes |
| Starting scan, five steps, five rescans, with real model calls (11 calls, $2.24) | Done |
| Second run on the finished code | Found all four implemented, wrote nothing (code hash identical before and after), cost $0.24 |
| Static checks on every changed file (balanced braces, JSON and CSV parse, `.generated.h` last) | Pass |
| Unreal build (UE 5.8.3, Windows, Visual Studio Build Tools 2022, MSVC 14.44) with `build_ue.bat` | **Pass.** `Result: Succeeded`, 29 compile and link steps, 0 errors, 172 s ([`build_01.log`](delve-deeper/assignment5_runs/windows_20261116/build_01.log)). No code was changed to make it build |
| Headless smoke test (17 checks: floor map, storeroom puzzle, quest) | **Pass.** 17 PASS, 0 FAIL, "DONE: 0 failure(s)" ([`smoke_result.txt`](delve-deeper/3_unreal_project/DelveDeeperTest/Tests/smoke_result.txt)). It does not cover the boss, gold or descent |
| Agent run on Windows with the build (`20261116-002713`) | Exit code 0, "SUCCESS". All four requirements scanned as implemented, nothing was written (`git status` showed no change to any source file), the build check passed (99 s), one model call, cost $0.34 |
| Playing `DemoLevel` | **Done by the author on 2026-11-16**, in the Unreal Editor on Windows (UE 5.8.3). The author reported that it ran fine. The result of each step below (locked descent, cannot pay, pay, win) was not written down, and no screenshot or recording was kept. Anyone can repeat the steps to check |

**Windows notes.** The first attempt at that run (`20261116-002643`) failed with exit code 1 before any model call ($0.00): the
runner sent its prompt to `claude` in the Windows default encoding (cp1252), which cannot encode the "●" character in the prompt.
The successful run (`20261116-002713`) got past it with `PYTHONUTF8=1` and no code change, and it was made before the runner fixes
below. It was not repeated afterwards. The runner was fixed after that (see "Manual changes"): the two `subprocess.run` calls use
UTF-8, `--claude-bin` keeps the backslashes of a Windows path, and console output no longer fails on a character outside cp1252.
Those fixes were checked on their own with `PYTHONUTF8` unset, using a stand-in `claude`, not with a real model call. `claude` was
not on the PATH on this machine, so the run used the Claude desktop app's bundled `claude.exe` through `--claude-bin`.

The runner exits with code 3, "IMPLEMENTED, BUILD UNVERIFIED", when every requirement scans as implemented but no build ran.
It reports success (exit 0) only after a passing build. The agent also listed what it could not confirm without a build, in
`RUN_LOG.md`. Reading the player code answers two of them on paper: interaction is by proximity (the nearest interactable in
range with a prompt, no line trace), and `DisableInput` on the pawn removes the pawn's Enhanced Input bindings, so movement
should stop at the win. Both still need confirming in play.

## Manual changes

**None to the feature code.** Every line under `Source/` and the two data files in the plugin's `Content/Data/` came from the
agent's five steps (`diff_01.patch` to `diff_05.patch`). If the Windows build or playtest needs a fix, it will be added below as
its own commit with the reason.

- `delve-deeper/tools/assignment5/run_agent.py` (commit `b7fb70e`): added `encoding="utf-8", errors="replace"` to the two `subprocess.run` calls (`call_claude`, `run_build`). On Windows they used cp1252 and the first run failed on the "●" in the prompt. This is runner code, not feature code.
- `delve-deeper/tools/assignment5/run_agent.py` (commit `2c95e1e`): new `split_command()` so `--claude-bin` keeps the backslashes of a Windows path (POSIX `shlex.split` had dropped them), and new `console_print()` so a run-log line with a character outside cp1252 no longer raises `UnicodeEncodeError` on a redirected console. Runner code, not feature code.
- `delve-deeper/tools/assignment5/tests/test_agent.py` (commit `92955ba`): the build commands the tests pass were POSIX shell strings (`true`, `echo '...'; exit 1`), which `cmd.exe` runs differently, so 3 tests failed on Windows. They now use `BUILD_OK` and `BUILD_FAIL`, which run the current Python. Test code, not feature code or runner code.

## Run the agent

Needs Python 3.8+ and the `claude` CLI signed in. From the repository root:

```bash
python3 delve-deeper/tools/assignment5/run_agent.py
```

On Windows with UE 5.8, add the build so a compile error is fed back to the agent and success is only reported after a build:

```bash
py -3 delve-deeper/tools/assignment5/run_agent.py --build-cmd "tools\assignment5\build_ue.bat"
```

`build_ue.bat` wraps the UE 5.8 build (set `UE_ROOT` if Unreal is not in `C:\Program Files\Epic Games\UE_5.8`). It ran on 2026-11-16 and succeeded.
If `claude` is not on your PATH, add `--claude-bin "<path to claude.exe>"` (Windows paths with backslashes work; quote the path if it has spaces).
Use `--scan-only` to scan and score without writing. Run the tests with `python3 delve-deeper/tools/assignment5/tests/test_agent.py`.

## See the feature in the game (needs Windows and UE 5.8)

1. Build: run the `--build-cmd` command above, or open `delve-deeper/3_unreal_project/DelveDeeperTest/DelveDeeperTest.uproject` and accept the rebuild prompt.
2. Open the project (it opens on `DemoLevel`) and press Play (Alt+P). Controls: WASD move, mouse look, **E** interact, **Q** alternate.
3. Boundary case, locked: walk to the `descent` (the dark block after the boss room) and press **E**. The demo must not end; the message log says the boss bars the way.
4. Cannot pay: at the boss (red pillar in `boss_room`), press **Q** with 0 gold. His "cannot pay" line shows, and gold stays at 0.
5. Pay: take the gold purse in the barracks with **E** (Gold: 100 shows in the top-left text), walk to the boss and press **Q**. Gold drops to 0, his "toll paid" line shows. A second **Q** does not charge again.
6. Unlocked: press **E** at the descent. The win banner shows and input stops.

Route from the start: entry hall, collapsed gallery, old ore lift, barracks (purse), toll hall, boss room, descent.

## Notes

- The 100 gold toll and purse are the Assignment 3 crew's `EXPERIMENTAL` values (`delve-deeper/inputs/crew_encounter/`), not approved GDD design.
- Unreal build output (`Binaries/`, `Intermediate/`) is gitignored and is rebuilt on the first build.
- The project and the GDD came from the Drive file "Assignment 4 Post Feedback revamp.zip".
