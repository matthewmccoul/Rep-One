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

// UNKNOWN: aging of Tome and effect of Aard of Being not stated in this part [c91]

// CONFLICT: healing item level is item spell level (original) vs level 1 (port) [c34]. Refused below IdLevel.All for spell items (port) [c39]
i32 itemUseProblem(const Ctx& ctx, const Character& c, const Item& item);  // FIXED: ItemUseProblem is declared nowhere; using i32


i64 priceOfItem(const Ctx& ctx, const Item& item, ItemAlign alignment);      // PriceOf; prices are Long [c33][c52]
// UNKNOWN: identify fee approximates 0.10-0.20 of base market value (wiki) or sellValue (Clerk's Rule); sellValue chosen [c87]
i64 identifyFee(const Ctx& ctx, const Item& item);
// UNKNOWN: identify-item effect on IdLevel is not stated; only the fee is [c52]
i32 identifyItem(Ctx& ctx, Character& c, Inventory& inv, i32 slot);  // FIXED: ServiceOutcome is declared nowhere; using i32

// ---- Companions: binding, identification and Pen pricing (Confinement) ----
inline bool isFreeWill(const Companion& comp) { return comp.bindingLevel < 0; } // free will cannot be bound [c50]


// Not declared here, covered elsewhere: thieving ability (chanceForAbility), Crest award (awardCrest),
// Library grade on give (UNKNOWN: condition not stated), Key and Charm of Opening (spell data).

}  // namespace mordor
