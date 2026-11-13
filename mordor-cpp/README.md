# mordor-cpp

A self-contained project, kept apart from the rest of this repository. Nothing outside this folder belongs to it.

## Goal

A C++ implementation of Mordor's rules (the 1995 game, *Mordor: The Depths of Dejenol*) that:

- behaves like the real 1995 game,
- is designed from OpenDejenol's Mordor.Core (C#, v0.1.3) as the reference,
- is plain C++17 that can be dropped into Unreal when wanted.

## How disagreements are decided

- If Mordor.Core and the real game agree and our C++ differs, the C++ is changed to match them.
- If Mordor.Core and the real game disagree, nothing is chosen; the case goes in a report.
- If a rule cannot be tested in the real game, it is reported as "Core result only".

## Stages

1. **Prune** (in progress): cut duplicate copies of each rule down to one.
2. **Translation** (next): a C++ translation of Mordor.Core, one function per Core method, checked against Core and the real game.

## Layout

| Folder | What it holds |
|---|---|
| `code/` | The C++ rules library, tests and console app. Its git history has one commit per pruned pile. Build: `cmake -G Ninja -B b && ninja -C b`. |
| `prune/` | The pruning record: `notes/` (inventory, piles, plan, surgery state), `scripts/` (gate, planning, surgery ants), `surgery/` (each ant's output). |
| `harness/` | The differential harness: the same scripted random draws are fed to Mordor.Core (C#) and to every C++ copy, and the answers are compared. `groups/gNNN/` holds the cases and results per pile; `state.json` holds the summary. |
| `tools/mdr/` | A save editor for the real game's `.MDR` files, built on OpenDejenol's record layout. `mdr dump <file> 2900 3 Character` reads a character, and `mdr set ... Field=value` writes one. |
| `ants/` | Ant-army scripts (`army.py` runs one-shot headless model calls) and the Mordor.Core-to-C++ method mapping. |
| `real-game/` | The real game as it is set up for testing: `MORDOR/` (the shareware install, including the test character's saves), `windows/` (`mordor.ini`, `wavemix.ini`, `WAVMIX16.DLL`; these go in the Wine prefix's `windows` folder), and `shots/` (screenshots). |
| `NOTES.md` | Working notes and checkpoints. |

## Running the real game

The game runs under Wine 9 (it uses winevdm for 16-bit programs) on a virtual X display:

- Copy `MORDOR/` to `drive_c/MORDOR` in the Wine prefix.
- Copy `mordor.ini` and `wavemix.ini` to `drive_c/windows`, and `WAVMIX16.DLL` to `drive_c/windows/system`.
- Use a null ALSA device for sound.
- Start it with `wine C:\MORDOR\MORDOR.EXE`.

Before editing a save, unload the character in the game. The game keeps the loaded character in memory and writes it back over the file.

## Notes

The scripts contain absolute paths from the session that made them, so set them to your own locations before running.

Licensing:

- Mordor.Core is GPLv3, and is not included here.
- `real-game/MORDOR` is the freely distributed 1995 shareware demo (first three dungeon floors), included so the test setup can be rebuilt.
