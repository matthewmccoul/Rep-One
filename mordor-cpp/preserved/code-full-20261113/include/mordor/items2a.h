#pragma once

// items2a: SRD c13p2 routines not already in the API INDEX.
// The source excerpt gives tables T-1..T-11 and notes, with no R-numbers,
// so comments cite the table id (T-n) instead.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/attributes.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"

namespace mordor {

// ---- constants ----

// T-1 pack slot numbering. Internal index 0 = player slot 1 (Hands).
inline constexpr i32 kHandsIndex = 0;
// T-1 usable pack items (array size kPackSlots = 40, usable 39).
inline constexpr i32 kUsableItemSlots = 39;

// Theft slot = Int(Rnd * kTheftSpan) + kTheftFirstSlot, giving 2..40.
inline constexpr i32 kTheftFirstSlot = 2;
inline constexpr i32 kTheftSpan = 39;

// Slime: roll 1..100; no effect when roll >= kSlimeBaseChance + kSlimePerLevel * dungeonLevel.
inline constexpr i32 kSlimeBaseChance = 50;
inline constexpr i32 kSlimePerLevel = 2;

// T-8 Seer band and carrier count.
inline constexpr i32 kSeerBands = 3;
inline constexpr i32 kSeerMaxCarriers = 3;

// T-8 stat ceiling = race Max + 5 (silent cap). Floor = race minimum.
inline constexpr i32 kStatCeilingAboveRace = 5;

// T-8 Asmodeus band: ItemDropLevel - 2 .. ItemDropLevel.
inline constexpr i32 kAsmodeusBandWidth = 2;

// T-9 spell buffer encoding.
inline constexpr i32 kBufferEmpty = -1;
inline constexpr i32 kBufferSpellBase = 40;

// T-4 SpecialType values (MDATA3).
inline constexpr i32 kSpecialStatMods = 1;
inline constexpr i32 kSpecialScrollOfSpells = 11;
inline constexpr i32 kSpecialTomeOfSpells = 12;
inline constexpr i32 kSpecialPotionOfYouth = 13;
inline constexpr i32 kSpecialDragonsBlood = 14;
// UNKNOWN: 25 may be a SpecialType or an item type. [c11] vs [c26].
inline constexpr i32 kSpecialAgeTypeTwentyFive = 25;

// T-4 spell point gains and age effects (days). 1 year = 365 days (kAgeStartDays = 16 * 365).
inline constexpr i32 kScrollSpellPoints = 50;
inline constexpr i32 kTomeSpellPoints = 200;

// T-7 guild item-use factors (other guilds UNKNOWN).
inline constexpr double kNomadUseFactor = 0.8;
inline constexpr double kHealerUseFactor = 1.5;
// Guild use level = max(1, (f + f * kGuildUseLevelAdd) * item.LevelScale).
inline constexpr double kGuildUseLevelAdd = 2.97;

inline constexpr i32 kRealignFarMultiplier = 2;

// T-8 chance clamps, indexed by ItemChance.
inline constexpr i32 kItemChanceMin[4] = {10, 5, 10, 25};
inline constexpr i32 kItemChanceMax[4] = {90, 80, 90, 90};

// ---- enums ----

// T-8 chance kinds.
enum class ItemChance : i32 {
    Identify = 0,
    Recognise,
    Rename,
    Destroy
};

// T-9 classification of a spell buffer value.
enum class BufferKind : i32 {
    Empty = 0,
    ForItem,
    ForSpell
};

// ---- rule routines ----

// T-1 pack slot numbering
bool isUsablePackIndex(i32 index);            // index in 1..kUsableItemSlots (store scan range)
i32 packIndexFromPlayerSlot(i32 playerSlot);  // player slot 1..40 -> internal index 0..39
i32 playerSlotFromPackIndex(i32 index);       // internal index 0..39 -> player slot 1..40

// T-3 item kind groups (numeric type, same scheme as MDATA3)
bool typeIsHands(i32 itemType);               // type 0
bool typeNeedsEquipToUse(i32 itemType);       // 1..22, 29, 32, 33, 34; type 0 is absent (quirk)
bool typeConsumedWhenEmpty(i32 itemType);     // 23..28, 30, 31

// T-2 wear class cap (MaxWornOfClass value)
// UNKNOWN: wear-class count is 15 [c27] or 16 [c26]; not needed for this routine.
// Quirk: cap -1 = no cap; cap 0 = cannot be worn (always full).
bool wearClassFull(i32 maxWorn, i32 wornCount);

// T-4 SpecialType effects
bool specialAppliesStatMods(i32 specialType);   // 1, 13, 14
i32 specialSpellPointGain(i32 specialType);     // 11 -> 50, 12 -> 200, else 0 (silent)
i32 specialAgeDays(i32 specialType);            // signed age delta in days; 13 -> -365, 14 -> -3650, 25 -> +28 (UNKNOWN); else 0
bool isGuildCrestType(i32 specialType);         // 500..511
i32 guildCrestType(i32 guildIndex);             // 500 + guild index (0..11)

// T-5 alignment fit
// CONFLICT: alignment encoding. Char Align (Good=0, Neutral=1, Evil=2) vs ItemAlign (Good=1, Neutral=2, Evil=3). Mapping UNKNOWN; this routine uses the ordinal distance.
// Unaligned item = FitClass::Full.
FitClass alignFit(Align charAlign, ItemAlign itemAlign);
bool fitCountsEffects(FitClass fit);            // true only for Full (opposed-fit curse is applied by applyEquipCurse)
i32 adjustAdForFit(FitClass fit, i32 raw);      // Full raw; Half round_half_even(raw / 2.0); Subtract -raw
i32 adjustStatModForFit(FitClass fit, i32 raw); // Full raw; Half floor(raw / 2.0); Subtract -raw

// T-6 identification levels
// Original codes count down (All = 0). Port maps 3 -> Barely, 2 -> Partial, 1 -> Mostly, 0 -> All.
IdLevel idLevelFromOriginal(i32 originalCode);

// T-7 guild item-use factor and minimum level
double itemUseFactorFor(Guild guild);
// CONFLICT: [c24] writes (f + 2.97) and rounds; [c10]/[c25] and engine give (f + f*2.97) unrounded. Unrounded preferred.
// UNKNOWN: what B is in [c24]. Result is max(1, ...).
double guildUseMinLevel(double itemUseFactor, i32 levelScale);

// T-8 chance clamps and thresholds
i32 clampItemChance(ItemChance kind, i32 raw);  // clamp to kItemChanceMin/Max[kind]
bool slimeTakesEffect(Ctx& ctx, i32 dungeonLevel);  // rolls ctx.rng.range(1, 100); true when roll < 50 + 2 * dungeonLevel
i32 rollTheftSlot(Ctx& ctx);                    // Int(Rnd * 39) + 2 -> 2..40
// UNKNOWN: slot 40 is off the 0..39 array per [c14]/[c28]; caller treats it as a miss.
i32 rollSeerBand(Ctx& ctx);                     // Rnd(3) -> 0..2
i32 seerCarrierCount(i32 n);                    // clamp(n, 1, kSeerMaxCarriers)
bool inAsmodeusBand(i32 itemDropLevel, i32 level);  // itemDropLevel - 2 <= level <= itemDropLevel
// Stat clamp for potion use: floor at race minimum, silent ceiling at race Max + 5.
i32 clampStatForPotion(i32 value, i32 raceMin, i32 raceMax);

// T-9 spell buffers
BufferKind classifyBuffer(i32 value);           // -1 empty; 0..39 item (pack index); >= 40 spell
i32 bufferForSpell(i32 spellIndex);             // 40 + spellIndex
i32 spellIndexFromBuffer(i32 value);            // value - 40; caller checks classifyBuffer first

// T-11 realign
// Quirk: uses Abs on the raw alignment index, not a true spectrum distance.
bool realignIsFullSpectrum(i32 fromAlignIndex, i32 toAlignIndex);  // abs(from - to) > 1
// round_half_even(0.72 * price * mult); mult 1 for neighbouring step, 2 for full spectrum.
Currency realignCost(Currency price, bool fullSpectrum);

}  // namespace mordor
