#pragma once
// Module combat1b: SRD part c07p1, rules R32-R52 (swings, monster round, companion pass,
// mishaps, victory sequence, archive formulas). Declarations only.
// Rules that use monsterCritChance (API INDEX, companions1b.h) call it from the source file.
// NOTE: state_character.h, state_common.h, state_item.h, state_companion.h and state_dungeon.h
// have no include guards. This header includes state_party.h only, and forward-declares FloorState.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_party.h"

namespace mordor {

struct FloorState;  // defined in state_dungeon.h

inline constexpr i32 kFreeAttackChangedLevelCap = 98;
// R34 target pick
inline constexpr i32 kTargetPickTries = 100;
// R35 melee modifiers
inline constexpr i32 kPercentDie = 100;
inline constexpr f64 kSizeStepDivisor = 10.0;
inline constexpr f64 kCritDamageBonus = 2.0;      // bit10
inline constexpr f64 kBackstabDamageBonus = 1.0;  // bit11
inline constexpr f64 kOpeningChestBonus = 0.75;
inline constexpr f64 kInvisibleMalus = 0.5;
// R36 protection bonus
inline constexpr f64 kProtectionLogScale = 3.0;
// R39 special attack range
inline constexpr f64 kSpecialRangeLogBase = 1.08;
inline constexpr f64 kSpecialRangeDivisor = 15.0;
inline constexpr i32 kSpecialRangeLevelDivisor = 10;
// R40 special attack raw damage
inline constexpr i32 kBreathShareDivisor = 4;
inline constexpr i32 kSpitShareDivisor = 2;
inline constexpr f64 kSpecialDamageScale = 0.7;
// R42 defend bonus
inline constexpr f64 kDefendFactor = 1.5;
// R44 mishap chance
inline constexpr i32 kMishapNumerator = 999;
inline constexpr i32 kMishapBodyDivisor = 20;
// R51 archive hit chance clamp
inline constexpr i32 kArchiveHitMin = 1;
inline constexpr i32 kArchiveHitMax = 97;

// ---- enums and small structs ----
// R33 free-attack kind (not in API INDEX)
enum class AttackKind : i32 { Moved = 0, ChangedLevel = 1 };

// R40 special attack kind
enum class SpecialAttackKind : i32 { Breath = 0, AcidSpit = 1, Electrocute = 2 };

// R44 mishap source
enum class MishapSource : i32 { Breath = 0, SpitOrMelee = 1, Spell = 2 };

// R44 tuple (factor, lo, hi). Tuple order is inferred from the SRD.
struct MishapTuning {
    f64 factor;
    f64 lo;
    f64 hi;
};
inline constexpr MishapTuning kMishapBreath{2.0, 2.0, 40.0};
inline constexpr MishapTuning kMishapSpitMelee{1.0, 0.5, 20.0};
inline constexpr MishapTuning kMishapSpell{1.5, 1.0, 40.0};

// R35 inputs to the monster melee modifier chain
struct MeleeModInput {
    f64 damageMod = 1.0;                 // DamageMod
    bool targetInvisible = false;
    bool attackerSeesInvisible = false;  // bit 0
    bool targetOpeningChest = false;
    bool critCapable = false;            // bit10
    bool backstabCapable = false;        // bit11
    i32 monsterLevel = 0;                // monLvl
    i32 levelFound = 0;                  // LevelFound
    i32 characterLevel = 0;              // charLvl
    i32 targetSize = 0;
    i32 monsterSize = 0;                 // def.Size
};

// R38 per-group attack state
struct GroupAttackFlags {
    bool spellUsed = false;
    bool attackEnded = false;
};

// ---- declarations ----

// R32 every swing of every hand counts as one weapon blow (hits, criticals, backstabs, severs, kills).
void countWeaponBlow(Ctx& ctx, Character& ch);

// R33 free-attack chance in percent: 50 + (monDex - charDex) * 5, cap 98 on ChangedLevel, floor 5. Pure.
// charDex is the Dex of the drawn character, not the party's best.
i32 freeAttackChance(AttackKind kind, i32 monDex, i32 charDex);

// R33 roll a free attack. Only on Moved or ChangedLevel; Round never rolls.
bool rollFreeAttack(Ctx& ctx, AttackKind kind, i32 monDex, i32 charDex);

// R34 target pick: 0 = character, 1..kCompanionSlots = living companion slot.
// Falls back to 0 (character) after kTargetPickTries failed companion picks.
// UNKNOWN: whether the returned pick indexes companions[pick-1] or matches Companion.slot.
i32 pickMonsterTarget(Ctx& ctx, const Party& party, const Arr<Companion, kCompanionSlots>& companions);

// R35 monster melee damage modifier chain (crit and backstab rolls), before MonsterBlow.
// Uses monsterCritChance (API INDEX). UNKNOWN: whether the LN(DamMod)^2 reduction sits here or in MonsterBlow.
// CONFLICT: R35 says DamageMod <= 0 becomes 1; T10 table says floor 0.1. R35 formula used.
f64 meleeDamageModifier(Ctx& ctx, const MeleeModInput& in);

// R35 size adjustment. Target smaller than monster: +sizeDiff/10. Target bigger: -sizeDiff/10. Pure.
f64 sizeDamageAdjust(i32 targetSize, i32 monsterSize);

// R36 protection bonus: P > 0 ? VbInt(Log(P) * (Log(P) * 3)) : 0. Pure.
i64 protectionBonus(i32 protection);

// R36 defence with the protection bonus added. Pure.
i32 defenceWithProtection(i32 defence, i32 protection);

// R37 MonsterBlow: monster melee resolution body. UNKNOWN: body not in sources.
// Returns damage. Engine player-side random adjust is not part of this declaration.
// CONFLICT: monster damage reduction form (engine dmg -= a vs archive Base - Random[0..n]). Unresolved.
i32 monsterBlow(Ctx& ctx, i32 monAtt, i32 targetDef, i32 monLvl, i32 monStr, f64 damMod);

// R37 melee resolution: modifiers (R35), then MonsterBlow.
i32 resolveMonsterMelee(Ctx& ctx, const MeleeModInput& in, i32 monAtt, i32 targetDef, i32 monStr);

// R38 only the first monster spell attack per group is used. Pure query.
bool groupMayCastSpell(const GroupAttackFlags& flags);

// R38 record the group's spell attack; it also ends the group's attack.
void recordGroupSpellAttack(Ctx& ctx, GroupAttackFlags& flags);

// R39 special attack range: Int((Log(a) + a/15) / Log(1.08) - targetLevel/10 + 1), a = attackerGuildLevel + 1. Pure.
// UNKNOWN: whether targetLevel/10 is integer or real division. Integer division is used here.
i32 specialAttackRange(i32 attackerGuildLevel, i32 targetLevel);

// R40 share added to the roll: range/4 for breath, range/2 for acid spit and electrocute. Pure.
i32 specialAttackShare(i32 range, SpecialAttackKind kind);

// R40 raw damage: VbInt((Int((Rnd*range + 1)/2) + share) * 0.7).
// CONFLICT: text says "rounded half to even per source wording" but the formula uses VbInt. VbInt (floor) used.
// Electrocute itself is declared in monsters1b.h (API INDEX).
i32 specialAttackRawDamage(Ctx& ctx, i32 range, SpecialAttackKind kind);

// R40 monster's own breath (sub0E85, MONSMOD.BAS:2702-2704). Not interchangeable with F11/F12.
// UNKNOWN: exact expression. Starts from level*Log(level+1), divides target level by 20, halves at the end.
i32 monsterBreathDamage(Ctx& ctx, i32 monLevel, i32 targetLevel);

// R41 monster natural weapons are data only (no routine). Data belongs in GameData.monsters.
// UNKNOWN: whether a roll is compared to chance, and how the damage bonus applies.

// R42 defend bonus: defence x1.5 when defending (engine comment).
// CONFLICT: audit says the +50% is overwritten and does not apply. Engine preference kept.
// UNKNOWN: rounding and where in code the factor is applied. vbInt used.
i32 defendingDefence(i32 defence, bool defending);

// R43 companion pass for one owner: four slots. A living companion breathes or spits, then casts or strikes.
// The owner's castThisRound flag is the shared one-cast-per-round gate.
// UNKNOWN: which foe list (Encounter or Party::foes) the companion pass targets. Encounter used.
void runOwnerCompanionPass(Ctx& ctx, Party& party, Arr<Companion, kCompanionSlots>& companions,
                           Encounter& enc, i32 ownerIndex);

// R44 mishap chance in percent: clamp((999/max(1,M)) * (partyBodies/20) * factor, lo, hi). Pure.
f64 mishapChance(MishapSource source, i32 monsterLevel, i32 partyBodies);

// R45 drawing the owner cancels the misdirection.
void cancelMisdirectionOnDraw(Ctx& ctx, Character& owner);

// R45 owner is freed from paralysis when companions finish the encounter.
void freeOwnerFromParalysis(Ctx& ctx, Character& owner, bool companionsFinishedEncounter);

// R46 victory sequence after the last monster falls. Steps 1-7 of the SRD, in order.
// Steps 1 and 5 (store room) are write-back of value state; no-op here.
void runVictorySequence(Ctx& ctx, Party& party, FloorState& floor, i32 roomIndex, Encounter& enc,
                        i64 nowMs, i32 headerAreaCount);

// R46 step 2: mark the room cleared, only if it belongs to a real area (Area > 0).
// UNKNOWN: exact Area fields written (emptiedAt, stamp, respawnT).
void markRoomCleared(Ctx& ctx, FloorState& floor, i32 roomIndex, i64 nowMs, i32 headerAreaCount);

// R46 step 3: gold share per party member. Pure. UNKNOWN: remainder handling.
Currency splitFightGold(Currency fightGold, i32 partySize);

// R46 step 4: pay the split gold to party members.
void payFightGold(Ctx& ctx, Party& party);

// R46 step 6: NoteMonster(killed: true) for one member and one dead monster.
// UNKNOWN: exact fields updated beyond monsterKills and kills.
void noteMonsterKill(Ctx& ctx, Character& member, const Creature& monster);

// R46 step 7: hall.Check(DeadliestCreature, killer, def.Id, PowerOf). Save is host responsibility.
// UNKNOWN: body of hall.Check and the MonsterPower definition.
void checkDeadliestCreature(Ctx& ctx, Character& killer, i32 monsterId);

// R47 announce experience only when it takes a character past a level (MONSMOD.BAS:305-307).
void announceExperienceIfLevelUp(Ctx& ctx, const Character& ch, i32 levelBefore);

// R48 fight chest availability. Belongs to the area it was made in; no Encounter source or roomArea <= 0
// means it belongs wherever you stand. Pure.
bool chestAvailableHere(bool hasEncounterSource, i32 roomArea, i32 standingArea);

// R49 attack/defence composition (StatAD + ItemAD + GuildAD) is already declared as totalAD and guildAD
// in attributes.h (API INDEX). Not redeclared. Archive form, conflicting.

// R50 archive weapon-resistance threshold: (ItemLevel * 2) + 1. Pure.
// CONFLICT with R13 bit 4 (engine: floor(LevelFound/2)). Not reconciled. UNKNOWN: level-band mapping.
i32 archiveWeaponResistThreshold(i32 itemLevel);

// R51 archive chance to hit, as percent in [kArchiveHitMin, kArchiveHitMax]. Pure.
// Fractions multiplied by 100 (inferred). CONFLICT: engine clamp is 2-98 (R17).
// UNKNOWN: case Atk == Def with Def != 0 is not stated.
f64 archiveHitChancePercent(i32 atk, i32 def);

// R52 archive monster base melee damage, with the random reduction Random[0..Int((CharDef-MonAtk)/8)].
// CONFLICT with R17/R35 engine path. UNKNOWN: lower bound when CharDef < MonAtk.
f64 archiveMonsterDamage(Ctx& ctx, i32 monStrength, i32 monGuildLevel, f64 damMod,
                         i32 charDef, i32 monAtk);

// R52 archive player form: Final = Base + Rnd[1 to Atk/8] (the "defense bug" form). Inferred shape.
i32 archivePlayerDamage(Ctx& ctx, f64 baseDamage, i32 atk);

}  // namespace mordor
