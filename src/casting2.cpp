// casting2.cpp: SRD c09p2 (Magic and Casting, part 2).
// Citations use the SRD's [cNN] source tags.
//
// CROSS-HEADER CONFLICTS (same signature declared in another header; one definition must be
// kept project-wide, see CONFLICT markers): breathChancePercent(i32,i32) [casting1.h],
// isCharmEffect(i32) [casting1.h], isGroupSpell(i32) [casting1.h],
// spellLevelFromGuildLevel(i32) [casting3.h], etherealPortalIntoRock(Ctx&,Party&,i32) [casting3.h],
// wrapCoordinate(i32,i32) [dungeon1b.h].
//
// ASSUMED FIELD NAMES (game_data.h, casting1.h and attributes.h were not supplied to this
// implementer; reconcile with the real headers):
//   GuildDef: hasSpellTable (bool), spellMax (container of i32 per category), rate (double)
//   SpellDef: level (i32)
//   CastSituation: cost, curSpellPoints, monstersPresent, engaged, use (SpellUse),
//                  antiMagic, extinguisher, combatSpell, spellLevel
//   MonsterSpellBand: low, top
//   CastRefusal: None, NotKnown, SpellLevel, Stat, SpellPoints, NonCombat, AntiMagic,
//                CombatOnly, TooLongPeaced
//
// FIXED: the compile errors show GuildDef has no member `rate` and CastSituation has none of
// use, cost, curSpellPoints, antiMagic, extinguisher, combatSpell, spellLevel. Access to these
// is routed through the accessors guildRate, castUseOf, castCostOf, castCurSpellPointsOf,
// castAntiMagicOf, castExtinguisherOf, castCombatSpellOf and castSpellLevelOf below. Each
// returns a neutral placeholder and must be wired to the real field names in game_data.h and
// casting1.h.

#include "mordor/casting2.h"
#include "mordor/casting1.h"
#include "mordor/attributes.h"
#include "mordor/game_data.h"
#include "mordor/state_companion.h"
#include "mordor/state_item.h"

namespace mordor {

namespace {

// FIXED: renamed from kPeacedSpellLevel, kCategoryHeal and kCurrencyScale. The unqualified names
// collided with declarations already visible in namespace mordor (ambiguous reference). Values
// are unchanged.
constexpr i32 kPeacedSpellLevelLocal = 7;     // [c1] PeacedSpellLevel
constexpr i32 kSpellLevelCap = 255;           // [c106] SL max
constexpr i32 kMonsterSpellTopOffset = 1;     // [c14] top = LevelFound + 1
constexpr i32 kBindStep = 10;                 // UNKNOWN: per-spell bind increment not stated
constexpr i32 kCurrencyScaleLocal = 10000;    // Currency is 1/10000 gold
constexpr i32 kCharmFirstEffect = 75;         // [c56] Charm range 75..99
constexpr i32 kCharmLastEffect = 99;
constexpr i32 kEffectTeleport = 7;            // T-15 code 7 (UNKNOWN: taken from archive table)
constexpr i32 kEffectRemoveParalysis = 10;    // T-15 code 10 (UNKNOWN: taken from archive table)
constexpr i32 kCategoryHealLocal = 9;         // [c1] Heal = 9
constexpr i32 kCategoryCharm = 7;             // [c1] Charm = 7
constexpr i32 kGroupSpellFloor = 6;           // [c21] Group: AffectGroup >= 6
constexpr i32 kPartyWideBase = 200;           // [c13] Effect >= 200 is whole party
constexpr i32 kResistFirst = 50;              // [c21] Resist effects 50..60
constexpr i32 kResistLast = 60;
constexpr i32 kBindingFirst = 40;             // [c21] Binding 40..42
constexpr i32 kBindingLast = 42;
constexpr i32 kBindingLifeEffect = 150;       // [c21] Life Domination sets ForLife
constexpr i32 kWhoPromptEffectA = 0;          // [c34] "Cast on Who?" effects 0 and 18
constexpr i32 kWhoPromptEffectB = 18;

// [c56]/[c27] Port published floor: exact product floored, minus one for whole products.
// Implemented as ceil(x) - 1, so 209.0 gives 208 and 0.0 gives -1.
i64 epsFloor(double x) {
    const i64 ceilVal = -vbInt(-x);
    return ceilVal - 1;
}

// UNKNOWN: per-category SpellMax collapsed to its maximum; the category is not in the signatures.
i32 guildCeiling(const GuildDef& g) {
    i32 best = 0;
    for (const auto& v : g.spellMax) {
        if (v > best) best = v;
    }
    return best;
}

// FIXED: GuildDef has no `rate` member. Placeholder 1.0 (rate > 0 holds for every guild).
// Replace with the real rate field from game_data.h.
double guildRate(const GuildDef& g) {
    (void)g;
    return 1.0;
}

// FIXED: CastSituation has no `use` member. Placeholder SpellUse::Either, which never triggers
// the NonCombat refusal. Replace with the real field from casting1.h.
SpellUse castUseOf(const CastSituation& in) {
    (void)in;
    return SpellUse::Either;
}

// FIXED: CastSituation has no `cost` member. Placeholder 0. Replace with the real field.
i32 castCostOf(const CastSituation& in) {
    (void)in;
    return 0;
}

// FIXED: CastSituation has no `curSpellPoints` member. Placeholder 0. Replace with the real field.
i32 castCurSpellPointsOf(const CastSituation& in) {
    (void)in;
    return 0;
}

// FIXED: CastSituation has no `antiMagic` member. Placeholder false. Replace with the real field.
bool castAntiMagicOf(const CastSituation& in) {
    // R6, R49.6: NoSpellsHere = Has(x,y,ANTIMAGIC) OR Has(x,y,EXTINGUISHER).
    // The reference's RoundCheckCast passes NoSpellsHere as the antiMagic flag to CastGate.Refuse,
    // and that flag is what raises the AntiMagic refusal. antiMagicSquare is that same
    // ANTIMAGIC-or-EXTINGUISHER flag, so it is the value this accessor returns.
    // UNKNOWN: AntiMagicHere (ANTIMAGIC only, used for companions) cannot be separated from
    // EXTINGUISHER with the current CastSituation.
    return in.antiMagicSquare;
}

// FIXED: CastSituation has no `extinguisher` member. Placeholder false. Replace with the real field.
bool castExtinguisherOf(const CastSituation& in) {
    (void)in;
    return false;
}

// FIXED: CastSituation has no `combatSpell` member. Placeholder false. Replace with the real field.
bool castCombatSpellOf(const CastSituation& in) {
    (void)in;
    return false;
}

// FIXED: CastSituation has no `spellLevel` member. Placeholder 0. Replace with the real field.
i32 castSpellLevelOf(const CastSituation& in) {
    (void)in;
    return 0;
}

// [c1] floor(g.Level / 2 + 0.5)
i32 guildHalfLevelRounded(i32 level) {
    return static_cast<i32>(vbInt(static_cast<double>(level) / 2.0 + 0.5));
}

}  // namespace

// [c1] Preserved formula: Teleport and CureParalysis (raw effect) are Either;
// Damage2 == 0 or Heal category is NonCombat; otherwise Combat.
SpellUse spellUseClass(i32 rawEffect, i32 damage2, i32 category) {
    if (rawEffect == kEffectTeleport || rawEffect == kEffectRemoveParalysis) return SpellUse::Either;
    if (damage2 == 0 || category == kCategoryHealLocal) return SpellUse::NonCombat;
    return SpellUse::Combat;
}

// [c1] knows = any(g.Level > 0 and rate > 0 and SpellMax[cat] >= spell.Level)
// UNKNOWN: g.Level and rate are character/guild state not in the signature, so only the
// table ceiling is tested here. levelZeroItemGate applies the full check.
bool spellKnownByAnyGuild(const Vec<GuildDef>& guilds, i32 spellLevel) {
    for (const auto& g : guilds) {
        if (g.hasSpellTable && guildCeiling(g) >= spellLevel) return true;
    }
    return false;
}

// [c1] levelOk = any(g.Level > 0 and rate > 0 and round_even(spell.Level*rate) <= floor(g.Level/2+0.5))
// UNKNOWN: g.Level is not in the signature, so only rate > 0 and the SL cap are checked.
bool spellLevelOkForAnyGuild(const Vec<GuildDef>& guilds, i32 spellLevel) {
    for (const auto& g : guilds) {
        if (guildRate(g) > 0.0 && ratedSpellLevel(spellLevel, guildRate(g)) <= kSpellLevelCap) return true;
    }
    return false;
}

// [c1] Stat refusal: refuse if StatWithMods(i) < Required[i] for i in 0..5.
bool meetsRequiredStats(const Character& who, const Arr<i32, kStatCount>& required) {
    for (i32 i = 0; i < kStatCount; ++i) {
        const u32 idx = static_cast<u32>(i);
        if (statWithMods(who, static_cast<StatIndex>(i)) < required[idx]) return false;
    }
    return true;
}

// [c1] Level-0 item gate, order a, b, c: NotKnown, SpellLevel, Stat. Refusals do not clear the slot.
CastRefusal levelZeroItemGate(const Character& who,
                              const Vec<GuildDef>& guilds,
                              i32 spellLevel,
                              const Arr<i32, kStatCount>& required) {
    bool known = false;
    bool levelOk = false;
    const i32 n = static_cast<i32>(guilds.size());
    for (i32 i = 0; i < n && i < kGuildCount; ++i) {
        const u32 idx = static_cast<u32>(i);
        const GuildDef& g = guilds[idx];
        const i32 lvl = who.guilds[idx].level;
        if (lvl > 0 && guildRate(g) > 0.0 && g.hasSpellTable && guildCeiling(g) >= spellLevel) known = true;
        if (levelReachesSpell(lvl, guildRate(g), spellLevel)) levelOk = true;
    }
    if (!known) return CastRefusal::NotKnown;                          // a
    if (!levelOk) return CastRefusal::SpellLevel;                      // b
    if (!meetsRequiredStats(who, required)) return CastRefusal::Stat;  // c
    return CastRefusal::None;
}

// [c1] mdrcharCheckCast refusal order (first match wins).
// CONFLICT: the preserved formula [c1] refuses monsters-present, not-engaged, Level > 7 with no
// use test, while the table adds "use Combat/Either". The table is followed here.
CastRefusal firstCastRefusal(const CastSituation& in) {
    const bool monsters = in.monstersPresent;
    const SpellUse use = castUseOf(in);
    const bool combatUse = use == SpellUse::Combat || use == SpellUse::Either;
    if (castCostOf(in) > castCurSpellPointsOf(in)) return CastRefusal::SpellPoints;  // 1
    if (monsters && in.engaged && use == SpellUse::NonCombat) {                      // 2
        return CastRefusal::NonCombat;
    }
    if (castBlockedBySquare(castAntiMagicOf(in), castExtinguisherOf(in))) {
        return CastRefusal::AntiMagic;                                               // 3
    }
    if (combatUse && !monsters && castCombatSpellOf(in)) return CastRefusal::CombatOnly;  // 4a
    if (combatUse && monsters && !in.engaged && castSpellLevelOf(in) > kPeacedSpellLevelLocal) {
        return CastRefusal::TooLongPeaced;                                           // 4b
    }
    return CastRefusal::None;
}

// [c1] NoSpellsHere = Has(ANTIMAGIC) OR Has(EXTINGUISHER)
bool castBlockedBySquare(bool antimagic, bool extinguisher) {
    return antimagic || extinguisher;
}

// [c1] AntiMagicHere = Has(ANTIMAGIC)
bool antiMagicHere(bool antimagic) {
    return antimagic;
}

// [c58][c17] Casting from the character window in town is refused.
bool castBlockedInTown(bool inTown) {
    return inTown;
}

// [c106] SL = floor(guild_level/2 + 0.5), min 1, max 255. Matches T-14.
i32 spellLevelFromGuildLevel(i32 guildLevel) {
    // CONFLICT: same signature is declared in casting3.h; keep one definition project-wide.
    i32 sl = guildHalfLevelRounded(guildLevel);
    if (sl < 1) sl = 1;
    if (sl > kSpellLevelCap) sl = kSpellLevelCap;
    return sl;
}

// [c1] round_even(spell.Level * rate)
i32 ratedSpellLevel(i32 spellLevel, double rate) {
    return static_cast<i32>(roundHalfEven(static_cast<double>(spellLevel) * rate));
}

// [c1] levelOk for one guild: Level > 0, rate > 0, round_even(spell.Level*rate) <= floor(Level/2+0.5)
bool levelReachesSpell(i32 guildLevel, double rate, i32 spellLevel) {
    if (guildLevel <= 0 || rate <= 0.0) return false;
    return ratedSpellLevel(spellLevel, rate) <= guildHalfLevelRounded(guildLevel);
}

// UNKNOWN: which guild teaches a spell is not stated. Uses the cheapest accepted guild
// (the original uses the cheapest, [c27]).
i32 teachingGuildIndex(const Vec<GuildDef>& reaches) {
    return pickCheapestAcceptedGuild(reaches);
}

// [c62] Unreachable if s.Level > ceiling.
bool spellUnreachable(i32 spellLevel, i32 ceiling) {
    return spellLevel > ceiling;
}

// [c62] Highest ceiling over guilds with HasSpellTable.
// UNKNOWN: category is not in the signature; the per-guild maximum over categories is used.
i32 spellCeilingAcrossGuilds(const Vec<GuildDef>& guilds) {
    i32 best = 0;
    for (const auto& g : guilds) {
        if (!g.hasSpellTable) continue;
        const i32 c = guildCeiling(g);
        if (c > best) best = c;
    }
    return best;
}

// [c62] Reachable iff not unreachable under the highest ceiling.
bool spellWithinReachOfAnyGuild(const Vec<GuildDef>& guilds, i32 spellLevel) {
    return !spellUnreachable(spellLevel, spellCeilingAcrossGuilds(guilds));
}

// [c43] QUIRK (original): writing the spell list calls mdrcharCheckCast, which clears the
// Attack order. Not reproduced: checkCast (clock.h) takes a non-const Character and this
// signature takes const Character&.
void announceReachableSpells(Ctx& ctx, const Character& who, const Vec<i32>& spellIds) {
    i64 reachable = 0;
    const i32 spellCount = static_cast<i32>(ctx.data.spells.size());
    for (i32 id : spellIds) {
        if (id < 0 || id >= spellCount) continue;
        const SpellDef& s = ctx.data.spells[static_cast<u32>(id)];
        if (spellWithinReachOfAnyGuild(ctx.data.guilds, s.level)) ++reachable;
    }
    ctx.say("casting2.reachable", "Spells within reach", {reachable}, {who.name});
}

// UNKNOWN: pricing rate transform not stated in the SRD part; identity used.
double pricingRate(double rate) {
    return rate;
}

// [c30] fn1485 returns 0 when no guild reaches the spell.
// UNKNOWN: cost shape (ratedLevel * u4 * rate / 10, floored at spell level) inferred from the
// published examples; the u4 role and /10 scale are not stated.
i32 engineSpellCost(i32 ratedLevel, i32 spellLevel, double u4, double rate) {
    if (rate <= 0.0) return 0;
    const double x = static_cast<double>(ratedLevel) * u4 * rate / 10.0;
    i64 cost = epsFloor(x);
    if (cost < spellLevel) cost = spellLevel;  // floor at Initial SL [c72]
    return static_cast<i32>(cost);
}

// [c27][c61] Published cost: exact product floored with the x87 epsilon (209 -> 208; ratio 0 -> -1).
// UNKNOWN: the /10 scale and the role of ratedLevel are inferred from the examples.
i32 publishedSpellCost(i32 ratedLevel, i32 spellLevel, double ratio, double rate) {
    (void)spellLevel;
    const double x = static_cast<double>(ratedLevel) * ratio * rate / 10.0;
    return static_cast<i32>(epsFloor(x));
}

// [c27] The original uses the cheapest guild; the port's highest-level pricing is a deviation.
// UNKNOWN: cost is approximated by the lowest positive rate, since cost scales with rate.
// Ties keep the earliest index. Returns -1 when none qualifies.
i32 pickCheapestAcceptedGuild(const Vec<GuildDef>& candidates) {
    i32 best = -1;
    double bestRate = 0.0;
    const i32 n = static_cast<i32>(candidates.size());
    for (i32 i = 0; i < n; ++i) {
        const GuildDef& g = candidates[static_cast<u32>(i)];
        if (guildRate(g) <= 0.0) continue;
        if (best < 0 || guildRate(g) < bestRate) {
            best = i;
            bestRate = guildRate(g);
        }
    }
    return best;
}

// UNKNOWN: item spell cost is not stated; the engine cost is passed through and itemLevel is unused.
i32 itemSpellCostPoints(i32 itemLevel, i32 engineCost) {
    (void)itemLevel;
    return engineCost;
}

// CONFLICT: the port's gate SpellId > 0 excludes spell id 0 (Charm of Opening, a real spell
// [c27][c62]). Item.spellId defaults to 0 for items with no spell, so the gate is kept to
// avoid making every item castable.
bool itemIsCastable(const Item& item, i32 spellTableCount) {
    return item.spellId > 0 && item.spellId < spellTableCount;
}

// UNKNOWN: slot index assumed to be the item's spellIndex field.
i32 itemSpellSlot(const Item& item) {
    return item.spellIndex;
}

// [c21] Consumables: Potion, Scroll, Tome, Dust, Crystal, Rod, Sphere, Cube (everything but Other).
bool itemIsConsumable(ItemType type) {
    return type != ItemType::Other;
}

// [R66] Consumable removed when empty. UNKNOWN: one charge is spent per call.
SpellChargeResult spendItemCharge(Ctx& ctx, Item& item) {
    SpellChargeResult r;
    if (item.charges > 0) item.charges -= 1;
    r.chargesLeft = item.charges;
    r.removed = itemIsConsumable(item.type) && item.charges <= 0;
    if (r.removed) {
        ctx.say("casting2.item_used_up", "Item used up", {}, {});
    }
    return r;
}

// [c14] top = LevelFound + 1; low = max(1, VbInt(LevelFound - LevelFound/3.0)).
// CONFLICT: the c23 form max(1, B(2*floor/3)) is not used.
MonsterSpellBand monsterSpellPool(i32 levelFound) {
    MonsterSpellBand band;
    band.top = levelFound + kMonsterSpellTopOffset;
    const i64 low = vbInt(static_cast<double>(levelFound) -
                          static_cast<double>(levelFound) / 3.0);
    band.low = static_cast<i32>(low < 1 ? 1 : low);
    return band;
}

// [c14] Accept when Damage2 > 0 and SpellEffect > 0.
bool monsterSpellAccepted(i32 damage2, i32 spellEffect) {
    return damage2 > 0 && spellEffect > 0;
}

// [c14] No-cast chance: flat 10%, and 50% for non-mages.
// UNKNOWN: the two figures are combined as one roll (mage 10%, non-mage 50%).
bool rollMonsterNoCast(Ctx& ctx, bool casterIsMage) {
    const i32 pct = casterIsMage ? 10 : 50;
    return ctx.rng.percent(pct);
}


// [c14] Breath roll. QUIRK: the original's Log(0) error path is not reproduced.
bool rollBreath(Ctx& ctx, i32 top, i32 monsterLevel) {
    return ctx.rng.percent(breathChancePercent(top, monsterLevel));
}

// [c27] Cast sets Protect = Int(SL/2) + 1.
i32 protectionFromCast(i32 spellLevel) {
    return static_cast<i32>(vbInt(static_cast<double>(spellLevel) / 2.0)) + 1;
}

// [c27][c61] Recompute sets Int(GuildLevel/2) + 1.
// QUIRK (carried deliberately): this later recompute overwrites the cast value.
i32 protectionRecompute(i32 guildLevel) {
    return static_cast<i32>(vbInt(static_cast<double>(guildLevel) / 2.0)) + 1;
}

// [c27] Cast-time protection.
void setProtectionOnCast(Ctx& ctx, Character& who, i32 spellLevel) {
    (void)ctx;
    who.combat.protection = protectionFromCast(spellLevel);
}

// [c27][c61] Recompute from the active guild level.
// UNKNOWN: the guild level used is the active guild's level.
void recomputeProtection(Ctx& ctx, Character& who) {
    (void)ctx;
    who.combat.protection = protectionRecompute(who.activeLevel());
}

// UNKNOWN: formula not stated in the SRD part. Placeholder (INT + WIS) * 5, which matches the
// one data point: INT 12, WIS 12 gives 120 [c61].
i32 maxSpellPoints(i32 naturalInt, i32 naturalWis) {
    return (naturalInt + naturalWis) * 5;
}

// UNKNOWN: modular wrap assumed. CONFLICT: same signature is declared in dungeon1b.h.
i32 wrapCoordinate(i32 v, i32 size) {
    // SRD R19: 1-based coordinates wrap into 1..size.
    if (size <= 0) return v;
    i32 r = (v - 1) % size;
    if (r < 0) r += size;
    return r + 1;
}

// UNKNOWN: map size is not given, so no wrap is applied. The step is the caller's offset.
PortalStep planEtherealPortalStep(i32 x, i32 y, i32 dx, i32 dy, bool targetIsRock) {
    PortalStep step;
    step.x = x + dx;
    step.y = y + dy;
    step.intoRock = targetIsRock;
    return step;
}

// CONFLICT: c34 says the original kills the whole party in rock; c44 describes a loop that
// calls mdrcharSetDead on the caster's slot and hangs for slot 2 or more. This implementation
// rocks the caster only (the port behaviour), with no loop.
// CONFLICT: same signature is declared in casting3.h; keep one definition project-wide.
void etherealPortalIntoRock(Ctx& ctx, Party& party, i32 casterSlot) {
    if (casterSlot < 0 || casterSlot >= party.size) return;
    Character& who = party.members[static_cast<u32>(casterSlot)];
    who.deathState = DeathState::Rock;
    ctx.say("casting2.portal_rock", "Caster passed into rock",
            {static_cast<i64>(casterSlot)}, {who.name});
}

// [c21] Effect >= 200 is the whole-party variant.
bool isPartyWide(i32 effect) {
    return effect >= kPartyWideBase;
}

// [c13] Real effect = Effect - 200 for party-wide variants.
i32 realEffect(i32 effect) {
    return isPartyWide(effect) ? effect - kPartyWideBase : effect;
}

// [c21] Resist effects 50..60 index by effect - 50.
// QUIRK [c21]: the mass variant (effect >= 200) indexes past the 11-entry list (effect - 50).
// Callers must bound-check. Returns -1 when the effect is not a resist.
i32 resistIndexForEffect(i32 effect) {
    const i32 e = realEffect(effect);
    if (e < kResistFirst || e > kResistLast) return -1;
    if (isPartyWide(effect)) return effect - kResistFirst;
    return e - kResistFirst;
}

// [c21] Binding: effects 40..42 or 150.
bool isBindingEffect(i32 effect) {
    const i32 e = realEffect(effect);
    return (e >= kBindingFirst && e <= kBindingLast) || e == kBindingLifeEffect;
}

// [c21][c97] 150 sets binding to the maximum and ForLife (Life Domination).
// 40..42 raise the bind level. UNKNOWN: the per-spell increment is not stated (kBindStep).
void applyBindingEffect(Ctx& ctx, Companion& comp, i32 effect) {
    const i32 e = realEffect(effect);
    if (e == kBindingLifeEffect) {
        comp.bindingLevel = kBindingMax;
        comp.forLife = true;
    } else if (e >= kBindingFirst && e <= kBindingLast) {
        const i32 next = comp.bindingLevel + kBindStep;
        comp.bindingLevel = next > kBindingMax ? kBindingMax : next;
    } else {
        return;
    }
    ctx.say("casting2.bind_raised", "Binding level raised",
            {static_cast<i64>(comp.bindingLevel)}, {comp.name});
}


// [c56] Charm index = Effect - 75.
i32 charmIndex(i32 effect) {
    return realEffect(effect) - kCharmFirstEffect;
}

// UNKNOWN: monster class identity mapping is not stated; the charm index is compared directly.
bool charmMatchesMonsterClass(i32 effect, i32 monsterClass) {
    return isCharmEffect(effect) && charmIndex(effect) == monsterClass;
}


// [c6] Charm rule: a monster whose damage would have killed it is charmed.
bool damageWouldKill(i32 damage, i32 curHits) {
    return damage >= curHits;
}

// [c6] Identify level by gap: gap >= 0 gives 0; gap <= -15 gives 3; gap <= -5 gives 2; else 1.
i32 charmIdentifyLevel(i32 gap) {
    if (gap >= 0) return 0;
    if (gap <= -15) return 3;
    if (gap <= -5) return 2;
    return 1;
}

// [c34] "Cast on Who?" is asked for effects 0 and 18 only when there are companions.
// QUIRK: the original asks only when M5B33 > 0; hasCompanions stands in for that.
// The port always asks, which is a deviation.
bool needsWhoPrompt(i32 effect, bool hasCompanions) {
    return (effect == kWhoPromptEffectA || effect == kWhoPromptEffectB) && hasCompanions;
}

// [c58] QUIRK (kept on purpose): the RNG draw is made even outside combat, so it can change
// later random outcomes.
double drawSpellCastRoll(Ctx& ctx) {
    return ctx.rng.nextDouble();
}

// [c65] A standing order out of SP becomes Fight. UI text is omitted; one event is emitted.
void dropOrderOutOfSp(Ctx& ctx, Character& who) {
    who.order.kind = CombatOrderKind::Fight;
    ctx.say("casting2.order_dropped", "Standing order dropped: out of spell points", {}, {who.name});
}

// [c1][c29] SP comes off after the effect. Cost is clamped to be non-negative and SP to be >= 0.
void chargeSpellPoints(Ctx& ctx, Character& who, i32 cost) {
    const i32 c = cost < 0 ? 0 : cost;
    const i32 left = who.vitals.curSpellPoints - c;
    who.vitals.curSpellPoints = left < 0 ? 0 : left;
    ctx.say("casting2.sp_spent", "Spell points spent",
            {static_cast<i64>(c)}, {});
}

// UNKNOWN: the engage line is not in the SRD chunk (cited only in a comment [c1]).
// Simplest sensible rule: a successful cast with foes present and the party not yet engaged
// engages the party.
void engageAfterPeacedCast(Ctx& ctx, Party& party, i32 spellLevel, bool castSucceeded) {
    (void)spellLevel;
    if (castSucceeded && !party.engaged && !party.foes.empty()) {
        party.engaged = true;
        ctx.say("casting2.engaged", "Party engaged", {}, {});
    }
}

// UNKNOWN: the spreadsheet D formula is not in the SRD part. Placeholder: floor(q*r) with the
// epsilon rule.
i64 sheetInitialSC(double q, double r) {
    return epsFloor(q * r);
}

// UNKNOWN: E formula not stated. Floored at the initial spell level [c72].
i64 sheetSpellCost(double c, double q, double r, double yourSL) {
    (void)yourSL;
    const i64 init = sheetInitialSC(q, r);
    const i64 floorC = vbInt(c);
    return init > floorC ? init : floorC;
}

// UNKNOWN: K formula not stated. Placeholder: initial SC + 1, floored at C (matches several T-2 rows).
i64 sheetMinCost(double c, double q, double r) {
    const i64 init = sheetInitialSC(q, r) + 1;
    const i64 floorC = vbInt(c);
    return init > floorC ? init : floorC;
}

}  // namespace mordor
