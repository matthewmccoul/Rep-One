#pragma once
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_companion.h"
#include "mordor/companions1a.h"  // provides the forward declaration of GameState
#include "mordor/casting1.h"      // provides CastRefusal (used by refusedOrder)

namespace mordor {

// ---- constants, tables and small result types ----

inline constexpr i32 kNoGuild = -1;        // R11 a score of zero wins no guild
inline constexpr i32 kBreathKillCap = 4;   // R16 sweep loop continues while Kills <= 4

// Per-ability parameters for the player chance formula (P2 table, c18/c23/c25).
struct ProcRule {
    i32 divisor;
    i32 weaponLog;
    i32 floor;
    i32 cap;
    i32 increment;   // R10 damage modifier increment
    i32 effectBit;   // item ability bit in field 0x0E
};

// Critical: divisor 3, weaponLog 10, floor 1, cap 50, +5, bit 4 (0x10).
inline constexpr ProcRule kCritRule{3, 10, 1, 50, 5, 4};
// Backstab: divisor 2 (engine), weaponLog 100, floor 2, cap 75, +2, bit 5 (0x20).
// CONFLICT: backstab divisor is 2 in engine, audit and fidelity, but 3 in archive (c97). Engine preferred.
inline constexpr ProcRule kBackstabRule{2, 100, 2, 75, 2, 5};

// Monster crit and backstab parameters (R12, c18).
inline constexpr i32 kMonsterCritDivisor = 650;
inline constexpr i32 kMonsterBackstabDivisor = 500;
inline constexpr i32 kMonsterCritFloor = 2;
inline constexpr i32 kMonsterCritCap = 50;
inline constexpr i32 kMonsterBackstabFloor = 4;
inline constexpr i32 kMonsterBackstabCap = 75;

// Which ability a player proc is for (R11).
enum class CombatProc : i32 {
    Critical = 0,
    Backstab = 1
};

// Guild coefficient (CritMod or BSMod) for R11 (wiki, c97).
// UNKNOWN: full coefficient table; guilds not listed here score zero.
struct GuildCoefficient {
    Guild guild;
    i32 coefficient;
};
inline constexpr GuildCoefficient kCritCoefficients[2] = {
    {Guild::Warrior, 9},
    {Guild::Seeker, 3}
};
inline constexpr GuildCoefficient kBackstabCoefficients[2] = {
    {Guild::Thief, 9},
    {Guild::Ninja, 2}
};


// Inputs to the player blow procedure (P2).
struct PlayerBlowInput {
    i32 monLevel = 0;          // M in the player formula
    i32 weaponFloor = 0;       // F in the weapon term; UNKNOWN: equals archive ItemDropLevel (c5, c23, c97)
    bool weaponCrit = false;   // weapon carries the critical bit
    bool weaponBackstab = false; // weapon carries the backstab bit
};

// Results of the player blow procedure (P2 steps 1, 2, 4, 5).
struct PlayerBlowRolls {
    bool crit = false;
    bool backstab = false;
    f64 critChance = 0.0;
    f64 backstabChance = 0.0;
    i32 damageModifier = 0;    // R10
    i32 critGuild = kNoGuild;  // R11
    i32 backstabGuild = kNoGuild; // R11
};

// Inputs to the player chance formula (R7, R8, c18).
struct PlayerChanceParams {
    i32 guildLevel = 0;        // GLvl of the guild selected by R11
    i32 coefficient = 0;       // CritMod or BSMod of that guild
    i32 monLevel = 0;          // M
    bool weaponHasAbility = false;
    i32 weaponFloor = 0;       // F
};

// ---- R7 and R8 player chance and roll (P2 steps 1 and 2) ----

// R7 R8 player crit or backstab chance from the engine formula (c18; weapon term CONFLICT: engine max(p+w, w) preferred).
f64 playerProcChance(const PlayerChanceParams& p, const ProcRule& rule);

// R7 R8 proc roll: below(100) < chance, against the unrounded chance (c5, c23).
// CONFLICT: monster roll uses <= with Int(Rnd*100)+1 (see rollMonsterProc); which is intended for monsters is UNKNOWN.
bool rollPlayerProc(Ctx& ctx, f64 chance);

// ---- R10 damage modifier ----

// R10 damage modifier: +5 if crit, +2 if backstab, both apply when severed (Terms, c23, c25). R10 text not supplied.
i32 blowDamageModifier(bool crit, bool backstab);

// ---- R11 guild selection per ability ----

// R11 guild for a proc ability: maximises coefficient x guildLevel, strict improvement in table order, zero wins none.
// Returns the Guild as i32 index, or kNoGuild. UNKNOWN: guild-table order, taken here as the Guild enum order.
i32 selectGuildForProc(const Character& ch, CombatProc proc);

// ---- P2 player blow (CombatEngine.SwingOnce) ----

// P2 steps 1, 2, 4, 5: crit roll, backstab roll, damage modifier, guild selection.
// Procs roll before the hit gate. The hit gate (R9) is UNKNOWN and is run by the caller after this.
PlayerBlowRolls rollPlayerBlow(Ctx& ctx, const Character& attacker, const PlayerBlowInput& in);

// ---- R12 and R13 monster crit and backstab (P3) ----

// R12 monster crit or backstab chance, clamped to the crit or backstab range.
// UNKNOWN: attackerFloor definition (c18).
f64 monsterProcChance(i32 attackerGuildLevel, i32 attackerFloor, i32 defenderLevel, bool backstab);

// R13 monster roll: Int(Rnd*100)+1 <= chance (fractions count). Accepts 5 values for a chance of 5.4 (derived, c18).
bool rollMonsterProc(Ctx& ctx, f64 chance);

// ---- R14 to R17 companion breath sweep (P4) ----

// R14 R15 R16 R17 no-mishap breath sweep over alive groups; kill cap kBreathKillCap; writes no message.
// R14 trigger for "breath does not hit the party" is UNKNOWN; pass the result in breathHitParty.
// Quirk: a kill calls Sub0D08 on owner's target record, not on the group the breath is in (c6).
void companionBreathSweep(Ctx& ctx, Encounter& enc, Character& owner, i32 compLevel, bool breathHitParty);

// P4 step 3 damage: Raw(Breath, compLvl, front.GuildLevel) after resistance. UNKNOWN: Raw formula not supplied (c6).
i32 companionBreathDamage(Ctx& ctx, i32 compLevel, const Creature& front);

// ---- R18 to R21 combat-setting spell refusal (P5) ----

// R18 the combat spell becomes the standing order (CombatSettingSpell) before the cast check (checkCast, clock.h).
// UNKNOWN: which CombatOrderKind marks a spell order; the spell id is stored in order.spellId.
void setCombatSettingSpell(Character& who, i32 spellId);

// R20 R21 apply RefusedOrder(refusal). AntiMagic sets the member to Fight; other refusals clear the order.
// CONFLICT: an item refusal leaves the order (rule text) vs clears it (engine else-branch); engine preferred, so it clears.
// CONFLICT: original sets Att = 1 (MULTIMOD.BAS:480-483); port sets Att = False. UNKNOWN whether sub102F reproduces Att = 1.
void refusedOrder(Character& who, CastRefusal refusal);

// ---- R22 to R24 fight-end Asmodeus check (P6) ----

// R22 R24 UNKNOWN: Asmodeus.Is helper; tests whether a creature is Asmodeus.
bool asmodeusIs(const Ctx& ctx, const Creature& monster);

// R22 UNKNOWN: BeatAsmodeus contents (c47).
void beatAsmodeus(Ctx& ctx, GameState& game);

// R22 R23 R24 before any other fight-end processing, and before a join-cleared fight (clearedByJoining, monsters1b).
// Only the lead group is checked (R24). leadSlot0 is the lead group's first member; the caller resolves it
// (Encounter: monsters[Foes[0].Ref] if in range; combat: combat.Monsters[0].Def).
void checkAsmodeusAtFightEnd(Ctx& ctx, GameState& game, const Creature& leadSlot0);

// ---- R25 and R26 quest settlement (P7) ----

// R25 quest kill settles during combat at each kill (CombatEngine.QuestKill).
// UNKNOWN: which guild membership's quest; the active guild is used.
// R26: end-of-fight code does not handle quests, so no fight-end quest routine is declared.
void questKill(Ctx& ctx, Character& who, i32 monsterId);

// ---- R27 and R28 combat experience per kill (P8) ----

// R27 GF = 1 / (1 + L/333). UNKNOWN: meaning of L.
f64 xpGroupFactor(i32 levelL);

// R27 PF = 1 - 0.04 * (n - 1). UNKNOWN: meaning of n.
f64 xpPartyFactor(i32 partyN);

// R27 XP = floor[ h(9 - ln(h+1))(20 - ln h)(M+50) * CF * GF / 1000 ] * PF.
// R28 overkill handling and final rounding are UNKNOWN; returns the unrounded value.
// UNKNOWN: h = 0 (ln h undefined); source of h; meanings of M and L. Engine implementation of this formula is UNKNOWN (c82).
f64 combatXpPerKill(i32 hits, i32 monsterM, f64 combatFactor, i32 levelL, i32 partyN);

// R27 award per-kill XP to a character. R28 final rounding is UNKNOWN; implementer uses vbInt on the value.
void awardKillXp(Ctx& ctx, Character& who, f64 xp);

// ---- R29 monster specials ----
// R29 monster crit, backstab and spells are speculation in the manual; no engine evidence was supplied, so nothing is declared.

}  // namespace mordor
