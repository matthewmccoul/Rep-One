// items2a.cpp: item-rule helpers (slot numbering, type groups, special types,
// alignment fit, identification, guild use levels, chances, theft/seer/slime
// rolls, stat clamps, spell buffers, realign).
#include "mordor/items2a.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {

// ---- Pack slot numbering (T-1) -------------------------------------------

// Store scan range: usable indices are 1..kUsableItemSlots (39).
// CONFLICT: 40-slot array vs 39 usable; resolved per header (index 0 is Hands).
bool isUsablePackIndex(i32 index) {
    return index >= 1 && index <= kUsableItemSlots;
}

// T-1: player slot 1 is internal index 0 (Hands).
// CONFLICT: Hands slot numbering; resolved by engine (internal 0 = player slot 1).
i32 packIndexFromPlayerSlot(i32 playerSlot) {
    return playerSlot - 1;
}

i32 playerSlotFromPackIndex(i32 index) {
    return index + 1;
}

// ---- Item type groups (T-3) ----------------------------------------------

// Type 0 is Hands.
bool typeIsHands(i32 itemType) {
    return itemType == 0;
}

// Must be equipped to use a spell: 1..22, 29, 32, 33, 34.
// Quirk: type 0 is absent from this list (kept; deliberate-or-off-by-one UNKNOWN).
bool typeNeedsEquipToUse(i32 itemType) {
    if (itemType >= 1 && itemType <= 22) {
        return true;
    }
    return itemType == 29 || itemType == 32 || itemType == 33 || itemType == 34;
}

// Consumed when empty: 23..28, 30, 31.
// CONFLICT: consumed-when-empty timing (removal on last use vs later check) UNKNOWN.
bool typeConsumedWhenEmpty(i32 itemType) {
    if (itemType >= 23 && itemType <= 28) {
        return true;
    }
    return itemType == 30 || itemType == 31;
}

// Wear class cap (T-2): -1 no cap; 0 cannot be worn; >0 one-per-class style cap.
// The equip routine caps only when max > 0 (fn11B6). Zero-class items are refused
// silently by the caller. UNKNOWN: the 0 case here returns true (nothing can be worn).
bool wearClassFull(i32 maxWorn, i32 wornCount) {
    if (maxWorn == -1) {
        return false;
    }
    if (maxWorn == 0) {
        return true;  // UNKNOWN: caller normally refuses zero-class items first.
    }
    return wornCount >= maxWorn;
}

// ---- Special types (T-4) --------------------------------------------------

// Stat modifiers apply for 1, 13, 14.
bool specialAppliesStatMods(i32 specialType) {
    return specialType == kSpecialStatMods ||
           specialType == kSpecialPotionOfYouth ||
           specialType == kSpecialDragonsBlood;
}

// 11 -> 50 spell points (Scroll of Spells); 12 -> 200 (Tome of Spells); else 0.
i32 specialSpellPointGain(i32 specialType) {
    if (specialType == kSpecialScrollOfSpells) {
        return kScrollSpellPoints;
    }
    if (specialType == kSpecialTomeOfSpells) {
        return kTomeSpellPoints;
    }
    return 0;
}

// Signed age delta in days: 13 -> -365, 14 -> -3650, 25 -> +28 (UNKNOWN), else 0.
// CONFLICT: type 25 is a SpecialType value or an item type (UNKNOWN); kept as stated in [c11].
i32 specialAgeDays(i32 specialType) {
    if (specialType == kSpecialPotionOfYouth) {
        return -365;
    }
    if (specialType == kSpecialDragonsBlood) {
        return -3650;
    }
    if (specialType == kSpecialAgeTypeTwentyFive) {
        return 28;  // UNKNOWN: magnitude and sign per [c11] only.
    }
    return 0;
}

// Guild crests are 500..511 (kGuildMarkerBase + guild index, kGuildCount guilds).
bool isGuildCrestType(i32 specialType) {
    return specialType >= kGuildMarkerBase &&
           specialType < kGuildMarkerBase + kGuildCount;
}

i32 guildCrestType(i32 guildIndex) {
    return kGuildMarkerBase + guildIndex;
}

// ---- Alignment fit (T-5) --------------------------------------------------

// Unaligned or same -> Full. One step -> Half. Two steps (Good<->Evil) -> Subtract.
// CONFLICT: alignment encoding. Item alignment is 1..3 (Good..Evil); the character
// index is item - 1 (per [c26] "M4EBF - 1 = char alignment"). Char Align: Good=0, Neutral=1, Evil=2.
FitClass alignFit(Align charAlign, ItemAlign itemAlign) {
    const i32 item = static_cast<i32>(itemAlign);
    if (item == 0) {
        return FitClass::Full;  // Unaligned
    }
    const i32 charIdx = static_cast<i32>(charAlign);
    const i32 itemIdx = item - 1;
    i32 diff = charIdx - itemIdx;
    if (diff < 0) {
        diff = -diff;
    }
    if (diff == 0) {
        return FitClass::Full;
    }
    if (diff == 1) {
        return FitClass::Half;
    }
    return FitClass::Subtract;
}

// Only Full counts effect bits; opposed-fit curse is applied by applyEquipCurse.
bool fitCountsEffects(FitClass fit) {
    return fit == FitClass::Full;
}

// Attack/defence: Full raw; Half round_half_even(raw / 2.0); Subtract -raw.
// Example: 5 -> 2, 7 -> 4.
i32 adjustAdForFit(FitClass fit, i32 raw) {
    switch (fit) {
        case FitClass::Full:
            return raw;
        case FitClass::Half:
            return static_cast<i32>(roundHalfEven(static_cast<double>(raw) / 2.0));
        case FitClass::Subtract:
        default:
            return -raw;
    }
}

// Stat mods: Full raw; Half floor(raw / 2.0); Subtract -raw. Example: 5 -> 2, 7 -> 3.
i32 adjustStatModForFit(FitClass fit, i32 raw) {
    switch (fit) {
        case FitClass::Full:
            return raw;
        case FitClass::Half:
            return static_cast<i32>(vbInt(static_cast<double>(raw) / 2.0));
        case FitClass::Subtract:
        default:
            return -raw;
    }
}

// ---- Identification (T-6) -------------------------------------------------

// Original codes count down: 0 All, 1 Mostly, 2 Partial, 3 Barely.
// UNKNOWN: codes outside 0..3 map to Unknown.
IdLevel idLevelFromOriginal(i32 originalCode) {
    switch (originalCode) {
        case 0:
            return All;
        case 1:
            return Mostly;
        case 2:
            return Partial;
        case 3:
            return Barely;
        default:
            return Unknown;
    }
}

// ---- Guild item use (T-7) -------------------------------------------------

// Nomad 0.8, Healer 1.5. Other guilds UNKNOWN; 1.0 used as the simplest sensible value.
double itemUseFactorFor(Guild guild) {
    switch (guild) {
        case Guild::Nomad:
            return kNomadUseFactor;
        case Guild::Healer:
            return kHealerUseFactor;
        default:
            return 1.0;  // UNKNOWN: factors for other guilds.
    }
}

// minLevel = max(1, (f + f * 2.97) * LevelScale). Unrounded per engine.
// CONFLICT: [c24] shows a rounding function B(...) over (f + 2.97) * LevelScale; the
// engine formula is preferred. UNKNOWN: what B is. The 3.97 product is kept as stated.
double guildUseMinLevel(double itemUseFactor, i32 levelScale) {
    const double level =
        (itemUseFactor + itemUseFactor * kGuildUseLevelAdd) * static_cast<double>(levelScale);
    return level < 1.0 ? 1.0 : level;
}

// ---- Chances (T-8) --------------------------------------------------------

// Clamp to kItemChanceMin/Max[kind]: Identify 10..90, Recognise 5..80, Rename 10..90, Destroy 25..90.
i32 clampItemChance(ItemChance kind, i32 raw) {
    const i32 idx = static_cast<i32>(kind);
    const i32 lo = kItemChanceMin[idx];
    const i32 hi = kItemChanceMax[idx];
    if (raw < lo) {
        return lo;
    }
    if (raw > hi) {
        return hi;
    }
    return raw;
}

// Roll 1..100; no effect when roll >= 50 + 2 * dungeonLevel. True when roll < threshold.
bool slimeTakesEffect(Ctx& ctx, i32 dungeonLevel) {
    const i32 roll = static_cast<i32>(ctx.rng.range(1, 100));
    return roll < kSlimeBaseChance + kSlimePerLevel * dungeonLevel;
}

// Int(Rnd * 39) + 2 -> 2..40. Slot 1 is never stolen (quirk, kept).
i32 rollTheftSlot(Ctx& ctx) {
    return static_cast<i32>(ctx.rng.below(kTheftSpan)) + kTheftFirstSlot;
}

// Rnd(3) -> 0..2.
// CONFLICT: rollSeerBand is also declared in items1b.h. Same signature; defined once here.
i32 rollSeerBand(Ctx& ctx) {
    return static_cast<i32>(ctx.rng.below(kSeerBands));
}

// clamp(n, 1, kSeerMaxCarriers).
i32 seerCarrierCount(i32 n) {
    if (n < 1) {
        return 1;
    }
    if (n > kSeerMaxCarriers) {
        return kSeerMaxCarriers;
    }
    return n;
}

// itemDropLevel - 2 <= level <= itemDropLevel.
bool inAsmodeusBand(i32 itemDropLevel, i32 level) {
    return itemDropLevel - kAsmodeusBandWidth <= level && level <= itemDropLevel;
}

// Ceiling race.Max + 5 (silent cap); floor at race minimum.
i32 clampStatForPotion(i32 value, i32 raceMin, i32 raceMax) {
    const i32 ceiling = raceMax + kStatCeilingAboveRace;
    i32 v = value;
    if (v > ceiling) {
        v = ceiling;
    }
    if (v < raceMin) {
        v = raceMin;
    }
    return v;
}

// -1 empty; 0..39 item (pack index); >= 40 spell.
// UNKNOWN: values below -1 are not produced by the original; treated as empty.
BufferKind classifyBuffer(i32 value) {
    if (value >= kBufferSpellBase) {
        return BufferKind::ForSpell;
    }
    if (value >= 0) {
        return BufferKind::ForItem;
    }
    return BufferKind::Empty;
}

// 40 + spellIndex.
// CONFLICT: items1b.h also defines an inline bufferForSpell with the same behaviour.
i32 bufferForSpell(i32 spellIndex) {
    return kBufferSpellBase + spellIndex;
}

// value - 40; caller checks classifyBuffer first.
i32 spellIndexFromBuffer(i32 value) {
    return value - kBufferSpellBase;
}

// Quirk: uses the raw alignment index difference, not a true spectrum distance.
bool realignIsFullSpectrum(i32 fromAlignIndex, i32 toAlignIndex) {
    i32 diff = fromAlignIndex - toAlignIndex;
    if (diff < 0) {
        diff = -diff;
    }
    return diff > 1;
}

// T-11: round_half_even(0.72 * Price), doubled when full spectrum.
// CONFLICT: items1a.h declares realignCost(Currency, i32 multiplier); this is the bool overload.
Currency realignCost(Currency price, bool fullSpectrum) {
    constexpr double kRealignPriceFactor = 0.72;
    double base = kRealignPriceFactor * static_cast<double>(price);
    if (fullSpectrum) {
        base = base * static_cast<double>(kRealignFarMultiplier);
    }
    return roundHalfEven(base);
}

}  // namespace mordor
