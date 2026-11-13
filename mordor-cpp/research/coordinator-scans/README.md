# Coordinator scans

Two whole-codebase scans. These are the coordinator's job, because no individual ant ever sees all of the code.

## 1. What the game actually runs (`live.json`)

Method: the console game was built at `-O0 -g -fno-inline` with `-ffunction-sections` and `-Wl,--gc-sections`. Every function the linker kept was then matched to its source line.

- The game runs 784 of the 3,300 functions in `src/` (24%). The remaining 2,516 are reached only by tests, or by nothing.
- Of the 779 functions mapped to Mordor.Core methods, 182 run in the game.
- Of the 63 functions sourced only from community material, 3 run.

## 2. Same behaviour under different names (`desc_pairs.json`)

Method: compare the stage A plain-language descriptions (TF-IDF cosine of 0.55 or more), across different function names.

- 106 pairs found. These are candidate duplicates that name-based piling missed. Examples:
  - `raceAllowsGuild` / `raceMayJoinGuild`
  - `chestFateTrap` / `chestFate`
  - four separate breath-chance functions
  - `inflictDisease` / `tryDisease`
  - `applyYounger` / `youngerBy`
  - three item spell-point gains
- Some pairs are false positives, such as get versus set.
- This scan covers only the 2,521 functions that have descriptions. The 779 Core-mapped functions were not described.
