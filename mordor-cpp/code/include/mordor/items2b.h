#pragma once
// items2b.h: SRD part c13p2 (Part B, Items and Equipment), rules R1-R83.
//
// Out of scope (text and screen only, not declared here): R9 name wording, R10 log text,
// R11 and R15 drag targets, R13 store naming text, R24 selection movement, R28-R29 captions,
// R46 attach sounds and text, R47 refusal text, R52 source location, R77 redraw,
// R78 window save and cancel, R79 display tables, R51 (townRecharge in guilds1.h),
// R40 (useKind25 in progression2.h), R20 (canWear in attributes.h).
// Routines return outcomes; callers emit message text through ctx.say.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"
#include "mordor/state_companion.h"
#include "mordor/state_party.h"
// FIXED: EquipOutcome, UseOutcome, DropOutcome, CombineOutcome, BuyOutcome, RealignOutcome and
// UncurseOutcome are declared in items1a.h, and SwapResult in items1b.h. Both were used here
// without being included.
#include "mordor/items1a.h"
#include "mordor/items1b.h"

namespace mordor {


// ---- constants ----

inline constexpr i32 kLastPackIndex = kPackSlots - 1;

// R56 swap input "a,b" longer than this many characters is refused.
inline constexpr i32 kSwapInputMaxChars = 5;

// R49 theft slot = Int(Rnd * (40 - 1)) + 2.
// CONFLICT: SRD says 39 of 40 slots are in play, but the formula reaches only 2..40.
// Slots 0 and 1 are unreachable. The formula is used as given.
// UNKNOWN: index 40 is past the last 0-based pack index (39).
inline constexpr i32 kTheftSlotFirst = 2;

// R7 library grades for a fully or mostly known item.
inline constexpr i32 kLibraryGradeAll = 4;
inline constexpr i32 kLibraryGradeMostly = 3;
inline constexpr i32 kLibraryGradeNone = 0;

// R25 port behaviour: no identification test on equip or use.
// CONFLICT: whether the original performs the test is disputed.
inline constexpr bool kEquipUseIdentifies = false;

// R30 port order: the drop confirmation comes first and the curse check afterwards.
// Deviation from the original, which checks the curse before asking (R28).
inline constexpr bool kPortDropConfirmsFirst = true;

// R58 typed swaps do not carry buffers with the swapped items.
// The dragged swap passes true (see swapSlots).
inline constexpr bool kTypedSwapBuffersFollow = false;

// R43 a spell cast over an item order keeps draining the item. M5766 is not reset.
inline constexpr bool kCastKeepsItemOrder = true;

// R70 port: free will is not retested on click.
inline constexpr bool kFreeWillRetestedOnClick = false;

// R64 uncurse fee = roundHalfEven(kUncurseFeeFactor * listPrice / kUncurseFeeDivisor).
// CONFLICT: the dossier's UNKNOWN list (c30) says uncurse fee formulas are not given,
// but the quick reference (c52) gives this one. The quick reference is used.
inline constexpr f64 kUncurseFeeFactor = 0.8;
inline constexpr f64 kUncurseFeeDivisor = 0.5;

// ---- result types ----

// R1-R6 give outcomes.
enum class GiveResult : i32 {
    Done,
    InCombat,
    SameCharacter,
    NotEnoughGold,
    NoItem,
    Equipped,
    SlotOutOfRange,
    GuildIssued,
    ReceiverPackFull
};

// R12, R14, R16, R17, R55 drag and drop outcomes.
enum class DragOutcome : i32 {
    BufferEmptied,
    VaultStored,
    VaultFull,
    HandsRefused,
    GuildRefused,
    EquippedRefused
};




// R41-R42 item combat order step.
enum class ItemOrderStep : i32 {
    NoOrder,
    Continues,
    RanOutOfCharges
};

// R38 stat item outcomes.
enum class StatItemOutcome : i32 {
    AlignmentRefused,
    Raised,
    Lowered
};

// R45 chest attach outcomes.
enum class ChestAttach : i32 {
    NoCurse,
    NotWearable,
    WearRefused,
    Attached
};

// R53 bank withdrawal outcomes.
enum class WithdrawOutcome : i32 {
    Withdrawn,
    PackFull
};


// FIXED: SwapResult has no NoComma member, so the parse status uses this local enum.
// R56 parse status for "a,b" input.
enum class SwapParse : i32 {
    NoComma,
    Refused,
    Swapped
};

// R56 parsed swap request. Indices are valid only when status is Swapped.
struct SwapRequest {
    SwapParse status = SwapParse::NoComma;
    i32 first = 0;
    i32 second = 0;
};

// R75-R76 exchange swap outcomes.
enum class ExchangeOutcome : i32 {
    Swapped,
    SameSlot,
    HandsSilent,
    OutOfRange
};

// R60-R62 sale outcomes.
enum class SaleResult : i32 {
    Sold,
    GuildIssued,
    EquippedCursed,
    PayeeRefused,
    EmptySlot
};

struct SaleOutcome {
    SaleResult result = SaleResult::EmptySlot;
    bool shelved = false;
    Currency gold = 0;
};


// R63 identify outcomes.
enum class IdentifyOutcome : i32 {
    Identified,
    NotNeeded,
    CannotAfford,
    EmptySlot
};



// R65 combine refusal reasons.
// CONFLICT: the dossier's combine message pairing (c8) is internally inconsistent.
// The conditions are listed here and the pairing is not encoded.
enum class CombineRefusal : i32 {
    NoRefusal,
    SingleItem,
    TypeDiffers,
    NotCompatibleClass,
    ItemIdDiffers,
    NotSameType,
    AlignmentTest
};

// FIXED: local struct CombineOutcome removed. The name is already declared elsewhere
// with a different kind, so combineItems returns that existing type.

// R67-R69 binding outcomes.
enum class BindOutcome : i32 {
    Bound,
    FreeWillRefused,
    AlreadyBound,
    CannotAfford
};

// R35 ItemUse.Problem result. An empty text means a silent refusal.
struct UseProblemResult {
    bool refused = false;
    Str text{};
};

// ---- Giving items and gold (R1-R8) ----



// R1 refused in combat. R2 self-give (giverIndex == receiverIndex) has no effect.
// R3 gold is moved first. wholeGold is in whole gold units.
// UNKNOWN: whether the typed amount compares to goldOnHand as whole gold
// (converted with currencyFromWhole) or in raw Currency. Whole gold is assumed.
GiveResult giveGold(Ctx& ctx, Character& giver, Character& receiver,
                    i32 giverIndex, i32 receiverIndex, i64 wholeGold, bool inCombat);

// R4 item from giver's pack slot to receiver's pack. Hands and empty slots give NoItem.
// Equipped items give Equipped. Slots past the last pack index give SlotOutOfRange.
// R5 guild-issued items give GuildIssued. Gold already moved by giveGold is not returned.
// R6 successful give updates both packs. R8 full receiver pack gives ReceiverPackFull.
GiveResult giveItem(Ctx& ctx, Character& giver, Inventory& giverInv, i32 giverIndex,
                    Character& receiver, Inventory& receiverInv, i32 receiverIndex,
                    i32 slot, bool inCombat);


// R4 empty pack slot.
// UNKNOWN: empty encoding. Assumed id == 0.
bool isEmptySlot(const Item& item);




// ---- Dragging a slot in town (R11-R17, R55) ----




// ---- Equip and unequip (R18-R25) ----




// R22 equip curse: sets the slot curse flag on the item.
void applyEquipCurse(Item& item);

// ---- Drop (R26-R32) ----


// ---- Using an item (R33-R40) ----


// R35 ItemUse.Problem refusal and text. Empty text means silent refusal.
// UNKNOWN: the position of the dungeon-only refusal in the use order (c26).
// FIXED: itemUseProblem(const Ctx&, const Character&, const Item&) is already declared
// by an earlier header (ambiguating new declaration). The redeclaration that was here is
// removed so the earlier declaration is the only one. Its return type is not in the
// API INDEX, so it could not be checked. Confirm it matches R35 if callers need it.








// ---- Items as combat orders (R41-R43) ----



// ---- Chest-found items (R44-R48) ----



// ---- Theft (R49) ----


// ---- Pack re-read on level gain (R50) ----


// ---- Guild recharge on entering town (R51-R52) ----
// R51-R52 are townRecharge(ctx, inventory) in guilds1.h. Not redeclared.

// ---- Bank withdrawal (R53-R55) ----


// ---- Swap Items (R56-R58) ----

// R56 parses "a,b". No comma, input longer than kSwapInputMaxChars, Hands (index 0),
// or an index past the last pack index are refused.
// Town refusals are silent and dungeon refusals are shown. The caller decides.



// ---- General Store (R59-R66) ----





// R61 the store accepts the item for this alignment (not cursed, and the store takes it).
// UNKNOWN: the store acceptance rule is not in the dossier.
bool storeShelvesItem(const Item& item, Align storeAlign);


// R63 identify charges a fee only for items not fully known.
bool needsIdentifyFee(const Item& item);

// R63 identify fee.
// UNKNOWN: the identify fee formula (c30). Only rounding behaviour (Long, half to even) is known.
Currency identifyFee(const Ctx& ctx, const Item& item);

// R63 identify. Fee is paid through chargePurse. Realign and identify share this fee path.
IdentifyOutcome identifyItem(Ctx& ctx, Character& payer, Item& item);







// R65 combine the named slots into the first named item, which is the target.
// A single item does nothing silently (SingleItem). Refusals are reported by reason.


// ---- Confinement (R67-R73) ----









// ---- Exchange window (R74-R78) ----



// ---- Gold charge (R80-R83) ----

// R80 purse is hand plus bank.
// R81 if hand plus bank is less than amount, the charge is refused and nothing is taken (false).
// R82 if hand covers amount, hand pays exactly amount.
// R83 otherwise rest = amount - hand, hand = 0, and bank -= rest.
bool chargePurse(Character& c, Currency amount);

// ---- Quick reference (no rule number) ----



}  // namespace mordor
