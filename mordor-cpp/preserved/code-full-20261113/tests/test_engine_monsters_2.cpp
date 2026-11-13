#include "test_support.h"

// No case in this batch has an equivalent function in the new headers, so every
// case is skipped. Ref: CombatEngine.RoundPaced, EncounterGenerator.Generate/InRoom/Capture,
// CombatMonster.Runs/LoadRecord.

// SKIP B_0056#6: needs a multi-round combat simulation over 400 runs (no RoundPaced equivalent)
// SKIP B_0056#7: needs a multi-round combat simulation over 400 runs (no RoundPaced equivalent)
// SKIP B_0056#8: EncounterGenerator.Capture has no equivalent in the headers
// SKIP B_0056#9: EncounterGenerator.Generate has no equivalent; needs 300 generated encounters
// SKIP B_0056#10: EncounterGenerator.InRoom (room store/read-back) has no equivalent in the headers
// SKIP B_0056#11: CombatMonster.Runs has no equivalent; depends on room read-back
// SKIP B_0056#12: CombatMonster.LoadRecord has no equivalent in the headers
// SKIP B_0057#28: EncounterGenerator.Capture has no equivalent in the headers
// SKIP B_0057#29: EncounterGenerator.Capture has no equivalent in the headers
// SKIP B_0057#31: EncounterGenerator.Capture has no equivalent in the headers

