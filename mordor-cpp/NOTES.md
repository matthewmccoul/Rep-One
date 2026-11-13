# Mordor C++ work: canon, plan, state (read first after any reset)

## Canon (stated by the user, 2026-11-13)
- Target: the C++ must operate mechanically like the REAL 1995 Mordor (shareware demo, run here under Wine).
- Design source: OpenDejenol's Mordor.Core (C#). Where Mordor.Core and the real game agree and the C++ differs, the C++ is changed to match.
- Where Mordor.Core and the real game disagree in testing: do not choose; put the case in a report for the user.
- Rules that cannot be tested in the real game: report as "Core result only, real game untested" (never counted as agreeing).
- Output must stay Unreal-portable C++ (no exceptions/RTTI/IO in the core; see UNREAL_PORTING.md in the C++ tree).

## Two forks
1. PRUNE fork (now): the uploaded mordor-cpp-demo, duplicates trimmed to one copy each, Mordor.Core + real game as references. Tree: prune/code (git, one commit per group).
2. TRANSLATION fork (after pruning): a NEW, SEPARATE C++ translation of Mordor.Core, one C++ function per Core method, verified against Core by the differential harness and against the real game where testable. It does not replace or modify the pruned version, which is kept as its own version permanently.
- Prune scope: duplicates only. Unique functions stay, including the ones the game never runs.

## Tools in this scratchpad
- prune/: inventory, piles, evidence ants, plan.json, surgery.py, gate.py (build exit status + both suites + 4 golden transcripts).
- harness/: differential harness (C# runner on Mordor.Core + C++ runner on libmordor_core.a; scripted RNG on both sides; renames + shims for same-name/file-local copies).
- real/: real Mordor under Wine (prefix/drive_c/MORDOR, DATA folder, mordor.ini BasePath=C:\MORDOR\, WAVMIX16 in windows\system, null ALSA device, Xvfb :77, main window resized to 1024x768). Character creation scripted with xdotool.
- Container is ephemeral: nothing here survives unless saved elsewhere.

## Checkpoint (harness-driven cuts)
- 9 groups cut where all copies == each other == Core: 35,44,71,72,76,95,100,107,116 (107 differs from Core only for bit outside 0..31).
- 3 groups cut where copies differ only on impossible inputs: 8 (max_hits<=0), 41 (index>=size), 90 (gold near 2^63/10^4).
- HELD, reachable difference: 31 (dead char negative hits rest cost: core 46 vs progression1b 31), 96 (level 0 = city: core fails, libraryPlaceOf answers).
- Reachable differences needing real game or report: g006 chest trap (combat2a copy does no damage), g043 store buy price (loot1a unrounded), g065 mishap kinds 1/2 swapped (hidden), g055 spell damage off-by-one, g017 equip messages, g018 give gold, g019 exp cap off-by-one (experienceCapOf), g020/g088 AD shift.
- g093 isRockSquare: equal in bounds; off-map Core=rock, C++=not rock. Only one copy reachable.
- surgery.py now supports ADD_INCLUDE; run_harness.py supports START_ROUND and REVIEW notes.
