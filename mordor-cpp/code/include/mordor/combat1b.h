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




// R34 target pick: 0 = character, 1..kCompanionSlots = living companion slot.
// Falls back to 0 (character) after kTargetPickTries failed companion picks.
// UNKNOWN: whether the returned pick indexes companions[pick-1] or matches Companion.slot.
i32 pickMonsterTarget(Ctx& ctx, const Party& party, const Arr<Companion, kCompanionSlots>& companions);





// R37 MonsterBlow: monster melee resolution body. UNKNOWN: body not in sources.
// Returns damage. Engine player-side random adjust is not part of this declaration.
// CONFLICT: monster damage reduction form (engine dmg -= a vs archive Base - Random[0..n]). Unresolved.
i32 monsterBlow(Ctx& ctx, i32 monAtt, i32 targetDef, i32 monLvl, i32 monStr, f64 damMod);




// R39 special attack range: Int((Log(a) + a/15) / Log(1.08) - targetLevel/10 + 1), a = attackerGuildLevel + 1. Pure.
// UNKNOWN: whether targetLevel/10 is integer or real division. Integer division is used here.





// R41 monster natural weapons are data only (no routine). Data belongs in GameData.monsters.
// UNKNOWN: whether a roll is compared to chance, and how the damage bonus applies.








// R46 step 3: gold share per party member. Pure. UNKNOWN: remainder handling.
Currency splitFightGold(Currency fightGold, i32 partySize);

// R46 step 4: pay the split gold to party members.
void payFightGold(Ctx& ctx, Party& party);

// R46 step 6: NoteMonster(killed: true) for one member and one dead monster.
// UNKNOWN: exact fields updated beyond monsterKills and kills.
void noteMonsterKill(Ctx& ctx, Character& member, const Creature& monster);




// R49 attack/defence composition (StatAD + ItemAD + GuildAD) is already declared as totalAD and guildAD
// in attributes.h (API INDEX). Not redeclared. Archive form, conflicting.



// R52 archive monster base melee damage, with the random reduction Random[0..Int((CharDef-MonAtk)/8)].
// CONFLICT with R17/R35 engine path. UNKNOWN: lower bound when CharDef < MonAtk.
f64 archiveMonsterDamage(Ctx& ctx, i32 monStrength, i32 monGuildLevel, f64 damMod,
                         i32 charDef, i32 monAtk);


}  // namespace mordor
