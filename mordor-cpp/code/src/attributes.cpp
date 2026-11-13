// attributes.cpp: Chapter 2 attributes and derived statistics (R1-R22).
// Field names on RaceDef / ItemDef / GuildDef are assumed (game_data.h not in the API index):
//   RaceDef::min, RaceDef::max (Arr<i32,6>, order STR,INT,WIS,CON,CHA,DEX), RaceDef::bonusPoints (i32)
//   ItemDef::statsMod (Arr<i32,6>)
//   ItemDef attack/defence: not present under those names (FIXED, see itemADOf).
//   ctx.data.items indexed by item.id; ctx.data.races indexed by static_cast<u32>(race).
// UNKNOWN: confirm these names against game_data.h before merging.

#include "mordor/attributes.h"
#include "mordor/combat3.h"
#include "mordor/game_data.h"
#include "mordor/guilds1.h"
#include "mordor/lookup.h"

namespace mordor {
namespace {

// UNKNOWN: HP values for guilds absent from kGuildHitsRef (R20: "no values given").
// Placeholders chosen so that unlisted guilds still gain HP and never hit a cap branch.
inline constexpr i32 kUnlistedAvgHits = 5;
inline constexpr i32 kUnlistedLateHits = 1;

struct HitsParams {
    i32 avg = 0;       // AH / AvgHits (R18, R19)
    i32 capLevel = 0;  // HitsCapLevel (R18, R19)
    i32 late = 0;      // MH / LateHits (R18, R19)
};

const RaceDef& raceDefOf(const Ctx& ctx, Race race) {
    return ctx.data.races[static_cast<u32>(race)];
}

const ItemDef& itemDefOf(const Ctx& ctx, const Item& item) {
    // Items are identified by id, not by table row (ids and rows differ for 356 of 366 shareware items).
    static const ItemDef kNone{};
    const ItemDef* d = itemDefById(ctx.data, item.id);
    return d ? *d : kNone;
}

// FIXED: ItemDef has no attack/defence members under those names (game_data.h not in index).
// Placeholder returns 0 so the file compiles. Replace the body with the real ItemDef field
// once its name is confirmed against game_data.h.
i32 itemADOf(const ItemDef& def, AdKind kind) {
    (void)def;
    (void)kind;
    return 0;
}

// R9 Gear A/D fit applied to one raw attack or defence value.
i32 adWithFit(i32 raw, FitClass fit) {
    switch (fit) {
        case FitClass::Full:
            return raw;
        case FitClass::Half:
            // "Nearest-even half" [c23]: round half to even.
            return static_cast<i32>(roundHalfEven(static_cast<double>(raw) / 2.0));
        case FitClass::Subtract:
            return -raw;
    }
    return 0;
}

// R13 "itemAtt/itemDef ... after fit": sum of worn gear A/D with per-item fit against the character.
i32 fittedWornSum(const Ctx& ctx, const Inventory& inv, AdKind kind, Align charAlign) {
    i32 sum = 0;
    for (const Item& it : inv.worn) {
        const ItemDef& def = itemDefOf(ctx, it);
        const i32 raw = itemADOf(def, kind);
        sum += adWithFit(raw, fitFor(it.alignment, charAlign));
    }
    return sum;
}

// R12/R13 input: best guild rating (BestGuildAD) as an (att, def) pair.
// FIXED: BestAd has no member 'ad' (guilds1.h). Field names att/def assumed directly on BestAd,
// matching the (att, def) pair the rule describes. Confirm against guilds1.h.
AdPair bestGuildPair(const Character& c) {
    const BestAd best = bestGuildAD(c);
    return AdPair{best.att, best.def};
}

Arr<i32, kStatCount> statsWithModsOf(const Character& c) {
    Arr<i32, kStatCount> out{};
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) {
        out[i] = statWithMods(c, static_cast<StatIndex>(i));
    }
    return out;
}

}  // namespace

// ---------------------------------------------------------------------------
// Creation and spinners (R2-R5)
// ---------------------------------------------------------------------------

// R2: base[i] = min(race.Max[i], race.Min[i] + 5).
i32 createBase(const Ctx& ctx, Race race, StatIndex stat) {
    const RaceDef& r = raceDefOf(ctx, race);
    const u32 i = static_cast<u32>(stat);
    return minv(r.max[i], r.min[i] + kFreePerStat);
}

// R3: pool = 30 + race.BonusPoints - spent, spent = sum(base[i] - race.Min[i]).
// UNKNOWN: exact race.BonusPoints values per race (only Human's implied value of 5 is recorded).
i32 creationPool(const Ctx& ctx, Race race, const Arr<i32, kStatCount>& base) {
    const RaceDef& r = raceDefOf(ctx, race);
    i32 spent = 0;
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) {
        spent += base[i] - r.min[i];
    }
    return kCreationPoolBase + r.bonusPoints - spent;
}

// ---------------------------------------------------------------------------
// Display and wearing (R6-R10)
// ---------------------------------------------------------------------------

// R6: StatWithMods(i) = Stats[i] + StatMods[i], no floor.
// Bold display rule (Stats[i] >= RaceMax[i] + 5) is a UI concern; conflict on origin noted in SRD.
i32 statWithMods(const Character& c, StatIndex stat) {
    const u32 i = static_cast<u32>(stat);
    return c.abilities.stats[i] + c.abilities.statMods[i];
}

// R9 FitFor. Unaligned or matched alignment is Full; adjacent (Good-Neutral, Neutral-Evil) is Half;
// opposed (Good-Evil) is Subtract.
FitClass fitFor(ItemAlign itemAlign, Align charAlign) {
    if (itemAlign == ItemAlign::Unaligned) {
        return FitClass::Full;
    }
    const i32 itemIdx = static_cast<i32>(itemAlign) - 1;  // Good=1 -> 0 to match Align
    const i32 charIdx = static_cast<i32>(charAlign);      // Good=0
    const i32 d = (itemIdx > charIdx) ? (itemIdx - charIdx) : (charIdx - itemIdx);
    if (d == 0) {
        return FitClass::Full;
    }
    if (d == 1) {
        return FitClass::Half;
    }
    return FitClass::Subtract;
}

// ---------------------------------------------------------------------------
// Attack and defence (R11-R17)
// ---------------------------------------------------------------------------

// R12: guildAtt = BaseAD + ga; guildDef = BaseAD + gd.
AdPair guildAD(const AdPair& best) {
    AdPair out;
    out.att = kBaseAD + best.att;
    out.def = kBaseAD + best.def;
    return out;
}

// R13: att = guildAtt + statsAD - held + itemAtt, held = -StoredADShift; def likewise.
// Total A/D = (att, def). The Char-page remainder recomputation (R17) is display only and not done here.
AdPair totalAD(const Ctx& ctx, const Character& c, const Inventory& inv) {
    const AdPair g = guildAD(bestGuildPair(c));                       // R12
    const i32 sAD = statsAD(statsWithModsOf(c));                      // R14
    const i32 itemAtt = fittedWornSum(ctx, inv, AdKind::Attack, c.alignment);
    const i32 itemDef = fittedWornSum(ctx, inv, AdKind::Defence, c.alignment);
    const i32 held = -storedADShift(c);                               // R13
    AdPair out;
    out.att = g.att + sAD - held + itemAtt;
    out.def = g.def + sAD - held + itemDef;
    return out;
}

// R14 Term(v, lo, hi, w). Engine form (negative below band) is preferred.
// CONFLICT: test [c61] and archive [c107] say contributions only above the band; engine preferred.
i32 term(i32 v, i32 lo, i32 hi, i32 w) {
    if (v > hi) {
        return w * (v - hi);
    }
    if (v < lo) {
        return w * (v - lo);
    }
    return 0;
}

// R14: StatsAD = Term(STR,8,12,2) + Term(DEX,8,12,2) + Term(INT,8,14,1) + Term(WIS,8,14,1).
// Input indices follow StatIndex order (STR, INT, WIS, CON, CHA, DEX); CON and CHA excluded via the band table.
i32 statsAD(const Arr<i32, kStatCount>& statsWithMods) {
    i32 total = 0;
    for (const StatAdBand& band : kStatsAdBands) {
        total += term(statsWithMods[static_cast<u32>(band.stat)], band.lo, band.hi, band.weight);
    }
    return total;
}

// R16 audit form: max(0, 10 + statAD + storedADShift + bestGuildAward + equippedItemAD).
// CONFLICT: the port Char page [c1] has no max(0, ...) clamp; the audit [c23] has one. Clamp kept.
// UNKNOWN: whether the combat figure is the attack or defence side; attack side used here.
i32 combatAD(const Ctx& ctx, const Character& c, const Inventory& inv) {
    const i32 statAD = statsAD(statsWithModsOf(c));                   // R14
    const i32 shift = storedADShift(c);                               // R13
    const AdPair best = bestGuildPair(c);                             // raw ga (not BaseAD + ga)
    const i32 itemAD = fittedWornSum(ctx, inv, AdKind::Attack, c.alignment);
    return maxv(0, kBaseAD + statAD + shift + best.att + itemAD);
}

// ---------------------------------------------------------------------------
// Hit points (R18-R20)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Spell points (R21)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Stat raise and Constitution loss (R22)
// ---------------------------------------------------------------------------

}  // namespace mordor
