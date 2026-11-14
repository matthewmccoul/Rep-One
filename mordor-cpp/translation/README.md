# Mordor.Core translated to C++ (fork 2)

A mechanical, file-by-file translation of OpenDejenol's Mordor.Core (C#, 116 files, ~29,500 lines) into plain C++17.
It is separate from the pruned version in `../code`, which it does not replace or change.

## Layout

| Path | What it holds |
|---|---|
| `src/cs.h`, `src/cs_runtime.cpp` | The support runtime: the .NET pieces Core uses, with C# behaviour (see below). |
| `src/cs_json.h` | System.Text.Json: parser, writer, generic contracts, converter reader/writer. |
| `src/cs_io.h` | SHA-256, zlib streams (ZLibStream / DeflateStream), ZipArchive (read), Convert.ToHexString. |
| `src/fwd.h`, `src/enums.h`, `src/tuples.h` | Generated from Core's Roslyn outlines: forward declarations, the 38 enums (names, flag operators), 57 named-tuple structs. |
| `src/gen/` | One .h/.cpp per C# file (Combat.cs is split into Combat_A, Combat_H, Combat_B1..B4). `JsonContracts.*` is generated (below). |
| `src/core.h` | Umbrella header, value types first in dependency order. |
| `tools/` | Outline output, type map, catalog generator, translation rules (RULES.md), translate/assemble/compile/fix scripts, `build.sh`, `gen_json.py`. |
| `tools/json-contract/` | Dumps .NET's own System.Text.Json contract for every type Core saves (`contract.json`), which `gen_json.py` turns into `src/gen/JsonContracts.*`. |
| `lighttest/` | The first light test (C# reference generator and C++ comparator). |
| `tests/` | The differential tests described below. |

Build: compile every `src/gen/*.cpp` and `src/cs_runtime.cpp` with `-std=c++17 -Isrc -Isrc/gen`, and link with zlib (`-lz`).
`tools/build.sh` does this into a static library. The scripts and `.csproj` files contain absolute paths from the
session that made them; set them to your own locations.

## The runtime's C# behaviour

- Classes are `cs::Ref<T>` (a shared pointer). Member access through a null reference throws `NullReferenceException`, as in C#.
- Exceptions follow the .NET hierarchy, so `catch (ArgumentException)` also takes `ArgumentOutOfRangeException`, and `catch (IOException)` takes `FileNotFoundException`.
- Lists, dictionaries and hash sets are reference handles. Dictionaries keep insertion order. LINQ is provided, and `Sum` over int/long is checked (OverflowException), as .NET's is.
- `double`/`float` to text matches .NET Core 3.0+: shortest round-trip, scientific notation from exponent 17 (double) / 9 (float) or below -4, and "Infinity"/"NaN".
- Number parsing follows .NET's NumberStyles: sign, thousands and decimal separators, exponent, currency symbol, white space, and `Infinity`/`NaN`. CurrentCulture defaults to the invariant culture's symbols (`cs::Parse::CurrentCulture()` can be set, e.g. CurrencySymbol "$" for en-US).
- File text is UTF-8 on disk (a BOM is skipped on reading) and Latin-1 in memory.
- JSON save files: same property order and names, the same `[JsonIgnore]` conditions, converters (SavedWholeJson, SavedExactJson, CompanionSlotsJson), `IJsonOnDeserialized`, the default encoder's escaping (`"`, `<`, non-ASCII as `\u00XX`), indentation and number text.
- The .NET seeded `Random` algorithm, exact `decimal`, `BigInteger`, `Math.Round` half-to-even.

## What was tested, and how it came out

Every test runs the same inputs through C# Mordor.Core (the prebuilt v0.1.3 DLL) and through this translation, and compares the output text exactly.

| Test | Size | Result |
|---|---|---|
| Light test (rules, loaders, prices, currency) | 13,679 comparisons | all match |
| JSON round trip: random documents for all 11 saved types (Character, Party roster, DungeonState, HallRecords, StoreStock, SharedMap, Guildmasters, Confinement, Librarian store, Roster, help topics), each read and written back | 6,600 documents, plus 600 `Character.Load` then `Save` | 1 difference (the null-string limit below); every other output identical, and where C# throws the C++ throws the same kind |
| Number parsing (`VbConvert.TryCLng`, `VbCurrency.TryParse`, `GiveCommand.CLng`, `double.TryParse`, `decimal.TryParse`) | 377 strings x 5 parsers | all match |
| ZIP installer reading (`InstallerUnpack.Parse`): valid, stored, case clashes, bad names, bad CRC, truncated, not a zip | 12 archives | all match (contents, order, and which ones fail) |
| PNG decode, every colour type and bit depth with all five filters; re-encode | 34 images | all match, and the re-encoded bytes are identical to .NET's |
| SHA-256: `GameArt.Key`, `GameFiles.Recognise` | the shareware folder | match |
| Fights: random parties (1-4 members, races, guilds, levels, gear, Fight/Spell/Defend orders) against `EncounterGenerator.Generate` encounters, fought to the end or 40 rounds, then spoils | 3,000 fights, 650,462 lines (every combat line, every HP/SP/XP/gold figure each round, the RNG state at the end) | identical |
| Level-ups over 25 rounds of experience per character, the store (add, take, prices, sell, identify, realign), the confinement's replenish, a 150-step dungeon walk with encounters on the floor | 1,000 seeds, 732,200 lines | identical |

### Fixed along the way (found by these tests)

- Null member access crashed instead of throwing `NullReferenceException`.
- `Sum` wrapped on overflow; .NET throws.
- `double.ToString()` used scientific notation from exponent 15; .NET switches at 17.
- The exception classes were unrelated to each other, so base-class `catch` clauses missed derived exceptions.
- `File.Move` overwrote the destination; .NET refuses unless told to overwrite.
- `new Party()` (a `params` constructor called with no arguments) had no C++ form.
- `monsters is Encounter` in CombatEngine was always false. It is now answered from the list's identity.
- HelpBundle's `object` items kept no runtime type. They now derive from `cs::Object`, so the type tests work.
- `Rules.GroupGold` drew two random numbers inside one expression. C++ does not fix that order, so the draws are now sequenced.

## Known limits

- **Null strings.** A C# string can be null, but here it is `std::string`, which cannot be. A save file field that is `null` or missing (where C# leaves it null) reads back as `""` and is written as `""` instead of `null`. This was the 1 difference in the JSON test.
- **Characters past U+00FF.** Strings are Latin-1, so such characters become `?`.
- **Evaluation order.** C# evaluates operands and arguments left to right; C++ does not guarantee it. Every expression with two or more visible random draws was checked, and the one unsequenced case was fixed. A draw hidden inside a helper called twice in one expression would not show up in that scan. None showed up in the 1.38 million compared lines.
- **Windows-only parts** (left as they are; these need Windows to test): the registry (`GameFolder.UninstallEntries` returns empty), P/Invoke (`SHGetKnownFolderPath`, `lstrcmpiW`; `LibrarySearch` uses its non-Windows fallback), special-folder paths (approximated from environment variables), and the audio device (WaveMix).
- **Culture-aware sorting.** `LibrarySearch.WordSort` compares ordinally, ignoring case, where .NET uses the invariant culture's linguistic compare.
- **Compression output.** PNG encoding uses the system zlib at .NET's level for `Optimal`. Its bytes matched .NET 8 on Linux. Another zlib build could compress differently and still decode the same.
