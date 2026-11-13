#pragma once
// items3a.h: SRD part c13p3, Items and Equipment.
// Covers the pack, Hands and bank layout, curses and attachment, wearing caps,
// dropping, giving and swapping, stat items, item use and charges, store
// services and companion binding. Declarations only.
// UNKNOWN: this SRD part gives no rule numbers; rules are grouped by topic and cited by source tag [cNN] or [terms].
// Already declared elsewhere and called, not redeclared: raiseStat (attributes.h, races.h),
// statCeiling (races.h), canWear, statWithMods, effectiveStat, townRecharge, awardCrest,
// identifySkill, chanceForAbility (thieving ability), Recharge, ageBy, useKind25.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_item.h"

namespace mordor {


// Guild crest six stat mods, in Stat enum order STR,INT,WIS,CON,CHA,DEX [c62]
inline constexpr i32 kCrestMods[kGuildCount][kStatCount] = {
    {1, 0, 0, 0, 0, 1}, {2, 0, 0, 2, 0, 2}, {2, 0, 0, 1, 2, 1}, {2, 0, 0, 0, 0, 3},
    {1, 1, 1, 1, 0, 2}, {2, 2, 2, 0, 0, 0}, {0, 0, 0, 0, 0, 4}, {2, 0, 0, 1, 0, 3},
    {0, 2, 2, 0, 2, 0}, {0, 3, 2, 1, 0, 0}, {0, 4, 2, 0, 0, 0}, {1, 2, 2, 0, 0, 1}};
// Guild crest Att and Def [c62]
inline constexpr i32 kCrestAttack[kGuildCount] = {6, 12, 6, 6, 9, 6, 3, 6, 0, 3, 3, 3};
inline constexpr i32 kCrestDefence[kGuildCount] = {6, 6, 12, 18, 9, 9, 6, 9, 12, 12, 12, 9};
// Guild crest ability mask; bits compared only for abilities [c62]
inline constexpr u32 kCrestAbilityMask[kGuildCount] = {0x0, 0x0, 0x4, 0x0, 0x0, 0x1, 0x0, 0x0, 0xA, 0x0, 0xE, 0x0};
// Guild crest cast spell name; empty means the crest casts nothing [c62]
inline constexpr const char* kCrestSpellName[kGuildCount] = {
    "", "", "Minor Heal", "", "Poison", "Displacement", "Charm of Opening", "Charm of Opening",
    "Soul Entrapment", "Pillar of Fire", "Power", "Restoration"};

// ---- Types ----
// Store shelf record; alignCounts holds counts for {U, G, N, E} [c35]
struct Shelf {
    i32 floor = 0;
    Arr<i32, 4> alignCounts{};
};

// Result of a typed give-gold answer. Units are whole gold, as CLng gives [c59]
struct GoldAnswer {
    bool refused = false;   // "Not enough gold!" [c59]
    i64 wholeGold = 0;      // amount when accepted [c59]
};

enum class DropProblem : i32 { None = 0, Hands, GuildIssued, GuildCrest };   // [c32][c44]
enum class WearRefusal : i32 { None = 0, NotWearable, ClassFull };          // [c56][c62]
enum class AttachOutcome : i32 { NotAttaching = 0, Attached, Refused };     // [c46]

// ---- Stat items and the stat ceiling ----
bool meetsStatItemRequirements(const Character& c);                          // needs 10 Wisdom and 10 Intelligence [c91]
// Stats(s) += item mods; clamp to race maximum + 5 (statCeiling); Hall of Records check [c32][c68]
void applyItemStatBonus(Ctx& ctx, Character& c, const Item& item);
void hallOfRecordsCheck(Ctx& ctx, Character& c, StatIndex stat, i32 value);  // mdrhalCheckRecord after the raise [c68]
// UNKNOWN: aging of Tome and effect of Aard of Being not stated in this part [c91]

// ---- Item use, item spells and charges ----
i32 itemSpellIndex(const Ctx& ctx, const Item& item);                        // M56C4: spells[M56C4-1]; -1 when none [c62]
// UNKNOWN: which field carries the item-level spell level versus caster level [c34]
i32 itemSpellLevel(const Ctx& ctx, const Character& user, const Item& item);
// UNKNOWN: meaning of p01AA not stated here; level-0 item casts charge SP unless p01AA != 0 [c34]
bool itemSpellChargesSP(i32 castLevel, bool p01AA);
// CONFLICT: healing item level is item spell level (original) vs level 1 (port) [c34]. Refused below IdLevel.All for spell items (port) [c39]
i32 itemUseProblem(const Ctx& ctx, const Character& c, const Item& item);  // FIXED: ItemUseProblem is declared nowhere; using i32
void spendCharge(Ctx& ctx, Inventory& inv, i32 slot);                        // SpendCharge on a charged item [c44]
bool castItemSpell(Ctx& ctx, Character& user, const Item& item);             // UseItemSpell [c44]
// Full use rule: paralysed return; problem; spell or spend charge; apply special [c44]
i32 useInventoryItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot);  // FIXED: ItemUseOutcome is declared nowhere; using i32
// Combine store service: merge charged items; totals above 32767 overflow and the game exits with code 6 (port) [c52]
i32 combineCharges(Ctx& ctx, Inventory& inv, i32 targetSlot, i32 sourceSlot);  // FIXED: CombineOutcome is declared nowhere; using i32

// ---- Money ----
// Hand first, rest from bank; refused if hand + bank < amount. Amount in Purse Currency units [c55]
// CONFLICT: original rounds the bank part to Long and can drive the bank negative [c55]
bool payFromPurse(Purse& purse, Currency amount);

// ---- Store pricing and services ----
i64 storeFeeFromPrice(i64 price);                                            // round_half_even(0.8 * Price / 0.5) [c52]
i64 priceOfItem(const Ctx& ctx, const Item& item, ItemAlign alignment);      // PriceOf; prices are Long [c33][c52]
i64 realignCost(const Ctx& ctx, const Item& item, ItemAlign current, ItemAlign wanted); // StoreStock.RealignCost [c52]
// UNKNOWN: identify fee approximates 0.10-0.20 of base market value (wiki) or sellValue (Clerk's Rule); sellValue chosen [c87]
i64 identifyFee(const Ctx& ctx, const Item& item);
bool floorStocksUnaligned(const Shelf& shelf);                               // floor 0 and counts[0] > 0 [c52][c35]
i32 shelfCountFor(const Shelf& shelf, ItemAlign alignment);                  // count for U, G, N or E [c35]
i32 sellItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot);   // FIXED: ServiceOutcome is declared nowhere; using i32. sale; no overflow handler, port refuses [c52]
// UNKNOWN: identify-item effect on IdLevel is not stated; only the fee is [c52]
i32 identifyItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot);  // FIXED: ServiceOutcome is declared nowhere; using i32
// Realign: equipped items refused; port does not recheck the current alignment block [c52]
i32 realignItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot, ItemAlign wanted);  // FIXED: ServiceOutcome is declared nowhere; using i32

// ---- Companions: binding, identification and Pen pricing (Confinement) ----
inline bool isFreeWill(const Companion& comp) { return comp.bindingLevel < 0; } // free will cannot be bound [c50]
i32 bindingTarget(i32 level, i32 step);                                      // min(level + step, kBindingMax) [c50]
// UNKNOWN: Binding.Cost formula not stated in this part [c50]
i64 bindingCost(i32 guildLevel, i32 level);
// UNKNOWN: Binding.Charge formula not stated in this part [c50]
i64 bindingCharge(i64 cost);
// Refuses free will, then already bound. Raises to bindingTarget; cost paid from purse [c50]
i32 bindCompanion(Ctx& ctx, Companion& comp, Purse& purse, i32 guildLevel, i32 step);  // FIXED: BindOutcome is declared nowhere; using i32
// Identify costs lower the owner ID level by 1; ID level 0 is a no-op in the port [c50]
i32 identifyCompanion(Ctx& ctx, Companion& comp, Purse& purse);  // FIXED: ServiceOutcome is declared nowhere; using i32
// UNKNOWN: Pen pricing formulas not stated in this part [c50]
i64 penPriceToSell(const Ctx& ctx, const Companion& comp);
i64 penPriceToIdentify(const Ctx& ctx, const Companion& comp);
i64 penPriceToRealign(const Ctx& ctx, const Companion& comp);

// ---- Hazards ----
// UNKNOWN: mapping of weapon name to resistance band not stated; item-to-band lookup chosen [c102]
bool weaponResisted(const Ctx& ctx, const Item& weapon, i32 dungeonLevel);
// Slime goo dissolves items: upper levels spare magical items, deeper levels destroy any item [terms]
// UNKNOWN: depth thresholds and slime chance not stated in this part
bool slimeDestroysItem(Ctx& ctx, const Item& item, i32 dungeonLevel);
// Theft slot draw: Int(Rnd * 39) + 2, range 2..40; slots 0 and 1 never drawn [c63]
// CONFLICT: slot 40 is one past the end; original relies on its error handler [c63]
i32 drawTheftSlot(Ctx& ctx);

// Not declared here, covered elsewhere: thieving ability (chanceForAbility), Crest award (awardCrest),
// Library grade on give (UNKNOWN: condition not stated), Key and Charm of Opening (spell data).

}  // namespace mordor
