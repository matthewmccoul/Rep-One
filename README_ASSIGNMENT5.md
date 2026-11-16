# Assignment 5: goal-oriented coding agent for Delve Deeper

| Deliverable | Where |
|---|---|
| Agent source | [`delve-deeper/tools/assignment5/`](delve-deeper/tools/assignment5/): `run_agent.py` (runner and `priority_score()`), `prompts.md`, `scope.md`, `tests/` |
| Feature the agent built | The first-floor ending: gold, the bandit boss's toll, and the descent that wins the demo. Code in [`delve-deeper/3_unreal_project/DelveDeeperTest/Source/DelveDeeperTest/`](delve-deeper/3_unreal_project/DelveDeeperTest/Source/DelveDeeperTest/) (commit "Add the Assignment 5 feature written by the agent") |
| Run logs | [`delve-deeper/assignment5_runs/20261116-042545/`](delve-deeper/assignment5_runs/20261116-042545/RUN_LOG.md) (the run that wrote the feature), [`20261116-043146/`](delve-deeper/assignment5_runs/20261116-043146/RUN_LOG.md) (the second run, which found nothing to do) |

**Status, stated plainly:** the agent ran and wrote the feature. The Unreal build and the in-game playtest have **not**
been done yet. They need UE 5.8 on Windows, and the sandbox that ran the agent has no Unreal Engine. See "What is verified".

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
| Runner logic tests (`tests/test_agent.py`, 21 tests): score formula, dependency order, tie-break, stop on failed build, no rewrite of finished work, evidence checks | Pass |
| Starting scan, five steps, five rescans, with real model calls (11 calls, $2.24) | Done |
| Second run on the finished code | Found all four implemented, wrote nothing (code hash identical before and after), cost $0.24 |
| Static checks on every changed file (balanced braces, JSON and CSV parse, `.generated.h` last) | Pass |
| **Unreal build (UE 5.8)** | **Not run** |
| **Playing `DemoLevel`** | **Not run** |

The runner exits with code 3, "IMPLEMENTED, BUILD UNVERIFIED", when every requirement scans as implemented but no build ran.
It reports success (exit 0) only after a passing build. The agent also listed what it could not confirm without a build, in
`RUN_LOG.md` (for example whether `DisableInput` stops Enhanced Input movement, and whether the purse and boss sit where the
player's interact trace can hit them).

## Manual changes

**None to the feature code.** Every line under `Source/` and the two data files in the plugin's `Content/Data/` came from the
agent's five steps (`diff_01.patch` to `diff_05.patch`). If the Windows build or playtest needs a fix, it will be added below as
its own commit with the reason.

- _(none yet)_

## Run the agent

Needs Python 3.8+ and the `claude` CLI signed in. From the repository root:

```bash
python3 delve-deeper/tools/assignment5/run_agent.py
```

On Windows with UE 5.8, add the build so a compile error is fed back to the agent and success is only reported after a build:

```bash
py -3 delve-deeper/tools/assignment5/run_agent.py --build-cmd "tools\assignment5\build_ue.bat"
```

`build_ue.bat` wraps the UE 5.8 build (set `UE_ROOT` if Unreal is not in `C:\Program Files\Epic Games\UE_5.8`). It has **not been run yet**.
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
