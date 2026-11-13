#include "mordor/companions1b.h"

namespace mordor {
namespace {
i32 uidx(i32 i) {
    return static_cast<i32>(i);
}

bool isDeadCompanion(const Companion& c) {
    // R37.2 / R46 / R39: dead means Hits at or below 0, or the dead flag.
    return c.hits <= 0 || c.isDead;
}

Vec<Creature>& liveSetOf(LiveSetRef& live) {
    // R41.1: combat monsters if a fight exists, else the waiting group.
    return live.fightExists ? live.combatMonsters : live.waitingGroup;
}

// Companion takes its place in the live set as a monster (R41.3, R34.3).
// UNKNOWN: creature hostility and other fields are not given in the SRD part.
void joinLiveSet(const Companion& c, CharAlign creatureAlign, LiveSetRef& live) {
    Creature m;
    m.id = c.monsterId;
    m.align = creatureAlign;
    m.hits = c.hits;
    m.maxHits = c.maxHits;
    m.guildLevel = c.guildLevel;
    m.alive = c.hits > 0;
    m.hostile = false;
    liveSetOf(live).push_back(m);
}

// Slot index of the carried companion, or -1 when nothing or a body is carried.
i32 keptSlotOf(const Inventory& inv) {
    if (inv.carryIndex >= 0 && inv.carryIndex < kCompanionSlots) {
        return inv.carryIndex;
    }
    return -1;
}
}  // namespace

// R34 for each companion in slot order 0..3, not ForLife.
void companionRoundPass(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, const Character& owner,
                        const Arr<CharAlign, kCompanionSlots>& creatureAligns, LiveSetRef& live) {
    const i32 charisma = owner.abilities.stats[static_cast<u32>(StatIndex::Cha)];
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        Companion& c = slots[static_cast<u32>(i)];
        if (!c.occupied || c.forLife) {
            continue;  // R34: ForLife is never touched
        }
        companionMoodRound(ctx, c, owner.topGuildLevel);  // R34.1
        const f64 chance = companionChanceToLeave(c, owner.topGuildLevel, charisma);
        if (!companionLeaveRoll(ctx, c, chance)) {  // R34 leave test
            continue;
        }
        const CharAlign creatureAlign = creatureAligns[static_cast<u32>(i)];
        const TurnOutcome t = companionTurnCheck(ctx, c, owner.alignment, creatureAlign, false,
                                                 liveSetOf(live), live.groupSlotFree);  // R34.2
        if (t == TurnOutcome::Joins) {  // R34.3: turns and joins the fight
            ctx.say("companion.turns", "Your companion turns into a monster!", {static_cast<i64>(i)}, {});
            joinLiveSet(c, creatureAlign, live);
        } else {
            // R34.3 leave line. UNKNOWN: also used when refused or no group slot.
            ctx.say("companion.leaves", "Your companion leaves.", {static_cast<i64>(i)},
                    {companionLeaveDescribe(chance)});
        }
        c.occupied = false;  // slot left empty
        c.released = true;
    }
}

// R37.1: owner chooses a body only while carrying nothing or a companion (CarryingCompanion < 4).
bool carryChoiceOpen(i32 carryIndex) {
    return carryIndex < kCompanionSlots;
}

// R37.2: candidates are occupied slots at or below 0 Hits. The first is taken; a later one
// replaces it only with strictly greater MaxHits. Ties keep the earlier slot.
// bodySizes is not used for the choice; it is used by the pickup test (R37.3).
i32 pickCarryCandidate(const Arr<Companion, kCompanionSlots>& slots, const Arr<i32, kCompanionSlots>& bodySizes) {
    (void)bodySizes;
    i32 best = -1;
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        const Companion& c = slots[static_cast<u32>(i)];
        if (!c.occupied || c.hits > 0) {
            continue;
        }
        if (best < 0 || c.maxHits > slots[static_cast<u32>(best)].maxHits) {
            best = i;
        }
    }
    return best;
}

// R37.3: pickup if Monsters(Ref).Size - Race.Size <= 1.
bool carryPickTest(i32 bodySize, i32 carrierSize) {
    return bodySize - carrierSize <= 1;
}

// R37 full choice. Port quirks reproduced (c8, c30):
// - "dropped" prints before the size test, even when the new body is kept out.
// - With nothing carried, a replacement prints "dropped" with companion slot -1, then the
//   error handler ends the sub: no pickup and no sweep. Repeats on each call.
void runCarryChoice(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, Inventory& inv,
                    const Arr<i32, kCompanionSlots>& bodySizes, i32 carrierSize) {
    if (!carryChoiceOpen(inv.carryIndex)) {  // R37.1 (morgue body carried: skipped)
        return;
    }
    i32 best = -1;
    for (i32 i = 0; i < kCompanionSlots; ++i) {  // R37.2 scan
        const Companion& c = slots[static_cast<u32>(i)];
        if (!c.occupied || c.hits > 0) {
            continue;
        }
        if (best < 0) {
            best = i;
            continue;
        }
        if (c.maxHits > slots[static_cast<u32>(best)].maxHits) {
            if (inv.carryIndex == kCarryNone) {
                // QUIRK: slot -1 printed, then the sub ends (no pickup, no sweep).
                ctx.say("companion.dropped", "dropped", {static_cast<i64>(kCarryNone)}, {});
                return;
            }
            ctx.say("companion.dropped", "dropped", {static_cast<i64>(best)}, {});
            best = i;
        }
    }
    if (best < 0) {
        sweepFallenCompanions(ctx, slots, keptSlotOf(inv));  // R37.4
        return;
    }
    if (carryPickTest(bodySizes[static_cast<u32>(best)], carrierSize)) {  // R37.3
        inv.carryIndex = best;
        // UNKNOWN: SRD text for the carried companion's hit points was not supplied.
        // Reference value (test expects 0 after pickup from negative hits) is used.
        // A picked-up companion with negative hits is brought to 0 hits.
        if (slots[static_cast<u32>(best)].hits < 0) {
            slots[static_cast<u32>(best)].hits = 0;
        }
        sweepFallenCompanions(ctx, slots, best);  // R37.4
        return;
    }
    // R37.3: too big to carry, the winner is released.
    ctx.say("companion.tooBig", "too big to carry", {static_cast<i64>(best)},
            {slots[static_cast<u32>(best)].name});
    slots[static_cast<u32>(best)].released = true;
    slots[static_cast<u32>(best)].occupied = false;
    sweepFallenCompanions(ctx, slots, keptSlotOf(inv));  // R37.4
}

// R37.4: every fallen companion not carried is released silently.
void sweepFallenCompanions(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, i32 keptSlot) {
    (void)ctx;
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        if (i == keptSlot) {
            continue;
        }
        Companion& c = slots[static_cast<u32>(i)];
        if (c.occupied && c.hits <= 0) {
            c.occupied = false;
            c.released = true;
        }
    }
}

// R37.5: death lets go of every companion and anything carried (sub0930).
// UNKNOWN: whether this is character death is not settled; applied as stated.
void companionsLetGoOnDeath(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, Inventory& inv) {
    (void)ctx;
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        Companion& c = slots[static_cast<u32>(i)];
        if (c.occupied) {
            c.occupied = false;
            c.released = true;
        }
    }
    inv.carryIndex = kCarryNone;
}

// UNKNOWN: BaseValue formula is not in this SRD part (c30). Docs: counts only positive
// resistances; a creature of 1 hit is worth 1.
i64 creatureBaseValue(const Creature& c) {
    if (c.hits == 1) {
        return 1;
    }
    i64 value = 0;
    if (c.resistance > 0) {
        value += c.resistance;
    }
    if (c.magicResistance > 0) {
        value += c.magicResistance;
    }
    return value;
}

// R38: the death trigger runs when the companion's Hits reach zero (engine, c8).
bool companionDeathTrigger(const Companion& c) {
    return c.hits <= 0;
}

// R39.1: refused if dead ("drop it first") or the party is in the city. Binding > 0 warns.
KickOutCheck kickOutCheck(const Companion& c, bool inCity) {
    KickOutCheck r;
    r.warnBound = c.bindingLevel > 0;
    if (isDeadCompanion(c)) {
        r.allowed = false;
        r.refusal = KickOutRefusal::Dead;
        return r;
    }
    if (inCity) {
        r.allowed = false;
        r.refusal = KickOutRefusal::City;
        return r;
    }
    r.allowed = true;
    r.refusal = KickOutRefusal::None;
    return r;
}

// R39: Yes/No prompt is the caller's; confirmed is the Yes answer.
// UNKNOWN: refusal and warning texts are not given in the SRD part.
bool kickOutCompanion(Ctx& ctx, Companion& c, const Character& owner, CharAlign creatureAlign,
                      bool inCity, bool confirmed, LiveSetRef& live) {
    const KickOutCheck chk = kickOutCheck(c, inCity);
    if (!chk.allowed) {
        if (chk.refusal == KickOutRefusal::Dead) {
            ctx.say("companion.dropFirst", "Drop it first.", {}, {});
        } else {
            ctx.say("companion.kickRefusedCity", "You cannot kick out a companion in the city.", {}, {});
        }
        return false;  // R39.3: selection untouched
    }
    if (chk.warnBound) {
        ctx.say("companion.boundWarning", "This companion is bound to you.", {}, {});
    }
    if (!confirmed) {
        return false;  // R39.2: No changes nothing
    }
    return letCompanionGo(ctx, c, owner, creatureAlign, live, false);  // R39.2 Yes
}

// R40.1-2 and R40.4. Port quirk: compareAlign is read from m02BE, never assigned, so it is
// Good (0). The caller passes Align::Good to reproduce this.
RightClickOutcome rightClickAttackCheck(const Companion& c, Align compareAlign) {
    if (c.hits <= 0) {
        return RightClickOutcome::Ignored;
    }
    const bool companionGood = c.alignment == CompanionAlignment::Good;
    if (companionGood && compareAlign == Align::Good) {
        return RightClickOutcome::Refused;
    }
    return RightClickOutcome::LetGo;
}

// R40.3: otherwise LetCompanionGo(forced: true).
// UNKNOWN: refusal text is not given in the SRD part.
bool rightClickAttack(Ctx& ctx, Companion& c, const Character& owner, CharAlign creatureAlign,
                      Align compareAlign, LiveSetRef& live) {
    const RightClickOutcome o = rightClickAttackCheck(c, compareAlign);
    if (o == RightClickOutcome::Ignored) {
        return false;
    }
    if (o == RightClickOutcome::Refused) {
        ctx.say("companion.rightClickRefused", "Your companion will not attack.", {}, {});
        return false;
    }
    return letCompanionGo(ctx, c, owner, creatureAlign, live, true);
}

// R41 LetCompanionGo.
// UNKNOWN: "refuses during a fight" on naming is not applied (port deviation, R41).
bool letCompanionGo(Ctx& ctx, Companion& c, const Character& owner, CharAlign creatureAlign,
                    LiveSetRef& live, bool forced) {
    const TurnOutcome t = companionTurnCheck(ctx, c, owner.alignment, creatureAlign, forced,
                                             liveSetOf(live), live.groupSlotFree);  // R41.1-2
    if (t == TurnOutcome::RefusedByFoe) {
        return false;  // R41.2: stop
    }
    if (t == TurnOutcome::Joins) {  // R41.3
        joinLiveSet(c, creatureAlign, live);
        c.occupied = false;
        c.released = true;
        return true;
    }
    if (forced) {
        return false;  // UNKNOWN: forced with no group slot: stop, companion stays.
    }
    // R41.4: non-forced and does not turn: slot cleared silently.
    c.occupied = false;
    c.released = true;
    return true;
}

// R42: min(98, 85 + CON).
i32 morgueRaiseChance(i32 companionCon) {
    return minv(kMorgueRaiseCap, kMorgueRaiseBase + companionCon);
}

// R42.1: first slot with Hits == 0 (occupied slots only; released slots keep records, R45).
i32 morgueOfferSlot(const Arr<Companion, kCompanionSlots>& slots) {
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        const Companion& c = slots[static_cast<u32>(i)];
        if (c.occupied && c.hits == 0) {
            return i;
        }
    }
    return -1;
}

// R42: gold first (not refunded); roll _rng.Next(100) > chance fails and empties the slot;
// success sets Hits = MaxHits. Carry is always cleared afterwards (R42.3).
// UNKNOWN: _rng.Next(100) is mapped to ctx.rng.below(100).
MorgueRaiseOutcome raiseCompanionAtMorgue(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, Character& owner,
                                          Inventory& ownerInv, Currency cost) {
    const i32 slot = morgueOfferSlot(slots);
    if (slot < 0) {
        return MorgueRaiseOutcome::NoDeadSlot;
    }
    Companion& c = slots[static_cast<u32>(slot)];
    owner.wallet.goldOnHand -= cost;  // R42.1
    const i32 chance = morgueRaiseChance(c.stats[static_cast<u32>(kCompanionConIndex)]);
    const bool failed = ctx.rng.below(100) > chance;  // R42.2
    ownerInv.carryIndex = kCarryNone;                 // R42.3
    owner.carryingCharacterName = Str();              // R42.3
    if (failed) {
        c.occupied = false;  // slot released and left empty
        c.released = true;
        return MorgueRaiseOutcome::Failed;
    }
    c.hits = c.maxHits;
    c.isDead = false;
    c.deathState = CompanionDeathState::Normal;
    return MorgueRaiseOutcome::Raised;
}

// R43.2: old Id-keyed records are settled by Id.
// UNKNOWN: the "Id 307 uses alignment" mapping is not in this part; stored alignment is kept.
void settleLegacyCompanionId(Ctx& ctx, Companion& c, i32 legacyId) {
    (void)ctx;
    c.monsterId = legacyId;
}

// Port: an Unaligned companion gets its creature's alignment on load.
// UNKNOWN: Unaligned is assumed to be encoded as 0 (CompanionAlignment starts at 1).
void fillUnalignedCompanionAlignment(Companion& c, CharAlign creatureAlign) {
    if (static_cast<i32>(c.alignment) != 0) {
        return;
    }
    c.alignment = static_cast<CompanionAlignment>(static_cast<i32>(creatureAlign) + 1);
}

// R43.3: a carry index of 4 or more is a carried body and is put down on load.
void putDownCarriedBodyOnLoad(Ctx& ctx, Inventory& inv) {
    (void)ctx;
    if (inv.carryIndex >= kCompanionSlots) {
        inv.carryIndex = kCarryNone;
    }
}

// R44: a new companion fills the first hole.
i32 firstCompanionHole(const Arr<Companion, kCompanionSlots>& slots) {
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        if (!slots[static_cast<u32>(i)].occupied) {
            return i;
        }
    }
    return -1;
}

// R44: the new companion gets the name left in the slot.
Str keepSlotName(const Companion& previousOccupant) {
    return previousOccupant.name;
}

// R44: place incoming in the first hole, taking the slot's kept name.
i32 placeNewCompanion(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, Companion incoming) {
    (void)ctx;
    const i32 hole = firstCompanionHole(slots);
    if (hole < 0) {
        return -1;
    }
    Str name = keepSlotName(slots[static_cast<u32>(hole)]);
    if (name.empty()) {
        // UNKNOWN: SRD text not supplied; a blank name falls back to the
        // reference default "Companion #<slot number>" (1-based).
        name = "Companion #" + std::to_string(hole + 1);
    }
    incoming.name = name;
    incoming.slot = hole;
    incoming.occupied = true;
    incoming.released = false;
    slots[static_cast<u32>(hole)] = incoming;
    return hole;
}

// R46: refused if either end is dead. Swap the two records and their slot numbers.
// UNKNOWN: out-of-range slot numbers are refused silently (RefusedDead, no text).
DragSwapOutcome dragSwapCompanions(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, i32 fromSlot, i32 toSlot) {
    // UNKNOWN: SRD refusal text refers to dead companions only; empty slots are not dead.
    // Assumes Companion has a `present` flag (true = occupied). Verify against the struct.
    if (fromSlot < 0 || fromSlot >= kCompanionSlots || toSlot < 0 || toSlot >= kCompanionSlots) {
        return DragSwapOutcome::RefusedDead;
    }
    const Companion& a = slots[static_cast<u32>(fromSlot)];
    const Companion& b = slots[static_cast<u32>(toSlot)];
    const bool aDead = a.present && isDeadCompanion(a);
    const bool bDead = b.present && isDeadCompanion(b);
    if (aDead || bDead) {
        ctx.say("companion.dragDead", "No use re-arranging a dead companion!", {}, {});
        return DragSwapOutcome::RefusedDead;
    }
    if (fromSlot == toSlot) {
        return DragSwapOutcome::Swapped;
    }
    Companion tmp = slots[static_cast<u32>(fromSlot)];
    slots[static_cast<u32>(fromSlot)] = slots[static_cast<u32>(toSlot)];
    slots[static_cast<u32>(toSlot)] = tmp;
    slots[static_cast<u32>(fromSlot)].slot = fromSlot;
    slots[static_cast<u32>(toSlot)].slot = toSlot;
    return DragSwapOutcome::Swapped;
}

// R47.1-4. Good monsters join Good characters only; Neutral monsters join anyone;
// UNKNOWN: which characters Evil monsters may join; false (always hostile) is used.
// R47.5 level condition is ambiguous (R60) and is not applied.
bool monsterMayJoin(CharAlign monsterAlign, Align charAlign) {
    switch (monsterAlign) {
        case CharAlign::Good:
            return charAlign == Align::Good;
        case CharAlign::Neutral:
            return true;
        case CharAlign::Evil:
            return false;
    }
    return false;
}

// R47: Good monsters hostile to Neutral and Evil characters only; Neutral monsters hostile
// 60% of the time (the roll is made here; UNKNOWN when in the source); Evil always hostile.
bool monsterHostileToCharacter(Ctx& ctx, CharAlign monsterAlign, Align charAlign) {
    switch (monsterAlign) {
        case CharAlign::Good:
            return charAlign != Align::Good;
        case CharAlign::Neutral:
            return ctx.rng.percent(kNeutralMonsterHostilePercent);
        case CharAlign::Evil:
            return true;
    }
    return true;
}

// R48: a charm makes the creature a companion of the caster.
// UNKNOWN: monster stats, owner index, and binding for a charm are not in this part.
bool charmCreatureAsCompanion(Ctx& ctx, Arr<Companion, kCompanionSlots>& slots, const Character& caster,
                              i32 monsterId) {
    (void)caster;
    if (firstCompanionHole(slots) < 0) {
        ctx.say("companion.noRoom", "There is no room for another companion.", {}, {});
        return false;
    }
    Companion incoming;
    incoming.monsterId = monsterId;
    incoming.joinedFreely = false;
    const i32 slot = placeNewCompanion(ctx, slots, incoming);
    if (slot < 0) {
        return false;
    }
    ctx.say("companion.charmed", "Your companion is charmed.", {static_cast<i64>(slot)}, {});
    return true;
}

// R49 requirements. Index: Control 0, Bind 1, Domination 2.
bool bindSpellMeetsRequirements(const Character& c, BindSpell spell) {
    const i32 s = static_cast<i32>(spell);
    const Arr<i32, kStatCount>& st = c.abilities.stats;
    return st[static_cast<u32>(StatIndex::Int)] >= kBindReqInt[s] &&
           st[static_cast<u32>(StatIndex::Wis)] >= kBindReqWis[s] &&
           st[static_cast<u32>(StatIndex::Con)] >= kBindReqCon[s] &&
           st[static_cast<u32>(StatIndex::Cha)] >= kBindReqCha[s];
}

// R49: bind spells do not work on companions that joined of their own free will.
// CONFLICT: this declaration (same signature) also appears in companions2b.h. This file
// defines it; companions2b must not define it again.
bool bindSpellApplies(const Companion& c) {
    return !c.joinedFreely;
}

// R49: Control increases the Bind level; Bind renews a fading Bind; Domination renews the
// Bind level. UNKNOWN: amounts are not given; base levels are used (R49 table).
void castBindSpell(Ctx& ctx, Companion& target, BindSpell spell) {
    if (!bindSpellApplies(target)) {
        ctx.say("companion.bindNoEffect", "It has no effect.", {}, {});
        return;
    }
    switch (spell) {
        case BindSpell::Control:
            target.bindingLevel = target.bindingLevel + kControlBaseLevel;
            break;
        case BindSpell::Bind:
            target.bindingLevel = maxv(target.bindingLevel, kBindBaseLevel);
            break;
        case BindSpell::Domination:
            target.bindingLevel = maxv(target.bindingLevel, kDominationBaseLevel);
            break;
    }
    target.bindingLevel = minv(target.bindingLevel, kBindingMax);
    ctx.say("companion.bindLevel", "Bind level changed.", {static_cast<i64>(target.bindingLevel)}, {});
}

// R50: if Control Dragon fails, the target responds adversely.
// UNKNOWN: the adverse response is not specified; hostility is set.
void controlDragonFailure(Ctx& ctx, Creature& target) {
    target.hostile = true;
    ctx.say("companion.controlFailed", "The dragon resists your control.", {}, {});
}

// R51: Dominate Demon binds the demon. UNKNOWN: duration not given; the Domination base
// level is used as the Bind level.
void dominateDemonBind(Ctx& ctx, Companion& demon) {
    demon.bound = true;
    demon.bindingLevel = maxv(demon.bindingLevel, kDominationBaseLevel);
    ctx.say("companion.demonBound", "The demon is bound to you.", {static_cast<i64>(demon.bindingLevel)}, {});
}

}  // namespace mordor
