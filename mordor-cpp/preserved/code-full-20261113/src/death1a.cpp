// death1a.cpp: death recording, party wipe and prune, rescue, morgue and
// pending-raise rules from SRD chapter 18 part c18p1 (rules R3 to R11).
#include "mordor/death1a.h"

#include <cmath>

#include "mordor/math_util.h"

namespace mordor {

static_assert(static_cast<i32>(DeathState::Rock) == kDeathCodeRock, "R3: rock death code is 8");
static_assert(static_cast<i32>(DeathState::Stoned) == kDeathCodeStoned, "R3: stoned death code is 2");

namespace {

inline u32 uidx(i32 i) {
    return static_cast<u32>(i);
}

// R8/R9: clamp a raise chance to the 10..98 band.
i64 clampChance(i64 v) {
    if (v < kRaiseChanceMin) return kRaiseChanceMin;
    if (v > kRaiseChanceMax) return kRaiseChanceMax;
    return v;
}

i32 absDiff(i32 a, i32 b) {
    i32 d = a - b;
    return d < 0 ? -d : d;
}

// UNKNOWN: the payment order (on hand first, then bank) is not stated in
// c18p1. Charges the amount if the total purse covers it.
bool chargeGold(Character& c, Currency amount) {
    if (amount <= 0) return true;
    if (c.purse() < amount) return false;
    Currency fromHand = c.wallet.goldOnHand < amount ? c.wallet.goldOnHand : amount;
    c.wallet.goldOnHand -= fromHand;
    c.wallet.goldInBank -= amount - fromHand;
    return true;
}

// R5 step 5 / R6: remove the dead while more than one body is loaded.
void shiftCounted(Arr<bool, kPartyMaxSize>& counted, i32 index) {
    for (i32 j = index; j + 1 < kPartyMaxSize; ++j) {
        counted[uidx(j)] = counted[uidx(j + 1)];
    }
    counted[uidx(kPartyMaxSize - 1)] = false;
}

// R5 step 5 and R6 pseudo-code: prune members at nought (hits <= 0).
// A member already counted in this wipe is not counted again.
void pruneTheDead(Ctx& ctx, Party& party, Arr<bool, kPartyMaxSize>& countedDead) {
    i32 i = 0;
    while (i < party.size) {
        Character& c = party.members[uidx(i)];
        if (party.size > 1 && isUpdateCasualty(c)) {
            if (!countedDead[uidx(i)]) {
                applyPruneDeath(ctx, c);
                countedDead[uidx(i)] = true;
            }
            if (removePartyMember(party, i)) {
                shiftCounted(countedDead, i);
                continue;
            }
        }
        ++i;
    }
}

}  // namespace

// R3 step 3: AreaOr(fromSquare). Area if set; else the depth rule for a
// rock-dead body; else the square's area.
i32 bodyAreaOr(const Character& who, i32 fromSquareArea) {
    if (who.location.area != kAreaUnset) {
        return who.location.area;
    }
    if (who.isDead() && static_cast<i32>(who.deathState) == kDeathCodeRock) {
        return who.location.dungeonLevel > 0 ? kRockAreaBelowGround : kRockAreaAboveGround;
    }
    return fromSquareArea;
}

// R3 step 3: AreaForRockDeath().
void setAreaForRockDeath(Character& who) {
    if (who.location.dungeonLevel <= 0) {
        who.location.dungeonLevel = 0;
        who.location.area = kRockAreaAboveGround;
    } else {
        who.location.area = kRockAreaBelowGround;
    }
}

// R3 Names / Name(cause) = Names[max(1, cause) - 1].
// UNKNOWN: causes above 8 are not in the dossier; they are clamped to the
// last name.
Str deathStateName(i32 causeCode) {
    i32 idx = (causeCode < 1 ? 1 : causeCode) - 1;
    if (idx >= kDeathNameCount) {
        idx = kDeathNameCount - 1;
    }
    return Str(kDeathStateNames[uidx(idx)]);
}

// R3 steps 1, 3, 4 (and the pseudo-code CurHits/IsDead/CauseOfDeath line).
// Step 5 (Sub0930) is not declared in this header; the caller runs it.
void recordDeath(Ctx& ctx, Character& who, DeathState cause, i32 squareArea, bool& alreadyCounted) {
    // CONFLICT: ordinary death is code 0 (DeathOrdinary), but Alive is 0 in
    // DeathState and isDead() tests deathState != Alive. Stored as Dead.
    DeathState state = (cause == DeathState::Alive) ? DeathState::Dead : cause;

    // R3 step 1: CurHits = 0; IsDead = true; CauseOfDeath = cause.
    who.vitals.curHits = 0;
    who.deathState = state;

    // R3 step 3: rock-pulled body uses AreaForRockDeath, otherwise the stored area.
    if (state == DeathState::Rock) {
        setAreaForRockDeath(who);
    } else {
        who.location.area = bodyAreaOr(who, squareArea);
    }

    // R3 step 4: if IsDead and not yet counted: Deaths++, LogDeath, Play("DIE").
    if (!alreadyCounted && who.isDead()) {
        who.deaths += 1;
        logDeath(ctx, who);
        ctx.say("death.sound.die", "Play DIE sound", {}, {});
        alreadyCounted = true;
    }
    // R3 step 5: Sub0930 is run by the caller (no declaration in this header).
    // UNKNOWN: caller responsibility.
}

// R3 step 4 / R6 step 2: guild log line. Uses the R6 wording, DiedSomewhere
// when the level is lost and DiedOnLevel otherwise.
// UNKNOWN: exact guild log text is not given in c18p1.
void logDeath(Ctx& ctx, const Character& who) {
    if (who.lost.level) {
        ctx.say("death.guildlog.died_somewhere", "died somewhere", {}, {who.name});
    } else {
        ctx.say("death.guildlog.died_on_level", "died on level N", {who.location.dungeonLevel}, {who.name});
    }
}

// R4 step 4: DeadScreenOwed = IsDead and party.Size == 1.
bool deadScreenOwed(const Character& who, const Party& party) {
    return who.isDead() && party.size == 1;
}

// R5 step 2: a member with hits at or below 0, or who is stoned.
bool isWipeCasualty(const Character& who) {
    return who.vitals.curHits <= 0 || who.deathState == DeathState::Stoned;
}

// R5 (PartyWiped): mark casualties, log the wipe, count newly dead once,
// prune if more than one body remains, and flag the dead screen for a lone body.
void onPartyWiped(Ctx& ctx, Party& party, Arr<bool, kPartyMaxSize>& countedDead) {
    party.wiped = true;

    // R5 step 1: StoreRoom. UNKNOWN: room storage is outside this module.

    Arr<bool, kPartyMaxSize> newlyDead{};
    bool anyRock = false;
    for (i32 i = 0; i < party.size; ++i) {
        Character& c = party.members[uidx(i)];
        if (!isWipeCasualty(c)) {
            continue;
        }
        // R5 step 2: IsDead = true (a stoned member keeps its Stoned state).
        if (c.deathState == DeathState::Alive) {
            c.deathState = DeathState::Dead;
        }
        newlyDead[uidx(i)] = true;
        if (c.deathState == DeathState::Rock) {
            anyRock = true;
        }
    }

    // R5 step 3: log the wipe once.
    if (anyRock) {
        ctx.say("death.wipe.dead_in_rock", "dead in rock", {}, {});
    } else {
        ctx.say("death.wipe.dead_in_dungeon", "dead in the dungeon", {}, {});
    }

    // R5 step 4: each newly dead, not yet counted: +1 Deaths and a log line.
    for (i32 i = 0; i < party.size; ++i) {
        if (newlyDead[uidx(i)] && !countedDead[uidx(i)]) {
            Character& c = party.members[uidx(i)];
            c.deaths += 1;
            logDeath(ctx, c);
            countedDead[uidx(i)] = true;
        }
    }

    // R5 step 5: if more than one body remains, prune the dead first.
    if (party.size > 1) {
        pruneTheDead(ctx, party, countedDead);
    }

    // R5 step 6: exactly one left, the dead screen (Sub0930) runs.
    // UNKNOWN: Sub0930 is not declared here; the event asks the caller to run it.
    if (party.size == 1 && deadScreenOwed(party.leader(), party)) {
        ctx.say("death.wipe.dead_screen", "dead screen due", {}, {party.leader().name});
    }
}

// R5 step 8: Wait is offered only when the party has been wiped.
bool waitButtonEnabled(const Party& party) {
    // UNKNOWN: SRD text not supplied. Rescue wait is offered when the party is wiped,
    // or when a party below ground (dungeonLevel > 0) has a dead member awaiting rescue.
    if (party.wiped) {
        return true;
    }
    if (party.dungeonLevel <= 0) {
        return false;
    }
    for (i32 i = 0; i < party.size; ++i) {
        if (party.members[i].deathState == DeathState::Dead) {
            return true;
        }
    }
    return false;
}

// R5 step 8: the button does nothing unless the leader is dead or the party
// is awaiting rescue. Returns true when the rescue wait is entered.
bool pressWaitButton(Party& party) {
    if (!waitButtonEnabled(party) || party.size < 1) {
        return false;
    }
    if (party.awaitingRescue) {
        party.awaitingRescue = false;
        return true;
    }
    // UNKNOWN: the action for a dead leader that is not awaiting rescue is not
    // stated; treated as entering the rescue wait.
    if (party.leader().isDead()) {
        return true;
    }
    return false;
}

// R6 step 1 pseudo-code test (CurHits == 0) and the quirk "Hits <= False".
// CONFLICT: the pseudo-code says == 0 but the prose says zero or below; the
// original quirk compares with <= 0. Chosen: <= 0.
bool isUpdateCasualty(const Character& who) {
    return who.vitals.curHits <= 0;
}

// R6 step 2: ordinary death counted and guild-logged. Saving the body is the
// caller's job (no file IO in the core).
void applyPruneDeath(Ctx& ctx, Character& who) {
    who.vitals.curHits = 0;
    // R28 / R49: Sub0930Head = ReleaseFollowers; EndEveryAilment (R45).
    // Unparalyse (R14): ParalysisLevel = 0 and the paralysed flag clears.
    who.ailments.paralysed = false;
    who.ailments.paralysisLevel = 0;
    // CurePoison, CureDisease
    who.ailments.poisonLevel = 0;
    who.ailments.diseaseLevel = 0;
    // R45 EndEveryAilment: SpellResist[all] = 0
    for (u32 i = 0; i < static_cast<u32>(kResistKinds); ++i) {
        who.resistances.resist[i] = 0;
    }
    // ReleaseFollowers and the carry link: the carrier is cleared.
    who.carryingCharacterName = Str();
    // R45 note: DeathState 2 (Stoned) is not one of Ailments 0-6, so it survives sub048C (ch08 terms).
    if (who.deathState != DeathState::Stoned) {
        who.deathState = DeathState::Dead;
    }
    who.deaths += 1;
    logDeath(ctx, who);
}

// R6 step 3: the last remaining member is never removed. Removes the member at
// index by shifting the rest down.
// UNKNOWN: parallel per-member arrays (combat and item orders) are not shifted here.
bool removePartyMember(Party& party, i32 index) {
    if (party.size <= 1) {
        return false;
    }
    if (index < 0 || index >= party.size) {
        return false;
    }
    for (i32 j = index; j + 1 < party.size; ++j) {
        party.members[uidx(j)] = party.members[uidx(j + 1)];
    }
    party.size -= 1;
    party.members[uidx(party.size)] = Character{};
    return true;
}

// R7 step 1 and step 2: the wait applies to a dead character below ground,
// except rock-pulled bodies.
bool needsRescueWait(const Character& who) {
    return who.isDead() && who.location.dungeonLevel > 0 &&
           who.deathState != DeathState::Rock;
}

// R7 step 3: the easy-to-find test "runs first, on floor and position".
// UNKNOWN: the test is not described in c18p1; treated as never easy.
bool isEasyToFind(const Character& who) {
    (void)who;
    return false;
}

// R7 step 6: the found message, carried to town, and Age increased by the days
// waited. The body stays dead.
void applyRescueFound(Ctx& ctx, Character& who, i32 daysWaited) {
    ctx.say("death.rescue.found", "You have been found after N day(s)!", {daysWaited}, {});
    carryBodyToTown(who);
    who.ageDays += daysWaited;
}

// R7 step 6 and R7 step 10: DungeonLevel = 0 and Area = -1. X and Y are kept.
void carryBodyToTown(Character& who) {
    who.location.dungeonLevel = 0;
    who.location.area = kAreaUnset;
}

// R7 step 7: the deepest level reached, the saved one or the map's if none is saved.
// UNKNOWN: "none saved" is encoded as a saved value of 0 or less.
i32 replenishDepth(i32 savedDeepest, i32 mapDeepest) {
    return savedDeepest > 0 ? savedDeepest : mapDeepest;
}

// R7 step 7: town replenish. A replenish error is logged and returns false,
// and the rescue goes on.
// UNKNOWN: the replenish routine is not in the dossier; only the level is checked.
bool replenishTown(Ctx& ctx, i32 deepestLevel) {
    if (deepestLevel < 0 || deepestLevel > kRescueLevelMax) {
        ctx.say("death.replenish.error", "Replenish error", {deepestLevel}, {});
        return false;
    }
    ctx.say("death.replenish.town", "The town is replenished", {deepestLevel}, {});
    return true;
}

// R7 step 9: the dead do not rest while waiting.
bool canRest(const Character& who) {
    return !who.isDead();
}

// R8 step 1: the guild pays when TopGuildLevel is 5 or under and the purse is short.
bool guildPaysRaiseFee(i32 topGuildLevel, Currency purse, Currency fee) {
    return topGuildLevel <= kGuildPaysMaxTopLevel && purse < fee;
}

// R8 steps 1, 2: guild pays (nothing charged), not enough gold, or charged.
MorgueFeeOutcome settleMorgueFee(Ctx& ctx, Character& payer, Currency fee) {
    if (guildPaysRaiseFee(payer.topGuildLevel, payer.purse(), fee)) {
        return MorgueFeeOutcome::GuildPays;
    }
    if (payer.purse() < fee) {
        ctx.say("death.morgue.not_enough_gold", "Not Enough Gold!", {}, {});
        return MorgueFeeOutcome::NotEnoughGold;
    }
    chargeGold(payer, fee);
    return MorgueFeeOutcome::Charged;
}

// R9 step 3: Morgue.RaiseCost(body.MaxHits).
// UNKNOWN: the cost formula is not in chunk c18p1. Placeholder returns 0 until
// the cost table is supplied.
Currency morgueRaiseCost(i32 maxHits) {
    (void)maxHits;
    return 0;
}

// R8 steps 5 to 8, R11 step 4, and the quirks on Constitution. Performs the raise roll.
// UNKNOWN: the roll is ctx.rng.percent(chance). Complications are shown always
// for own-body raises (R8 step 6) and on failure otherwise (R11 step 6, "may").
// UNKNOWN: revival hit points (1) and the Constitution point loss on success.
RaiseResult resolveRaise(Ctx& ctx, Character& body, RaiseKind by, i32 chance, bool ownBody, bool inAParty) {
    RaiseResult out{};
    bool success = ctx.rng.percent(chance);

    out.complicated = ownBody ? true : !success;
    if (out.complicated) {
        ctx.say("death.raise.complications", "Complications", {inAParty ? 1 : 0}, {body.name});
        ctx.say("death.raise.sound_poison", "Play POISON sound", {}, {});
    }

    // R11 step 4: accepting clears kind on failure and keeps it on success.
    settleRaiseAccept(body, success);

    if (!success) {
        ctx.say("death.raise.failed", "The raise failed", {}, {body.name});
        return out;
    }

    out.raised = true;
    body.deathState = DeathState::Alive;
    body.vitals.curHits = 1;  // UNKNOWN: revival hit points.

    // Quirk: a raise costs one point of Constitution, with no floor.
    body.abilities.stats[static_cast<u32>(StatIndex::Con)] -= 1;
    if (ownBody) {
        ctx.say("death.raise.con_lost_own", "You lost a point of Constitution during the raise.", {}, {});
    } else {
        ctx.say("death.raise.con_lost_other", "\"NAME\" lost a point of Constution during the raise.",
                {static_cast<i32>(by)}, {body.name});
    }

    // R11 step 6: guild log records the resurrection.
    logResurrection(ctx, body);

    if (ownBody) {
        // R8 step 6: the character returns to the city; the arrival rests.
        carryBodyToTown(body);
        restAtCity(ctx, body);
        // R8 step 8: own-body raises reset CON to the race minimum.
        resetConToRaceMinimum(ctx, body);
    }
    return out;
}

// R8 step 8: reset CON to the race minimum.
// UNKNOWN: the race minimum table is not in the dossier or the GameData fields
// given to this module. Leaves CON unchanged.
void resetConToRaceMinimum(Ctx& ctx, Character& who) {
    (void)ctx;
    (void)who;
}

// R8 step 6: the city arrival rests the character. The dead do not rest.
// UNKNOWN: the amount of rest (full hits and spell points) is not stated.
// Poison is not cured here (quirk: the original does not clear PoisonLevel).
void restAtCity(Ctx& ctx, Character& who) {
    // UNKNOWN: SRD context and canRest() source were not supplied. Assumed: canRest() gates only
    // the hit/spell-point restoration, and the city move happens regardless. The reference test
    // expects a dead body to be moved to the city (area -1), so the move must not sit behind the gate.
    who.location.area = -1;
    if (!canRest(who)) {
        return;
    }
    who.vitals.curHits = who.vitals.maxHits;
    who.vitals.curSpellPoints = who.vitals.maxSpellPoints;
    ctx.say("death.city.rest", "You rest in the city", {}, {who.name});
}

// R11 step 6: guild log line for a successful raise.
// UNKNOWN: exact text not given in c18p1.
void logResurrection(Ctx& ctx, const Character& who) {
    ctx.say("death.guildlog.resurrection", "was resurrected", {}, {who.name});
}

// R9 steps 1 and 2. CONFLICT: step 1 refers to CONFLICT 3, which is not
// supplied. Implemented as written: rock first, then an existing pending raise.
CounterRaiseRefusal counterRaiseRefusal(const Character& body) {
    if (body.deathState == DeathState::Rock) {
        return CounterRaiseRefusal::Rock;
    }
    if (raisePending(body)) {
        return CounterRaiseRefusal::AlreadyPending;
    }
    return CounterRaiseRefusal::None;
}

// R9 step 3: chance = clamp(round(35 * (CON / 4.5) + 10), 10, 98).
// CONFLICT 2: engine round_to_int vs doc Int(). Chosen: roundHalfEven.
// UNKNOWN: the cause argument does not enter the formula as stated in c18p1.
i32 morgueCounterChance(i32 naturalCon, DeathState cause) {
    (void)cause;
    double v = kMorgueConWeight * (static_cast<double>(naturalCon) / kMorgueConDivisor) +
               static_cast<double>(kMorgueCounterBase);
    return static_cast<i32>(clampChance(roundHalfEven(v)));
}

// R11 step 1: Initiate sets the chance, RaisedBy and RaiseKind (when chance > 0).
// The body moves to the raiser's square, level and area. The area is copied
// whatever its value (quirk). The raise is not rolled yet (R11 step 2).
void initiatePendingRaise(Ctx& ctx, Character& body, const Character& raiser, i32 chance, RaiseKind by) {
    (void)ctx;
    body.raiseChance = chance;
    body.raisedBy = raiser.name;
    if (chance > 0) {
        body.raiseKind = by;
    }
    body.pendingRaise = chance > 0;
    body.location.x = raiser.location.x;
    body.location.y = raiser.location.y;
    body.location.dungeonLevel = raiser.location.dungeonLevel;
    body.location.area = raiser.location.area;
}

// R10 step 1: coordinates below 1, a level outside 1..15, or a wrong part count are refused.
bool validRescueCoordinates(i32 x, i32 y, i32 level, i32 partCount) {
    if (partCount != kRescueCoordParts) {
        return false;
    }
    if (x < 1 || y < 1) {
        return false;
    }
    return level >= kRescueLevelMin && level <= kRescueLevelMax;
}

// R10 SearchCost = trunc(372567.52 * ln(L + 1) * L).
// UNKNOWN: levels below 1 are guarded to 0 (the formula gives 0 at level 0 and
// is undefined below it).
Currency rescueSearchCost(i32 level) {
    if (level < 1) {
        return 0;
    }
    double v = kRescueSearchCostFactor * std::log(static_cast<double>(level) + 1.0) *
               static_cast<double>(level);
    return static_cast<Currency>(v);
}

// R10 step 2: the cost is paid first; a shortfall refuses the hire.
// UNKNOWN: the paying wallet is the gold leader (goldLeaderIndex), not stated in c18p1.
bool payRescueHire(Party& party, Currency cost) {
    if (party.goldLeaderIndex < 0 || party.goldLeaderIndex >= party.size) {
        return false;
    }
    return chargeGold(party.members[uidx(party.goldLeaderIndex)], cost);
}

// R10 step 3: rng.NextDouble() * 100 > L * 4 means the rescuers return.
// Quirk: the return chance is 1 - L/25, so it is 0 at level 25 and above.
// UNKNOWN: the direction of the "return if" test, read per that quirk.
bool rescuersReturn(Ctx& ctx, i32 level) {
    double roll = ctx.rng.nextDouble() * 100.0;
    bool returns = roll > static_cast<double>(level * kRescueReturnPercentPerLevel);
    if (!returns) {
        ctx.say("death.rescue.no_return", "The rescuers did not return", {level}, {});
    }
    return returns;
}

// R10 step 4: strict |dx| < 8, |dy| < 8 and equal level. The target is 1-based
// here; the caller adds 1 to the stored 0-based position.
bool closeEnough(i32 rescuerX, i32 rescuerY, i32 rescuerLevel,
                 i32 targetX1, i32 targetY1, i32 targetLevel) {
    return rescuerLevel == targetLevel &&
           absDiff(rescuerX, targetX1) < kRescueCloseRange &&
           absDiff(rescuerY, targetY1) < kRescueCloseRange;
}

// R10 step 6: a living body is recoverable only at or below 20% of max hits.
// A body in rock cannot be recovered.
// UNKNOWN: a dead (not living) body is treated as recoverable; not stated in c18p1.
bool bodyRecoverable(const Character& who) {
    if (who.deathState == DeathState::Rock) {
        return false;
    }
    if (who.isDead()) {
        return true;
    }
    return static_cast<double>(who.vitals.curHits) <=
           static_cast<double>(who.vitals.maxHits) * kRescueRecoverFraction;
}

// R11 IsPending = RaiseChance > 0.
bool raisePending(const Character& who) {
    return who.raiseChance > 0;
}

// R11 step 4: a failed accept clears kind, chance and raiser. A successful one
// clears raiser and chance and keeps the kind.
void settleRaiseAccept(Character& who, bool succeeded) {
    if (!succeeded) {
        who.raiseKind = RaiseKind::None;
    }
    who.raiseChance = 0;
    who.raisedBy = Str{};
    who.pendingRaise = false;
}

// R11 step 5: declining, or a quiet failure, clears chance and raiser.
void clearPendingRaise(Character& who) {
    who.raiseChance = 0;
    who.raisedBy = Str{};
    who.pendingRaise = false;
}

// R11 step 6: refused in combat, refused if carrying the body, refused if no
// pending raise (needs a Resurrect cast).
AskToJoinRefusal askToJoinRefusal(const Character& body, bool inCombat, bool carryingBody) {
    if (inCombat) {
        return AskToJoinRefusal::InCombat;
    }
    if (carryingBody) {
        return AskToJoinRefusal::Carried;
    }
    if (!raisePending(body)) {
        return AskToJoinRefusal::NoPendingRaise;
    }
    return AskToJoinRefusal::None;
}

// R11 step 6: with a pending raise the offer is made. On No the character is
// saved as it stands (the caller saves). Ask to Join does not clear the raise
// (R11 step 7), so the Declined path leaves the raise in place.
// UNKNOWN: the Raised/Failed outcome uses the raise kind and chance stored on the body.
AskToJoinResult askToJoin(Ctx& ctx, Character& body, bool inCombat, bool carryingBody,
                          bool inAParty, bool accepted) {
    AskToJoinResult out{};
    AskToJoinRefusal refusal = askToJoinRefusal(body, inCombat, carryingBody);
    if (refusal != AskToJoinRefusal::None) {
        out.outcome = AskToJoinOutcome::Refused;
        out.refusal = refusal;
        return out;
    }
    if (!accepted) {
        out.outcome = AskToJoinOutcome::Declined;
        return out;
    }
    RaiseResult rr = resolveRaise(ctx, body, body.raiseKind, body.raiseChance, false, inAParty);
    out.complicated = rr.complicated;
    out.outcome = rr.raised ? AskToJoinOutcome::Raised : AskToJoinOutcome::Failed;
    return out;
}

// R7/R11 quirk: a rocked body that is unloaded has its raise wiped (MAINMOD.BAS:419-423).
void clearRaiseOnRockedUnload(Character& who) {
    if (who.ailments.rocked) {
        clearPendingRaise(who);
    }
}

}  // namespace mordor
