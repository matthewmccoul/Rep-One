#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_item.h"
#include "mordor/state_character.h"
#include "mordor/attributes.h"
#include "mordor/races.h"
#include "mordor/progression2.h"

namespace mordor {

// ---- constants (SRD c13p1) ----
// FIXED: removed duplicate definition of kHandsSlot (R2 Hands is internal index 0);
// the same constant is already declared in an included header.
inline constexpr i32 kFirstItemSlot = 1;              // R1 first item slot (player slot 2)
inline constexpr i32 kLastItemSlot = kPackSlots - 1;  // R1 last item slot (player slot 40)
inline constexpr i32 kPlayerSlotOffset = 1;           // R1 player number = internal index + 1
inline constexpr i32 kHandsTotal = 2;                 // R2 / R6 hands a character has
inline constexpr i32 kSlotEmptyBuffer = -1;           // R66 / R76 buffer reset value
inline constexpr i32 kWearClassNoCap = -1;            // Table T-2 no cap (Miscellaneous Object)
inline constexpr i32 kWearClassUnwearable = 0;        // Table T-2 cannot be worn (silent exit)
inline constexpr i32 kSlimeRollMax = 100;             // R68 slime roll 1..100
inline constexpr i32 kSlimeRollBase = 50;             // R68 threshold = 50 + 2 x dungeonLevel
inline constexpr i32 kSlimeRollPerLevel = 2;          // R68
inline constexpr i32 kTheftSlotBase = 2;              // R72 theft slot = Int(Rnd x 39) + 2
inline constexpr i32 kTheftSlotSpan = 39;             // R72
inline constexpr i32 kDestroyFloorOffset = 2;         // R71 floor - 2 <= LevelFound
inline constexpr i32 kDestructibleSpecialMax = kGuildMarkerBase + kGuildCount - 1; // R70 (511)
inline constexpr i32 kSeerBandSides = 3;              // R74 band = Rnd(3)
inline constexpr i32 kSeerTakeMax = 3;                // R75 take 1..3
inline constexpr i32 kFoundChargesDivisor = 2;        // R89 spread = Int(rng x charges / 2)

// ---- result enums and small structs ----
enum class EquipOutcome {
    Ok,
    SilentRefusal,
    NoFreeHands,
    TooManyWorn,
    ClassRestricted,
    GuildRestricted,
    GuildLevelTooLow,
    StatsTooLow,
    CursedStatsTooLow
};

enum class UseOutcome {
    Silent,
    ItemNoEffect,
    SlotOutOfCharges,
    MustBeEquipped,
    NotInDungeon,
    NothingHappens,
    Used,
    UsedAndDestroyed
};

enum class DropOutcome {
    GuildIssued,
    HandsSlot,
    NeedsConfirm,
    CursedEquipped,
    Destroyed
};

enum class GiveOutcome {
    GuildIssued,
    Equipped,
    ReceiverFull,
    Given
};

enum class VaultGiveOutcome {
    Empty,
    GuildIssued,
    ReceiverFull,
    Given
};

enum class CombineOutcome {
    Refused,            // R54 refusal checked first (UNKNOWN: refusal conditions not itemised)
    NothingToCombine,   // R54 list of one item
    FirstNotCombinable, // R54
    NotIdentified,      // R54
    TypeMismatch,       // R54 (messages swapped in original, see Quirks)
    ItemIdMismatch,     // R54
    AlignMismatch,      // R54
    Combined            // R55
};

struct CombineResult {
    CombineOutcome outcome = CombineOutcome::Refused;
    i32 merged = 0;      // R55 items whose charges were added
    i32 overflowed = 0;  // R56 items whose charges were not added
};

enum class BuyOutcome {
    NoFreeSlot,     // R58
    NotEnoughGold,  // R58
    Bought          // R58
};

enum class RealignOutcome {
    Equipped,     // R59 refused while equipped
    Realigned,    // R60
    Refused,      // ADDED: refusal (invalid or empty slot, or equipped item)
    NeedsConfirm, // ADDED: R66 confirmation required before charging
    CannotAfford  // ADDED: realign fee not payable
};

enum class UncurseOutcome {
    NotCursed,    // R61 slot flag clear
    NotEquipped,  // R61 flag set but item not equipped
    Uncursed,     // R61
    Refused,      // ADDED: refusal (invalid or empty slot, or equipped item)
    CannotAfford  // ADDED: uncurse fee not payable
};

struct LevelBand {
    i32 lo = 0;
    i32 hi = 0;
};

// R1 pack is 40 slots; items live in slots kFirstItemSlot..kLastItemSlot
// R2 Hands (internal index 0) cannot be dropped, given or swapped
bool isHandsSlot(i32 internalIndex);
// R3 first free item slot scanning from internal index 1; -1 if none
i32 firstFreePackSlot(const Inventory& inv);
// R4 slot emptiness (UNKNOWN: defined in Formulas section, which is not supplied; port uses id == 0)
bool slotIsEmpty(const Item& item);

// R6 hands check: hands in use plus item hand width must not exceed kHandsTotal
bool handsCheckPasses(const Inventory& inv, i32 handWidth);
// R7 class cap check; kWearClassNoCap always passes, kWearClassUnwearable refuses silently
bool classCapPasses(const Ctx& ctx, const Inventory& inv, const Item& item);
// R10 stats check (UNKNOWN: which stat value is compared is listed under Unknowns)
bool statsRequirementMet(const Ctx& ctx, const Character& c, const Inventory& inv, const Item& item);
// R5 equip gate in order: hands, class cap, guild, guild level, stats; emits refusal message
// R8 guild check via CanUse (API); R9 guild level via MeetsUseLevel (API); R12 cursed stats message
// storeLike skips the hands and class checks (store, guild hall, editor)
EquipOutcome equipCheck(Ctx& ctx, const Character& c, const Inventory& inv, const Item& item, bool storeLike);
// R5 equip a pack slot if equipCheck passes; sets equipped and applies R11 curse
bool equipSlot(Ctx& ctx, Character& c, Inventory& inv, i32 slot, bool storeLike);
// R11 ApplyEquipCurse: sets slot curse flag if Cursed != 0 or alignment fit is Opposed
void applyEquipCurse(Ctx& ctx, Item& slot, Align charAlign);

// R13 a slot with the curse flag cannot be unequipped or dropped while worn
// R26 unequip refused while the slot curse flag is set
bool unequipAllowed(const Item& slot);
// R26 unequip a pack slot (refused while flagged)
bool unequipSlot(Ctx& ctx, Character& c, Inventory& inv, i32 slot);
// R14 generation curse: Cursed == 2 attaches to slot; chest treasure into pack sets flag
void applyGenerationCurse(Ctx& ctx, Item& item, bool fromChestTreasure);
// R15 unequip clears the slot flag so curse returns on re-equip (UNKNOWN: which path clears a flagged slot)
void clearSlotCurseOnUnequip(Ctx& ctx, Item& slot);
// R16 test used by forced unequip: guild, guild level and stats only (no hands, no class cap)
bool equipProblem(Ctx& ctx, const Character& c, const Inventory& inv, const Item& item);
// R16 R17 R18 forced unequip pass; joiningGuild collapses per-item lines into one summary line
void forcedUnequipPass(Ctx& ctx, Character& c, Inventory& inv, bool joiningGuild);

// R19 fit comes from fitFor (API INDEX); worth per fit for attack and defence (Table T-5)
i32 adWorthOfFit(i32 raw, FitClass fit);
// R21 alignment flags: bit 0 unaligned, bit 1 good, bit 2 neutral, bit 3 evil
bool alignFlagsAllow(u8 alignFlags, Align charAlign);
// R21 alignment field values 0 or 1 mean Unaligned
ItemAlign normaliseItemAlign(i32 rawAlignField);

// R22 Asmodeus only: drop band runs from ItemDropLevel - 2 up to ItemDropLevel
LevelBand asmodeusDropBand(i32 itemDropLevel);
// R23 alignment roll (UNKNOWN: RollAlignment body is not in the SRD text)
ItemAlign rollItemAlignment(Ctx& ctx, u8 alignFlags);
// R23 R89 found charges: max(0, n) with random +/- Int(rng x charges / 2)
// UNKNOWN: exact value of n and the sign roll are in Formulas section, not supplied
i32 rollCharges(Ctx& ctx, i32 charges);

// R77 reading difficulty compared with identify skill; never reaches All (see Quirks for numbering)
IdLevel readableLevel(Ctx& ctx, const Character& reader, const Item& item);
// R25 identification on pickup via the reading formula
void readOnPickup(Ctx& ctx, const Character& reader, Item& item);

// R29 incapacitated check (UNKNOWN: definition of incapacitated not given; port treats dead or paralysed)
bool isIncapacitated(const Character& c);
// R33 R81 item types that must be worn to use
bool isMustBeEquippedType(i32 itemTypeCode);
// R36 R81 item types consumed on last charge
bool isConsumedType(i32 itemTypeCode);
// R28 use sequence; R29 R30 R31 R32 R34 R39 refusals and checks; R38 special effects
// Order: incapacitated, negative SpecialType, charges, must-be-equipped, spell alignment,
//        charge spend, non-spell alignment, special effects
UseOutcome useItem(Ctx& ctx, Character& user, Inventory& inv, i32 slot, bool inDungeon);
// R35 spell fires only if item is unaligned or alignment matches exactly (fit Fine)
bool spellFiresOnUse(const Item& item, Align userAlign);
// R36 charge spend via mdrcharSubtractItemUse; returns true if item was destroyed
bool subtractItemUse(Ctx& ctx, Inventory& inv, i32 slot);
// R37 non-spell effect by fit: Adjacent gives "Nothing Happens."; Opposed reverses positive modifiers
void applyNonSpellFit(Ctx& ctx, Character& user, Item& item, FitClass fit);
// R38 special effects after spell (Table T-4 not supplied; R84 opposed skips spell-point and age effects)
void applySpecialEffects(Ctx& ctx, Character& user, Inventory& inv, i32 slot, bool reversed);
// R82 Younger(years): age -= years x 365, floored at kYoungestAgeYears
void applyYounger(Ctx& ctx, Character& user, i32 years);
// R85 R86 stat modifier: ceiling race.Max + 5 (statCeiling, API), floor race.Min, Hall of Records check
void applyStatModifier(Ctx& ctx, Character& user, StatIndex stat, i32 mod, bool reversed);

// R40 combat item becomes the order; casts and takes one charge per round; points charged only for level 0
void combatItemRound(Ctx& ctx, Character& who, Inventory& inv, i32 slot);

// R41 R42 R43 R44 drop: guild-issued refused, hands refused, equipped cursed refused after confirmation,
//     any other item destroyed
DropOutcome dropItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot, bool confirmed);

// R45 typed swap, player numbering; refused if either number <= 1 or > 40; does not remap buffers
bool swapPackTyped(Ctx& ctx, Inventory& inv, i32 playerSlotA, i32 playerSlotB);
// R46 dragged swap; buffers follow the item (UNKNOWN: whether every non-zero swap remaps)
void swapPackDragged(Ctx& ctx, Character& c, Inventory& inv, i32 slotA, i32 slotB);
// R76 a buffer pointing at an emptied pack slot resets to empty
void resetEmptiedBuffers(Character& c, i32 slot);
// R76 helper: swapped pack slots move buffers with them
void swapBufferSlots(Character& c, i32 slotA, i32 slotB);

// R47 R48 give from pack: guild-issued, equipped, receiver full checks; receiver reads (R53)
GiveOutcome giveFromPack(Ctx& ctx, const Character& giver, Inventory& giverInv,
                         Character& receiver, Inventory& receiverInv, i32 slot);
// R53 give path reading (Party.ReadGiven); UNKNOWN: library filing routine is not in SRD text
void giveRead(Ctx& ctx, Character& receiver, Item& slot);
// R52 exchange path reading (Loot.OnReceived); reading uses Items[LevelScale], see Quirks
void lootReadOnReceive(Ctx& ctx, const Character& receiver, Item& slot);
// R49 R50 R51 give from vault (bank slot index)
VaultGiveOutcome giveFromVault(Ctx& ctx, Inventory& giverInv, Character& receiver,
                               Inventory& receiverInv, i32 bankSlot);

// R54 R55 R56 R57 store combine; costs nothing; slots are accumulated by caller in lblCombine
CombineResult storeCombine(Ctx& ctx, Inventory& storeInv, const Vec<i32>& slots);

// R58 store buy: scan kFirstItemSlot..kLastItemSlot for blank; pay price
BuyOutcome storeBuy(Ctx& ctx, Character& buyer, Inventory& inv, const Item& stock, Currency price);

// R60 realign cost = round_half_even(kRealignFactor x price x multiplier)
Currency realignCost(Currency price, i32 multiplier);
// R60 realign multiplier: 1 for neighbouring step, 2 for full spectrum
// Original quirk: uses Abs on raw alignment index, not true spectrum distance
i32 realignMultiplier(ItemAlign from, ItemAlign to);
// R59 R60 realign an item; refused while equipped
// FIXED: removed duplicate declaration of realignItem(Ctx&, Character&, Inventory&, i32, ItemAlign).
// Ambiguating declaration with a different return type already exists in an included header;
// that declaration is the one to use. Unable to verify its return type from the supplied index.

// R61 uncurse an item from its slot flag
UncurseOutcome uncurseItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot);

// R62 guild crest test; crests are refused before a price is quoted
bool isGuildCrest(const Item& item);
// R62 sell quote; returns false (no quote) for guild crests
bool sellQuote(Ctx& ctx, const Item& item, Currency& quote);

// R63 typed amount: empty, non-numeric or zero is ignored; returns false if ignored
bool parseTypedAmount(const Str& text, Currency& amount);
// R63 deposit allowed unless amount > gold on hand or amount <= 0 (beep otherwise)
bool bankDepositAllowed(Currency amount, Currency goldOnHand);
// R63 withdrawal allowed unless amount > bank or bank < 0 (beep otherwise)
bool bankWithdrawAllowed(Currency amount, Currency bankGold);
// R64 give gold: moves min(max(amount, 0), giver gold); overflow moves nothing; returns amount moved
// FIXED: removed duplicate declaration of giveGold(Ctx&, Character&, Character&, Currency).
// Ambiguating declaration with a different return type already exists in an included header;
// that declaration is the one to use. Unable to verify its return type from the supplied index.
// R65 deposit an item into the bank; equipped items refused
bool depositItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot);
// R66 retrieve from bank; buffers pointing at the slot set to kSlotEmptyBuffer (not remapped)
bool retrieveFromBank(Ctx& ctx, Character& c, Inventory& inv, i32 bankSlot);
// R91 bank retrieval re-identify test: re-identifies when test is greater-than threshold
bool bankRetrievalReidentifies(Ctx& ctx, const Character& c, const Item& item);

// R67 R87 city entry and guild-issued recharge: see townRecharge (API INDEX)
// R79 R80 guild use level and CanUse: see itemUseLevel and CanUse (API INDEX)

// R68 slime trigger: roll 1..kSlimeRollMax; true if roll reaches 50 + 2 x dungeonLevel
bool slimeRollReaches(Ctx& ctx, i32 dungeonLevel);
// R69 R71 pick one pack slot in kFirstItemSlot..kLastItemSlot
i32 pickItemSlot(Ctx& ctx);
// R70 destructible if SpecialType < 500 or > kDestructibleSpecialMax
bool isDestructibleItem(const ItemDef& def);
// R70 destroy slot item if destructible; writes "SLIME: <Name> Destroyed!"; returns true if destroyed
bool slimeDestroy(Ctx& ctx, Inventory& inv, i32 slot);

// R71 monster destroy check on picked slot; returns true if item was destroyed
// UNKNOWN: destroy chance is in Formulas section, not supplied
bool monsterDestroyCheck(Ctx& ctx, Inventory& inv, i32 slot, i32 monsterFloor, i32 levelFound);
// R72 theft slot = Int(Rnd x 39) + 2, giving 2..40
// Original quirk: slot 40 is off the array, so one roll in 39 misses
i32 theftSlotRoll(Ctx& ctx);
// R72 theft decided by score - monsterLevel x 2 (UNKNOWN: score formula is in Formulas section)
bool theftDecided(Ctx& ctx, i32 score, i32 monsterLevel);
// R73 quicksand slot roll; acts if slot occupied (UNKNOWN: quicksand outcome)
void quicksandSlotRoll(Ctx& ctx, Inventory& inv);

// R74 seer carrier hit: band = Rnd(kSeerBandSides); type match and drop level in floor..floor+band; named is certain
bool seerCarrierHit(Ctx& ctx, i32 itemTypeCode, i32 dropLevel, i32 floor, bool named);
// R75 seer take count between 1 and kSeerTakeMax
i32 seerTakeCount(Ctx& ctx);

// R78 item name at each ID level (All full name with alignment tag, charges and cursed; Mostly name; Partial type)
Str itemDisplayName(const Ctx& ctx, const Item& item, IdLevel level);
// Quick reference identify chance: clamp(int((dark ? skill x 0.75 : skill) + 20), 10, 90)
// CONFLICT: API identifySkill returns i32, but this formula uses the fractional skill; skill is taken as f64
i32 identifyChance(f64 skill, bool dark);
// Quick reference recognise chance: clamp(int((dark ? skill : skill x 1.25) + 20), 5, 80)
i32 recogniseChance(f64 skill, bool dark);
// Quick reference rename chance: clamp(p, 10, 90), p = (inFog ? skill x 0.75 : skill) + 20
i32 renameChance(f64 skill, bool inFog);

// R88 gear effect bits: OR over equipped items that count under fit (Fine only)
// Critical Hit and Backstab have no effect (see Quirks)
u32 gearEffectBits(const Ctx& ctx, const Character& c, const Inventory& inv);

// R92 prices (modItemCost, identifyFee, sellPrice, uncurse fee): UNKNOWN, formulas not in SRD text
Currency modItemCost(Ctx& ctx, const Item& item);
Currency identifyFee(Ctx& ctx, const Item& item);
Currency sellPrice(Ctx& ctx, const Item& item);
Currency uncurseFee(Ctx& ctx, const Item& item);

}  // namespace mordor
