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
    // UNKNOWN: items table indexed directly by item.id.
    return ctx.data.items[static_cast<u32>(item.id)];
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

// R19/R20 lookup: HP parameters for a guild. Unlisted guilds use placeholders (UNKNOWN above).
HitsParams hitsParamsFor(Guild guild) {
    for (const GuildHitsRef& ref : kGuildHitsRef) {
        if (ref.guild == guild) {
            HitsParams hp;
            hp.avg = ref.hitsPerLevel;
            hp.capLevel = ref.upToLevel;
            hp.late = kUnlistedLateHits;  // UNKNOWN: LateHits value not in chunks.
            return hp;
        }
    }
    HitsParams hp;
    hp.avg = kUnlistedAvgHits;  // UNKNOWN
    hp.capLevel = 0;            // UNKNOWN: no cap known, so every level takes the above-cap branch.
    hp.late = kUnlistedLateHits;
    return hp;
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

// R4.2: while stat > Min: stat -= 1, pool += 1 (quirk: may go below starting base).
bool lowerStat(Ctx& ctx, Character& c, StatIndex stat) {
    const RaceDef& r = raceDefOf(ctx, c.race);
    const u32 i = static_cast<u32>(stat);
    i32& value = c.abilities.stats[i];
    if (value > r.min[i]) {
        value -= 1;
        c.pointsPool += 1;
        return true;
    }
    return false;
}

// R5: engine Create(...) state.
// UNKNOWN: CombatSetting field absent from Character; kStartCombatSetting is not stored.
// UNKNOWN: the starting guild name is not given in the dossier; caller supplies startGuild.
void createCharacter(Ctx& ctx, Character& c, Race race, Guild startGuild,
                     const Arr<i32, kStatCount>& stats) {
    const RaceDef& r = raceDefOf(ctx, race);
    c.race = race;
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) {
        c.abilities.stats[i] = stats[i];         // natural values
        c.abilities.statMax[i] = r.max[i];       // R5: StatMax copied from race Max
    }
    const u32 g = static_cast<u32>(startGuild);
    c.activeGuild = startGuild;
    c.guilds[g].joined = true;
    c.guilds[g].level = 1;                       // R5: level 1
    c.guilds[g].experience = 0;                  // R5: 0 experience
    c.level = 1;
    c.experience = 0;
    c.vitals.maxHits = kStartHits;               // R5: MaxHits = CurHits = 15 (flat)
    c.vitals.curHits = kStartHits;
    c.wallet.goldOnHand = currencyFromWhole(kStartGoldWhole);  // R5: 1500 gold
    c.location.area = kStartArea;                // R5: Area = -1
    recalcSP(ctx, c, true);                      // R5: SP recalculated with refill
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

// R7: refuse if StatWithMods(i) + StatModWorth(it.StatsMod[i], fit) < RaceDef.Min[i] for any i.
// UNKNOWN: cursed-item wording is not in the SRD; treated as the normal rule.
// CONFLICT: per-item stat requirements are not tested; only race Min is used (the formula).
bool canWear(const Ctx& ctx, const Character& c, const Item& item) {
    const RaceDef& r = raceDefOf(ctx, c.race);
    const ItemDef& def = itemDefOf(ctx, item);
    const FitClass fit = fitFor(item.alignment, c.alignment);
    for (u32 i = 0; i < static_cast<u32>(kStatCount); ++i) {
        const i32 after = statWithMods(c, static_cast<StatIndex>(i))
                          + statModWorth(def.statsMod[i], fit);
        if (after < r.min[i]) {
            return false;
        }
    }
    return true;
}

// R8: v = Stats[i] + sum over equipped s of StatModWorth(s.StatsMod[i], FitFor(s.Alignment, charAlign)).
// EffectiveStat = max(1, v).
i32 effectiveStat(const Ctx& ctx, const Character& c, const Inventory& inv, StatIndex stat) {
    const u32 i = static_cast<u32>(stat);
    i32 v = c.abilities.stats[i];
    for (const Item& s : inv.worn) {
        const FitClass fit = fitFor(s.alignment, c.alignment);
        v += statModWorth(itemDefOf(ctx, s).statsMod[i], fit);
    }
    return maxv(1, v);
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

// R9 stat-modifier fit.
// CONFLICT: adjacent gives "half" [c2] vs "floored half" [c23]; floored half chosen.
// UNKNOWN: StatModWorth body is not in the chunks. Subtract is the negative of the raw value,
// by analogy with the A/D opposed rule.
i32 statModWorth(i32 mod, FitClass fit) {
    switch (fit) {
        case FitClass::Full:
            return mod;
        case FitClass::Half:
            return static_cast<i32>(vbInt(static_cast<double>(mod) / 2.0));
        case FitClass::Subtract:
            return -mod;
    }
    return 0;
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

// R13 WornSum: sum of raw worn-gear attack or defence values.
// Fit against the character is applied in fittedWornSum (which needs the character alignment).
i32 wornSum(const Ctx& ctx, const Inventory& inv, AdKind kind) {
    i32 sum = 0;
    for (const Item& it : inv.worn) {
        const ItemDef& def = itemDefOf(ctx, it);
        sum += itemADOf(def, kind);
    }
    return sum;
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

// R15 archive form: A/D = 2*(Str + Dex) + Int + Wis - 76. Valid only above the band floors.
i32 statsADArchive(i32 str, i32 dex, i32 intel, i32 wis) {
    return 2 * (str + dex) + intel + wis - 76;
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

// R18: HP gain on level-up. Returns the HP added to MaxHits.
// Caller is assumed to have already incremented the membership level (UNKNOWN; levelUp in guilds1.h).
// Gate: the new level must exceed every other membership's level, else no gain and no refill.
// UNKNOWN: the 999 cap appears only in the audit; applied here.
// UNKNOWN: whether the CON term is additive to the AH term; additive is chosen.
// UNKNOWN: overlap of "at or below" and "strictly below" at the cap; the cap level itself takes the AH branch only.
i32 hitsOnLevelUp(Ctx& ctx, Character& c, Guild guild) {
    const u32 gi = static_cast<u32>(guild);
    const i32 newLevel = c.guilds[gi].level;

    // R18.1: gate on every other guild's level.
    for (u32 g = 0; g < static_cast<u32>(kGuildCount); ++g) {
        if (g == gi) {
            continue;
        }
        if (c.guilds[g].level >= newLevel) {
            return 0;
        }
    }

    const HitsParams hp = hitsParamsFor(guild);
    const i32 con = c.abilities.stats[static_cast<u32>(StatIndex::Con)];  // natural CON
    i32 gain = 0;

    // R18.2 / R18.4: AH at or below cap, MH above cap.
    if (newLevel <= hp.capLevel) {
        gain += hp.avg;
        // R18.2: with probability 2/3 add I(U*AH/5)+1. below(3) < 2 gives 2 in 3.
        if (ctx.rng.below(kHitsGainDen) < kHitsGainNum) {
            gain += static_cast<i32>(vbInt(ctx.rng.nextDouble() * hp.avg / kAvgHitsDivisor)) + 1;
        }
    } else {
        gain += hp.late;
    }

    // R18.3: strictly below cap with natural CON > 16: add I(U*(CON-16))+1.
    if (newLevel < hp.capLevel && con > kHitsGainConThreshold) {
        gain += static_cast<i32>(vbInt(ctx.rng.nextDouble() * (con - kHitsGainConThreshold))) + 1;
    }

    // R18.5: cap at 999 (UNKNOWN, audit only), refill HP on level-up.
    const i32 newMax = minv(kHitsCap, c.vitals.maxHits + gain);
    c.vitals.maxHits = newMax;
    c.vitals.curHits = newMax;

    ctx.say("attributes.hits_gain", "Hit points gained on level-up.",
            {static_cast<i64>(gain), static_cast<i64>(newMax)}, {});
    return gain;
}

// R19: HP loss on forfeit. Levels processed lvl = from down to from - cost + 1.
// Quirk: a slice that truncates to 0 subtracts -1 (adds one HP back).
// Quirk: any non-highest level costs no hits.
void hitsOnForfeit(Ctx& ctx, Character& c, Guild guild, i32 from, i32 cost) {
    const u32 gi = static_cast<u32>(guild);
    const HitsParams hp = hitsParamsFor(guild);  // RESOLVED-BY-REFERENCE: AH=AvgHits, ML=HitsCapLevel, MH=LateHits (src/Mordor.Core/MdrLoader.cs:254 LoadGuilds; src/Mordor.Core/Character.cs:1456 ForfeitQuest)

    for (i32 k = 0; k < cost; ++k) {
        const i32 lvl = from - k;

        // R19.2: wasHighest = no other membership has Level >= lvl.
        bool wasHighest = true;
        for (u32 g = 0; g < static_cast<u32>(kGuildCount); ++g) {
            if (g == gi) {
                continue;
            }
            if (c.guilds[g].level >= lvl) {
                wasHighest = false;
                break;
            }
        }

        // R19.3: apply formula.
        if (lvl > hp.capLevel || !wasHighest) {
            if (wasHighest) {
                c.vitals.maxHits -= hp.late;
            }
        } else {
            // Roll rng.Next(3)+1 in {1,2}: below(3)+1 <= 2 (2 in 3).
            if (ctx.rng.below(kForfeitRollSides) + 1 <= kForfeitRollHitMax) {
                c.vitals.maxHits -=
                    static_cast<i32>(ctx.rng.nextDouble() * hp.avg / kAvgHitsDivisor) - 1;
            }
            c.vitals.maxHits -= hp.avg;
        }

        // R19.4: floor MaxHits at 15 after each step.
        if (c.vitals.maxHits < kHitsFloor) {
            c.vitals.maxHits = kHitsFloor;
        }
    }

    // R19.5: CurHits = MaxHits.
    c.vitals.curHits = c.vitals.maxHits;

    ctx.say("attributes.hits_forfeit", "Hit points reduced after forfeit.",
            {static_cast<i64>(c.vitals.maxHits)}, {});
}

// ---------------------------------------------------------------------------
// Spell points (R21)
// ---------------------------------------------------------------------------

// R21: maxSP = 5 * (natural INT + natural WIS).
i32 maxSP(const Character& c) {
    const i32 intel = c.abilities.stats[static_cast<u32>(StatIndex::Int)];
    const i32 wis = c.abilities.stats[static_cast<u32>(StatIndex::Wis)];
    return 5 * (intel + wis);
}

// R21: refill only when requested; otherwise clamp current SP to the new maximum.
void recalcSP(Ctx& ctx, Character& c, bool refill) {
    (void)ctx;
    const i32 mx = maxSP(c);
    c.vitals.maxSpellPoints = mx;
    if (refill) {
        c.vitals.curSpellPoints = mx;
    } else {
        c.vitals.curSpellPoints = minv(c.vitals.curSpellPoints, mx);
    }
}

// ---------------------------------------------------------------------------
// Stat raise and Constitution loss (R22)
// ---------------------------------------------------------------------------

// R22: P(lose 1 Con) = 50 - Con_natural * 2, percent.
// CONFLICT: a clamp to 0 (and 100) is implied by the archive (Ogre at max 27 goes negative) but not
// stated. The clamp to [0,100] is chosen.
i32 conLossChance(i32 conNatural) {
    return clampv(kConLossBase - kConLossPerPoint * conNatural, 0, 100);
}

// R22: after a successful stat raise, roll the chance and lose 1 natural CON. Gear does not count.
// UNKNOWN: ctx.rng.percent(p) assumed to take a 0..100 percentage.
// UNKNOWN: whether the caller or raiseStat triggers this roll; the caller is expected to.
void statRaiseConRoll(Ctx& ctx, Character& c) {
    const u32 con = static_cast<u32>(StatIndex::Con);
    const i32 chance = conLossChance(c.abilities.stats[con]);
    if (ctx.rng.percent(chance)) {
        c.abilities.stats[con] -= 1;
        ctx.say("attributes.con_loss", "Constitution lost after stat raise.",
                {static_cast<i64>(c.abilities.stats[con])}, {});
    }
}

}  // namespace mordor
