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

// ---- c49 c83 c104: joining and re-acquainting ----
// c83 c49 join cost: 0 unless the character already holds another guild; otherwise the join cost
// CONFLICT: wiki says first extra guild free [c83][c102]; engine governs.
// UNKNOWN: base join price value not in excerpt; new-character Nomad membership UNKNOWN [c49][c3].
// UNKNOWN: whether a 0 price displays as priced [c10].
Currency GuildJoinCost(const Ctx& ctx, const Character& c);
// c49 c30 cursed worn gear blocks join and MakeLevel; pure state check
bool HasCursedWorn(const Character& c);
// c49 c30 join a guild; refused while cursed gear is worn; re-joining a held guild is free
// CONFLICT: audit says cursed gear also refuses re-acquaint [c30]; engine governs (join only) [c49].
bool JoinGuild(Ctx& ctx, Character& c, i32 guild);
// c104 reactivate a held guild; refusal rule UNKNOWN in engine excerpt
bool ReacquaintGuild(Ctx& ctx, Character& c, i32 guild);

// ---- c49 c30 c96: levelling ----
// c49 c30 c96 tests whether Make Level is available (Visit, Form_Load, end of level); does not level up
// CONFLICT: wiki says Visit advances the level [c96]; engine governs (Make Level action performs it).
bool LevelAvailable(const Ctx& ctx, const Character& c, i32 guild);
// c49 c30 Make Level action: hands over quest item, deducts XP and gold, rolls HP and stats;
// refused by cursed gear. Quest item handed over before gold asked [c30][c49].
// CONFLICT: refusal text differs between audit and port [c30][c49]; engine governs.
bool MakeLevel(Ctx& ctx, Character& c, i32 guild);
// c18 gold cost to advance one level via Fn12B2 (kAdvanceGoldRate, guildGold / kAdvanceGuildGoldDivisor)
// UNKNOWN: whether guildGold equals the wiki GP factor [c18][c97].
Currency AdvanceGold(i32 level, Currency guildGold);
// c11 sitting guildmaster re-offered when level has risen above recorded level
// UNKNOWN: literal formula gives no claim at equal level [c11]; treated as strictly above.
bool GuildmasterReoffer(const Ctx& ctx, const GameState& game, i32 guild, const Character& c);

// ---- c19 c59 c97: quests ----
// c19 c97 quest tier from guild level; capped at kQuestTierCap
// CONFLICT: wiki "rounded down" [c97]; engine round_half_even governs.
i32 QuestTier(i32 guildLevel);
// c19 let-off: true when the rounded draw is <= kLetOffMaxRoll (quest value -1, no quest)
// CONFLICT: audit 6% and wiki 6% vs engine 5.5% [c24][c97][c19]; engine governs.
bool QuestLetOff(i32 roll);
// c59 c19 reroll a rare candidate: true when the candidate is rejected.
// Rare when weight <= kRareMonsterMaxWeight (monster) or <= kRareItemMaxWeight (item).
// Rejected with kRareRejectPercent percent probability.
bool QuestRerollRare(Ctx& ctx, i32 weight, bool isItem);
// c19 item fetch offered when guildLevel > kItemFetchMinGuildLevel
bool ItemFetchOffered(i32 guildLevel);
// c19 item level band = Int(tier / kItemBandDivisor) + 1
i32 ItemQuestBand(i32 tier);
// c19 picks a value in first+1..last; returns first+1 when span is 0
// CONFLICT: c59 says one past run end; formula governs [c19][c59].
i32 PickItemIndex(Ctx& ctx, i32 first, i32 last);
// c3 assigns a Defeat or Fetch quest
// UNKNOWN: entry point in Character.cs part 1 not in excerpt [c3].
// CONFLICT: fixed per-level quest tables on wiki [c96]; engine random governs [c2][c19][c59].
bool AssignQuest(Ctx& ctx, Character& c, i32 guild);
// c65 c102 only the killer counts toward a kill quest
// UNKNOWN: kill-XP recipient; behaviour for non-killer holder beyond message [c65][c102].
void OnMonsterKilled(Ctx& ctx, Character& killer, const Creature& creature);

// ---- c34 c2: item use ----
// c34 port: primary guild only; divergence from audit unresolved
bool CanUse(const Ctx& ctx, const Item& item, const Character& c);
// c2 true if any joined guild meets UseLevel and bitmask
// CONFLICT: wiki restricts by active guild [c83]; engine governs.
bool MeetsUseLevel(const Ctx& ctx, const Item& item, const Character& c);

// ---- c3 c61 c5: swings ----
// c3 c61 extra swings for one membership of guild; first extra at kNinjaSwingThreshold / kWarriorSwingThreshold
// UNKNOWN: whether swings sum across memberships [c3][c5][c82].
i32 SwingsFor(const GuildMembership& m, i32 guild);
// c5 picks one guild for ability; returns guild index or -1. Summing across guilds UNKNOWN.


// ---- c24 c104: forfeit ----
// c24 c104 forfeit a guild: resets XP to new level floor and rechecks equipment
// UNKNOWN: removal of A/D and abilities [c104][c24].
void ForfeitGuild(Ctx& ctx, Character& c, i32 guild);

// ---- c61 c99 c25 c88: attack, defence, hits ----
// c61 c99 max guild attack and defence across memberships
// UNKNOWN: whether non-joined memberships contribute; assumed not.
GuildAttDefResult GuildAttDef(const Character& c);
// Char-page value: GuildAttDef plus the 10/10 base (reference Rules.BaseAD added on display).
GuildAttDefResult GuildAttDefDisplay(const Character& c);
// c25 c88 hits gained: AH below ML, MH at or above ML; CON bonus below ML;
// pre-ML random Int(Rnd * (AH / kPreMlHitsDivisor) + 1) at kPreMlHitsChance.
// CONFLICT: wiki puts ML level on AH [c88][c104]; engine MH at ML governs [c25].
// CONFLICT: wiki applies CON bonus to Nomad only [c88]; engine governs.
i32 GuildHitsGain(Ctx& ctx, const GuildMembership& m, const Character& c, i32 guild);
// c25 Constitution hits bonus: random in 1..CON-16 when CON > kConHitsThreshold; otherwise 0
// UNKNOWN: distribution of the 1..CON-16 draw; uniform assumed.
i32 ConHitsBonus(Ctx& ctx, i32 con);

}  // namespace mordor
