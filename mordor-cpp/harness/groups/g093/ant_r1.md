I haven't written the three files, because I can't build the reference inputs from the information given without guessing at APIs.

**What's missing**

1. **Reference `Floor` instance.** `DungeonNav(Floor floor, int x, int y, ...)` needs a `Floor`, but the outline doesn't show a `Floor` constructor or any way to load a floor with chosen tiles. `H.Dungeon` is listed as a game table, but its type and how a level index selects a floor are not shown. Using `H.Dungeon` as a `Floor` would be an invented assumption.
2. **C++ `isRockSquare`.** It depends on `fieldAt(data, level, x, y)`, which isn't declared. `FloorDef` (the source of `GameData.floors`) isn't shown either, so I can't build the flags it reads.
3. **C++ `squareIsRock`.** It depends on `cellInState`, `sqAt`, `kBitRock`, and `SquareState`, none of which are shown. I can't set the `bits` it reads.
4. **Flag equivalence.** The C# side tests `Floor.ROCK`, and the C++ side assumes `FLAG_ROCK` in `FloorField.flags`. These need to be confirmed as the same bit.

**What the test needs to cover once the inputs are known**

- **Out-of-bounds.** The reference returns `true` (rock) for any point outside `0..Width-1`, `0..Height-1`, including `x=-1`, `y=-1`, `x=Width`, and `y=Height`. The C++ copies return `false` when the square is missing. That is a likely mismatch, and the test should report it as-is.
- **In-bounds edges.** `(0,0)`, `(Width-1,Height-1)`, and the four borders.
- **Rock and non-rock tiles.** At least one of each, taken from the shareware dungeon data.
- **Two sources in C++.** `isRockSquare` reads static floor flags and `squareIsRock` reads live square bits. They can disagree, and both should be compared against the reference.
- **Random draws.** None of the candidates takes a `Random`, so each case's `rng` should be a single placeholder draw such as `[0.5]`.

**What I need from you**

- How to construct or obtain a `Floor` in C# with specific tile flags, or the type and indexing of `H.Dungeon`.
- The `FloorDef` layout and the `fieldAt` helper.
- The `SquareState` definition, `kBitRock`, `cellInState`, and `sqAt`.
- Confirmation that C++ `FLAG_ROCK` and C# `Floor.ROCK` are the same bit.

Once those are provided I can write `cases.json`, `Case.cs`, `case.cpp`, and the shims.

REFERENCE: DungeonNav.IsRock