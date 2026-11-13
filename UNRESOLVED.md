# What could not be resolved, and why it does not matter for the system

These two tests check **counts in the original game's data file**, not rules of the system. The goal of this project is the system (the SRD rules running in code), not reproducing the original game's content, so they are outside the goal and could simply be dropped. They are kept below, gated, only because the reference suite contains them.

Of the 52 failing acceptance tests (translated from the reference implementation's own assertions), 50 were resolved by changing code, test translations or both. **Two cannot be satisfied with the data this project has**, and they are the only ones. They are handled the way the reference suite handles them.

## B_0063#113 "teleporters in the dungeon = 86" and B_0063#124 "squares across the 15 levels = 13,500"

| Evidence | Finding |
|---|---|
| Shipped dungeon file `src/headless-reference/data/MDATA11.MDR` | 68,166 bytes. Its first record is `03 00 ...`, so `numLevels = 3` (the reference loader reads the level count from that word, `MdrLoader.LoadDungeon`, `MdrLoader.cs:138`). |
| Same file, size check | 3 floors at about 22.7 KB each. A 15-level file would be about 340 KB. |
| `data/combat-data.json` | `source: "1995 MORDOR11.ZIP shareware, nested MORDOR.WIP"`; `sourceSha256["MDATA11.MDR"]` begins `f24989233bf0029b`, the same digest as the file above. 3 floors (levels 1, 2, 3), 900 squares each = 2,700 squares; teleporters 4 + 4 + 6 = 14. |
| Any other dungeon file in the materials | None. A search for `*.MDR` finds only MDATA1, 2, 3, 5 and 11 (shareware set). |
| Reference classifies this data | `GameFiles.Check` returns `Limited` when `MDATA11.MDR` holds fewer than `FullLevels = 15` levels (`GameFiles.cs:142-144`); `SharewareLevels = 3` (`GameFiles.cs:67-70`). |
| The reference's own suite | Both figures are asserted with `FullCheck` (`SelfCheckProbe.cs:2743` for 86; `SelfCheckProbe.GameFiles.cs:130` for the 15 levels). `FullCheck` is defined at `SelfCheckProbe.cs:58-67` as: skip with the message "full game only (this folder is the shareware)" when the folder is the shareware. |

Conclusion: the two numbers are properties of the **full game's data file**, which is not in the project materials and cannot be derived from the shareware file or from any SRD rule. (The SRD cannot hold a rule for them: they are counts over data, not mechanics.) The translated tests originally dropped the reference's `FullCheck` gate; they now reproduce it (`tsup::fullGameOnly`, in `tests/test_support.h`): on data with fewer than 15 levels they print `skip ... full game only` and assert the shareware figures instead (14 teleporter squares, 2,700 squares). With the full data file in `data/`, the same tests assert 86 and 13,500 unchanged. Nothing in the code is bent to produce those numbers.

The SRD states the full-game figures (13,500 squares, 15 levels of 30 x 30; 86 teleporters per the reference) only as reference facts; `SRD_FOUND_ERRORS.md` lists the addition.

## Honest limits of the other 50

- Some resolutions rest on the reference being treated as ground truth where the SRD was silent or imprecise; each is listed in `SRD_FOUND_ERRORS.md`.
- Two fixes are translation or API-convention corrections, not rule changes: the squares API is 0-based (the translated loops were 1-based, which also under-counted teleporters on level 3), and the Char page shows the best raw guild award plus the 10/10 base (`GuildAttDefDisplay`), while `GuildAttDef` keeps returning the raw award that the SRD-derived tests expect.
- The 18 failing SRD-derived tests (31 checks) were not part of this request and are unchanged.

## Phase 3: open items (not proven impossible; not done yet)

| Item | State | Why open |
|---|---|---|
| Confinement (pens) and companions in the game layer | module tested, not wired | companion storage is not reachable from `Party`; needs a companion model decision |
| Library, seer, guild log commands | not wired | the library model differs from the reference (`Librarian.*`, about 40 skipped assertions) |
| Spell effects outside combat; item spell effects | casting spends points and checks reach, effect not applied | `applyNonCombatSpell` / heal amount not defined in the modules |
| Chest traps 7 to 10 | defined in the reference, not applied | teleport, blackout, fear and withering need hooks into other modules |
| Store restock over time | DONE in phase 3 (rest wires the reference routine); the Library filing of restocked items is not wired | the library model differs |
| Encounter companion chains | `pickCompanionMonster` always restarts the encounter | needs the companion spawn tables |
| Modding API | placeholder types (`ModRef`/`ModTag`) | OpenDejenol's own engine feature, not a Mordor rule; low priority |
| Reference assertions on `MdrLoader.*`, `*.Paint` | will not be translated | file-format loaders and UI painting are outside the system |
| `UNKNOWN:` markers (1,596), `CONFLICT:` (246) | untriaged | each marks an SRD gap; the reference answers many |



## Update 2026-11-12 (features pass)
Closed: empty starting pens, pen index, companion fields on purchase, realign fee, crash in the smoke tests. Still open and new: see CODING_REPORT.md "Phase 4 / Open gaps". Observations that look odd but come straight from the reference formulas (not changed): pen prices are tens of thousands of gold for a new party with about 1,200 gold each (R22), and a first-level mage lists Cause Wounds at 0 SP.
