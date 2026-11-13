#pragma once
#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"

namespace mordor {

// ---- constants (R2..R22) ----
inline constexpr i32 kBaseAD = 10;                  // R11 BaseAD, every character starts 10/10
inline constexpr i32 kCreationPoolBase = 30;        // R3 hard-coded creation pool constant
inline constexpr i32 kFreePerStat = 5;              // R2 free points per stat above racial minimum
inline constexpr i32 kStartHits = 15;               // R5 creation MaxHits = CurHits (flat, screenshots)
inline constexpr i32 kHitsFloor = 15;               // R19 MaxHits floor after forfeit
inline constexpr i32 kHitsCap = 999;                // R18 UNKNOWN: 999 cap appears only in the audit
inline constexpr i32 kStartGoldWhole = 1500;        // R5 GoldOnHand in whole gold (use currencyFromWhole)
inline constexpr i32 kStartCombatSetting = 1;       // R5 UNKNOWN: no CombatSetting field on Character
inline constexpr i32 kStartArea = -1;               // R5 Area set to -1
inline constexpr i32 kConLossBase = 50;             // R22 percent chance = 50 - Con*2
inline constexpr i32 kConLossPerPoint = 2;          // R22
inline constexpr u8 kDefaultAlignMask = 7;          // R9 race AlignMask default; bits 0 Good, 1 Neutral, 2 Evil
// CONFLICT: R9 bit order (Good=bit0, Neutral=bit1, Evil=bit2) disagrees with kAlignBit* in state_item.h (Good=0x02, Neutral=0x04, Evil=0x08); engine preferred.
inline constexpr i32 kHitsGainConThreshold = 16;    // R18 natural CON must exceed this for the CON term
inline constexpr i32 kHitsGainNum = 2;              // R18 probability numerator of the AH-term (2/3)
inline constexpr i32 kHitsGainDen = 3;              // R18 probability denominator
inline constexpr i32 kAvgHitsDivisor = 5;           // R18/R19 "up to a fifth" divisor
inline constexpr i32 kForfeitRollSides = 3;         // R19 rng.Next(3) + 1
inline constexpr i32 kForfeitRollHitMax = 2;        // R19 branch applies when roll is 1 or 2
// R23/R24 racial min/max tables are static data in GameData.races; not redeclared here.

// ---- small enums and result structs ----
enum class FitClass : i32 { Full = 0, Half = 1, Subtract = 2 };   // R9 FitFor result
enum class AdKind : i32 { Attack = 0, Defence = 1 };              // R13 WornSum selector
enum class ItemSize : i32 { Small = 1, Normal = 2, Big = 3, VeryBig = 4 };  // R9 item size values

struct AdPair {                                                   // R12/R13 (att, def) pair
    i32 att = 0;
    i32 def = 0;
};

struct StatAdBand {                                               // R14 Stats A/D band row
    StatIndex stat;
    i32 lo;
    i32 hi;
    i32 weight;
};

// R14 Stats A/D bands: STR 8..12 w2, DEX 8..12 w2, INT 8..14 w1, WIS 8..14 w1
inline constexpr StatAdBand kStatsAdBands[4] = {
    {StatIndex::Str, 8, 12, 2},
    {StatIndex::Dex, 8, 12, 2},
    {StatIndex::Int, 8, 14, 1},
    {StatIndex::Wis, 8, 14, 1},
};

struct GuildHitsRef {                                             // R20 reference row (talk page)
    Guild guild;
    i32 hitsPerLevel;
    i32 upToLevel;
};

// R20 UNKNOWN: talk-page values from 2006; GuildDef in GameData is authoritative. Other guilds not given.
inline constexpr GuildHitsRef kGuildHitsRef[3] = {
    {Guild::Warrior, 6, 28},
    {Guild::Nomad, 5, 30},
    {Guild::Paladin, 5, 29},
};

// R1 attribute storage and index order (STR=0, INT=1, WIS=2, CON=3, CHA=4, DEX=5): no routine.
// R6 bold display and R17 Char page report are text-screen rules: out of scope, not declared.
// R9 weight/speed (R10): UNKNOWN, no source defines them; not declared.

// ---- routines by rule ----

// R2 creation base for one stat: min(race.Max[stat], race.Min[stat] + 5)
i32 createBase(const Ctx& ctx, Race race, StatIndex stat);

// R3 unspent creation pool: 30 + race.BonusPoints - sum(base[i] - race.Min[i])
// UNKNOWN: race.BonusPoints values other than Human's implied 5 are not in the chunks.
i32 creationPool(const Ctx& ctx, Race race, const Arr<i32, kStatCount>& base);

// R4 spinner raise: if pool > 0 and stat < StatMax, increment stat and decrement pool
bool raiseStat(Ctx& ctx, Character& c, StatIndex stat);

// R4 spinner lower: if stat > race.Min, decrement stat and increment pool (may go below base)
bool lowerStat(Ctx& ctx, Character& c, StatIndex stat);

// R5 engine Create: StatMax from race Max, one membership at level 1 in startGuild,
// Experience 0, MaxHits = CurHits = 15, GoldOnHand 1500, Area -1, RecalcSP with refill.
// UNKNOWN: the starting guild is not named in the dossier; caller supplies it.
void createCharacter(Ctx& ctx, Character& c, Race race, Guild startGuild,
                     const Arr<i32, kStatCount>& stats);

// R6 displayed stat: Stats[stat] + StatMods[stat], no floor (used for checks)
i32 statWithMods(const Character& c, StatIndex stat);

// R7 wearing check: false if StatWithMods(i) + StatModWorth(item.StatsMod[i], fit) < race.Min[i] for any i
// CONFLICT: rule text mentions item stat requirements; formula compares against race.Min only.
// UNKNOWN: whether per-item stat requirements are also tested.
bool canWear(const Ctx& ctx, const Character& c, const Item& item);

// R8 effective stat: max(1, Stats[stat] + sum over equipped items of fitted StatModWorth)
i32 effectiveStat(const Ctx& ctx, const Character& c, const Inventory& inv, StatIndex stat);

// R9 alignment fit: matched or unaligned Full, adjacent Half, opposed Subtract
FitClass fitFor(ItemAlign itemAlign, Align charAlign);

// R9 fitted stat modifier for one item modifier value.
// CONFLICT: adjacent Half rounding ("one step gives half" [c2] vs "floored half" [c23]); engine body UNKNOWN.
i32 statModWorth(i32 mod, FitClass fit);

// R11/R12 guild A/D: guildAtt = BaseAD + ga, guildDef = BaseAD + gd (input is BestGuildAD parts ga, gd)
AdPair guildAD(const AdPair& bestGuildAD);

// R13 worn-gear sum of Att or Def over worn items (kind selects which)
i32 wornSum(const Ctx& ctx, const Inventory& inv, AdKind kind);

// R13 total A/D: att = guildAtt + statsAD - held + itemAtt; def = guildDef + statsAD - held + itemDef
// held = -combat.storedAdShift. Best guild award comes from the character's guild memberships.
AdPair totalAD(const Ctx& ctx, const Character& c, const Inventory& inv);

// R14 single-stat band term: w*(v-hi) if v > hi; w*(v-lo) if v < lo; else 0
// CONFLICT: engine gives negative values below band; archive says A/D only above band. Engine preferred.
i32 term(i32 v, i32 lo, i32 hi, i32 w);

// R14 Stats A/D from six stats with modifiers applied (indexed STR..DEX); CON and CHA excluded
i32 statsAD(const Arr<i32, kStatCount>& statsWithMods);

// R15 archive form: 2*(Str + Dex) + Int + Wis - 76 (valid only above band floors; unverified archive)
i32 statsADArchive(i32 str, i32 dex, i32 intel, i32 wis);

// R16 combat A/D (audit form): max(0, 10 + statAD + storedADShift + bestGuildAward + equippedItemAD)
// CONFLICT: port Char page formula [c1] has no max(0, ...) clamp; audit clamp kept. Port behaviour UNKNOWN.
i32 combatAD(const Ctx& ctx, const Character& c, const Inventory& inv);

// R18 HP gain on level-up in guild; refills CurHits, caps MaxHits at kHitsCap. Returns HP gained.
// UNKNOWN: AH/MH/HitsCapLevel/LateHits values; whether CON term is additive to AH term.


// R19 HP loss on forfeit of cost levels starting at from, for guild membership `guild`.
// UNKNOWN: census signature omits the guild; it is needed to exclude this membership from wasHighest.
// UNKNOWN: AvgHits, LateHits, HitsCapLevel values (read from GameData guild data).
void hitsOnForfeit(Ctx& ctx, Character& c, Guild guild, i32 from, i32 cost);

// R21 maximum spell points: 5 * (natural INT + natural WIS); no ctx (pure arithmetic)
i32 maxSP(const Character& c);

// R5/R21 recalculate MaxSP; if refill, CurSP = MaxSP, otherwise clamp CurSP to MaxSP
void recalcSP(Ctx& ctx, Character& c, bool refill);

// R22 percent chance to lose 1 natural CON: 50 - conNatural*2
// UNKNOWN: clamp at 0 is implied but not stated (Ogre at archive max 27 goes negative).
i32 conLossChance(i32 conNatural);

// R22 after a successful stat raise, roll the chance to lose 1 natural CON (gear excluded)
// Source is an archive page, unverified against the engine.
void statRaiseConRoll(Ctx& ctx, Character& c);

}  // namespace mordor
