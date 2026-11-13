#pragma once
// NOTE: state_common.h, state_item.h, state_companion.h, state_character.h and state_dungeon.h have no include guards (foundation). This header assumes the foundation adds them before it is compiled.
#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/attributes.h"
#include "mordor/death1b.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_game.h"

namespace mordor {

// ---- result structs ----


struct IndexRange {
    i32 first = 0;
    i32 last = 0;
};

// Stands in for Option<Guild>.
struct GuildChoice {
    bool found = false;
    Guild guild = Guild::Nomad;
};

// T13 guild abilities, in engine index order.
enum class GuildAbility : i32 {
    Thieving = 0,
    Backstabbing = 1,
    CriticalHit = 2,
    MultipleSwings = 3,
    Fighting = 4,
    Perception = 5
};
inline constexpr i32 kGuildAbilityCount = 6;

// ---- constants from the SRD ----

// T2 takeover level = seeded level + kTakeoverOffset.
inline constexpr i32 kTakeoverOffset = 1;

// T2 seeded guildmaster levels, in Guild enum order.
inline constexpr i32 kSeedMasterLevel[kGuildCount] = {64, 56, 62, 48, 67, 77, 56, 54, 52, 61, 48, 65};

// T2 seeded guildmaster names.
// UNKNOWN: Healer spelled "Hands On Henry" on the page; source spelling used (R37).
inline constexpr const char* kSeedMasterName[kGuildCount] = {
    "Crashland", "Garth SliceMonger", "Eli", "Shogun", "Maxx", "Martin",
    "Nimblefingers", "Kane the Wise Axe", "Sparrowhawk", "Requnix", "Merlin", "Hands on Henry"};

// T3 crest item ids (kCrestItemIdBase + guild index).
inline constexpr i32 kCrestItemId[kGuildCount] = {299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310};


// T4 extra swing levels. At a listed level the count is index+1; one level earlier it is index.
// CONFLICT: R1 thresholds 56 and 91 (Integer rounding) vs 55 and 90 (no rounding); 56 and 91 used.
struct SwingLevelRow {
    Guild guild;
    i32 levelCount;
    i32 levels[4];
};
inline constexpr SwingLevelRow kSwingLevelRows[5] = {
    {Guild::Ninja, 4, {56, 122, 267, 585}},
    {Guild::Warrior, 2, {91, 324, 0, 0}},
    {Guild::Villain, 2, {105, 434, 0, 0}},
    {Guild::Paladin, 2, {111, 484, 0, 0}},
    {Guild::Scavenger, 2, {116, 531, 0, 0}}};

// T6 maximum attack per guild (Nomad, Sorcerer, Wizard values per R33/R34).
inline constexpr i32 kMaxAttack[kGuildCount] = {175, 254, 223, 254, 243, 208, 168, 196, 102, 98, 106, 124};
// T6 maximum defence per guild.
inline constexpr i32 kMaxDefence[kGuildCount] = {175, 276, 258, 254, 272, 251, 204, 222, 194, 195, 232, 200};
// T6 level at which each guild's maximum is reached.
// CONFLICT: Nomad 197 (c61,c88,c99) vs 198 (c103) (R33); 197 used.
inline constexpr i32 kMaxAdLevel[kGuildCount] = {197, 450, 430, 457, 433, 428, 394, 522, 475, 467, 622, 470};

// T9 Nomad title thresholds for the first twelve bands. Names of other bands: UNKNOWN.
inline constexpr i32 kNomadTitleLevel[12] = {10, 20, 30, 40, 50, 60, 80, 100, 130, 150, 190, 220};
// T9 "Master Nomad" level. UNKNOWN: wiki meaning unclear.
inline constexpr i32 kNomadMasterLevel = 220;
// T10 Ninja title threshold: below is Assassin, at or above is Shogun. UNKNOWN: Instructor threshold.
inline constexpr i32 kNinjaShogunLevel = 120;

// T16 average hits gained per level below the hit cap (AH).
// CONFLICT: Sorcerer and Wizard values from sheet (R30–R31).
inline constexpr i32 kAvgHits[kGuildCount] = {5, 6, 5, 4, 4, 4, 3, 4, 3, 2, 2, 2};
// T16 hits-cap level (ML).
// CONFLICT: Sorcerer ML 29 (R30); Wizard ML 41 (R31).
inline constexpr i32 kHitsCapLevel[kGuildCount] = {30, 28, 29, 25, 26, 27, 21, 24, 27, 29, 41, 28};
// T16 late hits per level at or above ML (MH).
// CONFLICT: boundary at level == ML is MH in engine, AH on wiki (R2).
inline constexpr i32 kLateHits[kGuildCount] = {1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
// T16 / T17 experience factor raw value (EP).
inline constexpr i32 kExpFactorRaw[kGuildCount] = {8, 16, 19, 21, 21, 18, 15, 18, 27, 27, 30, 32};
// T17 actual experience factor (raw / 8; Nomad = 1.0).
inline constexpr double kExpFactor[kGuildCount] = {1.0, 2.0, 2.375, 2.625, 2.625, 2.25, 1.875, 2.25, 3.375, 3.375, 3.75, 4.0};
// T16 quest percent (QP). Wizard 15% per R32.
inline constexpr i32 kQuestPercent[kGuildCount] = {20, 20, 25, 35, 25, 20, 15, 20, 15, 10, 15, 15};
// T17 actual guild gold factor (GP). UNKNOWN: not used by engine formula (R20).
// CODE_BUG fix: the guild gold factor is data (GameData guild goldFactor, SRD ch01 Guilds table),
// not a hand-typed table. kGoldFactor removed.

// T19 minimum stats per guild, order Str, Int, Wis, Con, Cha, Dex (matches StatIndex).
// UNKNOWN: engine index order for these columns.
inline constexpr i32 kGuildMinStat[kGuildCount][kStatCount] = {
    {1, 1, 1, 1, 1, 1},
    {14, 7, 5, 10, 3, 8},
    {14, 9, 9, 10, 16, 15},
    {13, 11, 7, 9, 3, 15},
    {15, 14, 14, 11, 6, 16},
    {10, 13, 13, 11, 5, 13},
    {8, 12, 8, 6, 5, 17},
    {11, 8, 8, 9, 4, 14},
    {6, 11, 13, 8, 10, 14},
    {6, 14, 13, 11, 5, 8},
    {8, 18, 18, 13, 6, 14},
    {8, 14, 14, 8, 7, 14}};

// T18 guild ability ratings, order per GuildAbility.
// UNKNOWN: blank cells written as 0 (research.txt convention); column semantics unknown.
inline constexpr f32 kGuildAbilityRating[kGuildCount][kGuildAbilityCount] = {
    {5.0f, 0.0f, 0.0f, 0.0f, 7.0f, 5.0f},
    {2.0f, 0.0f, 9.0f, 5.0f, 12.0f, 2.0f},
    {2.0f, 0.0f, 5.0f, 2.0f, 10.0f, 3.0f},
    {7.0f, 2.0f, 5.0f, 8.0f, 10.0f, 6.0f},
    {6.0f, 4.0f, 1.0f, 3.0f, 8.0f, 4.0f},
    {3.5f, 0.0f, 3.0f, 0.0f, 9.0f, 9.0f},
    {9.0f, 9.0f, 0.0f, 0.0f, 6.0f, 3.0f},
    {7.5f, 5.0f, 0.0f, 1.0f, 7.0f, 3.0f},
    {2.0f, 0.0f, 0.0f, 0.0f, 5.0f, 3.0f},
    {2.0f, 0.0f, 0.0f, 0.0f, 5.0f, 2.0f},
    {2.0f, 0.0f, 0.0f, 0.0f, 5.0f, 2.0f},
    {2.0f, 0.0f, 0.0f, 0.0f, 5.0f, 2.0f}};

// T20 alignment mask per guild, using kAlignBit* from state_item.h (preferred sheet values, R35).
inline constexpr u8 kGuildAlignMask[kGuildCount] = {
    kAlignBitGood | kAlignBitNeutral | kAlignBitEvil,   // Nomad
    kAlignBitGood | kAlignBitNeutral | kAlignBitEvil,   // Warrior
    kAlignBitGood,                                      // Paladin
    kAlignBitEvil,                                      // Ninja
    kAlignBitEvil,                                      // Villain
    kAlignBitGood | kAlignBitNeutral | kAlignBitEvil,   // Seeker
    kAlignBitNeutral,                                   // Thief
    kAlignBitGood | kAlignBitNeutral | kAlignBitEvil,   // Scavenger
    kAlignBitGood | kAlignBitEvil,                      // Mage
    kAlignBitGood | kAlignBitNeutral | kAlignBitEvil,   // Sorcerer
    kAlignBitGood | kAlignBitNeutral | kAlignBitEvil,   // Wizard
    kAlignBitNeutral};                                  // Healer

// T8 race/guild chart as bitmask; bit g set means guild g (Guild enum order) is allowed.
// Race order: Human, Elf, Giant, Gnome, Dwarf, Ogre, Morloch, Osiri, Troll.
// CONFLICT: chart totals 63 marks, source comment says 108 (R36).
inline constexpr u32 kRaceGuildMask[9] = {
    0xFFFu,  // Human: all
    0xFE5u,  // Elf
    0x023u,  // Giant
    0xBF5u,  // Gnome
    0xEE3u,  // Dwarf
    0x083u,  // Ogre
    0x3B3u,  // Morloch
    0xEE1u,  // Osiri
    0x0D9u}; // Troll

// Quest tier and draw constants (R6, R7, R9, R11, R12).
inline constexpr i32 kQuestTierDivisor = 25;
inline constexpr i32 kQuestTierMax = 10;
inline constexpr double kItemBandDivisor = 1.5;
inline constexpr i32 kItemQuestMinGuildLevel = 15;   // fetch only when guildLevel > 15
inline constexpr i32 kLetOffMaxDraw = 5;             // let-off when rounded draw <= 5
inline constexpr i32 kMonsterRerollWeight = 9;       // R7 monster reroll if weight <= 9
inline constexpr i32 kItemRerollWeight = 4;          // R7 item reroll if weight <= 4
inline constexpr i32 kQuestRerollPercent = 75;       // R7 reject probability

// Hits and advance constants (R3, R4, R20).
inline constexpr i32 kConBonusThreshold = 16;        // R3 bonus range 1..CON-16 when CON > 16
inline constexpr i32 kPreMlIncrementDivisor = 5;     // R4 Int(Rnd*(AH/5)+1), applied at 2/3 chance
inline constexpr double kAdvanceGoldRate = 60.2;     // R20 Fn12B2 rate
inline constexpr i64 kAdvanceGoldDivisor = 10;       // R20 input guildGold/10

// ---- rule routines ----

// T4 / R1 swing count for a guild at a level (pure table lookup).
i32 SwingsFor(Guild guild, i32 level);

// R2 R3 R4 hit gain at this level (random increment; uses ML/AH/MH and CON bonus).
i32 HitsGain(Ctx& ctx, const Character& ch, const GuildMembership& m, Guild guild);

// T16 R27 attack and defence gained per level within interval and max level.
AdPair AdGain(Ctx& ctx, Guild guild, i32 level, const Character& ch);

// T6 R34 maximum attack and defence for a guild.
AdPair MaxAttackDefence(Guild guild);

// T6 R27 maximum attack and defence across joined guilds, each maxed independently.
AdPair CharacterAdCap(const Character& ch);

// R17 join cost: GuildJoinCost if another guild is held, else 0.
Currency JoinCost(Ctx& ctx, const Character& ch);

// T8 R15 race chart, stat minimums, alignment, and no cursed worn gear.
bool CanJoin(Ctx& ctx, const Character& ch, Guild guild);

// R15 R17 sets joined, charges JoinCost, logs join; re-joining a held guild is free.


// R22 resets experience to new level floor and rechecks equipment.
bool Forfeit(Ctx& ctx, Character& ch, Guild guild);

// R16 R19 R20 hands over quest item, checks gold and XP, advances level, rolls HP and stats.
bool MakeLevel(Ctx& ctx, GameState& game, Character& ch, Guild guild);

// R20 advance gold via Fn12B2 (rate 60.2 applied to guildGold/10).
Currency AdvanceGold(Currency guildGold, i32 level);

// R9 quest tier = round_half_even(level / 25).
i32 QuestTier(i32 level);

// R11 item quest band = Int(tier / 1.5) + 1.
i32 ItemQuestBand(i32 tier);

// R12 item quest starts when guildLevel > 15.
bool ItemQuestStarts(i32 guildLevel);

// R10 monster eligible while Floor <= tier and tier <= 10.
bool MonsterEligible(i32 monsterFloor, i32 tier);

// R6 R7 R13 let-off, reroll rules, random type and target. Returns a full GuildQuest (target needed, not only kind).
GuildQuest RollQuest(Ctx& ctx, const Character& ch, Guild guild);

// R8 item pick range first+1..last; span 0 gives first+1.
IndexRange ItemPickRange(i32 first, i32 last);

// R23 advances kill quest for the killer only.
void CountKill(Ctx& ctx, Character& killer, const Creature& creature);

// T12 refills Item.charges to max charges.
void Recharge(Item& item);

// R14 primary guild only may use guild item.
bool GuildCanUse(Ctx& ctx, const Character& ch, const Item& item);

// R14 any joined guild meeting UseLevel and bitmask may use item.
bool MeetsUseLevel(Ctx& ctx, const Character& ch, const Item& item);

// T2 R24 R25 compares level with GuildMaster.level, takeover at level+1, re-awards crest each pass.
void TestGuildmaster(Ctx& ctx, GameState& game, Guild guild, Character& ch);

// R5 guild with the winning rating for an ability index.
// UNKNOWN: stale "rating from" display quirk needs a last-winner field; not in Character, caller keeps it.
GuildChoice AbilityLeader(const Character& ch, GuildAbility ability);

// Quirks: ability chance; thieving folds gear term, perception uses WIS + gearWIS/4.
f32 AbilityChance(const Character& ch, GuildAbility ability);

// T29 appends a GuildLogEvent stamped with game.today. Text is supplied by caller (16 wordings).
void GuildLogWrite(GameState& game, GuildLogKind kind, Guild guild, const Str& text);

// T9 T10 title for the guild's current level (Nomad bands, Ninja ladder).
// UNKNOWN: Nomad band names other than Master Nomad; Ninja Instructor threshold.
Str GuildTitle(const GuildMembership& m, Guild guild);

}  // namespace mordor
