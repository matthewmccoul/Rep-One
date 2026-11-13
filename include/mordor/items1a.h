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

// R11 ApplyEquipCurse: sets slot curse flag if Cursed != 0 or alignment fit is Opposed
void applyEquipCurse(Ctx& ctx, Item& slot, Align charAlign);

// R16 test used by forced unequip: guild, guild level and stats only (no hands, no class cap)
bool equipProblem(Ctx& ctx, const Character& c, const Inventory& inv, const Item& item);


// R23 R89 found charges: max(0, n) with random +/- Int(rng x charges / 2)
// UNKNOWN: exact value of n and the sign roll are in Formulas section, not supplied







// R47 R48 give from pack: guild-issued, equipped, receiver full checks; receiver reads (R53)
GiveOutcome giveFromPack(Ctx& ctx, const Character& giver, Inventory& giverInv,
                         Character& receiver, Inventory& receiverInv, i32 slot);



// R59 R60 realign an item; refused while equipped
// FIXED: removed duplicate declaration of realignItem(Ctx&, Character&, Inventory&, i32, ItemAlign).
// Ambiguating declaration with a different return type already exists in an included header;
// that declaration is the one to use. Unable to verify its return type from the supplied index.


// R62 guild crest test; crests are refused before a price is quoted
bool isGuildCrest(const Item& item);
// R62 sell quote; returns false (no quote) for guild crests
bool sellQuote(Ctx& ctx, const Item& item, Currency& quote);


// R67 R87 city entry and guild-issued recharge: see townRecharge (API INDEX)
// R79 R80 guild use level and CanUse: see itemUseLevel and CanUse (API INDEX)


// R72 theft slot = Int(Rnd x 39) + 2, giving 2..40
// Original quirk: slot 40 is off the array, so one roll in 39 misses





Currency identifyFee(Ctx& ctx, const Item& item);
Currency sellPrice(Ctx& ctx, const Item& item);

}  // namespace mordor
