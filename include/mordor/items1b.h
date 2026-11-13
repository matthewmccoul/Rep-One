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

// ---- Pack [c2], [c4] ----
// Pack: slot empty test (internal index 1..39, ItemId 0 or null means empty)
bool slotEmpty(const Inventory& inv, i32 internalIndex);
// Pack: first free slot in 1..39, or -1 (FirstFreeSlot)
i32 firstFreeSlot(const Inventory& inv);
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
// Wear class: item can be worn at all (MaxWornOfClass != 0)
bool isWearable(const Ctx& ctx, const Item& it);
// Wear class: number of worn items in a class (WornOfClass)
i32 wornOfClass(const Ctx& ctx, const Inventory& inv, i32 wearClass);

// ---- Curse [c2], [c12], [c22], [c27] ----
// Curse: a slot with SlotCursed set cannot be unequipped (CanUnequip)
bool canUnequip(const Item& slot);
// Curse: item puts itself on as it lands (Cursed == 2)
bool attachesItself(const Item& it);
// Curse: equip-time curse: sets SlotCursed if item cursed or fit is Opposed (ApplyEquipCurse).
// FitClass mapping assumed: Fine=Full, Adjacent=Half, Opposed=Subtract.
void applyEquipCurse(Ctx& ctx, Item& slot, FitClass fit);
// Curse: uncurse precondition check (not cursed / not equipped)
UncurseResult checkUncurse(const Item& slot);
// Curse: uncurse a slot. UNKNOWN: effect on success not in SRD; assumed to clear SlotCursed.
UncurseResult uncurseSlot(Ctx& ctx, Item& slot);
// Curse: forced unequip when EquipProblem != None and not SlotCursed
void forcedUnequipCheck(Ctx& ctx, const Character& c, const Inventory& inv, Item& slot);

// ---- Drop, give, bank, buy [c16], [c26], [c28], [c30] ----
// Drop: guild-issued items and cursed worn items cannot be dropped
bool canDropSlot(const Item& slot);
// Give: refusal reason for giving slot to recipient inventory
GiveRefusal checkGive(const Item& slot, const Inventory& recipient);
// Give: move item from one pack slot to first free slot of recipient
bool giveSlot(Ctx& ctx, Inventory& from, i32 fromSlot, Inventory& to);
// Bank: equipped items cannot be banked
bool canBankItem(const Item& slot);
// Bank: deposit gold from hand to bank
BankResult depositGold(Ctx& ctx, Purse& purse, Currency amount);
// Bank: withdraw gold from bank to hand
BankResult withdrawGold(Ctx& ctx, Purse& purse, Currency amount);
// Bank: re-identify on retrieval. Port reads FIDELITY fn12F0 < M4EEE as probe > threshold [c28].
// UNKNOWN: arguments of fn12F0 not given; caller supplies the probe value.
bool shouldReidentifyOnRetrieval(i32 probe, i32 threshold);
// Party gold: move min(max(amount,0), from.goldOnHand) to receiver
i32 transferGold(Ctx& ctx, Character& from, Character& to, Currency amount, bool inDungeon);
// Buy: add item to a free slot and pay. price comes from store formulas (UNKNOWN [c30]).
BuyResult buyItem(Ctx& ctx, Purse& purse, Inventory& inv, const Item& item, Currency price);

// ---- Swap [c2], [c16] ----
// Swap: validate a swap between two pack slots (internal indices), with refusal reason
SwapResult checkSwap(const Inventory& from, i32 slotFrom, const Inventory& to, i32 slotTo);
// Swap: swap two pack slots (internal indices); refuses silently on range error
bool swapPackSlots(Ctx& ctx, Inventory& from, i32 slotFrom, Inventory& to, i32 slotTo);
// Swap: swap two player-numbered slots; spell buffers follow only if buffersFollow
bool swapPlayerSlots(Ctx& ctx, Character& c, Inventory& inv, i32 playerA, i32 playerB, bool buffersFollow);

// ---- Slime and destruction [c4], [c14] ----
// Slime: first roll. True if an effect applies (rng.below(100)+1 < 50 + dungeonLevel*2).
bool slimeStrikes(Ctx& ctx, i32 dungeonLevel);
// Slime: target internal slot 1..39 (rng.below(39)+1)
i32 slimeSlot(Ctx& ctx);
// Slime: destroy chance clamped to 25..90.
// Normal: (45 - levelFound) + (dex+dex)*2 - monDex*2.
// Paralysed: VbInt((45 - levelFound) + (dex+dex)/2.0 - monDex*2).
// CONFLICT: normal form uses *2 and paralysed form uses /2.0; both kept as written.
i32 destroyChance(i32 levelFound, i32 charDex, i32 monDex, bool paralysed);
// Slime: destructible iff SpecialType < 500 or SpecialType > 500 + GuildCount - 1
bool isDestructible(i32 specialType);
// Slime: gold taken = Int(Rnd * ((dungeonLevel+monAtt)*(3353.452*monLvl)) / max(1, guildLevel))
Currency slimeGold(Ctx& ctx, i32 dungeonLevel, i32 monAtt, i32 monLvl, i32 guildLevel);
// Slime: theft flags from score. value = score - monLvl*2; gold if value < 50; item if value <= 10.
// UNKNOWN: score definition not in SRD; caller supplies the destroy-chance value.
TheftFlags theftFlags(i32 score, i32 monLvl);
// Theft: player-numbered slot 2..40 (TheftSlot = Int(Rnd*39)+2)
i32 theftSlot(Ctx& ctx);
// Slime: full sequence (trigger, slot, destroy roll, gold, theft). Mutates inv.
SlimeResult rollSlime(Ctx& ctx, Inventory& inv, const SlimeInput& in);

// ---- Alignment fit and worth [c5], [c13], [c26] ----
// Worth: attack/defence worth. Fine=raw, Adjacent=round_half_even(raw/2.0), Opposed=-raw.
i32 attDefWorth(i32 raw, FitClass fit);
// Halving for attack/defence. CONFLICT: text says Int(x)/2 with half-to-even, but examples
// (5->2, 7->4) are round_half_even(x/2.0); implemented as the examples.
i32 attackDefenceHalved(i32 x);
// Halving for stat modifiers: floors (5->2, 7->3)
i32 statModHalved(i32 x);
// Effect bits count iff item is unaligned or itemAlign-1 == charAlign [c26]
bool effectBitCounts(ItemAlign itemAlign, Align charAlign);

// ---- Gear effects [c26] ----
// Effect bits: OR of (Abilities & 0x3F) over counting equipped items
u32 gearEffects(const Ctx& ctx, const Character& c, const Inventory& inv);
// Effect bits: test a bit
inline bool hasEffect(u32 effects, EffectBit bit) {
    return ((effects >> static_cast<u32>(bit)) & 1u) != 0u;
}

// ---- Combine [c28] ----
// Combine: item type can be combined (types 23..28, 30, 31)
bool isCombinableType(i32 typeNumber);
// Combine: compatibility and overflow check of first with next
CombineCheck checkCombine(const Ctx& ctx, const Item& first, const Item& next);
// Combine: first.charges = first+next charges; next.id cleared. Returns refusal or Ok.
CombineCheck combineCharges(Ctx& ctx, Item& first, Item& next);

// ---- Use [c11], [c26] ----
// Use: item must be equipped to use (types 1..22, 29, 32..34)
bool mustBeEquipped(i32 typeNumber);
// Use: item is consumed when empty (types 23..28, 30, 31)
bool isConsumedType(i32 typeNumber);
// Use: gate check before using internalSlot
UseCheck checkUse(const Ctx& ctx, const Character& c, const Inventory& inv, i32 internalSlot, bool inDungeon);
// Use: remove a consumed-type item from its slot when its charges are empty
void removeIfExhausted(Ctx& ctx, Inventory& inv, i32 internalSlot);
// Guild-issued recharge: rule is Recharge(Item&) in guilds2.h (already declared).

// ---- Special effects [c11], [c26] ----
// Special: apply a SpecialType effect (1, 11, 12, 13, 14) to a character
void applySpecialType(Ctx& ctx, Character& c, const ItemDef& def, i32 specialType);
// Special: Younger(years): age -= years*365; clamp at 16*365 with message
void youngerBy(Ctx& ctx, Character& c, i32 years);
// Stat mods: apply a modifier to one stat; reversed when two steps away. Returns new stat.
// UNKNOWN: Hall of Records check on each raise [c11] not specified further.
i32 applyStatMod(Ctx& ctx, Character& c, StatIndex stat, i32 mod, bool reversed);

// ---- Identification and rename [c12], [c18], [c19], [c29] ----
// Identify: difficulty = 25 + (Log(DamageMod+1)+0.3) * Log(LevelScale+1) * Floor. No floor at 0.
f64 identifyDifficulty(const ItemDef& def);
// Identify: highest readable identification level given skill (ReadableLevel)
IdLevel readableLevel(const ItemDef& def, i32 identifySkill);
// Identify: per-tick identify chance clamped to 10..90
i32 identifyChance(f64 skill, bool dark);
// Identify: per-tick recognise chance clamped to 5..80
i32 recogniseChance(f64 skill, bool dark);
// Rename: chance clamped to 10..90
i32 renameChance(f64 identifySkill, bool inFog);
// Identify: roll the identify chance for this tick
bool rollIdentifyTick(Ctx& ctx, f64 skill, bool dark);
// Identify: roll the recognise chance for this tick
bool rollRecogniseTick(Ctx& ctx, f64 skill, bool dark);
// Rename: roll the rename chance. UNKNOWN: success roll assumed rng.percent(p).
bool rollRename(Ctx& ctx, f64 identifySkill, bool inFog);
// Item name at ID level: All=full+tag+charges+(cursed), Mostly=name, Partial=type,
// Barely=vaguer text. UNKNOWN: Barely wording not given [c12].
Str itemNameAtLevel(const Ctx& ctx, const Item& it, IdLevel level);

// ---- Alignment flags and found alignment [c13] ----
// Alignment: allowed alignments from AlignmentFlags (bit 0..3); empty gives Unaligned
Vec<ItemAlign> allowedAlignments(u8 alignmentFlags);
// Alignment: found alignment. foundAlign <= 1 gives Unaligned; else uniform among set bits.
ItemAlign rollFoundAlignment(Ctx& ctx, u8 alignmentFlags, i32 foundAlign);
// Found charges: base +/- Int(rng*base/2) (clamped at 0); base <= 0 gives 0
i32 rollFoundCharges(Ctx& ctx, i32 baseCharges);

// ---- Asmodeus drop band [c1] ----
// FIXED: this declaration conflicted with an existing asmodeusDropBand(i32) declaration
// elsewhere in the project (ambiguating new declaration). Removed here; the existing
// declaration is authoritative. Intended meaning: low = itemDropLevel-2 , top = itemDropLevel
// (kAsmodeusBandBelow), returned as DropBand.

// ---- Type names [c26] ----
// Type name: ItemDescriptions[ItemTypes[kind].Index]
Str itemKindName(const Ctx& ctx, i32 kind);

// ---- Seer carrier [c20] ----
// Seer: band offset 0..2 (Rnd(3))
i32 rollSeerBand(Ctx& ctx);
// Seer: treasure entry matches item by type and drop level window
bool seerMatchesByType(i32 entry, const ItemDef& item, i32 band, i32 monsterDropLevel);
// Seer: treasure entry names the item (entry < -1 and -entry-2 == itemId)
bool seerMatchesById(i32 entry, i32 itemId);
// Seer: number of carriers taken from n: Round(n - Rnd*n), clamped to 1..min(3,n)
i32 seerTakeCount(Ctx& ctx, i32 n);

// ---- Spell buffers [c20] ----
// Spell buffer value for a pack slot (ForItem)
inline i32 bufferForItem(i32 slot) { return slot; }
// Spell buffer value for a spell index (ForSpell)
inline i32 bufferForSpell(i32 spellIndex) { return kSpellBufferSpellBase + spellIndex; }

// ---- Lock-opening spell [c27] ----
// Lock: spell from SpellDefs[M56C4-1]; null if M56C4 is 0.
// UNKNOWN: which Item field holds M56C4 (spellIndex assumed).
const SpellDef* lockOpeningSpell(const Ctx& ctx, const Item& it);

// ---- Realign [c22] ----
// Realign: cost = round_half_even(0.72 * price * mult); mult 2 if |slot - (store+1)| > 1.
// Price rounding and other store prices (modItemCost, identifyFee, sellPrice, uncurse fee)
// are UNKNOWN [c30].
i64 realignCost(i64 basePrice, i32 slotAlignIndex, i32 storeAlignIndex);
// Realign: refused while equipped
bool canRealign(const Item& slot);

}  // namespace mordor
