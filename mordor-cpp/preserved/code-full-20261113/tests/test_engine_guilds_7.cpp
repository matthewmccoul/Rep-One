#include "test_support.h"

// SKIP B_0065#120: No header function returns the per-guild ability rating that is displayed for a dual-guild character. abilityRating(GuildDef, f64) maps a value to a rating and does not choose between guilds.
// SKIP A_03#17: The guild list importer (CurrentGuild word, GuildsJoined slots, M579B experience copy, file offsets) is not in the headers. Needs a save-file import.
// SKIP A_03#30: The guildmaster importer (MDATA7 parsing) is not in the headers. Needs a save-file import.
// SKIP A_03#32: The guild log importer (MDATA9 parsing) is not in the headers. Needs a save-file import.
// SKIP A_03#44: No GuildLog.All accessor exists. GameState::guildLog is a plain Vec, and the importer that fills it is not in the headers.
