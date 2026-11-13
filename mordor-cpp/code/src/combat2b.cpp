// combat2b: player and monster crit/backstab procs, companion breath sweep,
// combat-setting spell refusal, Asmodeus fight-end check, quest kill, kill XP.
// Source: SRD part c07p2 (R11, P2, R12-R17, P3-P4, R18-R24, P5-P6, R25-R28).

#include "mordor/combat2b.h"

#include <cmath>

#include "mordor/casting1.h"
#include "mordor/ctx.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

constexpr i32 kCoefficientRows = 2;  // kCritCoefficients and kBackstabCoefficients each have 2 rows

// UNKNOWN: Asmodeus monster id not supplied in SRD. -1 never matches a real monster.
constexpr i32 kAsmodeusMonsterId = -1;

const GuildCoefficient* coefficientTable(CombatProc proc) {
    return proc == CombatProc::Critical ? kCritCoefficients : kBackstabCoefficients;
}

const ProcRule& procRule(CombatProc proc) {
    return proc == CombatProc::Critical ? kCritRule : kBackstabRule;
}

// Coefficient (CritMod or BSMod) of the given guild for the given ability; 0 if absent.
i32 coefficientFor(CombatProc proc, i32 guildIndex) {
    if (guildIndex == kNoGuild) {
        return 0;
    }
    const GuildCoefficient* table = coefficientTable(proc);
    for (i32 i = 0; i < kCoefficientRows; ++i) {
        if (static_cast<i32>(table[i].guild) == guildIndex) {
            return table[i].coefficient;
        }
    }
    return 0;
}

// GLvl of the guild chosen by R11; 0 when no guild is chosen.
i32 guildLevelOf(const Character& ch, i32 guildIndex) {
    if (guildIndex == kNoGuild) {
        return 0;
    }
    return ch.guilds[static_cast<u32>(guildIndex)].level;
}

f64 maxF(f64 a, f64 b) {
    return a > b ? a : b;
}

f64 clampF(f64 v, f64 lo, f64 hi) {
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

}  // namespace

// R11: the chosen guild maximizes coefficient x guildLevel. Ties are resolved by
// strict improvement in table order; a zero score wins no guild.
// UNKNOWN: guild-table order not supplied; the coefficient-table row order is used.
i32 selectGuildForProc(const Character& ch, CombatProc proc) {
    const GuildCoefficient* table = coefficientTable(proc);
    i32 bestScore = 0;
    i32 bestGuild = kNoGuild;
    for (i32 i = 0; i < kCoefficientRows; ++i) {
        i32 guildIndex = static_cast<i32>(table[i].guild);
        i32 level = ch.guilds[static_cast<u32>(guildIndex)].level;
        i32 score = table[i].coefficient * level;
        if (score > bestScore) {  // strict improvement; zero never wins
            bestScore = score;
            bestGuild = guildIndex;
        }
    }
    return bestGuild;
}

// Player formula (engine form, CONFLICT resolved per engine):
//   p = ((Ln(GLvl/999+1)*100) * Ln(Mod+1)) / divisor
//   p -= Ln(M/999+1)*100
//   p /= 2
//   if weapon has ability: w = Ln(F+1)*Ln(weaponLog); p = max(p+w, w)   // CONFLICT: engine form
//   if p < floor and (GLvl > 0 or weapon has ability): p = floor        // CONFLICT: engine form
//   clamp p <= cap
f64 playerProcChance(const PlayerChanceParams& p, const ProcRule& rule) {
    f64 guildLevel = static_cast<f64>(p.guildLevel);
    f64 monLevel = static_cast<f64>(p.monLevel);
    f64 chance = ((std::log(guildLevel / 999.0 + 1.0) * 100.0) *
                  std::log(static_cast<f64>(p.coefficient) + 1.0)) /
                 static_cast<f64>(rule.divisor);
    chance -= std::log(monLevel / 999.0 + 1.0) * 100.0;
    chance /= 2.0;
    if (p.weaponHasAbility) {
        f64 fromWeapon = std::log(static_cast<f64>(p.weaponFloor) + 1.0) *
                         std::log(static_cast<f64>(rule.weaponLog));
        chance = maxF(chance + fromWeapon, fromWeapon);
    }
    if (chance < static_cast<f64>(rule.floor) && (p.guildLevel > 0 || p.weaponHasAbility)) {
        chance = static_cast<f64>(rule.floor);
    }
    if (chance > static_cast<f64>(rule.cap)) {
        chance = static_cast<f64>(rule.cap);
    }
    return chance;
}

// R7/R8 comparison. Player form: the roll is an integer draw 0-99 compared against
// the unrounded chance, so a chance of 5.4 wins six rolls in a hundred (c5, c23).
bool rollPlayerProc(Ctx& ctx, f64 chance) {
    i64 draw = static_cast<i64>(ctx.rng.below(100));
    return static_cast<f64>(draw) < chance;
}

// R10: damage modifier increments. Crit +5, backstab +2; both on one blow ("Severed") adds both.
i32 blowDamageModifier(bool crit, bool backstab) {
    i32 modifier = 0;
    if (crit) {
        modifier += kCritRule.increment;
    }
    if (backstab) {
        modifier += kBackstabRule.increment;
    }
    return modifier;
}

// Procedure P2 (player blow, SwingOnce). Guild selection (step 5) is done first because
// the chance formula needs the chosen guild's level and coefficient. Guild selection
// draws no random numbers, so the RNG order is unchanged: crit roll, then backstab roll.
// Hit gate (step 3) is UNKNOWN and not modelled here.
PlayerBlowRolls rollPlayerBlow(Ctx& ctx, const Character& attacker, const PlayerBlowInput& in) {
    PlayerBlowRolls out;

    // R11 selection for each ability.
    out.critGuild = selectGuildForProc(attacker, CombatProc::Critical);
    out.backstabGuild = selectGuildForProc(attacker, CombatProc::Backstab);

    // Step 1: roll crit against the player chance.
    PlayerChanceParams critParams;
    critParams.guildLevel = guildLevelOf(attacker, out.critGuild);
    critParams.coefficient = coefficientFor(CombatProc::Critical, out.critGuild);
    critParams.monLevel = in.monLevel;
    critParams.weaponHasAbility = in.weaponCrit;  // UNKNOWN: meaning of wCrit argument (c5)
    critParams.weaponFloor = in.weaponFloor;      // UNKNOWN: F equals ItemDropLevel (c5, c23, c97)
    out.critChance = playerProcChance(critParams, kCritRule);
    out.crit = rollPlayerProc(ctx, out.critChance);

    // Step 2: roll backstab against the player chance.
    PlayerChanceParams backParams;
    backParams.guildLevel = guildLevelOf(attacker, out.backstabGuild);
    backParams.coefficient = coefficientFor(CombatProc::Backstab, out.backstabGuild);
    backParams.monLevel = in.monLevel;
    backParams.weaponHasAbility = in.weaponBackstab;  // UNKNOWN: meaning of wBack argument (c5)
    backParams.weaponFloor = in.weaponFloor;
    out.backstabChance = playerProcChance(backParams, kBackstabRule);
    out.backstab = rollPlayerProc(ctx, out.backstabChance);

    // Step 4: damage modifier per R10.
    out.damageModifier = blowDamageModifier(out.crit, out.backstab);

    // Step 5 was performed above (R11); the guild indices are already in out.
    return out;
}

// Monster formula (R12):
//   p = Ln(attackerGL/divisor + 1)*100 * Ln(attackerFloor+1) - Ln(defenderLevel/999 + 1)*100
//   clamp p to [floor, cap], with divisor/floor/cap by backstab flag.
// UNKNOWN: attackerFloor definition (c18).
f64 monsterProcChance(i32 attackerGuildLevel, i32 attackerFloor, i32 defenderLevel, bool backstab) {
    f64 divisor = static_cast<f64>(backstab ? kMonsterBackstabDivisor : kMonsterCritDivisor);
    f64 lo = static_cast<f64>(backstab ? kMonsterBackstabFloor : kMonsterCritFloor);
    f64 hi = static_cast<f64>(backstab ? kMonsterBackstabCap : kMonsterCritCap);
    f64 chance = std::log(static_cast<f64>(attackerGuildLevel) / divisor + 1.0) * 100.0 *
                     std::log(static_cast<f64>(attackerFloor) + 1.0) -
                 std::log(static_cast<f64>(defenderLevel) / 999.0 + 1.0) * 100.0;
    return clampF(chance, lo, hi);
}

// R13 / P3: monster roll is Int(Rnd*100)+1 <= chance. "Fractions count" (c18).
// CONFLICT: with a chance of 5.4 this accepts five values, not six; intended form UNKNOWN.
bool rollMonsterProc(Ctx& ctx, f64 chance) {
    i64 draw = static_cast<i64>(ctx.rng.below(100)) + 1;
    return static_cast<f64>(draw) <= chance;
}

// Procedure P4 (companion breath, no-mishap sweep). R14: the sweep applies only when the
// breath does not hit the party. The trigger check itself is UNKNOWN (c6).
void companionBreathSweep(Ctx& ctx, Encounter& enc, Character& owner, i32 compLevel, bool breathHitParty) {
    if (breathHitParty) {
        return;
    }
    // Original quirk (P4, c6): a kill calls Sub0D08 on the owner's target record, not on the
    // group the breath is in. Sub0D08 is not supplied, so that kill-record step is UNKNOWN and
    // not modelled here. Members of other groups after a first kill are therefore not
    // touched by this record step.
    (void)owner;

    // R15: sweep every alive group.
    for (i32 gi = 0; gi < kMaxGroups; ++gi) {
        MonsterGroup& g = enc.groups[static_cast<u32>(gi)];
        if (g.livingCount <= 0) {
            continue;  // not an alive group
        }
        // Step 2: Kills = 0 for this group.
        i32 kills = 0;
        do {
            // Step 3: the front member takes Raw(Breath, compLvl, front.GuildLevel) after resistance.
            i32 dmg = companionBreathDamage(ctx, compLevel, g.front);
            g.front.hits -= dmg;
            if (g.front.hits > 0) {
                break;  // R15: a surviving member ends the group's turn
            }
            // Step 4: the front member dies.
            kills += 1;
            g.livingCount -= 1;
            if (g.livingCount > 0) {
                // R15: the next member also takes the breath.
                // UNKNOWN: next-member identity is not represented in MonsterGroup; modelled as
                // a fresh member of the same record with hits at maxHits.
                g.front.hits = g.front.maxHits;
                g.front.alive = true;
            } else {
                g.front.hits = 0;
                g.front.alive = false;
            }
        } while (g.livingCount > 0 && kills <= kBreathKillCap);  // R16: continue while Kills <= 4
    }
    // R17: no message is written by the sweep.
}

// Raw(Breath, compLvl, GuildLevel) after resistance. The breath damage formula and the
// resistance rule are not in the supplied SRD material.
// UNKNOWN: breath damage formula and resistance; returns 0 until supplied, so no member is
// killed by the sweep in this build.
i32 companionBreathDamage(Ctx& ctx, i32 compLevel, const Creature& front) {
    (void)ctx;
    (void)compLevel;
    (void)front;
    return 0;
}

// R18: the combat spell becomes the standing order (CombatSettingSpell) before the cast check.
// UNKNOWN: whether the order kind also changes is not supplied; only the spell id is set.
void setCombatSettingSpell(Character& who, i32 spellId) {
    who.order.spellId = spellId;
}

// R20 / P5 step 3: RefusedOrder(refusal).
//   Att = False                       (MAINMOD2.BAS:433; no Att field on Character, UNKNOWN)
//   if refusal == AntiMagic: sub102F(pv, -1)  -> member fights (approximated as Fight; UNKNOWN)
//   else: M56C4 = -1                   -> no order (mapped to CombatOrderKind::None; UNKNOWN)
//   SpellId = 0
// CONFLICT: R21 says an item's refusal leaves the standing order. The formula's else-branch
// clears it. The formula is preferred; whether the else-branch covers item refusals is UNKNOWN.
// UNKNOWN: enumerator spelling CastRefusal::AntiMagic is taken from the SRD text.
void refusedOrder(Character& who, CastRefusal refusal) {
    if (refusal == CastRefusal::AntiMagic) {
        who.order.kind = CombatOrderKind::Fight;
    } else {
        who.order.kind = CombatOrderKind::None;
    }
    who.order.spellId = 0;
}

// Asmodeus.Is(monster). The identity test is not in the supplied SRD material.
// UNKNOWN: identity test; monster id placeholder never matches.
bool asmodeusIs(const Ctx& ctx, const Creature& monster) {
    (void)ctx;
    return monster.id == kAsmodeusMonsterId;
}

// BeatAsmodeus(). Contents not supplied in the SRD material.
// UNKNOWN: effects of beating Asmodeus; no-op until supplied.
void beatAsmodeus(Ctx& ctx, GameState& game) {
    (void)ctx;
    (void)game;
}

// R22-R24 / P6: Only the lead group's slot 0 is checked. The caller supplies that record.
// The check runs before any other fight-end processing and before a join-cleared fight.
void checkAsmodeusAtFightEnd(Ctx& ctx, GameState& game, const Creature& leadSlot0) {
    if (asmodeusIs(ctx, leadSlot0)) {
        beatAsmodeus(ctx, game);
    }
}

// R25 / P7: quest kills settle at each kill. Only a Defeat quest on the active guild
// whose target matches the killed monster is affected.
// UNKNOWN: which guild's quest is settled, and the completion threshold, are not supplied.
void questKill(Ctx& ctx, Character& who, i32 monsterId) {
    (void)ctx;
    GuildMembership& membership = who.guilds[static_cast<u32>(who.activeGuild)];
    GuildQuest& quest = membership.quest;
    if (quest.kind != QuestKind::Defeat || quest.state != QuestState::Active) {
        return;
    }
    if (quest.target != monsterId) {
        return;
    }
    quest.count += 1;  // UNKNOWN: completion threshold (quest.level?) not supplied
}

// Combat XP group factor. The formula is not in the supplied SRD material (R27, R28).
// UNKNOWN: placeholder returns 1.0.
f64 xpGroupFactor(i32 levelL) {
    (void)levelL;
    return 1.0;
}

// Combat XP party factor. The formula is not in the supplied SRD material (R27, R28).
// UNKNOWN: placeholder returns 1.0.
f64 xpPartyFactor(i32 partyN) {
    (void)partyN;
    return 1.0;
}

// Combat XP per kill (R27). The formula is not in the supplied material. Overkill handling,
// final rounding, the source of h, and h = 0 handling are UNKNOWN (R28, c82).
// UNKNOWN: placeholder returns 0.0 so no XP is awarded until the formula is supplied.
f64 combatXpPerKill(i32 hits, i32 monsterM, f64 combatFactor, i32 levelL, i32 partyN) {
    (void)hits;
    (void)monsterM;
    (void)combatFactor;
    (void)levelL;
    (void)partyN;
    return 0.0;
}

// Awards kill XP to the character. R28: final rounding is UNKNOWN; round half even is used.
// Level-up handling is not in the supplied material and is not modelled.
void awardKillXp(Ctx& ctx, Character& who, f64 xp) {
    i64 amount = roundHalfEven(xp);
    if (amount <= 0) {
        return;
    }
    who.experience += amount;
    ctx.say("combat.kill_xp", "Kill experience gained.", {amount}, {});
}

}  // namespace mordor
