#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_game.h"
#include "mordor/state_item.h"

namespace mordor {

// Convention for this part: `guild` is an index into Character::guilds and
// GameState::guildMasters, i.e. static_cast<i32>(Guild::X).

inline constexpr i32 kQuestTierCap = 10;
// c19 let-off when rounded draw <= kLetOffMaxRoll (rate 5.5%)
inline constexpr i32 kLetOffMaxRoll = 5;
// c59 rare monster: weight <= kRareMonsterMaxWeight; rare item: weight <= kRareItemMaxWeight
inline constexpr i32 kRareMonsterMaxWeight = 9;
inline constexpr i32 kRareItemMaxWeight = 4;
// c19 item fetch offered when guildLevel > kItemFetchMinGuildLevel
// CONFLICT: wiki "begin at gLevel 15" [c97]; engine > 15 governs.
inline constexpr i32 kItemFetchMinGuildLevel = 15;
inline constexpr i64 kAdvanceGuildGoldDivisor = 10;
// c5 ability index that WinningGuild selects a guild for (swings)
inline constexpr i32 kAbilitySwings = 3;
// c3 c61 first extra swing thresholds (membership level)
// CONFLICT: audit 55/90 vs tests 56/91 [c25][c61][c3]; engine 56/91 governs.
// UNKNOWN: membership field the thresholds compare against; level assumed.
inline constexpr i32 kNinjaSwingThreshold = 56;
inline constexpr i32 kWarriorSwingThreshold = 91;
inline constexpr i32 kNinjaGuild = static_cast<i32>(Guild::Ninja);
inline constexpr i32 kWarriorGuild = static_cast<i32>(Guild::Warrior);
// c25 CON hits bonus applies when CON > kConHitsThreshold
inline constexpr i32 kConHitsThreshold = 16;
// c25 pre-ML random hits: Int(Rnd * (AH / kPreMlHitsDivisor) + 1) at kPreMlHitsChance
inline constexpr i32 kPreMlHitsDivisor = 5;
inline constexpr double kPreMlHitsChance = 2.0 / 3.0;

// ---- result structs ----
struct GuildAttDefResult {
    i32 att = 0;
    i32 def = 0;
};

// c49 c30 join a guild; refused while cursed gear is worn; re-joining a held guild is free
// CONFLICT: audit says cursed gear also refuses re-acquaint [c30]; engine governs (join only) [c49].





// c5 picks one guild for ability; returns guild index or -1. Summing across guilds UNKNOWN.




}  // namespace mordor
