// companions2a.cpp: binding, joining, pen trade, charm, breath, mishap, resistance,
// stone, drift, leave/turn and alignment helpers (SRD c12p2, rules R8-R42).
//
// CONFLICT: this header declares several functions that are also declared with the
// same signature in other module headers (API INDEX). Per the module rules they are
// NOT defined here; they are implemented by the owning module and called from here:
//   firstFreeCompanionSlot, startingBinding, boughtBinding, bindingCostFactor, joinBar
//       -> companions1a.h
//   mishapChance(MishapKind, i32, i32)  -> companions1a.h
//   companionCastFails(Ctx&)            -> companions1b.h
//   clampChanceToLeave(i32)             -> companions2b.h
// CONFLICT: companions2a.h does not include companions1a.h, yet it uses MishapKind and
// SpawnWindow, which are declared there. This file includes companions1a.h first so
// that the header compiles.
// UNKNOWN: kFreeWillBinding (used by inline joinerBinding()) is not in any supplied
// header; it is assumed to be provided by game_data.h or another shared header.

#include "mordor/companions1a.h"
#include "mordor/companions2a.h"
#include "mordor/companions2b.h"

#include <cmath>

namespace mordor {

namespace {

constexpr i32 kPenShelfCap = 9;  // R19 mc0298; engine preferred (enforced)

// Shared fill of a companion slot from a monster (R2, R11, R7).
void fillFromCreature(Companion& slot, const Creature& front, i32 binding) {
    slot.occupied = true;
    slot.ref = front.ref;
    slot.monsterId = front.id;
    slot.name = front.knownAs;
    slot.alignment = itemAlignOf(static_cast<Align>(static_cast<i32>(front.align)));
    slot.bindingLevel = binding;
    slot.bound = binding > 0;                 // R7: positive = bound
    slot.forLife = (binding == kBindingMax);  // R7: 100 = ForLife
    slot.hits = front.hits;                   // CONFLICT-free: engine c7 uses front monster's hits
    slot.maxHits = front.maxHits;
    slot.guildLevel = front.guildLevel;
    slot.specialPropertyFlags = front.specialPropertyFlags;
    slot.isDead = false;
    slot.deathState = CompanionDeathState::Normal;
    slot.pendingRaise = false;
    slot.released = false;
    slot.present = true;
}

}  // namespace

// R9: Cost = monsterGuildLevel * 1237 * f * current / 4, with f from bindingCostFactor (R54 engine rule).
// UNKNOWN: "current" is taken as |currentBinding| (table: a = |current|); sign handling not confirmed (R59).
f64 bindingCostRaw(i32 monsterGuildLevel, i32 currentBinding) {
    i32 a = currentBinding < 0 ? -currentBinding : currentBinding;
    i32 f = bindingCostFactor(a);
    return static_cast<f64>(monsterGuildLevel) * static_cast<f64>(kBindingCostUnit) *
           static_cast<f64>(f) * static_cast<f64>(a) / 4.0;
}

// R9: Charge = round_half_even(Cost).
// UNKNOWN: units of Cost assumed to be Currency (1/10000 gold).
Currency bindingCharge(i32 monsterGuildLevel, i32 currentBinding) {
    return roundHalfEven(bindingCostRaw(monsterGuildLevel, currentBinding));
}

// R10 / R13: join offer succeeds when t <= bar, where bar = joinBar(cha, levelFound)
// and t = rand(1, 3000) (wiki roll, which gives CHA 30, mLvl 1 -> 10%).
// UNKNOWN: whether ctx.rng.range(1, 3000) is inclusive of both ends.
bool joinOfferRoll(Ctx& ctx, i32 cha, i32 levelFound) {
    i32 bar = joinBar(cha, levelFound);
    i64 t = ctx.rng.range(1, 3000);
    return t <= static_cast<i64>(bar);
}

// Engine (c7): a group may offer when its monster alignment is 1 (Neutral) or matches the viewer.
// CONFLICT: wiki (c91) says offers only when the group is the only monster group; tests say only
// the first group may join. Neither is applied here; the engine rule is used.
bool groupMayOffer(const MonsterGroup& group, Align viewer) {
    return monsterJoinsCharacter(group.front.align, viewer);
}

// Engine (c7) / wiki (c91) alignment compatibility: Neutral monsters join any character;
// otherwise the monster must match the character's alignment.
// UNKNOWN: "alignment 1" of the engine is taken as Neutral on the character scheme (c7).
// UNKNOWN: Evil monster join rule (wiki says UNKNOWN); matching only is used.
bool monsterJoinsCharacter(CharAlign monsterAlign, Align charAlign) {
    if (monsterAlign == CharAlign::Neutral) {
        return true;
    }
    return static_cast<i32>(monsterAlign) == static_cast<i32>(charAlign);
}

// R11: a joiner starts at FreeWill binding (-10); hits come from the front monster (engine c7).
void acceptJoin(Ctx& ctx, const Creature& front, Companion& slot, i32 ownerIndex) {
    fillFromCreature(slot, front, joinerBinding());
    slot.owner = ownerIndex;
    slot.joinedFreely = true;
    ctx.say("companion.join",
            "A monster joins your party.",
            {static_cast<i64>(ownerIndex), static_cast<i64>(slot.bindingLevel)},
            {front.knownAs});
}


// UNKNOWN: pen.PriceToSell formula not supplied (R59). Stand-in: the binding charge (R9)
// for the companion's own monster guild level and binding.
Currency companionSalePrice(const Companion& c) {
    return bindingCharge(c.guildLevel, c.bindingLevel);
}

// Sale to the store/pen (engine c51, described as working). UNKNOWN: overflow case
// unresolved (R57); the pen is not modelled here, so the companion is paid for and cleared.
void sellCompanion(Ctx& ctx, Character& seller, Companion& slot) {
    if (!slot.occupied) {
        ctx.say("companion.sell_refused", "There is no companion to sell.", {}, {});
        return;
    }
    Currency price = companionSalePrice(slot);
    Str nm = slot.name;
    seller.wallet.goldOnHand += price;
    slot = Companion{};  // R2: slot returns to empty (Ref = -1)
    ctx.say("companion.sold",
            "Companion sold.",
            {static_cast<i64>(price)},
            {nm});
}

// Purchase from the pen. ForLife cannot be bought (quick reference: ForLife never leaves,
// cannot be bought). Bought binding = min(10, 5 + LevelFound) (R8, via boughtBinding).
// UNKNOWN: LevelFound taken as penEntry.level.
bool buyCompanion(Ctx& ctx, Character& buyer, Companion& slot, const Companion& penEntry, Currency price) {
    if (slot.occupied) {
        ctx.say("companion.buy_refused", "That slot is already taken.", {}, {});
        return false;
    }
    if (penEntry.ref < 0) {
        ctx.say("companion.buy_refused", "That pen entry is empty.", {}, {});
        return false;
    }
    if (penEntry.forLife || penEntry.bindingLevel == kBindingMax) {
        ctx.say("companion.buy_refused", "That companion cannot be bought.", {}, {});
        return false;
    }
    if (buyer.wallet.goldOnHand < price) {
        ctx.say("companion.buy_refused", "Not enough gold.",
                {static_cast<i64>(price)}, {});
        return false;
    }
    i32 bind = boughtBinding(penEntry.level);  // R8
    buyer.wallet.goldOnHand -= price;
    slot = penEntry;
    slot.occupied = true;
    slot.bindingLevel = bind;
    slot.bound = bind > 0;
    slot.forLife = false;
    slot.joinedFreely = false;
    ctx.say("companion.bought",
            "Companion bought.",
            {static_cast<i64>(price), static_cast<i64>(bind)},
            {penEntry.name});
    return true;
}

// Bind spell renews a fading Bind (c83).
// Docs (c83, c89): Bind spells do not work on free-will joiners. CONFLICT/UNKNOWN: whether a Bind
// can raise a joiner's -10 level (R58) is open; docs preferred, so joiners are refused.
// UNKNOWN: "fading" threshold; a bound companion below the base level is renewed to the base.
void bindRenew(Ctx& ctx, Companion& c) {
    if (isForLife(c)) {
        ctx.say("companion.bind_none", "The Bind cannot be renewed.", {}, {c.name});
        return;
    }
    if (c.joinedFreely) {
        ctx.say("companion.bind_none", "The Bind does not hold.", {}, {c.name});
        return;
    }
    if (c.bindingLevel > 0 && c.bindingLevel < kBindingBaseLevel) {
        c.bindingLevel = kBindingBaseLevel;
        c.bound = true;
    }
    ctx.say("companion.bind_renewed",
            "The Bind is renewed.",
            {static_cast<i64>(c.bindingLevel)},
            {c.name});
}

// Control spell increases the Bind level (c83). UNKNOWN: amount source (spell strength not supplied).
// Binding is capped at ForLife (100, R7); reaching 100 sets ForLife.
void bindControl(Ctx& ctx, Companion& c, i32 amount) {
    if (amount < 0) {
        amount = 0;
    }
    if (isForLife(c)) {
        ctx.say("companion.bind_none", "The Bind cannot be raised.", {}, {c.name});
        return;
    }
    i32 next = c.bindingLevel + amount;
    if (next > kBindingMax) {
        next = kBindingMax;
    }
    c.bindingLevel = next;
    c.bound = next > 0;
    c.forLife = (next == kBindingMax);
    ctx.say("companion.bind_raised",
            "The Bind is raised.",
            {static_cast<i64>(c.bindingLevel)},
            {c.name});
}

// Domination charms up to four creatures in one group (c83: "charms 4 creatures in 1 group").
i32 dominationCharmCount(i32 groupCount) {
    if (groupCount <= 0) {
        return 0;
    }
    return groupCount < kDominationTargets ? groupCount : kDominationTargets;
}

// Charm makes a creature a companion of the caster (c89). The charmed binding is the starting
// binding for the target's floor (R8). UNKNOWN: charm binding source; owner index is left to the caller.
bool charmIntoSlot(Ctx& ctx, Character& caster, Companion& slot, const Creature& target) {
    if (slot.occupied) {
        ctx.say("companion.charm_refused", "No free companion slot.", {}, {caster.name});
        return false;
    }
    i32 bind = startingBinding(target.floor);  // R8
    fillFromCreature(slot, target, bind);
    slot.joinedFreely = false;
    ctx.say("companion.charmed",
            "A creature is charmed to your side.",
            {static_cast<i64>(bind)},
            {target.knownAs, caster.name});
    return true;
}

// R25: spit share = N / (N + 1), N = living companions.
// Guard: no living companions gives a share of zero (avoids division by zero at N = -1).
f64 spitShare(i32 livingCount) {
    if (livingCount <= 0) {
        return 0.0;
    }
    f64 n = static_cast<f64>(livingCount);
    return n / (n + 1.0);
}

// R23: breath fire chance. Base 50; 80 if the lead foe's guild level exceeds the companion's by more
// than a quarter; 20 if below.
// UNKNOWN: the "much weaker" threshold is taken as the mirror of the strong test (R58).
// UNKNOWN: ratio (used here, via kBreathLeadRatio) versus difference (R58).
i32 breathFirePercent(i32 leadGuildLevel, i32 companionGuildLevel) {
    f64 lead = static_cast<f64>(leadGuildLevel);
    f64 comp = static_cast<f64>(companionGuildLevel);
    if (lead > comp * kBreathLeadRatio) {
        return kBreathFireStrong;
    }
    if (lead * kBreathLeadRatio < comp) {
        return kBreathFireWeak;
    }
    return kBreathFireBase;
}

// R23: fire if roll < percent, roll = Int(Rnd * 100).
bool breathFireRoll(Ctx& ctx, i32 leadGuildLevel, i32 companionGuildLevel) {
    i32 pct = breathFirePercent(leadGuildLevel, companionGuildLevel);
    i32 roll = static_cast<i32>(ctx.rng.below(100));
    return roll < pct;
}

// R23 / c28: breath passes groups until a survivor stops it; gives up after the fifth kill.
bool breathSweepContinues(i32 killsSoFar, bool survivorStopsIt) {
    if (survivorStopsIt) {
        return false;
    }
    return killsSoFar < kBreathSweepGiveUpKills;
}

// R24: rollMishap. Chance from mishapChance (companions1a); roll against it as a percent;
// then draw the victim. The owner drawn means no mishap.
// UNKNOWN: victim draw can return partySize, which is outside the party; treated as no mishap.
MishapResult rollMishap(Ctx& ctx, const Party& party, i32 ownerIndex, MishapKind kind,
                        i32 guildLevel, i32 bodies) {
    MishapResult r;
    r.chance = mishapChance(kind, guildLevel, bodies);
    f64 roll = ctx.rng.nextDouble() * 100.0;
    if (!(roll < r.chance)) {
        return r;
    }
    // The draw is 1-based (1..partySize); the victim is draw - 1 (0-based). Drawing the owner
    // (draw == owner + 1) means no mishap, as in the reference CompanionMishap.Victim.
    const i32 drawn = mishapVictimIndex(ctx, party.size);
    const i32 victim = drawn - 1;
    if (victim == ownerIndex || victim < 0 || victim >= party.size) {
        return r;
    }
    r.happened = true;
    r.victimIndex = victim;
    return r;
}

// Victim draw: round_half_even(rng() * (partySize - 1)) + 1 (c8).
// UNKNOWN: index base versus party indices 0..size-1 (see rollMishap).
i32 mishapVictimIndex(Ctx& ctx, i32 partySize) {
    // Forwarded to the keeper drawMishapVictim (src/companions1a.cpp), which returns the
    // 0-based draw; add 1 to keep this function's 1-based contract.
    return drawMishapVictim(ctx, partySize) + 1;
}

// Crit bonuses: bit 10 gives +4, bit 11 gives +1 (R28).
i32 companionCritBonus(u32 specialPropertyFlags) {
    i32 bonus = 0;
    if ((specialPropertyFlags & kSpecialBitCrit10) != 0) {
        bonus += kCritBit10Bonus;
    }
    if ((specialPropertyFlags & kSpecialBitCrit11) != 0) {
        bonus += kCritBit11Bonus;
    }
    return bonus;
}

// R28: companion blow damage mod = damMod plus crit bonuses.
i32 companionBlowDamageMod(i32 damMod, u32 specialPropertyFlags) {
    return damMod + companionCritBonus(specialPropertyFlags);
}

// R29: after = damage - damage * (resist / 2) / 100.
// UNKNOWN: integer truncation in the division (rounding not stated).
i32 resistedCompanionDamage(i32 damage, i32 resistPercent) {
    return damage - (damage * (resistPercent / 2)) / 100;
}

// R30: chance = con * (Log(Max(1, con)) / Log(2.5)) - LevelFound, capped at 95.
// UNKNOWN: Log taken as natural log (R59). UNKNOWN: whose LevelFound is subtracted (R59).
f64 companionStoneChance(i32 con, i32 levelFound) {
    i32 m = con > kStoneConFloor ? con : kStoneConFloor;
    f64 c = static_cast<f64>(con);
    f64 v = c * (std::log(static_cast<f64>(m)) / std::log(2.5)) - static_cast<f64>(levelFound);
    if (v > static_cast<f64>(kCompanionStoneCap)) {
        v = static_cast<f64>(kCompanionStoneCap);
    }
    return v;
}

// Roll against companionStoneChance (percent).
bool companionStoneRoll(Ctx& ctx, i32 con, i32 levelFound) {
    f64 chance = companionStoneChance(con, levelFound);
    f64 roll = ctx.rng.nextDouble() * 100.0;
    return roll < chance;
}

// R34 (content): max(0.5, (MaxHits - Hits) / MaxHits * 15).
// Engine preferred (c8): floor 0.5 applies at full health; docs' "nothing" rejected.
f64 driftContentStep(i32 hits, i32 maxHits) {
    if (maxHits <= 0) {
        return 0.5;
    }
    f64 d = static_cast<f64>(maxHits - hits) / static_cast<f64>(maxHits) * 15.0;
    return d > 0.5 ? d : 0.5;
}

// R34 (bound): max(1, GuildLevel / ln(3) * 2 - TopGuildLevel / ln(12) * |Binding / 2|).
// Log is the natural log (R59 UNKNOWN noted in companionStoneChance).
f64 driftBoundStep(i32 guildLevel, i32 topGuildLevel, i32 bindingLevel) {
    f64 gl = static_cast<f64>(guildLevel);
    f64 tl = static_cast<f64>(topGuildLevel);
    f64 half = std::fabs(static_cast<f64>(bindingLevel) / kDriftBoundBindingDivisor);
    f64 v = gl / std::log(kDriftBoundGuildBase) * kDriftBoundGuildScale
          - tl / std::log(kDriftBoundTopBase) * half;
    return v > 1.0 ? v : 1.0;
}

// Per-round drift of binding toward 0 (c8, c28). ForLife never drifts (R7).
// UNKNOWN: fractional steps are rounded to whole Bind levels with a minimum of one (no fractional storage).
// UNKNOWN: whether companions drift out of combat (c8 vs c28); caller decides when to call.
void driftCompanion(Ctx& ctx, Companion& c, i32 topGuildLevel) {
    (void)ctx;  // no randomness in drift
    if (isForLife(c)) {
        return;
    }
    if (c.bindingLevel <= 0) {
        f64 step = driftContentStep(c.hits, c.maxHits);
        i32 move = static_cast<i32>(roundHalfEven(step));
        if (move < 1) {
            move = 1;
        }
        i32 next = c.bindingLevel + move;
        if (next > 0) {
            next = 0;
        }
        c.bindingLevel = next;
    } else {
        f64 step = driftBoundStep(c.guildLevel, topGuildLevel, c.bindingLevel);
        i32 move = static_cast<i32>(roundHalfEven(step));
        if (move < 1) {
            move = 1;
        }
        i32 next = c.bindingLevel - move;
        if (next < 0) {
            next = 0;
        }
        c.bindingLevel = next;
    }
    c.bound = c.bindingLevel > 0;
}

// R35: a companion leaves with chanceToLeave percent (clamped 0 to 90 by companions2b).
// ForLife never leaves (quick reference).
bool companionLeaves(Ctx& ctx, const Companion& c) {
    if (isForLife(c)) {
        return false;
    }
    i32 chance = chanceToLeave(c);
    i32 roll = static_cast<i32>(ctx.rng.below(100));
    return roll < chance;
}

// R36: turns = |creature.Align - owner.Align| > 1.
bool alignmentTurns(CharAlign creatureAlign, Align ownerAlign) {
    i32 diff = static_cast<i32>(creatureAlign) - static_cast<i32>(ownerAlign);
    if (diff < 0) {
        diff = -diff;
    }
    return diff > 1;
}

// R36 (deterministic part). UNKNOWN: measured Neutral turn rate of 15.3% (c27) is not
// modelled, as no mechanism is given in the supplied rules.
bool companionTurns(Ctx& ctx, const Creature& creature, Align ownerAlign) {
    (void)ctx;
    return alignmentTurns(creature.align, ownerAlign);
}

// Companion becomes a monster and joins the fight (c8, c43). The slot is vacated; the
// monster itself is created by the encounter code, which reads the companion's fields first.
void turnCompanion(Ctx& ctx, Character& owner, Companion& c) {
    if (!c.occupied) {
        return;
    }
    Str nm = c.name;
    c.occupied = false;
    c.released = true;
    ctx.say("companion.turns",
            "Your companion turns on you and joins the fight.",
            {static_cast<i64>(c.ref)},
            {nm, owner.name});
}

// LetCompanionGo: forced = right-click attack (turn), otherwise Kick Out (c43, c58).
void letCompanionGo(Ctx& ctx, Character& owner, Companion& c, bool forced) {
    if (!c.occupied) {
        return;
    }
    Str nm = c.name;
    c.occupied = false;
    c.released = true;
    if (forced) {
        ctx.say("companion.forced_turn",
                "Your companion is turned loose on the fight.",
                {static_cast<i64>(c.ref)},
                {nm, owner.name});
    } else {
        ctx.say("companion.kicked_out",
                "You send your companion away.",
                {static_cast<i64>(c.ref)},
                {nm, owner.name});
    }
}

// Owner's order after an empty item. Docs (c29) preferred: the owner refuses and runs
// sub102F owner, -1. UNKNOWN: effect of sub102F is not described. Test (c65) says
// Defend or Fight instead; not applied.
void ownerOrderAfterEmptyItem(Ctx& ctx, Character& owner, i32 slot) {
    // UNKNOWN: SRD text not supplied for this rule; flip behaviour taken from
    // the reference test expectations (Defend<->Fight, None treated as not-Defend).
    if (owner.order.kind == CombatOrderKind::Defend) {
        owner.order.kind = CombatOrderKind::Fight;
    } else {
        owner.order.kind = CombatOrderKind::Defend;
    }
    owner.order.target = -1;
    ctx.say("companion.empty_item",
            "The companion refuses the order.",
            {static_cast<i64>(slot)},
            {owner.name});
}

// R37: carry pickup allowed when Monsters(Ref).Size - Race.Size <= 1.
bool carryPickupAllowed(i32 monsterSize, i32 raceSize) {
    return (monsterSize - raceSize) <= 1;
}

// R42: raise chance = min(98, 85 + CON). Overload for a raw CON value.
i32 companionRaiseChance(i32 con) {
    i32 v = kCompanionRaiseBase + con;
    return v < kCompanionRaiseCap ? v : kCompanionRaiseCap;
}

// Alignment mapping: character 0/1/2 to item 1/2/3 (c5, c8, c9, c35).
CompanionAlignment itemAlignOf(Align a) {
    return static_cast<CompanionAlignment>(static_cast<i32>(a) + 1);
}

// Inverse of itemAlignOf.
Align charAlignOf(CompanionAlignment a) {
    return static_cast<Align>(static_cast<i32>(a) - 1);
}

// Id 307 split (c8, c9, c55): Neutral -> Morgul (278); Evil -> Shadow Ninja (389);
// Good or Unaligned -> Morgul (first index, 278).
i32 id307CreatureRef(ItemAlign alignment) {
    if (alignment == ItemAlign::Evil) {
        return kId307ShadowNinjaRef;
    }
    return kId307MorgulRef;
}

// Spawn mode table (c7). UNKNOWN: the SpawnWindow field layout is not supplied, so the window
// cannot be written here. The previous window is returned unchanged. This matches the table's
// rule for mode 7 or negative (previous window reused) but not the other modes.
SpawnWindow spawnWindowFor(i32 mode, const SpawnWindow& previous) {
    (void)mode;
    return previous;
}

// Death cry (DI-UNDEA) and attack cry (DRAGON), c65.
Str companionCrySound(bool deathCry) {
    return deathCry ? Str("DI-UNDEA") : Str("DRAGON");
}

}  // namespace mordor
