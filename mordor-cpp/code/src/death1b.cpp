// death1b.cpp: death and raise rules (R12 to R23, constants of R24).
// Module death1b.

// CONFLICT: death1b.h declares resolveRaise returning RaiseResult, which is defined in
// death1a.h. death1a.h is included first so the declaration is complete.
#include "mordor/death1a.h"
#include "mordor/death1b.h"
#include "mordor/math_util.h"

#include <cmath>
#include <cstddef>

// CONFLICT: death1b.h uses HallRecord (HallOfRecords::records). No given header defines
// HallRecord, so the header does not compile until it is defined elsewhere. Nothing here
// defines it.

// CONFLICT: hallStatField and hallDateField are declared both in death1b.h and in death3.h.
// They are defined once here (this module owns the death1b.h declarations). death3.h must not
// define them as well, or the link fails.

namespace mordor {

namespace {

// Module-local constants (constexpr only, no mutable state).
constexpr i64 kGoldToCurrency = 10000;             // Currency is 1/10000 gold
constexpr double kMorgueFeeFactor = 2912.32;       // R15 morgue fee factor
constexpr double kMorgueFeeDivisor = 1.5;          // R15 fee divisor
constexpr double kCompanionMorgueFactor = 1244.34; // R16 companion cost factor
constexpr i32 kCompanionMorgueBase = 75;           // R16 min(98, 75 + CON)
constexpr i32 kRaiseChanceCap = 98;                // R13, R14, R16 upper clamp
constexpr i32 kSpellChanceFloor = 10;              // R13 lower clamp
constexpr i32 kRockSpellChance = 10;               // R13 rock chance on the spell path
constexpr i32 kMorgueOwnBonus = 40;                // R14 own body bonus (CONFLICT 1)
constexpr i32 kMorgueCounterBonus = 10;            // R14 counter bonus (CONFLICT 1)
constexpr i32 kMorgueChanceFloor = 1;              // R14 quirk: tier lifted to 1
constexpr i32 kGuildPaysMaxLevel = 5;              // R15 lowLevel bound
constexpr i32 kRaiseRollRange = 100;               // Int(Rnd*100)
// UNKNOWN: the complication chance used on a quiet raise failure is not given in the SRD chunk.
// Stub 0 means a quiet failure never complicates.
constexpr i32 kComplicationChanceStub = 0;
// UNKNOWN: racial minimum stat values are not given in the SRD chunk, and RaceDef field names
// are not given. Stub floor 1.
constexpr i32 kRacialMinimumStub = 1;
constexpr i32 kCompanionConFallback = 10;          // R16 "?? 10" fallback
constexpr double kOldAgeThresholdPct = 98.0;       // R19 threshold
constexpr double kOldAgeChanceBase = 5.0;          // R19 "5 + Ln(...)"
constexpr i64 kHallStatMultiplier = 10000;         // R22 stat field factor
constexpr i64 kHallDateOffset = 2;                 // R22 date field offset
constexpr i32 kStoneConLoss = 1;                   // R12 quirk: stone costs 1 CON

// UNKNOWN: racial minimum lookup. Stub returns kRacialMinimumStub.
i32 racialMinimumStub(const Character& ch, i32 statIndex) {
    (void)ch;
    (void)statIndex;
    return kRacialMinimumStub;
}

}  // namespace

// ---------------------------------------------------------------------------
// Raise resolution helpers (R12)
// ---------------------------------------------------------------------------

i32 raiseRefusalFor(const Character& body) {
    // R12.2 / Table 3: RaiseRefusal by cause of death.
    // UNKNOWN: Table 3 is not in the SRD chunk. No refusal is modelled; 0 means none.
    (void)body;
    return 0;
}

i32 raiseFailAgeDays(DeathState state) {
    // R12 AgeDays = 84 if stoned, 124 if rock, else 54.
    if (state == DeathState::Stoned) return kAgeStonedDays;
    if (state == DeathState::Rock) return kAgeRockDays;
    return kAgeOrdinaryDays;
}

i32 raisedCurHits(RaiseKind kind, i32 maxHits) {
    // R12 CurHits = (kind in {22, 23}) ? MaxHits : 1.
    if (kind == RaiseKind::Resurrect || kind == RaiseKind::Morgue) return maxHits;
    return 1;
}

bool raiseRollFails(Ctx& ctx, i32 chance, bool rock) {
    // R12.5: roll = 0 if rock else chance; the raise fails if Next(100) > roll.
    const i32 roll = rock ? 0 : chance;
    const i32 draw = ctx.rng.below(kRaiseRollRange);
    return draw > roll;
}

bool rollRaiseComplication(Ctx& ctx, i32 complicationChance) {
    // R12.6: complication when the roll falls under the complication chance.
    // CONFIRMED-BY-REFERENCE: Raising.cs Complicates: "rng.Next(100) < ComplicationChance(...)" is a
    // strict "<". Rock is tested first by the caller, as in "c.CauseOfDeath == Rules.DeathRock ||".
    const i32 roll = ctx.rng.below(kRaiseRollRange);
    return roll < complicationChance;
}

void applyRaiseComplication(Ctx& ctx, Character& body) {
    (void)ctx;
    // R12.8 step 1: complication count up.
    // UNKNOWN: Character has no complication-count field; this step is omitted.

    // R12.8 step 2: each of the six stats down by 5, but not below the racial minimum.
    for (i32 s = 0; s < kStatCount; ++s) {
        i32& stat = body.abilities.stats[static_cast<u32>(s)];
        const i32 floor = racialMinimumStub(body, s);
        const i32 lowered = stat - kComplicationStatLoss;
        if (lowered >= floor) {
            stat = lowered;
        } else if (stat > floor) {
            stat = floor;
        }
    }

    // R12.8 step 3: age +5475 days (15 years).
    body.ageDays += kComplicationAgeDays;

    // R12.8 step 4: MaxHits = int(MaxHits / 1.2), minimum 1.
    i32 newMax = static_cast<i32>(static_cast<double>(body.vitals.maxHits) / kComplicationDivisor);
    if (newMax < kComplicationMinMaxHits) newMax = kComplicationMinMaxHits;
    body.vitals.maxHits = newMax;

    // R12.8 step 5: CurHits = the new MaxHits.
    body.vitals.curHits = newMax;

    // R12.8 step 6: equipped items whose stat requirements now fail are unequipped.
    // UNKNOWN: ItemDef stat-requirement fields are not given; this step is omitted.
}

void applyRaiseSuccess(Ctx& ctx, Character& body) {
    (void)ctx;
    // R12.9: age added (Table 4). CONFIRMED-BY-REFERENCE: Raising.cs Resolve charges
    // AgeDaysFor(CauseOfDeath): AgeDaysStoned, AgeDaysRock or AgeDaysOrdinary. raiseFailAgeDays
    // keys the same mapping on the state before the raise (assumes raiseFailAgeDays has that mapping).
    body.ageDays += raiseFailAgeDays(body.deathState);

    // R12.9: no longer dead or stoned.
    body.deathState = DeathState::Alive;

    // R12.9: cause becomes ordinary.
    // RESOLVED-BY-REFERENCE: Raising.cs Resolve: "c.CauseOfDeath = Rules.DeathOrdinary;". The port
    // has no separate cause field, so the Alive state carries the ordinary cause.

    // R12.9 / Table 2: hits set per kind.
    body.vitals.curHits = raisedCurHits(body.raiseKind, body.vitals.maxHits);
}

void applyStoneConLoss(Character& body) {
    // R12.9 / R12.11 and quirk: stone costs 1 CON on any raise, regardless of outcome.
    // CONFIRMED-BY-REFERENCE: Raising.cs Resolve: "c.Stats[(int)Stat.Con] -= 1;" with no floor.
    body.abilities.stats[static_cast<u32>(StatIndex::Con)] -= kStoneConLoss;
}

void applyMorgueConRoll(Ctx& ctx, Character& body) {
    // R12.9 / R12.11 and quirk: the CON-loss roll applies only to kind 23 (Morgue).
    if (body.raiseKind != RaiseKind::Morgue) return;
    // UNKNOWN: the probability of the morgue CON-loss roll is not in the SRD chunk.
    // Stub: no loss is applied. No RNG is drawn.
    (void)ctx;
    (void)body;
}

void resetConToRacialMinimum(Ctx& ctx, Character& body) {
    // R12.9: own-body raises reset CON to the racial minimum.
    (void)ctx;
    const u32 con = static_cast<u32>(StatIndex::Con);
    body.abilities.stats[con] = racialMinimumStub(body, static_cast<i32>(con));
}

RaiseResult resolveRaise(Ctx& ctx, Character& body) {
    RaiseResult result{};
    // UNKNOWN: RaiseResult fields are not given here (they are defined in death1a.h). The result
    // is returned default-constructed.

    // R12.1: points are spent first. UNKNOWN: the point cost is not in the SRD chunk; the caller
    // is assumed to spend it.

    // R12.2: refusal by cause (Table 3).
    if (raiseRefusalFor(body) != 0) return result;

    const DeathState before = body.deathState;
    const bool rock = before == DeathState::Rock;
    const bool stoned = before == DeathState::Stoned;

    // R12.4: rock zeroes the chance before the offer, so the roll can only fail.
    const i32 chance = rock ? 0 : body.raiseChance;

    // R12.5: roll. The raise fails when the roll is greater than the chance.
    if (!raiseRollFails(ctx, chance, rock)) {
        // R12.9: success effects (state, age, cause, hits), see applyRaiseSuccess.
        ctx.say("raise.success", "Success!", {}, {});
        applyRaiseSuccess(ctx, body);
    } else {
        // Every failure says "Raise Failed!" before the complication roll.
        // RESOLVED-BY-REFERENCE: Raising.cs Resolve: "outcome.Lines.Add("Raise Failed!");" comes
        // before the Complicates test, for every failure including rock.
        ctx.say("raise.failed", "Raise Failed!", {}, {});
        // R12.6: complication roll. Rock always complicates.
        const bool complicated = rock || rollRaiseComplication(ctx, kComplicationChanceStub);
        if (!complicated) {
            // R12.7: a quiet failure ages 54 days whatever the cause and the character stays dead.
            // RESOLVED-BY-REFERENCE: Raising.cs Resolve: "outcome.Lines.Add(c.AgeBy(AgeDaysOrdinary,
            // inAParty)); return outcome;" (still dead). The reference returns here, so no stone
            // CON loss and no morgue roll follow a quiet failure.
            body.ageDays += kAgeOrdinaryDays;
            return result;
        }
        // R12.8: complication effects, in order.
        applyRaiseComplication(ctx, body);
        // R12.9 / R12.10 / R12.11: a complicated raise revives the character (Raising.cs: "c.IsDead
        // = false" follows ApplyComplications with no return), ages per cause, and sets hits.
        // RESOLVED-BY-REFERENCE: Raising.cs Resolve: "if (!Complicates(...)) {...} ApplyComplications(...)"
        // then the common tail: "c.IsDead = false; ... c.CurHits = kind is ByResurrect or ByMorgue ? c.MaxHits : 1;"
        applyRaiseSuccess(ctx, body);
    }

    // R12.9 / R12.11: CON loss for stone, reached only when the raise did not end quietly.
    if (stoned) applyStoneConLoss(body);

    // R12.9 / R12.11: morgue CON-loss roll (kind 23 only).
    applyMorgueConRoll(ctx, body);

    // R12.9: own-body CON reset. UNKNOWN: this 2-argument resolveRaise has no ownBody flag, so
    // the reset is not applied here. The 6-argument resolveRaise in death1a.h handles it.

    // R12.12: a raise does not clear ailments. No ailment is touched.
    return result;
}

// ---------------------------------------------------------------------------
// Raise chances and fees (R13 to R15)
// ---------------------------------------------------------------------------

i32 spellRaiseTier(RaiseKind effect) {
    // R13: tier = max(1, effect - 19). 20 -> 1, 21 -> 2, 22 -> 3.
    const i32 tier = static_cast<i32>(effect) - 19;
    return tier < 1 ? 1 : tier;
}

i32 spellRaiseChance(i32 naturalCon, RaiseKind effect, bool causeIsRock) {
    // R13: chance = clamp(round_half_even(35 * (CON / 4.5) + tier * 10), 10, 98);
    // if cause == Rock: chance = 10.
    if (causeIsRock) return kRockSpellChance;
    const i32 tier = spellRaiseTier(effect);
    const double raw =
        kSpellChanceConFactor * (static_cast<double>(naturalCon) / kSpellChanceConDivisor) +
        static_cast<double>(tier * kSpellTierMultiplier);
    i64 rounded = roundHalfEven(raw);
    if (rounded < kSpellChanceFloor) rounded = kSpellChanceFloor;
    if (rounded > kRaiseChanceCap) rounded = kRaiseChanceCap;
    return static_cast<i32>(rounded);
}

i32 morgueRaiseChance(i32 naturalCon, bool ownBody) {
    // R14: own body and counter use different constants (CONFLICT 1).
    // UNKNOWN: the formula shape is not given. Assumed chance = CON + bonus, clamped to 1..98;
    // the lower bound of 1 is the documented quirk (tier lifted to 1).
    const i32 bonus = ownBody ? kMorgueOwnBonus : kMorgueCounterBonus;
    i32 chance = naturalCon + bonus;
    if (chance > kRaiseChanceCap) chance = kRaiseChanceCap;
    if (chance < kMorgueChanceFloor) chance = kMorgueChanceFloor;
    return chance;
}

Currency morgueFeeCounter(i32 maxHits) {
    // R15 counter: cost = round_half_even(2912.32 * (MaxHits / 1.5)).
    // UNKNOWN: the fee is read as whole gold and stored in 1/10000 gold.
    const double raw = kMorgueFeeFactor * (static_cast<double>(maxHits) / kMorgueFeeDivisor);
    return roundHalfEven(raw) * kGoldToCurrency;
}

Currency morgueFeeOwnBody(i32 maxHits) {
    // R15 own-body offer: cost = Int(2912.32 * (MaxHits / 1.5)). Int is floor (vbInt).
    const double raw = kMorgueFeeFactor * (static_cast<double>(maxHits) / kMorgueFeeDivisor);
    return vbInt(raw) * kGoldToCurrency;
}

bool morgueGuildPays(const Character& ch, Currency cost) {
    // R15: lowLevel = TopGuildLevel <= 5; guildPays = lowLevel && Purse < cost.
    // Purse is taken as Character::purse() (on hand plus bank).
    const bool lowLevel = ch.topGuildLevel <= kGuildPaysMaxLevel;
    return lowLevel && ch.purse() < cost;
}

// ---------------------------------------------------------------------------
// Companion raises (R16)
// ---------------------------------------------------------------------------

bool companionRaiseRefused(const Party& party) {
    // R16.1: refused if the caster is carrying a character.
    // UNKNOWN: "carrying a character" is mapped to CarryingSlot::DeadCharacter.
    return party.carryingSlot == CarryingSlot::DeadCharacter;
}

Vec<i32> companionRaiseCandidates(const Party& party, const Vec<Companion>& companions,
                                  i32 casterSlot, i32 area) {
    // R16.2: candidates are dead, not the caster, not already pending a raise, not carried by
    // any party member, on this level and in this area. Companions are listed by slot.
    Vec<i32> out;
    for (std::size_t i = 0; i < companions.size(); ++i) {
        const Companion& c = companions[i];
        const i32 idx = static_cast<i32>(i);
        if (!c.occupied) continue;
        // UNKNOWN: "dead" is taken as isDead, or Hits 0 (the R16.2 listing rule).
        const bool dead = c.isDead || c.hits == 0;
        if (!dead) continue;
        if (c.pendingRaise) continue;
        // Carried by any party member.
        const bool carried =
            c.carry != -1 ||
            (party.carryingSlot == CarryingSlot::Companion && party.carryingCompanion == idx);
        if (carried) continue;
        // UNKNOWN: "not the caster" compares the companion slot with casterSlot.
        if (c.slot == casterSlot) continue;
        if (c.dungeonLevel != party.dungeonLevel) continue;
        if (c.area != area) continue;
        out.push_back(c.slot);
    }
    return out;
}

void putDownCarriedCompanion(Party& party) {
    // R16.4: the carried companion is put down, silently, either way.
    if (party.carryingSlot == CarryingSlot::Companion) {
        party.carryingSlot = CarryingSlot::None;
        party.carryingCompanion = -1;
    }
}

i32 companionRaiseCon(const GameData& data, const Companion& comp) {
    // R16.3: con = MonsterOf(comp).Stats[3] ?? 10.
    // UNKNOWN: MonsterDef stat field names are not given. The companion's own stats[3] (CON) is
    // used when the monster exists; the fallback 10 applies when it does not.
    if (comp.monsterId < 0 ||
        static_cast<std::size_t>(comp.monsterId) >= data.monsters.size()) {
        return kCompanionConFallback;
    }
    return comp.stats[static_cast<std::size_t>(static_cast<u32>(StatIndex::Con))];
}

Currency companionMorgueCost(i32 maxHits, i32 floor) {
    // R16.5: cost = round_half_even(MaxHits * 1244.34 * Floor).
    // UNKNOWN: the cost is read as whole gold and stored in 1/10000 gold.
    const double raw = static_cast<double>(maxHits) * kCompanionMorgueFactor *
                       static_cast<double>(floor);
    return roundHalfEven(raw) * kGoldToCurrency;
}

i32 companionMorgueChance(i32 naturalCon) {
    // R16.5: chance = min(98, 75 + CON).
    const i32 raw = kCompanionMorgueBase + naturalCon;
    return raw < kRaiseChanceCap ? raw : kRaiseChanceCap;
}

bool rollMorgueCompanion(Ctx& ctx, i32 naturalCon) {
    // R16.5: fail if Int(Rnd*100) > chance. This function returns true when the raise passes.
    // UNKNOWN: the return sense (true = success) is chosen here. The companion's second check,
    // CompanionRaiseChance, is not part of this function.
    const i32 chance = companionMorgueChance(naturalCon);
    const i32 draw = ctx.rng.below(kRaiseRollRange);
    return !(draw > chance);
}

// ---------------------------------------------------------------------------
// Death routines (R17, R18)
// ---------------------------------------------------------------------------

void killPartyMember(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot,
                     DeathState state, bool runCleanup) {
    (void)companions;
    if (slot < 0 || slot >= party.size) return;
    Character& m = party.members[static_cast<std::size_t>(slot)];
    m.deathState = state;
    if (runCleanup) {
        // UNKNOWN: the cleanup run by the original Kill (sub0930) is not described in the SRD
        // chunk. Nothing is done here.
    }
    ctx.say("death.kill", m.name + " has died.", {slot}, {m.name});
}

TeleportRockResult teleportIntoRock(Ctx& ctx, Party& party, Vec<Companion>& companions,
                                    Vec<FloorState>& levels, OffMapStore& offMap,
                                    i32 level, i32 x, i32 y) {
    TeleportRockResult res{};
    const i32 count = static_cast<i32>(levels.size());

    // R17.1: the in-floor test. UNKNOWN: floor extent fields are not in the given state or data
    // headers. Placeholder: level in 1..Count with x, y >= 0.
    // UNKNOWN: writing the rock square to the automap needs the square index for (x, y), which
    // is not given. The square write is therefore omitted.
    const bool onMap = level >= 1 && level <= count && x >= 0 && y >= 0;

    if (level <= 0) {
        // R17.4: level 0 is a separate ordinary death.
        // CONFLICT: the R17 pseudo-code gives every member the rock cause at level 0 too. The
        // header and R17.4 give an ordinary death; that is followed here.
        res.outcome = TeleportRockOutcome::LevelZeroDeath;
        res.area = -1;   // R17.3
    } else if (onMap) {
        res.outcome = TeleportRockOutcome::LandedInRock;
        res.area = 0;    // R17.3
    } else {
        res.outcome = TeleportRockOutcome::BodiesOffMap;
        res.area = 0;    // R17.3
    }

    if (level > 0) {
        // R17.1 / quirk: X, Y and DungeonLevel are kept past the deepest level.
        party.dungeonLevel = level;
        party.x = x;
        party.y = y;
    }
    if (level > 0 && !onMap) {
        // R17.2: bodies stored off the map at (level, x, y).
        offMap.level = level;
        offMap.x = x;
        offMap.y = y;
    }

    // R17.3: every member dies with cause rock (ordinary at level 0, per CONFLICT above).
    const DeathState state = (level <= 0) ? DeathState::Dead : DeathState::Rock;
    for (i32 i = 0; i < party.size; ++i) {
        party.members[static_cast<std::size_t>(i)].location.area = res.area;
        killPartyMember(ctx, party, companions, i, state, true);
    }

    if (level > 0 && !onMap) {
        for (i32 i = 0; i < party.size; ++i) {
            offMap.bodies.push_back(party.members[static_cast<std::size_t>(i)]);
        }
    }
    return res;
}

void fellOutOfTheSky(Ctx& ctx, Party& party, Vec<Companion>& companions) {
    // R17.5: each member dies with DeathOrdinary and a tell box is shown.
    // CONFLICT 30: R17 gives DeathOrdinary cause 0; R18 and R24 give 1. DeathState::Dead
    // (value 1) is used, per kOrdinaryDeathValue.
    for (i32 i = 0; i < party.size; ++i) {
        killPartyMember(ctx, party, companions, i, DeathState::Dead, true);
    }
    ctx.say("death.fall", "You fall out of the sky!", {}, {});
}

OpenerTrapOutcome chestOpenerDeathCheck(Ctx& ctx, Party& party, Vec<Companion>& companions,
                                        SquareState& chest, i32 openerSlot) {
    (void)chest;   // The chest stays untrapped with its gold; it is not changed here.

    // R18: the leader is at 1 or more hits, so the trap rule does not apply.
    if (party.members[static_cast<std::size_t>(kFirstMemberIndex)].vitals.curHits >= 1) {
        return OpenerTrapOutcome::LeaderStanding;
    }

    // R18.1: only the opener in slot 0, and not already dead, is set dead (ordinary).
    OpenerTrapOutcome outcome = OpenerTrapOutcome::NobodyKilled;
    const Character& leader = party.members[static_cast<std::size_t>(kFirstMemberIndex)];
    if (openerSlot == kFirstMemberIndex && !leader.isDead()) {
        // R18.3: Kill with sub0930 false, so no cleanup.
        killPartyMember(ctx, party, companions, openerSlot, DeathState::Dead, false);
        outcome = OpenerTrapOutcome::OpenerKilled;
    }
    // Quirk: the leader is down but no one is set dead. This is the deliberate deviation for
    // a non-slot-0 opener, which avoids the original's hang.

    // R18: ShowChest(); RefreshCombat(); RefreshParty(); SaveChar(). These are presentation
    // and save duties of the caller. The core emits a single event.
    ctx.say("chest.opener_check", "The chest's trap has struck the party.", {}, {});
    return outcome;
}

// ---------------------------------------------------------------------------
// Old-age death (R19)
// ---------------------------------------------------------------------------

double oldAgePercent(i32 ageDays, i32 raceLifespanYears) {
    // R19.1: pct = (ageDays / 365) / raceLifespanYears * 100.
    // UNKNOWN: a non-positive lifespan returns 0 (guard).
    if (raceLifespanYears <= 0) return 0.0;
    return (static_cast<double>(ageDays) / kDaysPerYear) /
           static_cast<double>(raceLifespanYears) * 100.0;
}

bool rollOldAgeDeath(Ctx& ctx, double pct) {
    // R19.2: if pct >= 98 and Int(Rnd*100) < 5 + Ln(pct - 98), the character dies.
    // CONFLICT 13: "passes 98%" (rule text) and "pct >= 98" (formula) differ. The formula is
    // followed. The sign of 5 + Ln(...) is negative for pct between 98 and about 98.37, so no
    // clamp is applied.
    if (pct < kOldAgeThresholdPct) return false;
    const double excess = pct - kOldAgeThresholdPct;
    // Documented quirk: Ln(0) at exactly 98% is a runtime error in the original. The port
    // guards it: no roll is drawn and the character does not die.
    if (excess <= 0.0) return false;
    const double threshold = kOldAgeChanceBase + std::log(excess);
    const i32 roll = ctx.rng.below(kRaiseRollRange);
    return static_cast<double>(roll) < threshold;
}

// ---------------------------------------------------------------------------
// Character creation, deletion and kills (R20, R21)
// ---------------------------------------------------------------------------

void registerNewCharacter(Ctx& ctx, Vec<Character>& roster, Character& ch,
                          HallOfRecords& hall, i64 day) {
    // R20.1: on successful creation the new character is saved and placed in the roster.
    // UNKNOWN: "saved" is taken to be the Character.saved flag.
    ch.saved = true;
    roster.push_back(ch);
    // R20.1: the guild they were born into logs a join.
    appendGuildLog(ctx, hall, GuildLogKind::Join, ch.activeGuild, day, ch);
    // R20.2: all six natural stats are offered to the Hall of Records.
    offerNaturalStatsToHall(ctx, hall, ch, day);
}

void offerNaturalStatsToHall(Ctx& ctx, HallOfRecords& hall, const Character& ch, i64 day) {
    // R22 code: for i in 0..5: CheckAndLog(Kind(i), ch, ch.Stats[i], alone: true).
    // Kind(i) is taken as category i.
    for (i32 i = 0; i < kStatCount; ++i) {
        const i64 value = static_cast<i64>(ch.abilities.stats[static_cast<u32>(i)]);
        offerHallRecord(ctx, hall, i, ch, value, true, day);
    }
}

bool characterDeletable(const Character& ch, bool saveFileExists) {
    // R20.3: deletable only once its save is gone. Delete Character is the only path;
    // that path is the caller's concern.
    (void)ch;
    return !saveFileExists;
}

bool isKillRecordable(bool groupIdentified, bool monsterInvisible, const Character& killer) {
    // R21.1: recordable only for the killer, and only when the group is identified. An invisible
    // monster also needs the killer to see invisible.
    return groupIdentified && (!monsterInvisible || killer.ailments.seeInvisible);
}

bool recordKill(Ctx& ctx, Character& killer, const Creature& monster,
                bool groupIdentified, bool monsterInvisible) {
    (void)ctx;
    if (!isKillRecordable(groupIdentified, monsterInvisible, killer)) return false;
    killer.kills += 1;
    killer.monsterKills.push_back(monster.id);
    return true;
}

i32 chestFindCredit(i32 leadIndex, i32 foes0KnownAs, i32 leadId,
                    bool leadInvisible, const Character& taker) {
    // R21.2: ChestFindFrom = (leadIndex == foes0KnownAs && !(lead.Invisible &&
    // !taker.SeeingInvisible)) ? lead.Id : -2.
    // UNKNOWN: -2 is the "unknown monster" credit (no further meaning given).
    const bool hidden = leadInvisible && !taker.ailments.seeInvisible;
    return (leadIndex == foes0KnownAs && !hidden) ? leadId : -2;
}

void settleMonsterQuestAtKill(Ctx& ctx, Character& killer, i32 monsterId) {
    // R21.3: a monster quest is settled at the kill, for the current guild only, whether or not
    // the fight is won. The quest message names the character.
    // UNKNOWN: quest target matching (target == monsterId) and the quest count/level handling
    // are not given. Completion sets the state to Done.
    GuildMembership& g = killer.guilds[static_cast<u32>(killer.activeGuild)];
    GuildQuest& q = g.quest;
    if (q.kind != QuestKind::Defeat) return;
    if (q.state != QuestState::Active) return;
    if (q.target != monsterId) return;
    q.state = QuestState::Done;
    g.questDone = true;
    ctx.say("guild.quest_done", killer.name + " has completed a monster quest.", {},
            {killer.name});
}

// ---------------------------------------------------------------------------
// Hall of Records and guild log (R22)
// ---------------------------------------------------------------------------

bool offerHallRecord(Ctx& ctx, HallOfRecords& hall, i32 category, const Character& ch,
                     i64 value, bool alone, i64 day) {
    // R22.3: the Hall file is saved when any offer is made.
    hall.dirty = true;
    // UNKNOWN: the comparison against hall.records[category] needs HallRecord fields, which are
    // not given. No record is taken here, and false is returned. The offer is still logged as
    // dirty.
    (void)ctx;
    (void)category;
    (void)ch;
    (void)value;
    (void)alone;
    (void)day;
    return false;
}

void checkLevelRecords(Ctx& ctx, HallOfRecords& hall, const Character& ch, i64 day) {
    // R22.1: after a level, the Fighting, Magic and Thieving skill records are checked.
    // UNKNOWN: category ids are not given. Guild indices are used: Warrior (Fighting), Mage
    // (Magic) and Thief (Thieving). The value is the guild level; alone is taken as true.
    offerHallRecord(ctx, hall, static_cast<i32>(Guild::Warrior), ch,
                    static_cast<i64>(ch.guilds[static_cast<u32>(Guild::Warrior)].level),
                    true, day);
    offerHallRecord(ctx, hall, static_cast<i32>(Guild::Mage), ch,
                    static_cast<i64>(ch.guilds[static_cast<u32>(Guild::Mage)].level),
                    true, day);
    offerHallRecord(ctx, hall, static_cast<i32>(Guild::Thief), ch,
                    static_cast<i64>(ch.guilds[static_cast<u32>(Guild::Thief)].level),
                    true, day);
}

void checkWealthiestRecord(Ctx& ctx, HallOfRecords& hall, const Character& ch,
                           Currency wealth, i64 day) {
    // R22.2: if pv0104 > M73F5 then M73F5 = pv0104 (exact comparison).
    (void)day;
    if (wealth > hall.wealthiest) {
        hall.wealthiest = wealth;
        hall.dirty = true;   // R22.3
        ctx.say("hall.wealthiest", ch.name + " is the wealthiest adventurer.", {wealth},
                {ch.name});
    }
}

void appendGuildLog(Ctx& ctx, HallOfRecords& hall, GuildLogKind kind, Guild guild,
                    i64 day, const Character& ch) {
    // R22.4 and R20.1: guild log lines for death, resurrection, demotion and join.
    // UNKNOWN: the wording of each line is not given. English text is supplied here.
    Str text = ch.name;
    switch (kind) {
        case GuildLogKind::Death:
            text += " has died.";
            break;
        case GuildLogKind::Resurrection:
            text += " has been raised.";
            break;
        case GuildLogKind::Demotion:
            text += " has been demoted.";
            break;
        case GuildLogKind::Join:
            text += " has joined the guild.";
            break;
    }
    GuildLogLine line{};
    line.guild = guild;
    line.day = day;
    line.text = text;
    hall.log.push_back(line);
    hall.dirty = true;   // R22.3
    ctx.say("guild.log", text, {day}, {ch.name});
}

i64 hallStatField(i64 value) {
    // R22: stored = value * 10000 (the byte-level encoding is the file layer's concern).
    return value * kHallStatMultiplier;
}

i64 hallDateField(i64 daysSince1900) {
    // R22: dateField = daysSince(1900-01-01) + 2.
    return daysSince1900 + kHallDateOffset;
}

// ---------------------------------------------------------------------------
// Retrieve Soul (R23) and input helpers
// ---------------------------------------------------------------------------

SoulCheck retrieveSoulStatus(const Character& ch, i32 currentDungeonLevel) {
    // R23.1: lists only characters dead in rock (DeathState 8), on this level, in area 0.
    // R23.2: typed errors "Character is not on this level!" and "Character is not in ROCK!".
    // CONFLICT: the quirk note says the port offers any dead character. R23 (the original's
    // rule) is followed here.
    if (ch.location.dungeonLevel != currentDungeonLevel) return SoulCheck::NotOnThisLevel;
    if (ch.deathState != DeathState::Rock || ch.location.area != 0) {
        return SoulCheck::NotInRock;
    }
    return SoulCheck::Ok;
}

Vec<Str> splitCoordinateInput(const Str& text) {
    // Morgue input: coordinates split on commas; a trailing comma is ignored.
    Vec<Str> parts;
    Str current;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == ',') {
            parts.push_back(current);
            current = Str();
        } else {
            current += c;
        }
    }
    const bool endsWithComma = !text.empty() && text[text.size() - 1] == ',';
    if (!endsWithComma) {
        parts.push_back(current);
    }
    return parts;
}

i64 valLikeOriginal(const Str& text) {
    // Morgue input, VB Val: skips spaces, reads an optional sign, digits and an optional
    // fraction, and stops at the first non-numeric character (1-2 -> 1; x -> 0).
    // The result is rounded (3.7 -> 4).
    // UNKNOWN: the rounding mode for Val is not stated. roundHalfEven is used.
    const std::size_t n = text.size();
    std::size_t i = 0;
    double sign = 1.0;
    double value = 0.0;

    while (i < n && text[i] == ' ') ++i;
    if (i < n && text[i] == '-') {
        sign = -1.0;
        ++i;
    } else if (i < n && text[i] == '+') {
        ++i;
    }

    while (i < n) {
        const char c = text[i];
        if (c == ' ') {
            ++i;
            continue;
        }
        if (c >= '0' && c <= '9') {
            value = value * 10.0 + static_cast<double>(c - '0');
            ++i;
        } else {
            break;
        }
    }

    if (i < n && text[i] == '.') {
        ++i;
        double scale = 0.1;
        while (i < n) {
            const char c = text[i];
            if (c == ' ') {
                ++i;
                continue;
            }
            if (c >= '0' && c <= '9') {
                value += static_cast<double>(c - '0') * scale;
                scale *= 0.1;
                ++i;
            } else {
                break;
            }
        }
    }

    return roundHalfEven(sign * value);
}

}  // namespace mordor
