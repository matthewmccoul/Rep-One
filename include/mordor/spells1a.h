#pragma once
// spells1a.h: SRD c10p1 Part A, rules R1-R55.
// Covers pre-cast checks, targeting, the shared damage shape, the group loop,
// healing and cure, party-wide encoding, spell states and resistance, caster
// levels, companion and monster special hits, charm, and teleport,
// displacement and passwall.
// CONFLICT: state_character.h, state_common.h, state_companion.h, state_item.h
// and state_dungeon.h have no include guards. This header includes state_party.h
// and state_dungeon.h, which both pull in state_character.h. Those files must be
// made idempotent (#pragma once) or this header will not compile.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_party.h"
#include "mordor/state_dungeon.h"
#include "mordor/casting1.h"  // FIXED: CastRefusal is declared in casting1.h; it was used without this include.

namespace mordor {

// ---- constants ----
inline constexpr i32 kPartyEffectBase = 200;        // R27 effect ids >= this apply party-wide
inline constexpr i32 kDamageGroupSlots = 4;         // R12 fixed group slots

// R4 dispatch route for an effect id
// UNKNOWN: Table A (effect id to route) is not included in this chunk.
enum class SpellRoute : i32 {
    Unhandled = 0,
    Damage,
    Heal,
    Cure,
    Buff,
    Resist,
    Charm,
    Teleport,
    Displace,
    Passwall,
    Fate
};

// R15-R17 verdict on one victim
enum class VictimVerdict : i32 {
    Hit = 0,
    MagicImmune,
    BlockedByCharmBit,
    AllOrNothingMiss,
    CharmSpared,
    ClassGateBlocked
};

// R53 outcome of a destination test
enum class ArrivalOutcome : i32 {
    FallsFromTheSky = 0,
    IntoRock,
    Arrives
};

// ---- small structs ----
// R32/R34/R38 caster description for the shared spell paths
struct SpellCasterInfo {
    i32 level = 1;             // SL: spell level used in the formulas (R32, R38 for companions and monsters)
    bool isCompanion = false;  // companion caster: R33 heals fail, R34 immunity test reads the caster
    bool magicImmune = false;  // R34 caster companion's bit 2 (companion casts only)
};

// R14 group-loop counters
struct DamageLoopCounters {
    i32 groupCount = 0;
    i32 step = 0;
};

// R12-R22 result of a damage spell over foe groups
struct DamageSpellOutcome {
    i32 groupsReached = 0;          // R20 one caption per group reached
    i32 victimsHit = 0;
    i32 targetMark = -1;            // R22 aimed group restored; -1 when nothing is left standing
    bool nothingDone = false;       // R20
    bool casterOrderToFight = false;  // R17.1 / R20
    bool itemOrderLost = false;     // R20
};

// R50 typed coordinates, relative to facing
struct TypedOffset {
    i32 right = 0;
    i32 forward = 0;
    i32 down = 0;
};

// R43-R48 result of a charm cast
struct CharmOutcome {
    bool refusedNoRoom = false;     // R43
    i32 charmedCount = 0;           // R46
    bool joinSkipped = false;       // R47 companion caster: creature vanishes, no join
    bool casterOrderToFight = false;  // R46 nothing charmed
};

// ---- declarations ----

// R1 pre-cast checks, in order; the first failure refuses the cast
CastRefusal precheckCast(Ctx& ctx, const Character& caster, i32 spellId, bool fromItem,
                         const Item& item, bool antiMagicArea);

// R2 mod dungeon-cast refusal (nothing is done when refused)
// FIXED: ModCastRefusal exists nowhere; replaced by the closest existing type, CastRefusal.
CastRefusal checkModDungeonCast(Ctx& ctx, const Party& party, i32 casterIndex, i32 spellId,
                                bool monstersOnSquare, bool targetGiven);

// R3 "Cast on Who?" question for a spell
// CONFLICT: c36 says companions are never offered; c21 and c65 offer them. Companions are offered per R3 (c21).
// FIXED: TargetPrompt exists nowhere; replaced by a plain i32 result.
i32 targetPromptFor(Ctx& ctx, i32 spellId);

// R4 route a cast on its effect id; party-wide ids (>= 200) apply to the whole party
SpellRoute routeSpell(Ctx& ctx, i32 spellId);

// R5 a cast becomes the caster's order when engaged
// UNKNOWN: which CombatOrderKind a spell order uses is not stated; the engine mapping is needed.
// Item use in combat is cast at once in the port (c34); the caller handles that path.
void makeCastTheOrder(Ctx& ctx, Character& caster, i32 spellId, bool engaged);

// R7/R8 character or companion spell damage against a foe (base, log term, power term, random cut, resistance)
// R8 audit form is identical except that SL is clamped to 1..255; the clamp is applied here.
i32 playerSpellDamage(Ctx& ctx, i32 base, i32 specialEffect, i32 spellLevel, i32 resistPercent);

// R10 monster spell damage; resistance applies only when hasResist (ResistIndex > -1)
i32 monsterSpellDamage(Ctx& ctx, i32 monsterLevel, i32 damage2, i32 specialEffect,
                       bool hasResist, i32 resistPercent);

// R11 a monster spell replaces the turn's damage total (assignment, not addition);
// any earlier melee blow in the turn is discarded
void setTurnDamageFromSpell(i32& turnDamageTotal, i32 spellDamage);

// R13 start group: the aimed living group, else the first living group; -1 if none
i32 spellStartGroup(const Encounter& enc, i32 aimedGroup);

// R21 companion caster: the opening group is resolved at cast time, not from the window's drawn target
i32 companionOpeningGroup(const Encounter& enc);

// R14 advance the loop counters; a processed group bumps both, an empty slot or dead group bumps only step
void advanceDamageLoop(DamageLoopCounters& counters, bool groupProcessed);

// R14 loop continues while the group count stays below Damage1
bool damageLoopContinues(const DamageLoopCounters& counters, i32 damage1Groups);

// R12-R22 damage spell over up to Damage1 groups, AffectGroup victims per group, stopping at the first survivor
DamageSpellOutcome castDamageSpellOnFoes(Ctx& ctx, Encounter& enc, const SpellCasterInfo& caster,
                                         i32 spellId, i32 aimedGroup);

// R16 resistance percent used for a victim; chosen by the spell's resistance index
// UNKNOWN: mapping from resistIndex to Creature field is not stated; magicResistance is assumed.
i32 victimResistPercent(const Creature& victim, i32 resistIndex);

// R17 filters in order: magic immune; bit 3 vs charm; effect 100 all-or-nothing; charm kill-only; class gate
// Companion casts skip the monster immunity test (R34 quirk).
VictimVerdict judgeSpellVictim(const Creature& victim, i32 effectId, i32 damage, i32 gateClass,
                               bool casterIsCompanion);

// R18 class gate for effects 75-99: pass only when monsterClass == effect - 75
// CONFLICT: c27 reads the class from the group aimed at; c62 reads it per target. The aimed group's class (c27) is used via gateClass.
bool classGatePasses(i32 effectId, i32 monsterClass);

// R20 captions: one per group reached; "Failed!" when nothing did anything
void emitGroupCaptions(Ctx& ctx, const DamageSpellOutcome& outcome);

// R23 charm category check
// UNKNOWN: mapping of charm to effect ids is not fully stated; read from the spell's category (kind 7).
bool spellIsCharm(Ctx& ctx, i32 spellId);

// R24 heal roll: base + base*Ln(SL)/Ln(10) + power term - random cut; the minimum heal is 1
// UNKNOWN: SpellHeal's healLevel parameter is not defined in this chunk; SL is passed through.
i32 spellHealRoll(Ctx& ctx, i32 base, i32 specialEffect, i32 spellLevel);

// R25 one heal roll on a party member, capped at max hits; returns the amount healed
i32 healCharacter(Ctx& ctx, Vitals& vitals, i32 base, i32 specialEffect, i32 spellLevel);

// R25 one heal roll on a companion; changes Hits, capped at MaxHits; returns the amount healed
i32 healCompanion(Ctx& ctx, Companion& target, i32 base, i32 specialEffect, i32 spellLevel);

// R26 effect 18 general Cure: clears poison and disease
void castGeneralCure(Ctx& ctx, Character& target);

// R26 Cure Poison: clears poison without refusal
void castCurePoison(Ctx& ctx, Character& target);

// R26 Cure Disease: clears disease without refusal
void castCureDisease(Ctx& ctx, Character& target);

// R26 Cure Paralysis: clears the caster's paralysis whoever was picked
void castCureParalysis(Ctx& ctx, Character& caster);

// R28 cast-time state: sets the effect bit and flag; does not recompute
// UNKNOWN: Character has no SpellEffects bitset field; the effect id is recorded in activeEffectIds.
void applyCastState(Ctx& ctx, Character& target, i32 effectId, i32 casterSpellLevel);

// R28 Protect value written at cast time: Int(CasterSpellLevel/2) + 1
i32 protectValueAtCast(i32 casterSpellLevel);

// R29 recompute: protection, gear and spell bits, then Levitate, Invisible, See Invisible and Protect tests
// UNKNOWN: gear bit meanings are not restated in this chunk.
void recomputeSpellStates(Ctx& ctx, Character& target);

// R29 protection value written by recompute: Int(guild level/2) + 1
i32 protectValueOnRecompute(i32 guildLevel);

// R29 clear used by antimagic or blackout: SpellEffects = 0, spell resistance cleared, recompute
// UNKNOWN: spell-granted resistance (SpellResist) has no field in Resistances; resist[] is assumed.
void clearSpellStates(Ctx& ctx, Character& target);

// R30 protection defence bonus: B(3 * ln(Protection)^2)
i32 protectionDefenceBonus(i32 protection);

// R31 resistance grant: kind = effect - 50; raise to 95 if lower, then recompute; repeat casts do not stack
void grantSpellResistance(Ctx& ctx, Character& target, i32 effectId);

// R32 companion caster level: clamp(Floor(compDef.GuildLevel / 6.0), 1, 255)
i32 companionCasterLevel(i32 companionGuildLevel);

// R33 companion heal: AffectGroup 0 means the cast fails; emits "Comp. #N failed in casting"
bool companionHealFailsCast(Ctx& ctx, i32 spellId, i32 companionNumber);

// R34 companion caster with its own magic immunity: the spell does nothing
bool companionCastBlockedByOwnImmunity(const SpellCasterInfo& caster);

// R35 companion-to-party cast: zero damage if the target companion has bit 2
i32 applyTargetImmunityToPartyDamage(i32 damage, u32 targetSpecialPropertyFlags);

// R36 breath sweep applies to a group not aimed at when the special attack bit 3 is set and guild level < 40
bool breathSweepApplies(u32 specialAttackFlags, i32 attackerGuildLevel);

// R36 breath against a non-aimed group: first creature falls, each kill reloads the aimed front,
// and the room is written back with that group's count at 0
void companionBreathSweep(Ctx& ctx, Encounter& enc, i32 aimedGroup, i32 targetGroup);

// R37 special hit range: l040E = Int((Log(l040C) + l040C/15)/Log(1.08) - targetLevel/10 + 1), l040C = attackerLevel + 1
i32 specialHitRange(i32 attackerLevel, i32 targetLevel);

// R37 companion breath damage: (Int((Rnd*E+1)/2) + E/4) * 0.7, then resistance halved
i32 companionBreathDamage(Ctx& ctx, i32 attackerLevel, i32 targetLevel, i32 resistPercent);

// R37 companion spit or electrocution damage: as breath but adds a half of the range
// UNKNOWN: the exact spit and electrocution expression is not given; E/2 per the summary is used.
i32 companionSpitOrShockDamage(Ctx& ctx, i32 attackerLevel, i32 targetLevel, i32 resistPercent);

// R38 monster caster level: max(1, Floor(GuildLevel/4)), capped at 255
i32 monsterCasterLevel(i32 monsterGuildLevel);

// R39 monster spell on one target: R10 damage; companions with bit 2 take nothing; effect 100 counts only if it kills
i32 monsterSpellDamageToTarget(Ctx& ctx, i32 monsterGuildLevel, i32 spellId, bool targetIsCompanion,
                               u32 targetFlags, i32 targetHits, bool hasResist, i32 resistPercent);

// R40 acid spit hit chance in percent: 50, or 90 if character level > monster level + 25%, or 10 if below monster level - 25%
i32 acidSpitChancePercent(i32 monsterGuildLevel, i32 charGuildLevel);

// R40 roll for the acid spit to land
bool rollAcidSpit(Ctx& ctx, i32 monsterGuildLevel, i32 charGuildLevel);

// R40 raw acid damage; resistance (index 10) is applied by the caller to the chosen target
i32 acidSpitDamage(Ctx& ctx, i32 monsterGuildLevel, i32 charGuildLevel);

// R41 acid target slot: 0 = character, 1..4 = companion slots
// Original quirk: slot 0 is never reached in companion terms, slot 4 is empty and still counts as a target.
// CONFLICT: the second draw l0474 uses mc01A0 (value not given) and differs from the first; the first draw is used.
i32 rollAcidTargetSlot(Ctx& ctx);

// R42 monster breath damage: half of ((Rnd*range+1)/2 + range/4) * 0.7, with range from monsterLevel * Log(monsterLevel+1)
// Original quirk: monster breath uses currentGuildLevel/20, companion breath uses targetLevel/10 (kept separate).
i32 monsterBreathDamage(Ctx& ctx, i32 monsterLevel, i32 currentGuildLevel);

// R43 charm needs a free companion slot; false means refused before anything is spent
bool charmHasRoom(i32 occupiedCompanionSlots);

// R43-R48 charm cast: refused without room; each victim rolled as a damage spell keeping only what it would kill;
// taken creatures join unless the caster is a companion (R47); a slot filled mid-cast is taken back (R48)
CharmOutcome castCharm(Ctx& ctx, Encounter& enc, Arr<Companion, kCompanionSlots>& companions,
                       const SpellCasterInfo& caster, i32 spellId, i32 aimedGroup);

// R45 companion slot for a charmed creature: hits at cast, rolled max, bound at spell level, alignment copied
// UNKNOWN: the identifying routine fn1272 is not described; identification is taken from the foe's record.
// CONFLICT: Creature.align (CharAlign) and Companion.alignment (CompanionAlignment) use different numbering; mapped by name.
Companion makeCharmedCompanion(Ctx& ctx, const Creature& foe, i32 hitsAtCast, i32 rolledMaxHits,
                               i32 spellLevel, i32 slot);

// R49 kill credit for a charm: counted as the caster's kill with no hits; experience is paid on the hits
// Port deviation: the port also increments the player's kills on charm (c23); the SRD count is kept here.
void creditCharmKill(Ctx& ctx, Character& caster);

// R50 typed coordinates: teleport takes Right, Forward, Down (max 3); displacement takes Right, Forward
// Returns false, and nothing happens, with fewer than two values.
bool readTypedOffsets(const Vec<i32>& values, bool teleport, TypedOffset& out);

// R51 offset relative to facing; stepping off the 30 x 30 grid wraps to the other side
// Original quirk: the order is right, then forwards, relative to facing.
Location offsetFromFacing(const Location& from, const TypedOffset& off, Facing facing);

// R53 destination outcome: level 0 falls from the sky; above 15 is rock death; target rock is rock death; else arrives
ArrivalOutcome arrivalOutcome(i32 destLevel, bool destIsRock);

// R52 teleport or displacement: outcome decided before anyone moves, then settled
// UNKNOWN: the square field that records rock is not named in this chunk; rockRecorded is assumed.
ArrivalOutcome castTeleportOrDisplace(Ctx& ctx, Party& party, FloorState& floor, i32 casterIndex,
                                      i32 spellId, const TypedOffset& off);

// R52 passwall: moves first, then tests the arrival square
// UNKNOWN: the distance the passwall step covers is not stated; one square forward is assumed.
ArrivalOutcome castPasswall(Ctx& ctx, Party& party, FloorState& floor, i32 casterIndex, i32 spellId);

// R54 rock death: the whole party dies; bodies are left on that rock square and level
void applyRockDeath(Ctx& ctx, Party& party, FloorState& floor);

// R55 settle an arrival: a death costs no points; otherwise the caster arrives, points are spent,
// and the arrival square is resolved
void settleArrival(Ctx& ctx, Party& party, FloorState& floor, i32 casterIndex,
                   ArrivalOutcome outcome, i32 spellPointCost);

}  // namespace mordor
