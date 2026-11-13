// combat1a.cpp: melee rules (SRD c07p1, R1-R31).
// Assumptions about shared headers not visible to this ant:
//   - Event.nums is a Vec<i64> and Event.strs is a Vec<Str> (events.h), so ctx.say is called with {i64...} and {Str...}.
//   - Vec<T> supports size(), operator[], push_back(); Arr<T,N> supports operator[].
//   - std::log is used for VB Ln (math_util.h has no log helper).
#include "mordor/combat1a.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/events.h"

#include <cmath>

namespace mordor {

namespace {

// VB Ln.
f64 lnOf(f64 x) {
    return std::log(x);
}

// Int(Rnd*n) using exactly one uniform draw (safe for any n, including n <= 0).
i64 drawBelow(IRng& rng, f64 n) {
    return vbInt(rng.nextDouble() * n);
}

// Ceiling of x as i32, built from vbInt so no <cmath> ceil is needed.
i32 ceilToI32(f64 x) {
    const i64 f = vbInt(x);
    return static_cast<i32>(static_cast<f64>(f) == x ? f : f + 1);
}

// A monster group still has a living member (R2, R3, R5).
bool groupStanding(const MonsterGroup& g) {
    return g.livingCount > 0;
}

bool validGroupIndex(i32 index) {
    return index >= 0 && index < kMaxGroups;
}

// First group still standing, or -1 when none is (R5).
i32 firstStandingGroup(const Encounter& enc) {
    for (i32 i = 0; i < kMaxGroups; ++i) {
        if (groupStanding(enc.groups[static_cast<u32>(i)])) {
            return i;
        }
    }
    return -1;
}

}  // namespace

// ---------------------------------------------------------------------------
// Property bits
// ---------------------------------------------------------------------------

// T5: monster property bit test ("Has(bit n)", R15).
bool monsterHasBit(const Creature& m, i32 bit) {
    if (bit < 0 || bit > 31) {
        return false;
    }
    return ((m.specialPropertyFlags >> static_cast<u32>(bit)) & 1u) != 0u;
}

// ---------------------------------------------------------------------------
// Setup, turn order and aim (R1-R6)
// ---------------------------------------------------------------------------

// R1: no effect if the group is empty, not hostile (first monster), or has no living member.
bool arrivalCombatStarts(const MonsterGroup& group) {
    if (group.count <= 0) {
        return false;
    }
    if (!group.front.hostile) {
        return false;
    }
    if (group.livingCount <= 0) {
        return false;
    }
    return true;
}

// R2, R3, R5: aim must move if the aimed group is missing or no longer standing.
bool aimMustMove(const Encounter& enc, i32 aimedGroup) {
    if (!validGroupIndex(aimedGroup)) {
        return true;
    }
    return !groupStanding(enc.groups[static_cast<u32>(aimedGroup)]);
}

// R3: further swings stay on the aimed group while it stands.
// Otherwise aim moves to fn0DCD(-1).
i32 furtherSwingAim(const Encounter& enc, i32 aimedGroup) {
    if (!aimMustMove(enc, aimedGroup)) {
        return aimedGroup;
    }
    // UNKNOWN: fn0DCD body not given; using first standing group (R5 wording).
    return firstStandingGroup(enc);
}

// R4: the combat setting applies only to characters with no order.
bool combatSettingApplies(const CombatOrder& order) {
    return order.kind == CombatOrderKind::None;
}

// R6: an order of Defend suppresses the character's own attack.
bool defendSuppressesAttack(const CombatOrder& order) {
    return order.kind == CombatOrderKind::Defend;
}

// ---------------------------------------------------------------------------
// Swings (R7-R12)
// ---------------------------------------------------------------------------

// R7: swings = actor.Swings + w.Swings.
i32 swingsPerWeapon(i32 actorSwings, i32 weaponSwings) {
    return actorSwings + weaponSwings;
}

// R8: no weapon or total swings below 1 -> "Using no weapon!" and actor on Defend.
// Returns true when the no-weapon defend was applied.
bool applyNoWeaponDefend(Ctx& ctx, Character& actor, bool hasWeapon, i32 totalSwings) {
    if (hasWeapon && totalSwings >= 1) {
        return false;
    }
    ctx.say("combat.no_weapon", "Using no weapon!", {}, {actor.name});
    actor.order.kind = CombatOrderKind::Defend;
    return true;
}

// R9: every equipped weapon slot in place of the two-hand loop.
// Returns indices into inv.worn.
// UNKNOWN: weapon identity is not stored on Item (ItemType has no weapon kind);
// every equipped worn item is treated as a weapon slot. Hands item (R8 quirk) not modelled.
Vec<i32> equippedWeaponSlots(const Inventory& inv) {
    Vec<i32> slots;
    const i32 count = static_cast<i32>(inv.worn.size());
    for (i32 i = 0; i < count; ++i) {
        if (inv.worn[static_cast<u32>(i)].equipped) {
            slots.push_back(i);
        }
    }
    return slots;
}

// R10: extra swings earned by the Multiple Swings ladder.
//   step = ln(11 - m); T = B(50*step); for level 1..guildLevel:
//     if level > T: extra++; T = B(2*T*step)
// B = round half to even. Original threshold is an Integer (quirk); i64 used here.
i32 extraSwingsFromMultiple(f64 m, i32 guildLevel) {
    if (m <= 0.0) {
        return 0;
    }
    // UNKNOWN: m >= 11 makes ln(11-m) undefined in the SRD; grant nothing.
    if (m >= 11.0) {
        return 0;
    }
    const f64 step = lnOf(10.0 - m + 1.0);
    i64 threshold = roundHalfEven(step * 50.0);
    i32 extra = 0;
    for (i32 level = 1; level <= guildLevel; ++level) {
        if (static_cast<i64>(level) > threshold) {
            ++extra;
            threshold = roundHalfEven(step * static_cast<f64>(threshold) * 2.0);
        }
    }
    return extra;
}

// R11: swings never fall; a guild only raises the figure if it beats it.
i32 raiseSwingsIfHigher(i32 currentSwings, i32 award) {
    return award > currentSwings ? award : currentSwings;
}

// R10, R11: recompute the character's swing count from each guild's Multiple Swings rating.
// UNKNOWN: SRD says m = guild.Abilities[3] (static guild data). GameData guild field names are
// not visible here, so the per-character GuildMembership::abilityRating[kMultipleSwingsAbility]
// is used as the nearest known source for m.
void recalcSwings(Ctx& ctx, Character& c) {
    (void)ctx;
    for (i32 g = 0; g < kGuildCount; ++g) {
        const u32 gi = static_cast<u32>(g);
        const f64 m = static_cast<f64>(c.guilds[gi].abilityRating[static_cast<u32>(kMultipleSwingsAbility)]);
        const i32 award = extraSwingsFromMultiple(m, c.guilds[gi].level);
        c.combat.swings = raiseSwingsIfHigher(c.combat.swings, award);
    }
}

// ---------------------------------------------------------------------------
// Weapon effect gates (R13-R15)
// ---------------------------------------------------------------------------

// R13 / R15: bit 4 needs weapon floor >= (int)(LevelFound / 2).
bool weaponBlockedByNeedsGoodWeapon(const Creature& target, i32 weaponFloor) {
    return monsterHasBit(target, kBitNeedsGoodWeapon) && weaponFloor < target.levelFound / 2;
}

// R14 / R15: bit 5 nullifies the weapon.
bool weaponNullifiedByImmunity(const Creature& target) {
    return monsterHasBit(target, kBitImmuneToWeapons);
}

// R13-R15: gate order is bit 4 first, then bit 5.
WeaponGateOutcome checkWeaponGate(const Creature& target, i32 weaponFloor) {
    if (weaponBlockedByNeedsGoodWeapon(target, weaponFloor)) {
        return WeaponGateOutcome::NeedsGoodWeaponBlocked;
    }
    if (weaponNullifiedByImmunity(target)) {
        return WeaponGateOutcome::ImmuneNoEffect;
    }
    return WeaponGateOutcome::Pass;
}

// R13: hand finished, actor switched to Defend, item order cleared.
// UNKNOWN: Character::order and Party::combatOrders both hold a CombatOrder; both are set to Defend.
// The "hand finished" step is the caller's (weapon loop ends).
void applyBlockedHandDefend(Ctx& ctx, Party& party, i32 actorSlot) {
    (void)ctx;
    if (actorSlot < 0 || actorSlot >= party.size) {
        return;
    }
    const u32 s = static_cast<u32>(actorSlot);
    party.itemOrders[s] = ItemOrder{};
    party.members[s].order.kind = CombatOrderKind::Defend;
    party.combatOrders[s].kind = CombatOrderKind::Defend;
}

// R16: unseen flag passed when the target is invisible and the actor lacks See Invisible.
// UNKNOWN: "target is invisible" mapped to Creature::unseen.
bool swingUnseen(const Character& actor, const Creature& target) {
    return target.unseen && !actor.ailments.seeInvisible;
}

// ---------------------------------------------------------------------------
// Hit gate and damage (R17-R22)
// ---------------------------------------------------------------------------

// R17 (fn0DEE): one swing's damage, including the final hit gate (R18).
// Draw order: U1 (roll), U2 (adjustment share), U3 (999-in-1000 check), U4 (hit gate).
// No draws happen in the threshold computation.
i32 meleeHitAndDamage(Ctx& ctx, i32 atk, i32 def, i32 level, i32 strength, f64 damMod) {
    // R18: threshold first.
    const i32 threshold = hitThresholdPercent(atk, def);

    if (damMod <= 0.0) {
        damMod = kMinDamMod;
    }

    // str = max(1, strength); str = B(str - Ln(str)*((Ln(str)-2.4)*2.25)) + 1
    const i32 base = strength < 1 ? 1 : strength;
    const f64 lnS = lnOf(static_cast<f64>(base));
    const f64 s = static_cast<f64>(roundHalfEven(static_cast<f64>(base) - lnS * ((lnS - 2.4) * 2.25))) + 1.0;

    // if damMod > 1: damMod -= Ln(damMod)^2
    if (damMod > 1.0) {
        const f64 l = lnOf(damMod);
        damMod -= l * l;
    }

    const f64 lvlTerm = (lnOf(static_cast<f64>(level) + 5.0) + 1.0) / lnOf(1.2);
    const f64 strTerm = s / 30.0;

    // roll = Int(U1*((s+10)/2) + (s+10)/4) / 10
    const f64 roll = static_cast<f64>(vbInt(ctx.rng.nextDouble() * ((s + 10.0) / 2.0) + (s + 10.0) / 4.0)) / 10.0;

    i64 dmg = vbInt(lvlTerm * strTerm * roll * damMod);

    // R19: adj = Int(Int((def-atk)/2)/4)
    i64 adj = attackDefenceTerm(atk, def);
    // adj = adj - Int(U2*(adj+1)) + 1   (U2 always drawn)
    adj = adj - drawBelow(ctx.rng, static_cast<f64>(adj + 1)) + 1;

    // if Int(U3*1000)+1 > 1: dmg -= adj   (U3 always drawn; applies 999 in 1000)
    if (drawBelow(ctx.rng, 1000.0) + 1 > 1) {
        dmg -= adj;
    }

    if (dmg < 1) {
        dmg = 0;
    }

    // R18 gate: miss when Int(U4*100)+1 >= threshold (U4 always drawn).
    if (!rollHitGate(ctx, threshold)) {
        return 0;
    }
    return static_cast<i32>(dmg);
}

// R18: hit-gate threshold, audit form, clamped to [2, 98], returned as the integer
// threshold ceil(T) so that "roll < T" equals "roll < ceil(T)" over integer rolls.
// T = 50 when atk == def (includes both zero).
// UNKNOWN: denominator max(atk,def) == 0 with atk != def is undefined; neutral 50 used.
i32 hitThresholdPercent(i32 atk, i32 def) {
    f64 p = static_cast<f64>(kHitGateNeutralPercent);
    if (atk == def) {
        p = static_cast<f64>(kHitGateNeutralPercent);
    } else {
        const i32 denom = atk > def ? atk : def;
        if (denom != 0) {
            p = 50.0 + 50.0 * static_cast<f64>(atk - def) / static_cast<f64>(denom);
        }
    }
    if (p < static_cast<f64>(kHitGateMinPercent)) {
        p = static_cast<f64>(kHitGateMinPercent);
    }
    if (p > static_cast<f64>(kHitGateMaxPercent)) {
        p = static_cast<f64>(kHitGateMaxPercent);
    }
    return ceilToI32(p);
}

// R18: gate passes when Int(Rnd*100)+1 < threshold (miss when >= threshold). One draw.
bool rollHitGate(Ctx& ctx, i32 thresholdPercent) {
    const i64 roll = vbInt(ctx.rng.nextDouble() * 100.0) + 1;
    return roll < static_cast<i64>(thresholdPercent);
}

// R19: term = Int(Int((Def - Att)/2)/4). Equivalent to I((D-A)/8).
i32 attackDefenceTerm(i32 att, i32 def) {
    const i64 half = vbInt(static_cast<f64>(def - att) / 2.0);
    return static_cast<i32>(vbInt(static_cast<f64>(half) / 4.0));
}

// R16, R22: damage below 1 is a miss.
bool isMissedSwing(i32 damage) {
    return damage < 1;
}

// R27: only a kill continues the chain (engine reading).
// CONFLICT: community reading says a third swing needs a kill followed by another one-hit kill
// (c103). Engine reading (any kill continues) is used.
bool weaponChainContinues(i32 damage, bool killedTarget) {
    return killedTarget && !isMissedSwing(damage);
}

// ---------------------------------------------------------------------------
// Character melee modifiers (R23-R26)
// ---------------------------------------------------------------------------

// R23 base: F = 0.6 + ((Ln(100+L)/1.75 - 2.3) * fightingMod^2 / 2).
f64 meleeBaseFactor(i32 guildLevel, f64 fightingMod) {
    return 0.6 + ((lnOf(100.0 + static_cast<f64>(guildLevel)) / 1.75 - 2.3) * (fightingMod * fightingMod) / 2.0);
}

// R23 / R24: character damage modifier.
// CONFLICT: T5 lists crit +2 and backstab +1 while F5 (R23) and kCritDamBonus/kBackstabDamBonus
// give +5 and +2. The F5 values are used.
f64 characterDamageMod(i32 guildLevel, f64 fightingMod, f64 weaponMod, bool crit, bool backstab, bool invisibleTarget, i32 sizeDiff) {
    f64 damMod = meleeBaseFactor(guildLevel, fightingMod);
    damMod *= (weaponMod <= 0.0 ? 1.0 : weaponMod);
    if (crit) {
        damMod += kCritDamBonus;
    }
    if (backstab) {
        damMod += kBackstabDamBonus;
    }
    if (invisibleTarget) {
        damMod -= damMod / 2.0;
    }
    if (sizeDiff < 0) {
        damMod -= static_cast<f64>(-sizeDiff) / static_cast<f64>(kSizeDiffDivisor);
    }
    if (sizeDiff > 0) {
        damMod += static_cast<f64>(sizeDiff) / static_cast<f64>(kSizeDiffDivisor);
    }
    return damMod;
}

// R16, R21, R23: player melee damage. Defence is passed as kPlayerMeleeDefence (0).
i32 characterDamage(Ctx& ctx, i32 guildLevel, f64 fightingMod, f64 weaponMod, i32 strength, i32 sizeDiff, bool crit, bool backstab, i32 attack, bool unseen) {
    const f64 damMod = characterDamageMod(guildLevel, fightingMod, weaponMod, crit, backstab, unseen, sizeDiff);
    return meleeHitAndDamage(ctx, attack, kPlayerMeleeDefence, guildLevel, strength, damMod);
}

// R25: companion defence gets +4 for crit and +1 for backstab (no size adjustment).
i32 companionGateDefence(i32 targetDefence, bool crit, bool backstab) {
    i32 d = targetDefence;
    if (crit) {
        d += kCompanionCritDefence;
    }
    if (backstab) {
        d += kCompanionBackstabDefence;
    }
    return d;
}

// R25: companion modifier is halved against an unseen target.
f64 companionDamageMod(f64 baseMod, bool unseen) {
    return unseen ? baseMod / 2.0 : baseMod;
}

// R26: which guild's level is passed to CharacterDamage.
// UNKNOWN: audit says "largest F", but guild fighting coefficients are static data whose field
// names are not visible here. The active guild index is returned.
i32 chooseFightingGuild(const Ctx& ctx, const Character& c) {
    (void)ctx;
    return static_cast<i32>(c.activeGuild);
}

// ---------------------------------------------------------------------------
// Deaths, round loop and records (R28-R31)
// ---------------------------------------------------------------------------

// R28: sub0930 runs when the character's Hits are at nought or below.
bool needsDeathProcessing(const Character& c) {
    return c.vitals.curHits <= 0;
}

// R28: sub0E85 leaves at once for anyone with a DeathState.
bool leavesRoundAtOnce(const Character& c) {
    return c.isDead();
}

// R28: sub0930 for one member: ailments cleared, member saved, member pruned from the party.
// UNKNOWN: companion release needs the companion table, which is not reachable from Party here;
// it is not performed. Pruned members are kept on party.morgueList so they are not lost.
void processMidRoundDeath(Ctx& ctx, Party& party, i32 memberIndex) {
    if (memberIndex < 0 || memberIndex >= party.size) {
        return;
    }
    const u32 idx = static_cast<u32>(memberIndex);
    Character& m = party.members[idx];
    if (!needsDeathProcessing(m)) {
        return;
    }

    m.ailments = Ailments{};   // ailments are cleared
    m.saved = true;            // UNKNOWN: "saved" taken as the character's saved flag

    const Str name = m.name;
    party.morgueList.push_back(m);  // UNKNOWN: destination of pruned members

    const u32 last = static_cast<u32>(party.size - 1);
    for (u32 i = idx; i < last; ++i) {
        party.members[i] = party.members[i + 1];
        party.combatOrders[i] = party.combatOrders[i + 1];
        party.itemOrders[i] = party.itemOrders[i + 1];
    }
    party.members[last] = Character{};
    party.combatOrders[last] = CombatOrder{};
    party.itemOrders[last] = ItemOrder{};
    party.size -= 1;

    ctx.say("combat.member_pruned", "Party member removed after death.", {static_cast<i64>(memberIndex)}, {name});
}

// R29: the round loop ends when one member (or none) is left, so a lone survivor escapes the
// remaining groups that round.
bool roundLoopEnds(i32 membersLeft) {
    return membersLeft <= 1;
}

// R30: experience is paid as each blow lands. Level message is judged by the total before and
// after the award.
// UNKNOWN: bodies of sub0CDF, sub0F11 and fn12A1 are not given. The "enough experience to make a
// level" test is taken as experience crossing xpRequired. The "now pinned." message is not
// implemented.
void awardBlowExperience(Ctx& ctx, Character& actor, i64 xp) {
    const i64 before = actor.experience;
    actor.experience += xp;
    const bool readyBefore = before >= actor.xpRequired;
    const bool readyAfter = actor.experience >= actor.xpRequired;
    if (!readyBefore && readyAfter) {
        ctx.say("combat.level_ready", "now has enough experience to make a level.", {}, {actor.name});
    }
}

// R31: recorded_hits = min(hits_taken, monster_hp_remaining).
i32 recordedHitsFor(i32 hitsTaken, i32 monsterHpRemaining) {
    return hitsTaken < monsterHpRemaining ? hitsTaken : monsterHpRemaining;
}

// R31: per monster struck.
//   - not charmed: one damage record with recorded hits (credited to owner).
//   - killing blow: one kill record after the damage record (credited to owner).
// UNKNOWN: owner is credited in both the player and companion cases; companionCredit only
// labels the event kind. Port credits companion kills to the owner; the original credits nobody (quirk).
void recordMonsterStrike(Ctx& ctx, Character& owner, const Creature& target, i32 hitsTaken, bool killingBlow, bool charmed, bool companionCredit) {
    const i32 recorded = recordedHitsFor(hitsTaken, target.hits);
    const i64 kindFlag = companionCredit ? 1 : 0;

    if (!charmed) {
        owner.damageRecorded += static_cast<i64>(recorded);
        ctx.say("combat.damage_record", "Damage recorded.", {static_cast<i64>(recorded), kindFlag}, {});
    }
    if (killingBlow) {
        owner.kills += 1;
        owner.monsterKills.push_back(target.id);
        ctx.say("combat.kill_record", "Kill recorded.", {static_cast<i64>(target.id), kindFlag}, {});
    }
}

}  // namespace mordor
