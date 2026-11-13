# What the C++ has that Mordor.Core does not

Question asked: if Mordor.Core is an accurate port of the real game, what content does our C++ have that Core lacks, why do we have it, and what inspired it?

Method: three stages of one-question ants (Haiku). Each stage's verdicts come from a single run, so treat counts as approximate.

| Stage | Question (one per ant) | Ants | Cost |
|---|---|---|---|
| A | What kind of thing is this function? | 2,521 | $0.62 |
| B | Does OpenDejenol (Core, App, mod system) do the same thing? (keyword search, 8 candidates) | 1,838 | $1.72 |
| C1 | Second search with a query built from the description, not code names | 329 | |
| C2 | Does the SRD describe this mechanic, and with which source tags? | 329 | |
| C3 | Original 1995 game, or added by a re-implementation? | 329 | $0.80 for C |

## Counts

- The C++ has 3,300 functions after pruning.
- 779 map to Mordor.Core methods (earlier mapping).
- The other 2,521 split as follows:

| Kind (stage A) | Count | Result |
|---|---|---|
| Plumbing (formatting, text, math, lookups) | 562 | not game content |
| Stub / placeholder | 121 | not game content |
| Rules, flow, data, mod | 1,838 | see below |

Of those 1,838 (stage B):
- 310 are SAME as OpenDejenol.
- 1,199 match PARTIALLY.
- 329 were not found.

Where the matches sit: 1,128 in Mordor.Core, 329 in Mordor.App (game screens), 52 in the mod system.

Of the 329 not found, the second search (C1) found 79 partly. 250 remain:

| Group | Count | What they are |
|---|---|---|
| SRD cites only community sources | 63 | Forum archive, wiki, spell spreadsheet, manual/research notes. |
| Not in the SRD excerpts at all | 26 | Mostly our own console layer (typed commands, help, roster, options), our save format, the mod JSON reader, and 128-bit math for currency. Also a few unsourced values: slime hit, 50% companion cast failure. |
| SRD cites OpenDejenol code or notes, but both searches missed | about 160 | Probably present in OpenDejenol in another form. Unresolved. |

## Why they exist

- The SRD was compiled from all of OpenDejenol (Core, App, mod system, audits, tests) plus community material. Its own precedence rule ranks engine code first and community material last, and it flags community claims as reconstructions or speculation.
- The coding ants implemented SRD rules regardless of where they came from.
- The coding ants also worked one chapter part at a time and could not see each other's code. So rules restated across chapters were written more than once, as were the helpers.

## Inspired by community material (the 63)

Two kinds:

1. **Analysis formulas turned into game code:**
   - ECV score and price-by-linear-fit
   - Projected XP and projected level
   - "Archive route" target levels from an optimal-leveling forum post
   - Spell-spreadsheet columns
   - Approximate race experience multipliers

   These describe or approximate the game; they are not the game's own rules.
2. **Community claims about mechanics:**
   - charm and join chances from Charisma
   - pariahs only on floors 12–15
   - magic sack capacity
   - item spawn table
   - weapon resistance by level
   - crest spells
   - Control Dragon failure

Under the canon, each needs checking against Core and the real game before it stays.

## Files

- `all_functions.csv`: one row per function with every stage's answer.
- `b_rows.json` and `c_rows.json`: the raw verdicts.
- `c_meta.json`: the citation tags and UNKNOWN/CONFLICT markers read from each function's comments.
- `scripts/`: the stage scripts.
