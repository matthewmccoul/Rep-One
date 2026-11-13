#include <cmath>
#include "mordor/spells1a.h"

#include "test_support.h"
#include "mordor/combat1b.h"

using namespace mordor;

namespace {

bool approxEq(f64 a, f64 b) {
    return std::fabs(a - b) < 1e-9;
}

// Living companion: occupied, not dead, has hits.
void makeLiving(Companion& c) {
    c.occupied = true;
    c.isDead = false;
    c.deathState = CompanionDeathState::Normal;
    c.hits = 10;
    c.maxHits = 10;
}

}  // namespace

// R33: chance = 50 + (monDex - charDex) * 5
TEST(FreeAttack_ChanceFromDexDifference) {
    CHECK_EQ(freeAttackChance(AttackKind::Moved, 10, 5), 75);
    CHECK_EQ(freeAttackChance(AttackKind::Moved, 10, 10), 50);
    CHECK_EQ(freeAttackChance(AttackKind::ChangedLevel, 9, 0), 95);
}

// R33: floor of 5 applies to both kinds.
TEST(FreeAttack_FloorOfFive) {
    CHECK_EQ(freeAttackChance(AttackKind::Moved, 0, 20), 5);
    CHECK_EQ(freeAttackChance(AttackKind::ChangedLevel, 0, 20), 5);
    CHECK_EQ(freeAttackChance(AttackKind::Moved, 0, 9), 5);   // exactly 5
    CHECK_EQ(freeAttackChance(AttackKind::Moved, 0, 8), 10);  // above floor
}

// R33: ChangedLevel is capped at 98; the cap starts at chance 99.
TEST(FreeAttack_ChangedLevelCapAt98) {
    CHECK_EQ(freeAttackChance(AttackKind::ChangedLevel, 10, 0), 98);
    CHECK_EQ(freeAttackChance(AttackKind::ChangedLevel, 20, 0), 98);
    CHECK_EQ(freeAttackChance(AttackKind::ChangedLevel, 9, 0), 95);
}

// R33 quirk: Moved has no ceiling.
TEST(FreeAttack_MovedHasNoCeiling) {
    CHECK_EQ(freeAttackChance(AttackKind::Moved, 10, 0), 100);
    CHECK_EQ(freeAttackChance(AttackKind::Moved, 20, 0), 150);
}

// R33: attack = (Int(Rnd*100)+1) < chance.
// chance 75: Rnd 0.735 -> 73+1=74 < 75 attacks; Rnd 0.745 -> 74+1=75 does not.
TEST(RollFreeAttack_ThresholdFromScriptedDraws) {
    tsup::Env env({0.735, 0.745});
    CHECK(rollFreeAttack(env.ctx, AttackKind::Moved, 10, 5));
    CHECK(!rollFreeAttack(env.ctx, AttackKind::Moved, 10, 5));
}

// R33: ChangedLevel uses the 98 cap.
// capped chance 98: Rnd 0.965 -> 96+1=97 < 98 attacks; Rnd 0.975 -> 97+1=98 does not.
TEST(RollFreeAttack_ChangedLevelUsesCap) {
    tsup::Env env({0.965, 0.975});
    CHECK(rollFreeAttack(env.ctx, AttackKind::ChangedLevel, 20, 0));
    CHECK(!rollFreeAttack(env.ctx, AttackKind::ChangedLevel, 20, 0));
}

// R33 quirk: the same pairing gives 100% on a walk but not on stairs.
TEST(RollFreeAttack_WalkCertainWhileStairsCapped) {
    tsup::Env walk({0.999, 0.999});
    // Moved chance 150: 99+1=100 < 150 always attacks.
    CHECK(rollFreeAttack(walk.ctx, AttackKind::Moved, 20, 0));
    CHECK(rollFreeAttack(walk.ctx, AttackKind::Moved, 20, 0));

    tsup::Env stairs({0.999, 0.999});
    // ChangedLevel chance 98: 99+1=100 < 98 is false, never attacks on that draw.
    CHECK(!rollFreeAttack(stairs.ctx, AttackKind::ChangedLevel, 20, 0));
}

// R34: with no living companions the character is always the target,
// and no draw is needed (short-circuit on alive > 0).
TEST(PickTarget_NoLivingCompanionsHitsCharacter) {
    tsup::Env env({0.9, 0.9});
    Party party{};
    Arr<Companion, kCompanionSlots> companions{};
    CHECK_EQ(pickMonsterTarget(env.ctx, party, companions), 0);
}

// R34: alive = 4, character share = 100/(4+1) = 20.
// Rnd 0.175 -> 17+1=18 < 20 -> character.
// Rnd 0.195 -> 19+1=20 not < 20 -> companion (any living slot is nonzero).
TEST(PickTarget_CharacterShareWithFourCompanions) {
    Party party{};
    Arr<Companion, kCompanionSlots> companions{};
    makeLiving(companions[0]);
    makeLiving(companions[1]);
    makeLiving(companions[2]);
    makeLiving(companions[3]);

    tsup::Env low({0.175, 0.175});
    CHECK_EQ(pickMonsterTarget(low.ctx, party, companions), 0);

    tsup::Env high({0.195, 0.195});
    CHECK(pickMonsterTarget(high.ctx, party, companions) != 0);
}

// R34 quirk: one living companion, but the pick always lands on an empty slot.
// Rnd 0.6: check 60+1=61 not < 50 -> companion branch; pick = Int(2.4)+1 = 3, empty.
// After 100 tries the character is hit.
TEST(PickTarget_FallsBackToCharacterAfterMisses) {
    Party party{};
    Arr<Companion, kCompanionSlots> companions{};
    makeLiving(companions[0]);
    tsup::Env env({0.6, 0.6});
    CHECK_EQ(pickMonsterTarget(env.ctx, party, companions), 0);
}

// R32: every swing counts as one weapon blow.
TEST(CountWeaponBlow_EachSwingCounts) {
    tsup::Env env({0.5, 0.5});
    Character ch{};
    countWeaponBlow(env.ctx, ch);
    countWeaponBlow(env.ctx, ch);
    CHECK_EQ(ch.weaponBlows, 2);
}

// R45: drawing the owner cancels the misdirection.
TEST(CancelMisdirection_DrawClearsIt) {
    tsup::Env env({0.5, 0.5});
    Character owner{};
    owner.misdirected = true;
    cancelMisdirectionOnDraw(env.ctx, owner);
    CHECK(!owner.misdirected);
}

// R45: paralysis is not lifted while companions have not finished the encounter.
TEST(FreeOwnerParalysis_NotFreedBeforeCompanionsFinish) {
    tsup::Env env({0.5, 0.5});
    Character owner{};
    owner.ailments.paralysed = true;
    freeOwnerFromParalysis(env.ctx, owner, false);
    CHECK(owner.ailments.paralysed);
}

// R38: only the first monster spell attack per group is used; it ends the attack.
TEST(GroupSpell_OnlyFirstSpellPerGroup) {
    GroupAttackFlags flags{};
    CHECK(groupMayCastSpell(flags));
    tsup::Env env({0.5, 0.5});
    recordGroupSpellAttack(env.ctx, flags);
    CHECK(flags.spellUsed);
    CHECK(flags.attackEnded);
    CHECK(!groupMayCastSpell(flags));
}

// R39: range = Int((Log(a) + a/15)/Log(1.08) - targetLevel/10 + 1), a = level+1.
// a=1: 0.866+1 = 1.866 -> 1.
// a=10: 39.581 -> 39; target 20 subtracts 2 -> 37.581 -> 37.
TEST(SpecialRange_LogFormulaFloors) {
    CHECK_EQ(specialHitRange(0, 0), 1);
    CHECK_EQ(specialHitRange(9, 0), 39);
    CHECK_EQ(specialHitRange(9, 20), 37);
}

// R40: share = range/4 for breath, range/2 for acid spit and electrocute.
TEST(SpecialShare_BreathQuarterOthersHalf) {
    CHECK_EQ(specialAttackShare(40, SpecialAttackKind::Breath), 10);
    CHECK_EQ(specialAttackShare(40, SpecialAttackKind::AcidSpit), 20);
    CHECK_EQ(specialAttackShare(40, SpecialAttackKind::Electrocute), 20);
    CHECK_EQ(specialAttackShare(4, SpecialAttackKind::Breath), 1);
    CHECK_EQ(specialAttackShare(4, SpecialAttackKind::AcidSpit), 2);
}

// R40: dmg = VbInt((Int((Rnd*range+1)/2) + share) * 0.7), range 40.
// Breath, Rnd 0.85: Int(35/2)=17; 17+10=27; 27*0.7=18.9 -> 18.
// Electrocute, Rnd 0.1: Int(5/2)=2; 2+20=22; 22*0.7=15.4 -> 15.
// Acid spit, Rnd 0.95: Int(39/2)=19; 19+20=39; 39*0.7=27.3 -> 27.
TEST(SpecialRaw_ScriptedDraws) {
    tsup::Env env({0.85, 0.1, 0.95});
    CHECK_EQ(specialAttackRawDamage(env.ctx, 40, SpecialAttackKind::Breath), 18);
    CHECK_EQ(specialAttackRawDamage(env.ctx, 40, SpecialAttackKind::Electrocute), 15);
    CHECK_EQ(specialAttackRawDamage(env.ctx, 40, SpecialAttackKind::AcidSpit), 27);
}

// R36: bonus = VbInt(3 * Log(P)^2) for P > 0, else 0. Added to defence.
// P=10: 3*5.3019 = 15.9 -> 15. P=100: 63.6 -> 63. P=2: 1.44 -> 1. P=1: 0.
TEST(ProtectionBonus_LogSquaredTimesThree) {
    CHECK_EQ(protectionBonus(10), 15);
    CHECK_EQ(protectionBonus(100), 63);
    CHECK_EQ(protectionBonus(2), 1);
    CHECK_EQ(protectionBonus(1), 0);
    CHECK_EQ(protectionBonus(0), 0);
    CHECK_EQ(protectionBonus(-5), 0);
    CHECK_EQ(defenceWithProtection(10, 10), 25);
    CHECK_EQ(defenceWithProtection(10, 0), 10);
}

// R35: damMod = DamageMod (no flags, equal sizes -> unchanged).
// Invisible target without see-invisible: -0.5. With see-invisible: no change.
TEST(MeleeModifier_BaseAndInvisibility) {
    tsup::Env env({0.5, 0.5});
    MeleeModInput in{};
    in.damageMod = 1.5;
    CHECK(approxEq(meleeDamageModifier(env.ctx, in), 1.5));

    in.targetInvisible = true;
    CHECK(approxEq(meleeDamageModifier(env.ctx, in), 1.0));

    in.attackerSeesInvisible = true;
    CHECK(approxEq(meleeDamageModifier(env.ctx, in), 1.5));
}

// R35: opening chest +0.75; size: target smaller than monster +diff/10,
// target larger -diff/10. Diff 2 gives 0.2 steps.
TEST(MeleeModifier_OpeningChestAndSize) {
    tsup::Env env({0.5, 0.5});

    MeleeModInput chest{};
    chest.damageMod = 1.0;
    chest.targetOpeningChest = true;
    CHECK(approxEq(meleeDamageModifier(env.ctx, chest), 1.75));

    MeleeModInput smaller{};
    smaller.damageMod = 1.0;
    smaller.targetSize = 1;
    smaller.monsterSize = 3;
    CHECK(approxEq(meleeDamageModifier(env.ctx, smaller), 1.2));

    MeleeModInput larger{};
    larger.damageMod = 2.0;
    larger.targetSize = 4;
    larger.monsterSize = 1;
    CHECK(approxEq(meleeDamageModifier(env.ctx, larger), 1.7));

    MeleeModInput combo{};
    combo.damageMod = 1.0;
    combo.targetInvisible = true;
    combo.targetOpeningChest = true;
    CHECK(approxEq(meleeDamageModifier(env.ctx, combo), 1.25));
}

// R48: chest is available only in its own area; no encounter source or
// Area <= 0 means it belongs wherever you stand.
TEST(ChestAvailable_OnlyInItsArea) {
    CHECK(chestAvailableHere(true, 3, 3));
    CHECK(!chestAvailableHere(true, 3, 2));
    CHECK(chestAvailableHere(false, 3, 2));
    CHECK(chestAvailableHere(true, 0, 2));
    CHECK(chestAvailableHere(true, -1, 2));
}

// Archive R51: both zero gives 50 (agreed by all forms).
TEST(ArchiveHit_ZeroZeroIsFifty) {
    CHECK(approxEq(archiveHitChancePercent(0, 0), 50.0));
}

// R46 step 3: gold is split across the party; exact division only.
TEST(SplitFightGold_ExactDivision) {
    CHECK_EQ(splitFightGold(120000, 4), 30000);
    CHECK_EQ(splitFightGold(50000, 1), 50000);
}

// R44: chance = clamp((999/max(1,M)) * (bodies/20) * factor, lo, hi).
// Breath (2, 2, 40). Inputs chosen so integer and float division agree.
// M=999, bodies=20: 1*1*2 = 2. M=999, bodies=40: 1*2*2 = 4.
// M=1: 999*2 clamps to 40. M=2000: 0.4995*2 clamps to 2.
TEST(MishapChance_BreathClamp) {
    CHECK(approxEq(mishapChance(MishapSource::Breath, 999, 20), 2.0));
    CHECK(approxEq(mishapChance(MishapSource::Breath, 999, 40), 4.0));
    CHECK(approxEq(mishapChance(MishapSource::Breath, 1, 20), 40.0));
    CHECK(approxEq(mishapChance(MishapSource::Breath, 2000, 20), 2.0));
}

// R44: spit/melee (1, 0.5, 20), spells (1.5, 1, 40).
// M=999, bodies=20: spit 1.0; spell 1.5. M=1: spit clamps to 20; spell to 40.
// M=2000: spit 0.4995 clamps to 0.5; spell 0.749 clamps to 1.
TEST(MishapChance_SpitAndSpellClamp) {
    CHECK(approxEq(mishapChance(MishapSource::SpitOrMelee, 999, 20), 1.0));
    CHECK(approxEq(mishapChance(MishapSource::SpitOrMelee, 1, 20), 20.0));
    CHECK(approxEq(mishapChance(MishapSource::SpitOrMelee, 2000, 20), 0.5));
    CHECK(approxEq(mishapChance(MishapSource::Spell, 999, 20), 1.5));
    CHECK(approxEq(mishapChance(MishapSource::Spell, 1, 20), 40.0));
    CHECK(approxEq(mishapChance(MishapSource::Spell, 2000, 20), 1.0));
}
