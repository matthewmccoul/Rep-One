#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"
#include "mordor/attributes.h"

// SRD part c13p1: item rules (pack, equip, curse, swap, give/drop, slime and theft,
// alignment fit, gear effects, combine, use, special effects, identification,
// rename, bank, seer, realign, party gold).
// The SRD text for this part carries no numbered rules, so groups are labelled by
// SRD section with their source cite [cN].
// Item is the content of one pack slot. Slot fields (equipped, slotCursed, charges,
// idLevel) are read from Item.
// CONFLICT: Item.type is ItemType (enum 0..8) but the SRD numeric item type runs 0..34.
// Functions here take the SRD numeric type as i32 (typeNumber).
// CONFLICT: Item::isGuildItem() tests ailments >= 500, but the destructible rule
// protects only SpecialType 500..511 (GuildCount - 1 upper bound). isDestructible()
// follows the SRD.

namespace mordor {

inline constexpr i32 kSlimeBaseRoll = 50;
inline constexpr i32 kSlimeDungeonFactor = 2;
inline constexpr i32 kSlimeSlotDraw = 39;     // rng.below(39)+1 -> internal 1..39
inline constexpr i32 kDestroyBase = 45;
inline constexpr i32 kDestroyDexFactor = 2;
inline constexpr i32 kDestroyMonDexFactor = 2;
inline constexpr i32 kDestroyMin = 25;
inline constexpr i32 kDestroyMax = 90;
inline constexpr f64 kSlimeGoldCoef = 3353.452;
inline constexpr i32 kTheftScoreLimit = 50;   // gold theft if value < 50
inline constexpr i32 kItemTheftScoreLimit = 10; // item theft if value <= 10
inline constexpr i32 kMonLvlScoreFactor = 2;
inline constexpr i32 kTheftDraw = 39;         // TheftSlot = Int(Rnd*39)+2
inline constexpr i64 kChargesMin = -32768;

// Use [c11], [c26]
inline constexpr i32 kMustEquipTypeMin = 1;
inline constexpr i32 kMustEquipTypeMax = 22;
inline constexpr i32 kConsumedTypeMin = 23;
inline constexpr i32 kConsumedTypeMax = 28;
inline constexpr i32 kHandsTypeNumber = 0;

// Special effects [c11], [c26]
inline constexpr i32 kYearDays = 365;
inline constexpr i32 kYoungerFloorYears = 16;
inline constexpr i32 kYouthYears = 1;         // SpecialType 13
inline constexpr i32 kDragonsBloodYears = 10; // SpecialType 14
inline constexpr i32 kUseLevelMin = 1;
inline constexpr f64 kItemUseFactorNomad = 0.8;
inline constexpr f64 kItemUseFactorHealer = 1.5;

// Identification [c29], [c30]
inline constexpr f64 kIdentifyDifficultyBase = 25.0;
inline constexpr f64 kDamageModOffset = 1.0;
inline constexpr f64 kDamageModAddend = 0.3;
inline constexpr i32 kMostlyFloorMax = 5;
inline constexpr i32 kBarelyGap = 20;
inline constexpr i32 kChanceOffset = 20;
inline constexpr f64 kDarkIdentifyScale = 0.75;
inline constexpr f64 kRecogniseScale = 1.25;
inline constexpr f64 kFogRenameScale = 0.75;
inline constexpr i32 kRenameMin = 10;
inline constexpr i32 kRenameMax = 90;

// Asmodeus band and Seer [c1], [c20]
inline constexpr i32 kAsmodeusBandBelow = 2;  // band = ItemDropLevel-2 .. ItemDropLevel

// Spell buffers [c20]
inline constexpr i32 kSpellBufferEmpty = -1;
inline constexpr i32 kSpellBufferSpellBase = 40;

inline constexpr i32 kRealignNearMult = 1;
inline constexpr i32 kRealignFarMult = 2;
inline constexpr i32 kRealignFarGap = 1;      // |diff| > 1 is far

// Effect bits [c26]
enum class EffectBit : i32 {
    Levitate = 0,
    Invisible = 1,
    Protect = 2,
    SeeInvisible = 3,
    CriticalHit = 4,
    Backstab = 5
};
inline constexpr u32 kEffectMask = 0x3F;

// SpecialType values [c11], [c26]
enum class SpecialEffect : i32 {
    None = 0,
    StatMods = 1,
    SpellPoints50 = 11,
    SpellPoints200 = 12,
    PotionOfYouth = 13,
    DragonsBlood = 14
};

// ---- Result enums ----
enum class EquipProblem : i32 {
    None = 0,
    NoFreeHands,
    UsingTooMany,
    ClassRestricted,        // "Your current guild cannot use this item!"
    GuildsCannotUse,        // "Your currently joined guilds cannot use this item!"
    GuildLevelTooLow,       // "Your guild level is too low to use item!"
    AllGuildLevelsTooLow,   // "All guild levels too low to use item!"
    StatsTooLow,            // "One or more stats are too low!" (engine)
    TooWeak,                // "You are too weak to equip this item!" (FIDELITY)
    CursedStatsTooLow       // "Your stats are too low for the item to properly attach!"
};

enum class UncurseResult : i32 { Ok = 0, NotCursed, NotEquipped };

enum class SwapResult : i32 {
    Ok = 0,
    OutOfRange,
    SourceGuildIssued,
    DestGuildIssued,
    SourceEquippedOrBanked,
    DestEquippedOrBanked
};

enum class GiveRefusal : i32 { None = 0, GuildIssued, Equipped, NoFreeSlot };

enum class CombineCheck : i32 {
    Ok = 0,
    NotCompatibleClass,
    NotSameType,
    NotCompatibleAlignment,
    Overflow
};

enum class UseCheck : i32 { Ok = 0, NoEffect, OutOfCharges, MustBeEquipped, NotInDungeon };

enum class BankResult : i32 { Ok = 0, Beep };

enum class BuyResult : i32 { Ok = 0, NoFreeSlot, NotEnoughGold };

// ---- Small result structs ----
struct DropBand {
    i32 low = 0;
    i32 top = 0;
};

struct TheftFlags {
    bool gold = false;
    bool item = false;
};

struct SlimeInput {
    i32 dungeonLevel = 0;
    i32 monAtt = 0;
    i32 monLvl = 0;
    i32 monDex = 0;
    i32 levelFound = 0;
    i32 charDex = 0;
    i32 guildLevel = 0;
    bool paralysed = false;
};

struct SlimeResult {
    bool destroyed = false;     // slot item destroyed
    TheftFlags theft{};         // gold and/or item theft
    i32 slot = -1;              // internal slot hit; -1 = no effect
    Currency goldTaken = 0;
};

// Pack: hands in use. UNKNOWN: source of HandsUsed (Inventory.freeHands or derived)
i32 handsUsed(const Ctx& ctx, const Inventory& inv);
// FIXED: kHandsSlot was not declared in any scope visible to this header (compile error).
// Defined here as internal index 0, the Hands slot.
inline constexpr i32 kHandsSlot = 0;
// Pack: Hands slot test (internal index 0)
inline bool isHandsSlot(i32 internalIndex) { return internalIndex == kHandsSlot; }
// FIXED: kPlayerSlotFirst/kPlayerSlotLast were undeclared. Defined locally from the
// documented range below (2..40; 1 is Hands).
inline constexpr i32 kPlayerSlotFirst = 2;
inline constexpr i32 kPlayerSlotLast = 40;
// Pack: valid player-numbered slot for swap (2..40; 1 is Hands)
inline bool isValidPlayerSlot(i32 playerSlot) {
    return playerSlot >= kPlayerSlotFirst && playerSlot <= kPlayerSlotLast;
}

// ---- Equip checks [c2], [c10], [c26] ----
// Equip: first failing equip check, or EquipProblem::None
EquipProblem equipProblem(const Ctx& ctx, const Character& c, const Inventory& inv, const Item& it);
// Equip: verbatim refusal text for a problem (T-10)
Str equipProblemText(EquipProblem problem);
// Equip: wear the item in internalSlot; applies equip curse and attach-itself rules
bool equipItem(Ctx& ctx, Character& c, Inventory& inv, i32 internalSlot);

// ---- Wear class [c27] ----
// Wear class: class of an item (WearClassOf)
i32 wearClassOf(const Ctx& ctx, const Item& it);
// Wear class: class cap from MDATA1 ItemDescriptions[c].Index, 0 if out of range
i32 maxWornOfClass(const Ctx& ctx, i32 wearClass);
// Wear class: number of worn items in a class (WornOfClass)
i32 wornOfClass(const Ctx& ctx, const Inventory& inv, i32 wearClass);

// Curse: item puts itself on as it lands (Cursed == 2)

// Curse: equip-time curse: sets SlotCursed if item cursed or fit is Opposed (ApplyEquipCurse).
// FitClass mapping assumed: Fine=Full, Adjacent=Half, Opposed=Subtract.
void applyEquipCurse(Ctx& ctx, Item& slot, FitClass fit);

// Give: move item from one pack slot to first free slot of recipient
bool giveSlot(Ctx& ctx, Inventory& from, i32 fromSlot, Inventory& to);
// Bank: deposit gold from hand to bank
BankResult depositGold(Ctx& ctx, Purse& purse, Currency amount);




// Effect bits: test a bit
inline bool hasEffect(u32 effects, EffectBit bit) {
    return ((effects >> static_cast<u32>(bit)) & 1u) != 0u;
}


// Guild-issued recharge: rule is Recharge(Item&) in guilds2.h (already declared).


// Item name at ID level: All=full+tag+charges+(cursed), Mostly=name, Partial=type,
// Barely=vaguer text. UNKNOWN: Barely wording not given [c12].



// ---- Asmodeus drop band [c1] ----
// FIXED: this declaration conflicted with an existing asmodeusDropBand(i32) declaration
// elsewhere in the project (ambiguating new declaration). Removed here; the existing
// declaration is authoritative. Intended meaning: low = itemDropLevel-2 , top = itemDropLevel
// (kAsmodeusBandBelow), returned as DropBand.



// ---- Spell buffers [c20] ----
// Spell buffer value for a pack slot (ForItem)
inline i32 bufferForItem(i32 slot) { return slot; }
// Spell buffer value for a spell index (ForSpell)
inline i32 bufferForSpell(i32 spellIndex) { return kSpellBufferSpellBase + spellIndex; }



}  // namespace mordor
