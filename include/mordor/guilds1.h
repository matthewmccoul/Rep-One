#pragma once

// guilds1.h: public API for SRD c04p1 (guild roster, membership, join rules,
// levelling and Make Level, quests, guildmaster seat and crest, derived combat
// and item rules).
//
// UNKNOWN: Character has no Inventory member. Routines that read the pack,
// worn gear or vault take Inventory& explicitly alongside the Character.
// CONFLICT: state_common.h, state_item.h, state_companion.h, state_character.h
// and state_dungeon.h have no include guards, so including state_game.h and
// state_party.h double-includes them. Foundation headers need guards before
// this header compiles.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_game.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants (SRD tables and literals) ----

// R1 bitmask bit per guild, table order (Nomad=1 ... Healer=2048)
inline constexpr i32 kGuildBitmask[kGuildCount] = {
    1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048};
// R28 bitmask of all guilds (2^12 - 1)
inline constexpr i32 kGuildAllMask = 4095;

// R26 join cost weights
inline constexpr f64 kJoinAbilityWeight = 1.25;
inline constexpr f64 kJoinSpellDivisor = 3.0;
inline constexpr i64 kJoinCostFactor = 5356;

// R13 warning threshold on TopGuildLevel
inline constexpr i32 kJoinWarnTopLevel = 10;

// R20 level cap (hard-coded in port, R69)
inline constexpr i32 kMaxGuildLevel = 999;

// R25 gold engine rate (computed in extended precision in the original)
// CONFLICT: the original uses extended precision; a Double rounds some halves differently (R25 quirk).
inline constexpr f64 kGoldRate = 60.2;

// R32 hits
inline constexpr i32 kMaxHits = 999;
inline constexpr i32 kHitsConThreshold = 16;   // CON > 16 gives a bonus

// R30 extra swings
inline constexpr i32 kSwingThresholdScale = 50;
inline constexpr i32 kSwingModBase = 10;

// R35 ability chance clamps and floors
inline constexpr i32 kThievingMin = 5;
inline constexpr i32 kThievingMax = 98;
inline constexpr i32 kPerceptionMin = 5;
inline constexpr i32 kPerceptionMax = 90;
inline constexpr i32 kMonsterGuildLevelFloor = 2;

// Ability indices (Guild.Abilities index, R35/R36)
inline constexpr i32 kAbilityThieving = 0;
inline constexpr i32 kAbilityFighting = 4;
inline constexpr i32 kAbilityPerception = 5;

// R36 Rules.FightingBase
inline constexpr f64 kFightingBase = 0.6;

// R37 rating floor
inline constexpr i32 kRatingMin = 1;

// R38 A/D
inline constexpr i32 kNomadAdBonus = 3;

// R40 Nomad A/D sheet: even levels in this range
inline constexpr i32 kNomadEvenMin = 2;
inline constexpr i32 kNomadEvenMax = 30;

// R42-R53 quest rules
inline constexpr i32 kQuestDefeatMinLevel = 5;     // monster quests begin at gLevel 5
inline constexpr i32 kQuestFetchMinLevel = 15;     // item quests need gLevel > 15
inline constexpr i32 kFetchPercent = 25;           // fetch roll when gLevel > 15
inline constexpr i32 kLetOffPercent = 5;           // R43 let-off threshold
inline constexpr i32 kLevelsPerTier = 25;          // R46
inline constexpr i32 kTierMin = 1;
inline constexpr i32 kTierMax = 10;
inline constexpr i32 kItemLevelMax = 7;            // R49 cap
inline constexpr i32 kRareMonsterWeight = 10;      // R51 Rarity < 10
inline constexpr i32 kRareItemWeight = 5;          // R50 Rarity < 5
inline constexpr i32 kRareRejectPercent = 25;      // R50/R51 reject when Rnd*100 > 25
inline constexpr i32 kQuestItemMinPrice = 100;     // R50 Price < 100 rejected
inline constexpr i32 kQuestItemMinFloor = 1;       // R50 Floor < 1 rejected

// R62 town recharge slot range in the pack
inline constexpr i32 kRechargeFirstSlot = 1;
inline constexpr i32 kRechargeLastSlot = 39;

// ---- result types ----

// R16 display style for a guild list entry
enum class EligibilityStyle { Hidden, Italic, Bold, Plain };

// R59 crest placement result
enum class CrestOutcome { AlreadyHeld, NoRoom, Placed };

// R29 spell access result (canCast and floored shown level)
struct SpellAccessResult {
    bool canCast = false;
    i32 shownLevel = 0;
};

// R38 accumulated attack and defence (GuildAD)
struct AdValue {
    f64 att = 0.0;
    f64 def = 0.0;
};

// R39 best attack and defence across joined guilds
struct BestAd {
    i32 att = 0;
    i32 def = 0;
};

// R43 roll outcomes (RollQuest return value)
inline constexpr i32 kRollNone = 0;
inline constexpr i32 kRollLetOff = -1;
inline constexpr i32 kRollDefeat = 1;
inline constexpr i32 kRollFetch = 2;

// ---- R4 crest ----
// R4 crest item id = 299 + guild index
i32 crestId(Guild guild);

// ---- R13 joining a guild ----
// R13 join at the guild hall; a joined guild is re-acquainted instead
bool joinGuild(Ctx& ctx, GameState& game, Character& character, Inventory& inventory, Guild guild);
// R13.1 re-acquaint: free, makes the membership active (UNKNOWN: reactivation meaning)
bool reacquaintGuild(Ctx& ctx, Character& character, Guild guild);
// R13.3 0 if no other guild joined, else Round(sum^2 * 5356)
Currency guildJoinCost(const GuildDef& guildDef, const Character& character);
// R13.4 true if any worn item is cursed (CONFLICT: re-acquaint refusal, see R13 step 4)
bool gearCheck(const Inventory& inventory);
// R14 eligibility: race, alignment and stats all pass
bool eligibleToJoin(const GuildDef& guildDef, const Character& character);
// R14/R15 lists unmet stats, alignment and race; non-empty means refuse
Vec<Str> unmet(const GuildDef& guildDef, const Character& character);
// R15 all stat[i] >= ReqStats[i]
bool meetsStats(const GuildDef& guildDef, const Arr<i32, kStatCount>& stats);
// R15 (AlignMask & (1 << a)) != 0
bool allowsAlignment(const GuildDef& guildDef, Align align);
// R15 raceIndex < 0 or absent from RaceData is allowed; else RaceMask bit test
bool allowsRace(const GuildDef& guildDef, i32 raceIndex);
// R13.7 recalculates SpellPoints; refills when refill is true
void recalcSpellPoints(Ctx& ctx, Character& character, bool refill);
// R8 / R13.7 appends a log entry stamped with game.today
// FIXED: GuildLogKind is declared nowhere in the API INDEX or state headers; using plain i32 for the kind
void addGuildLog(Ctx& ctx, GameState& game, Guild guild, i32 kind, Str text);

// ---- R17/R18 active guild ----
// R17 move the named membership to the front; true if already front
bool setActiveGuild(Ctx& ctx, Character& character, Guild guild);

// ---- R20 Make Level, R19 levelling ----
// R20/R55 Make Level procedure at the guild hall
void makeLevel(Ctx& ctx, GameState& game, Character& character, Inventory& inventory, Guild guild);
// R20.3 true if experience meets the threshold for the next level
// UNKNOWN: engine guild XP threshold table (GuildData.cs:73-112); threshold passed in
bool canLevelUp(const GuildMembership& membership, i64 xpRequired);
// R20.5/R25 fee for the next level
Currency goldForNextLevel(const GuildMembership& membership);
// R20.6/R32 raises the level by one; hits and grants follow in the caller
// UNKNOWN: stat change on level (R20 step 9); body not in this part
void levelUp(Ctx& ctx, GuildMembership& membership);
// R25 engine gold formula for advancing from level L
Currency goldForLevel(Currency guildGold, i32 level);




// ---- R32 hits ----
// R32 hits gained on a level; below ML uses AH+random+CON bonus, at/above ML uses MH; cap 999
// CONFLICT: Nomad rule and CON bonus scope (R33)
i32 hitsGain(Ctx& ctx, const Character& character, const GuildMembership& membership,
             const GuildDef& guildDef, i32 con);


// R38 accumulated attack and defence up to min(level, AdMaxLevel)
AdValue guildAD(const GuildMembership& membership, const GuildDef& guildDef);
// R38 base A/D added on display (Rules.BaseAD)
// UNKNOWN: body of BaseAD is not given in the SRD
i32 baseAD(Ctx& ctx, const Character& character);
// R34.5/R39 max attack and max defence across joined guilds
BestAd bestGuildAD(const GameData& data, const Character& character);
BestAd bestGuildAD(const Character& character);

// ---- R42-R44 quest rolls and assignment ----
// R43 quest roll: returns kRollNone, kRollLetOff, kRollDefeat or kRollFetch
// CONFLICT: let-off rule (R53 wiki 6% vs engine 5%)
i32 rollQuest(Ctx& ctx, const GuildMembership& membership, const GuildDef& guildDef, i32 guildLevel);
// R42.1 QuestType = 0, QuestTarget = 0, QuestDone = false
void clearQuest(Ctx& ctx, GuildMembership& membership);
// R42/R44 roll and set QuestType and QuestTarget; nothing set if draw fails
void assignQuest(Ctx& ctx, GuildMembership& membership, const GuildDef& guildDef, i32 guildLevel);

// ---- R46-R52 quest tier, band, levels, picks ----
// R46 clamp(round_half_even(guildLevel / 25.0), 1, 10)
i32 questTier(i32 guildLevel);
// R47 Int(tier / 1.5) + 1
i32 itemBand(i32 tier);
// R51 defeat target as index into ctx.data.monsters; -1 when none (null)
// R52 rarity reroll (wiki 74%, engine 75%): CONFLICT
i32 questMonster(Ctx& ctx, i32 tier);
// R50 fetch item as index into ctx.data.items; -1 when none (null)
// CONFLICT: pick range (R50)
i32 questItem(Ctx& ctx, i32 tier);
// R69 port pre-check: false if no item can pass the fetch selection
bool questItemReachable(Ctx& ctx, i32 tier);

// ---- R45 / R54 / R55 / R56 completion and abandonment ----
// R45/R55 quest satisfaction; consume removes a fetch item and increments ItemQuestsDone
bool questSatisfied(Ctx& ctx, Character& character, Inventory& inventory, Guild guild, bool consume);

// ---- R57 / R58 / R59 mastership and crest ----
// R57 mastership claim: level > 0 and (seat empty or held.Level < level)
bool claimMastership(Ctx& ctx, GameState& game, const Character& character, Guild guild, i32 level);
// R58 mastership procedure: claim, announce, award crest
// UNKNOWN: crest equip and trade rules (R58 steps 4-5 are wiki only)
void mastershipProcedure(Ctx& ctx, GameState& game, const Character& character,
                         Inventory& inventory, Guild guild);
// R58.3/R59 places crest 299+guild in first free pack slot, Known=All, Identified
CrestOutcome awardCrest(Ctx& ctx, Inventory& inventory, Guild guild);

// ---- R60 titles, R61 identify skill ----
// R60 title from level bands; unknown guild returns "Adventurer"
Str guildTitle(Guild guild, i32 level);



// R68 installs the twelve seeded guildmasters where no named holder exists
void newGame(Ctx& ctx, GameState& game);

}  // namespace mordor
