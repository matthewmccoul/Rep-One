# Mordor.Core translated to C++ (fork 2)

A mechanical, file-by-file translation of OpenDejenol's Mordor.Core (C#, 116 files, ~29,500 lines) into plain C++17.
It is separate from the pruned version in `../code`, which it does not replace or change.

- `src/cs.h`, `src/cs_runtime.cpp`: the support runtime. The .NET pieces Core uses, with C# behaviour: reference-type
  List/Dictionary/HashSet (insertion order), LINQ, .NET rounding and number formatting, .NET's seeded Random,
  exact decimal, BigInteger, files, encodings.
- `src/fwd.h`, `src/enums.h`, `src/tuples.h`: generated from Core's Roslyn outlines (forward declarations, the 38 enums
  with names and flag operators, 57 named-tuple structs).
- `src/gen/`: one .h/.cpp per C# file (Combat.cs is split into Combat_A, Combat_H, Combat_B1..B4).
- `src/core.h`: umbrella header, value types first in dependency order.
- `tools/`: outline extractor output, the C# -> C++ type map, catalog generator, translation rules (RULES.md),
  translation, assembly, compile and fix scripts.
- `lighttest/`: the light test (C# reference generator and the C++ comparator).

Build: compile every `src/gen/*.cpp` and `src/cs_runtime.cpp` with `-std=c++17 -Isrc -Isrc/gen`.

Strings are Latin-1 bytes (std::string). Not translated (members kept, bodies throw NotSupportedException):
System.Text.Json save/load, zip extraction, streams, reflection, the audio device.
