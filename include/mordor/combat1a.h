#pragma once
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- Constants (SRD c07p1 melee) ----

// R21 player melee passes defence 0 (original unassigned local, MODULE4.BAS:433)
inline constexpr i32 kPlayerMeleeDefence = 0;
// R1 monsters do not move first on arrival into an engaged group
inline constexpr bool kArrivalMonstersMoveFirst = false;
// R10 index of the Multiple Swings coefficient in guild abilityRating (guild.Abilities[3])
inline constexpr i32 kMultipleSwingsAbility = 3;
// R18 hit-gate clamp (percent) and neutral value when attack and defence are both 0
inline constexpr i32 kHitGateMinPercent = 2;
inline constexpr i32 kHitGateMaxPercent = 98;
inline constexpr i32 kHitGateNeutralPercent = 50;
// R17 damage modifier floor: values <= 0 become this
inline constexpr f64 kMinDamMod = 0.1;
// R23 melee modifier bonuses and size divisor
inline constexpr f64 kCritDamBonus = 5.0;
inline constexpr f64 kBackstabDamBonus = 2.0;
inline constexpr i32 kSizeDiffDivisor = 10;
// R25 companion gate defence bonuses (defence side)
inline constexpr i32 kCompanionCritDefence = 4;
inline constexpr i32 kCompanionBackstabDefence = 1;
// T5 monster property bit indices used in melee
inline constexpr i32 kBitSeeInvisible = 0;
inline constexpr i32 kBitNeedsGoodWeapon = 4;
inline constexpr i32 kBitImmuneToWeapons = 5;
inline constexpr i32 kBitCrit = 10;
inline constexpr i32 kBitBackstab = 11;

// ---- Tables (R12, T1 and T2) ----

// T2 maximum extra swings per guild, row 628 raw cells (index = Guild enum value)
inline constexpr Arr<i32, kGuildCount> kMaxExtraSwings = {0, 2, 2, 4, 2, 0, 0, 2, 0, 0, 0, 0};

// T1 community/audit milestone levels (cumulative guild level) per guild; -1 = none.
// CONFLICT: Ninja 55 and Warrior 90 in fidelity summary vs 56 and 91 in audit/community; audit values used.
// CONFLICT: sheet milestones (T1 column) differ from community values; community values used.
inline constexpr i32 kMilestonesPerGuild = 4;
inline constexpr Arr<Arr<i32, kMilestonesPerGuild>, kGuildCount> kCommunityMilestones = {{
    {{-1, -1, -1, -1}},  // Nomad
    {{91, 324, -1, -1}}, // Warrior
    {{111, 484, -1, -1}}, // Paladin
    {{56, 122, 267, 585}}, // Ninja
    {{105, 434, -1, -1}}, // Villain
    {{-1, -1, -1, -1}},  // Seeker
    {{-1, -1, -1, -1}},  // Thief
    {{116, 531, -1, -1}}, // Scavenger
    {{-1, -1, -1, -1}},  // Mage
    {{-1, -1, -1, -1}},  // Sorcerer
    {{-1, -1, -1, -1}},  // Wizard
    {{-1, -1, -1, -1}}   // Healer
}};

// ---- Result enums ----

// R15 weapon gate outcome, checked before the hit roll
enum class WeaponGateOutcome : i32 {
    Pass = 0,
    NeedsGoodWeaponBlocked = 1, // bit 4: no effect, hand finished, actor -> Defend, item order cleared
    ImmuneNoEffect = 2          // bit 5: no effect, order unchanged
};

// ---- Monster property helper (T5) ----

// T5 Has(bit n): monster property flag test
bool monsterHasBit(const Creature& m, i32 bit);

// ---- R1 Arrival into an engaged group ----

// R1 arrival combat starts only if group is non-empty, hostile (group[0]) and has a living member
bool arrivalCombatStarts(const MonsterGroup& group);
// R1 monster arrival round and StartCombat/MonstersRound/EndCombat are owned by other modules (not declared here)

// ---- R2-R6 Character turn order and aim ----

// R2 aim must move off a dead group before a character acts (also after a killing swing and after the characters' turns)
bool aimMustMove(const Encounter& enc, i32 aimedGroup);
// R3 further swings stay on the aimed group while it stands; otherwise aim moves to fn0DCD(-1)
// UNKNOWN: fn0DCD body not given in source; returns -1 as placeholder
i32 furtherSwingAim(const Encounter& enc, i32 aimedGroup);
// R4 combat setting applies only to characters with no order
bool combatSettingApplies(const CombatOrder& order);
// R5 retargeting when the aimed group is gone: use retarget() from clock.h (already declared)
// R6 an order of Defend suppresses the character's own attack
bool defendSuppressesAttack(const CombatOrder& order);

// ---- R7-R12 Weapon swings and extra swings ----

// R7 swings per equipped weapon = actor swings + weapon swings
i32 swingsPerWeapon(i32 actorSwings, i32 weaponSwings);
// R8 no weapon or total swings below 1: "Using no weapon!" and actor put on Defend; returns true if applied
// UNKNOWN: whether the "Hands" item (2 swings) counts as an equipped weapon (decides hasWeapon)
bool applyNoWeaponDefend(Ctx& ctx, Character& actor, bool hasWeapon, i32 totalSwings);
// R9 every equipped weapon is used (port replaces the original two-hand loop); returns indices into inv.worn
// UNKNOWN: weapon classification of worn items; equipped flag used as the test
Vec<i32> equippedWeaponSlots(const Inventory& inv);
// R10 extra swings from Multiple Swings coefficient m at guildLevel (threshold ladder with round half to even)
i32 extraSwingsFromMultiple(f64 m, i32 guildLevel);
// R11 swings never fall; returns the higher of current swings and the award (guilds do not stack)
i32 raiseSwingsIfHigher(i32 currentSwings, i32 award);
// R10/R11 recompute Character.combat.swings from every joined guild (Character.RecalcSwings)
void recalcSwings(Ctx& ctx, Character& c);
// R12 milestone tables: see kMaxExtraSwings and kCommunityMilestones above

// ---- R13-R15 Weapon effect gates ----

// R13 bit 4 NeedsGoodWeapon: weapon floor below (int)(LevelFound / 2) does nothing
bool weaponBlockedByNeedsGoodWeapon(const Creature& target, i32 weaponFloor);
// R14 bit 5 ImmuneToWeapons: no weapon does anything
bool weaponNullifiedByImmunity(const Creature& target);
// R15 weapon gate summary; bit 4 is tested before bit 5
WeaponGateOutcome checkWeaponGate(const Creature& target, i32 weaponFloor);
// R13 blocked hand is finished: actor switched to Defend and its item order cleared (party slot)
void applyBlockedHandDefend(Ctx& ctx, Party& party, i32 actorSlot);

// ---- R16-R19 Physical hit gate and damage ----

// R16 unseen flag passed to damage when target is invisible and actor lacks See Invisible
bool swingUnseen(const Character& actor, const Creature& target);
// R17 fn0DEE: shared hit gate and damage routine (engine form); returns damage, 0 on miss
i32 meleeHitAndDamage(Ctx& ctx, i32 atk, i32 def, i32 level, i32 strength, f64 damMod);
// R18 hit-gate threshold as percent, clamped to [2, 98]; 50 when atk and def are both 0
i32 hitThresholdPercent(i32 atk, i32 def);
// R18 hit roll: miss when Int(Rnd*100)+1 >= threshold; returns true when the gate passes
bool rollHitGate(Ctx& ctx, i32 thresholdPercent);
// R19 attack/defence term: Int(Int((Def - Att) / 2) / 4), before the random share is removed
i32 attackDefenceTerm(i32 att, i32 def);

// ---- R22 Miss and R27 chain continuation ----

// R22 a damage below 1 is a miss; a miss ends the weapon's swings
bool isMissedSwing(i32 damage);
// R27 chain continues only after a kill; a nonlethal blow or miss ends the weapon's swings
// CONFLICT: community reads a third swing needs a kill then another one-hit kill; engine treats any kill as continuing (engine used)
bool weaponChainContinues(i32 damage, bool killedTarget);

// ---- R23-R26 Character melee modifiers ----

// R23/R24 base damage factor F = 0.6 + ((ln(100 + L) / 1.75 - 2.3) * c^2) / 2, c = fighting value / 10
f64 meleeBaseFactor(i32 guildLevel, f64 fightingMod);
// R23 character melee damage modifier before the shared routine (crit, backstab, invisible halving, size)
f64 characterDamageMod(i32 guildLevel, f64 fightingMod, f64 weaponMod, bool crit, bool backstab, bool invisibleTarget, i32 sizeDiff);
// R16/R23 Rules.CharacterDamage: modifier then fn0DEE with defence kPlayerMeleeDefence; returns damage
i32 characterDamage(Ctx& ctx, i32 guildLevel, f64 fightingMod, f64 weaponMod, i32 strength, i32 sizeDiff, bool crit, bool backstab, i32 attack, bool unseen);
// R25 companion melee gate defence: target defence plus crit and backstab bonuses on the defence side; no size term
i32 companionGateDefence(i32 targetDefence, bool crit, bool backstab);
// R25 companion melee modifier: halved against an unseen target
f64 companionDamageMod(f64 baseMod, bool unseen);
// R26 fighting guild whose melee factor is largest among joined guilds
// UNKNOWN: which guild level the port passes to characterDamage; the audit only says pick the largest F
i32 chooseFightingGuild(const Ctx& ctx, const Character& c);

// ---- R28-R29 Mid-round deaths and round loop ----

// R28 a character with Hits at nought or below goes through sub0930 when a group picks a target
bool needsDeathProcessing(const Character& c);
// R28 sub0E85 leaves at once for anyone with a DeathState
bool leavesRoundAtOnce(const Character& c);
// R28 a member killed mid-round: companions released, ailments cleared, party saved and pruned (sub0930)
void processMidRoundDeath(Ctx& ctx, Party& party, i32 memberIndex);
// R29 the round loop ends only when one member is left; a lone survivor of two escapes the remaining groups
bool roundLoopEnds(i32 membersLeft);
// Original quirk (R29): a lone character killed by a companion's mishap gets one draw and no blow (no declaration; companions module)

// ---- R30 Experience timing ----

// R30 experience paid as each blow lands; "now pinned." and level messages judged by totals before and after the award
// UNKNOWN: bodies of sub0CDF, sub0F11 and fn12A1 are not given in source
void awardBlowExperience(Ctx& ctx, Character& actor, i64 xp);

// ---- R31 Damage and kill records ----

// R31 recorded hits = min(hits taken, monster hits remaining); overkill is not counted
i32 recordedHitsFor(i32 hitsTaken, i32 monsterHpRemaining);
// R31 records raised per monster struck: a killing blow raises damage then kill; charmed raises kill only;
// companion damage and kills are credited to the owner (owner passed as owner)
void recordMonsterStrike(Ctx& ctx, Character& owner, const Creature& target, i32 hitsTaken, bool killingBlow, bool charmed, bool companionCredit);

}  // namespace mordor
