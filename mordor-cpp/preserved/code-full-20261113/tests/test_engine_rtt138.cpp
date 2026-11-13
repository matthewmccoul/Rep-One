// Batch t138: translated from reference assertions B_0059, B_0065, B_0066.
// None of the eight cases can be checked against our API yet: the declarations
// for encounter generation, companion-slot JSON, and the seeded combat round are
// not in our headers. Each case is recorded as SKIP until those declarations exist.

#include "test_support.h"

using namespace mordor;

// SKIP B_0066#82: EncounterGenerator.Generate has no declaration in our API (NEED above).
// SKIP B_0066#83: EncounterGenerator.Generate has no declaration in our API (NEED above).
// SKIP B_0059#74: CompanionSlots JSON deserialize (old format, empty slot name) has no declaration in our API (NEED above).
// SKIP B_0059#71: CompanionSlots JSON serialize (null slots text check) has no declaration in our API (NEED above).
// SKIP B_0065#3: seeded combat Round(party, dummy, [dragon, frail], seed) has no declaration in our API (NEED above).
// SKIP B_0065#5: seeded combat Round death-cry line ordering has no declaration in our API (NEED above).
// SKIP B_0065#9: seeded combat Round accidental-hit count has no declaration in our API (NEED above).
// SKIP B_0065#2: seeded combat Round electrocution hit-line check has no declaration in our API (NEED above).
