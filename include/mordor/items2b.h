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

// R3 typed gold answer read as VB Val. Empty, sub-1 and non-numeric answers give 0.
// Quirk kept: "1e3" reads as 1000 (scientific notation accepted by Val).
i64 parseGiveGoldAnswer(const Str& answer);

// R4 typed slot answer to a 0-based pack index: max(0, CLng(Val(answer)) - 1).
i32 slotFromAnswer(const Str& answer);

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

// R8 receiver pack has a free slot.
bool hasFreePackSlot(const Inventory& inv);

// R4 empty pack slot.
// UNKNOWN: empty encoding. Assumed id == 0.
bool isEmptySlot(const Item& item);

// R7 receiver's knowledge of the item, re-read on give.
// CONFLICT: the re-read uses the wrong item in the original (c64). Reported as given.
IdLevel rereadKnowledge(const Ctx& ctx, const Character& receiver, const Item& item);

// R7 Library grade for a known level: All 4, Mostly 3, otherwise 0.
i32 libraryGradeFor(IdLevel known);

// R7 the Library grade rises only: returns max(currentGrade, grade for knownNow).
i32 libraryGradeAfterRead(i32 currentGrade, IdLevel knownNow);

// ---- Dragging a slot in town (R11-R17, R55) ----

// R12 dropping a dragged slot on a spell buffer empties that buffer. R11 any slot may be dragged.
DragOutcome dropOnBuffer(Ctx& ctx, Character& c, i32 bufferIndex);

// R14 full vault gives VaultFull. R16 Hands never go into the vault (HandsRefused).
// R17 guild-issued items are refused. R55 equipped items cannot be banked.
// Otherwise the item is stored in the first free bank slot (VaultStored).
DragOutcome dropOnVault(Ctx& ctx, Inventory& inv, i32 slot);

// R55 an item may be swapped to the Bank only if it is not equipped.
bool canBankItem(const Item& item);

// ---- Equip and unequip (R18-R25) ----

// R18 paralysed characters cannot equip (also gates R26 drop).
bool paralysedBlocksPackAction(const Character& c);

// R19 potions and scrolls cannot be worn.
// UNKNOWN: whether other non-wearable item types exist beyond Potion and Scroll.
bool isWearableType(ItemType type);

// R21-R23 toggle the Equipped flag of pack slot. Equipping applies the equip curse (R22),
// which sets the slot curse flag. Unequipping a cursed slot gives CursedLocked (R21).
// Unequipping clears the slot curse (R22). Effects are recomputed with existing routines.
// CONFLICT: the flag blocks unequip (c2) and is cleared by unequip (c27). The path that
// clears a flagged slot is UNKNOWN; the normal unequip is refused here.
// R20 WearRefusal is canWear() in attributes.h and is not redeclared.
EquipOutcome toggleEquip(Ctx& ctx, Character& c, Inventory& inv, i32 slot);

// R22 equip curse: sets the slot curse flag on the item.
void applyEquipCurse(Item& item);

// ---- Drop (R26-R32) ----

// R26 paralysed characters cannot drop (Paralysed).
// R27 guild items and crests cannot be dropped (GuildItem).
// R28 cursed worn item is refused (Cursed). Original checks the curse before asking.
// R30 port order: when kPortDropConfirmsFirst, confirmed must be true before the curse check.
// R31 a successful drop destroys the item and leaves nothing behind.
// R32 the slot index is the on-screen slot, so Hands and empty slots are not skipped.
DropOutcome dropPackItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot, bool confirmed);

// ---- Using an item (R33-R40) ----

// R33 paralysed characters are silently refused Use. An already buffered item skips this test.
bool paralysedBlocksUse(const Character& c, bool buffered);

// R35 ItemUse.Problem refusal and text. Empty text means silent refusal.
// UNKNOWN: the position of the dungeon-only refusal in the use order (c26).
// FIXED: itemUseProblem(const Ctx&, const Character&, const Item&) is already declared
// by an earlier header (ambiguating new declaration). The redeclaration that was here is
// removed so the earlier declaration is the only one. Its return type is not in the
// API INDEX, so it could not be checked. Confirm it matches R35 if callers need it.

// R33-R39 use the item at a pack slot. Order: guards (R33-R35), then spell check (R36),
// then the charge (R37), then the item's other effects. The charge is taken only when a
// spell cast goes through (R36). Stat items follow R38. Used-up items are removed (R39).
UseOutcome useItemFromPack(Ctx& ctx, Character& user, Inventory& inv, i32 slot, bool buffered);

// R36 item holds a spell, cast at the item's spell level.
// UNKNOWN: the source of the item's spell level (Item.level assumed).
// Returns true when the cast goes through.
bool castItemSpell(Ctx& ctx, Character& user, Item& item);

// R37 / R38 step 1 SpendCharge (mdrcharSubtractItemUse at line 6145).
// Returns true if the item now has zero charges.
// CONFLICT: zero-charge removal timing. c11 removes zero-charge items, c26 destroys them on
// the last charge. UNKNOWN which applies.
bool spendCharge(Ctx& ctx, Item& item);

// R37 ApplySpecial dispatch on the item's special type. Stat items are routed to applyStatItem.
// UNKNOWN: the full dispatch table (c26). Kind 25 uses useKind25 in progression2.h (R40).
void applyItemSpecial(Ctx& ctx, Character& user, Inventory& inv, i32 slot);

// R38 stat item (sub15CD). Steps: alignment test (2), then raise path (3) with the
// statCeiling clamp from races.h, or reversed item lowering of positive modifiers (4).
// The Hall of Records check at line 6219 is UNKNOWN in semantics and not separately declared.
// reversed comes from the item's reversed state, UNKNOWN in the dossier.
StatItemOutcome applyStatItem(Ctx& ctx, Character& user, Item& item, bool reversed);

// R38 step 2 alignment test (lines 6147-6152).
// CONFLICT: alignment encodings differ (c13 bits, c22 1..3, c26 M4EBF - 1). Mapping UNKNOWN.
bool statItemAlignmentPasses(const Character& user, const Item& item);

// R39 item used up: removes the item at the slot (lines 5695-5701).
void consumeUsedItem(Inventory& inv, i32 slot);

// ---- Items as combat orders (R41-R43) ----

// R41 an item used in a fight becomes the member's combat order (ItemOrder in Party).
void setItemOrder(Party& party, i32 memberIndex, i32 slot);

// R41-R42 each round the item is used and spends its charges.
// At zero charges the order clears to Fight.
// R43 a spell cast over an item order keeps draining it (kCastKeepsItemOrder).
ItemOrderStep itemOrderRound(Ctx& ctx, Party& party, i32 memberIndex, Inventory& inv);

// ---- Chest-found items (R44-R48) ----

// R44 opening a chest sets the slot curse flag from the drop's curse value.
// Any nonzero value (Cursed != CurseNone) sets it.
void applyChestDropCurse(Item& item);

// R45 items with Cursed == 2 attach. If wearable and canWear() passes, the item is
// equipped and the equip curse applied (applyEquipCurse). R46-R48 text is out of scope.
ChestAttach chestItemAttach(Ctx& ctx, Character& c, Inventory& inv, i32 slot);

// ---- Theft (R49) ----

// R49 theft pack slot: Int(Rnd * (40 - 1)) + 2 via ctx.rng.below.
// CONFLICT: see kTheftSlotFirst and kTheftSlotSpan. The formula is used as given.
i32 drawTheftSlot(Ctx& ctx);

// ---- Pack re-read on level gain (R50) ----

// R50 for index 1..kLastPackIndex: records the known level, rereads the item (Loot.OnReceived)
// and, if it rose, counts it. The caller announces and files the Library grade via
// libraryGradeFor. Returns the number of items whose known level rose.
i32 rereadPackOnLevelGain(Ctx& ctx, const Character& c, Inventory& inv);

// ---- Guild recharge on entering town (R51-R52) ----
// R51-R52 are townRecharge(ctx, inventory) in guilds1.h. Not redeclared.

// ---- Bank withdrawal (R53-R55) ----

// R53 the withdrawn item goes to the first free pack slot. If none, PackFull and it stops.
// R54 the readable level is re-read. If it rose, the item is re-identified and the caller
// logs and files it. R55 is canBankItem for the reverse direction.
WithdrawOutcome withdrawFromBank(Ctx& ctx, const Character& c, Inventory& inv, i32 bankIndex);

// ---- Swap Items (R56-R58) ----

// R56 parses "a,b". No comma, input longer than kSwapInputMaxChars, Hands (index 0),
// or an index past the last pack index are refused.
// Town refusals are silent and dungeon refusals are shown. The caller decides.
SwapRequest parseSwapRequest(const Str& input);

// R57 empty slots may be swapped. R58 buffersFollow is kTypedSwapBuffersFollow (false) for
// typed swaps and true for dragged swaps.
// CONFLICT: buffer remap on swap (c2, c20, c28). The caller chooses via buffersFollow.
SwapResult swapSlots(Ctx& ctx, Character& c, Inventory& inv, i32 a, i32 b, bool buffersFollow);

// ---- General Store (R59-R66) ----

// R59 a dungeon item, or an aligned copy of a floor-0 item, depletes the shelf.
// UNKNOWN: the floor-0 test (c30) looks at the floor alone.
bool buyDepletesShelf(bool dungeonItem, bool alignedFloorZeroCopy);

// R59 purchase. Checks for a free pack slot before gold (NoPackRoom), then charges via
// chargePurse (CannotAfford). Bought items are fully identified and carry stock's charges.
// Buying does not file into the Library.
BuyOutcome buyFromStore(Ctx& ctx, Character& buyer, Inventory& inv, const Item& stock,
                        Currency price, bool depletesShelf);

// R60 sale price from store pricing.
// UNKNOWN: the sell price formula (c30).
Currency storeSalePrice(const Ctx& ctx, const Item& item);

// R60 sell. Guild-issued items are refused. Equipped and cursed items are refused.
// Potions and charge items are sold in whole-item units.
// R61 shelved only if the item is not cursed and storeShelvesItem is true.
// R62 gold goes to the town-income hook or the seller. payeeCanReceive is the port check
// that refuses the sale if the payee cannot receive. The original has no overflow handler.
SaleOutcome sellToStore(Ctx& ctx, Character& seller, Inventory& inv, i32 slot,
                        Align storeAlign, bool payeeCanReceive);

// R61 the store accepts the item for this alignment (not cursed, and the store takes it).
// UNKNOWN: the store acceptance rule is not in the dossier.
bool storeShelvesItem(const Item& item, Align storeAlign);

// R61 a part-used item (charges below a whole item's charges) is not shelved.
bool isPartUsed(const Item& item);

// R63 identify charges a fee only for items not fully known.
bool needsIdentifyFee(const Item& item);

// R63 identify fee.
// UNKNOWN: the identify fee formula (c30). Only rounding behaviour (Long, half to even) is known.
Currency identifyFee(const Ctx& ctx, const Item& item);

// R63 identify. Fee is paid through chargePurse. Realign and identify share this fee path.
IdentifyOutcome identifyItem(Ctx& ctx, Character& payer, Item& item);

// R66 realign fee.
// UNKNOWN: the realign fee formula (c30).
Currency realignFee(const Ctx& ctx, const Item& item);

// R66 realign. Equipped items give Equipped ("Cannot realign Equipped item!").
// Otherwise a Yes/No confirm shows the cost (confirmed must be true to proceed).
RealignOutcome realignItem(Ctx& ctx, Character& payer, Inventory& inv, i32 slot,
                           ItemAlign target, bool confirmed);

// R64 uncurse fee for any item, cursed or not: roundHalfEven(0.8 * listPrice / 0.5).
// CONFLICT: see kUncurseFeeFactor. The quick reference formula is used.
Currency uncurseFee(Currency listPrice);

// R64 uncurse. The cursed check runs only when Uncurse is pressed. The refusal state depends
// on the slot curse flag and the equipped state.
// UNKNOWN: whether the fee is charged before the cursed check and the exact refusal mapping.
UncurseOutcome uncurseItem(Ctx& ctx, Character& payer, Inventory& inv, i32 slot);

// R65 combine is enabled once the named slot list is nonempty.
bool combineEnabled(const Vec<i32>& slots);

// R65 check of other against target. The conditions are listed in CombineRefusal.
// CONFLICT: message pairing (c8). UNKNOWN: alignment test text (c8).
CombineRefusal combineCheck(const Ctx& ctx, const Item& target, const Item& other);

// R65 combine the named slots into the first named item, which is the target.
// A single item does nothing silently (SingleItem). Refusals are reported by reason.


// ---- Confinement (R67-R73) ----

// R69 bind cost, quoted at the current binding level.
// UNKNOWN: the cost formula is not in the dossier.
Currency bindingCost(i32 bindingLevel);

// R67 free will (BindingLevel < 0) is refused (FreeWillRefused).
// R68 at or above kBindingMax the companion is already bound (AlreadyBound).
// R69 otherwise BindingLevel moves to the next level, capped at kBindingMax.
// The cost is quoted at the current level and charged via chargePurse.
// R70 the ceiling is retested on click. kFreeWillRetestedOnClick is false (port).
BindOutcome bindCompanion(Ctx& ctx, Character& payer, Companion& comp);

// R71 identify. Costs priceToIdentify(def) via chargePurse. OwnerIdLevel moves one step
// toward Completely (lower numeric value). The caller files the new level in the Library.
bool identifyCompanion(Ctx& ctx, Character& payer, Companion& comp, const MonsterDef& def);

// R72 realign is offered only at OwnerIdLevel::Completely.
bool canOfferRealign(const Companion& comp);

// R72 the two alignments the companion does not have.
Arr<CompanionAlignment, 2> realignOptions(const Companion& comp);

// R72 realign to alignment `to`. Priced by priceToRealign(def, current, to), charged via chargePurse.
bool realignCompanion(Ctx& ctx, Character& payer, Companion& comp,
                      CompanionAlignment to, const MonsterDef& def);

// R73 sell. A companion with hits == 0 is refused. Priced by priceToSell and paid to payer.
bool sellCompanion(Ctx& ctx, Character& payer, Companion& comp, const MonsterDef& def);

// Confinement prices used by R71-R73.
// UNKNOWN: formulas are not in the dossier. Only the call shapes are given.
Currency priceToIdentify(const MonsterDef& def);
Currency priceToRealign(const MonsterDef& def, CompanionAlignment from, CompanionAlignment to);
Currency priceToSell(const MonsterDef& def, CompanionAlignment align, OwnerIdLevel idLevel);

// ---- Exchange window (R74-R78) ----

// R74 global slot base for exchange side: side 0 gives 0..39 and side 1 gives 40..79.
i32 exchangeSlotBase(i32 side);

// R75 dropping a slot on another swaps them. Dropping a slot on itself does nothing (SameSlot).
// R76 a refusal shows a "Sorry!" box. If either end is Hands, nothing is said and nothing changes
// (HandsSilent).
// CONFLICT: the slot 0 refusal is referenced by R76 and not defined in the dossier.
// R77 redraw, R78 window cancel and save are out of scope.
ExchangeOutcome exchangeSwap(Ctx& ctx, Inventory& from, i32 fromSlot,
                             Inventory& to, i32 toSlot);

// ---- Gold charge (R80-R83) ----

// R80 purse is hand plus bank.
// R81 if hand plus bank is less than amount, the charge is refused and nothing is taken (false).
// R82 if hand covers amount, hand pays exactly amount.
// R83 otherwise rest = amount - hand, hand = 0, and bank -= rest.
bool chargePurse(Character& c, Currency amount);

// ---- Quick reference (no rule number) ----

// Quick reference wear cap: 0 = not wearable, -1 = uncapped, n = cap on worn items of the class.
// Returns true when wearing another item of the class is allowed.
// UNKNOWN: whether n counts items already worn of the class (wornOfClass < n assumed).
bool wearCapAllows(i32 cap, i32 wornOfClass);

// Quick reference spell index: spells[M56C4 - 1] if M56C4 > 0. Returns -1 when none.
// UNKNOWN: M56C4 is mapped to Item.spellId.
i32 castSpellRecordIndex(const Item& item);

}  // namespace mordor
