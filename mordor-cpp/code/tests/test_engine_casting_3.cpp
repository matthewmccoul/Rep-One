// Translated assertions from reference tests B_0061 (casting chapter 3).
// No case in this batch could be mapped to a header function with confirmed
// inputs, so every case is recorded as a skip below.
#include "test_support.h"
#include "mordor/casting3.h"

// SKIP B_0061#57: Spellbook.Cost has no direct equivalent in the headers. Cheapest-guild selection needs GuildDef/SpellDef table data that is not provided, and the mapping of "spell point ratio 40" onto spellPriceForGuild's costRatio/rate parameters is unconfirmed.
// SKIP B_0061#58: Rules.Fn1485Price has no equivalent function in the provided headers.
// SKIP B_0061#59: Rules.Fn1485Price has no equivalent function in the provided headers.
// SKIP B_0061#60: Rules.Fn1485Price has no equivalent function in the provided headers.
// SKIP B_0061#61: Spellbook.Cost equivalent unconfirmed. The Shock spell ID and the "running original's Points column" semantics are not available from the provided data.
// SKIP B_0061#62: Spellbook.Cost equivalent unconfirmed. The Firebolt spell ID and the "running original's Points column" semantics are not available from the provided data.
// SKIP B_0061#63: Spellbook.Cost equivalent unconfirmed. The Charm of Opening spell ID and the Nomad/Sorcerer dual-guild setup are not mapped to a confirmed API.
// SKIP B_0061#64: Spellbook.Cost equivalent unconfirmed. The Cold Blast spell ID and the Nomad/Sorcerer dual-guild setup are not mapped to a confirmed API.
// SKIP B_0061#65: Spellbook.Cost equivalent unconfirmed. The Sight Veil spell ID and the Nomad/Sorcerer dual-guild setup are not mapped to a confirmed API.
// SKIP B_0061#66: Spellbook.Cost equivalent unconfirmed. The See Invisible spell ID and the Nomad/Sorcerer dual-guild setup are not mapped to a confirmed API.
