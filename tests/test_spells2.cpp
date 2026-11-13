#include "test_support.h"
#include "mordor/spells2.h"

#include <cmath>
#include <cstring>
#include <memory>

using namespace mordor;

namespace {

SpellRow makeRow(bool baseBlank, i64 base, i64 power, bool hpFlag) {
    SpellRow row{};
    row.index = 0;
    row.type = SpellEffectType::DETRIMENTAL;
    row.baseBlank = baseBlank;
    row.base = base;
    row.power = power;
    row.hpFlag = hpFlag;
    return row;
}

MagnitudeResult makeMag(bool blank, double dmg, double heal) {
    MagnitudeResult m{};
    m.blank = blank;
    m.dmgMod = dmg;
    m.healMod = heal;
    return m;
}

bool nearly(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

}  // namespace

// Constants transcribed from the SRD and header comments.
TEST(ConstantsMatchSrd) {
    CHECK(nearly(kLnDamageBase, 5.0, 1e-12));
    CHECK(nearly(kLnHealBase, 10.0, 1e-12));
    CHECK(nearly(kArchiveDamageDivisor, 1.61, 1e-12));
    CHECK(nearly(kArchiveHealDivisor, 2.3, 1e-12));
    CHECK(nearly(kSheetFactor, 1.9, 1e-12));
    CHECK(nearly(kSheetEpsilon, 0.0001, 1e-12));
    CHECK_EQ(kResistGrantPercent, 95);
    // R10/R21 style: Charm casts are verb CHARMED.
    CHECK(kCharmCastPlan.verb == SpellVerb::CHARMED);
}

// Table E (spreadsheet per-spell constants C, Q, R).
TEST(SheetSpellTableMatchesSrdE) {
    CHECK_EQ(static_cast<int>(sizeof(kSheetSpellConsts) / sizeof(kSheetSpellConsts[0])), 11);
    CHECK(std::strcmp(kSheetSpellConsts[0].spell, "Dominate Devil") == 0);
    CHECK(nearly(kSheetSpellConsts[0].c, 8.0, 1e-12));
    CHECK(nearly(kSheetSpellConsts[0].q, 1.0, 1e-12));
    CHECK(nearly(kSheetSpellConsts[0].r, 62.0, 1e-12));
    CHECK(std::strcmp(kSheetSpellConsts[4].spell, "Cure Disease") == 0);
    CHECK(nearly(kSheetSpellConsts[4].c, 10.0, 1e-12));
    CHECK(nearly(kSheetSpellConsts[4].q, 2.0, 1e-12));
    CHECK(nearly(kSheetSpellConsts[4].r, 23.0, 1e-12));
    CHECK(std::strcmp(kSheetSpellConsts[10].spell, "Field of Death") == 0);
    CHECK(nearly(kSheetSpellConsts[10].c, 14.0, 1e-12));
    CHECK(nearly(kSheetSpellConsts[10].r, 120.0, 1e-12));
}

// DmgMod power term is INT((SL/10)*(H-1)), a floor. The log term is shared
// by both calls, so comparing differences avoids the LN(SL) vs LN(SL+1) question.
TEST(DamagePowerTermIsFlooredInt) {
    // INT(0.5*3) = INT(1.5) = 1
    CHECK(nearly(damageMod(10, 4, 5) - damageMod(10, 1, 5), 1.0, 1e-9));
    // INT(2.5*10) = 25
    CHECK(nearly(damageMod(10, 11, 25) - damageMod(10, 1, 25), 25.0, 1e-9));
    // INT(1.5*3) = INT(4.5) = 4
    CHECK(nearly(damageMod(10, 4, 15) - damageMod(10, 1, 15), 4.0, 1e-9));
}

// HealMod at SL 1: LN(1) = 0, leaving Base plus the floored power term.
TEST(HealMagnitudeAtSlOne) {
    CHECK(nearly(healMod(10, 1, 1), 10.0, 1e-9));
    // INT(0.1*10) = 1
    CHECK(nearly(healMod(10, 11, 1), 11.0, 1e-9));
    // INT(0.1*3) = INT(0.3) = 0
    CHECK(nearly(healMod(10, 4, 1), 10.0, 1e-9));
}

// HealMod uses log base 10: LN(100)/LN(10) = 2.
TEST(HealLogUsesBaseTen) {
    // J = 10 + 10*2 + INT(10*0) = 30
    CHECK(nearly(healMod(10, 1, 100), 30.0, 1e-9));
    // J = 10 + 20 + INT(10*10) = 130
    CHECK(nearly(healMod(10, 11, 100), 130.0, 1e-9));
}

// R3: blank Base gives a blank magnitude; otherwise magnitudes match the formulas.
TEST(SpellMagnitudeBlankBaseIsBlank) {
    MagnitudeResult blank = spellMagnitude(makeRow(true, 10, 1, false), 25);
    CHECK(blank.blank);

    MagnitudeResult live = spellMagnitude(makeRow(false, 10, 11, false), 25);
    CHECK(!live.blank);
    CHECK(nearly(live.dmgMod, damageMod(10, 11, 25), 1e-9));
    CHECK(nearly(live.healMod, healMod(10, 11, 25), 1e-9));
}

// R4 worked case with DmgMod = HealMod = 30:
// min = ROUNDDOWN(30*0.5-1) = 14, max = ROUNDDOWN(30*1-1) = 29.
TEST(RangeWorkedExample) {
    MagnitudeResult m = makeMag(false, 30.0, 30.0);

    ValueRange d = damageRange(SpellEffectType::DETRIMENTAL, m);
    CHECK(d.present);
    CHECK_EQ(d.min, 14);
    CHECK_EQ(d.max, 29);

    ValueRange h = healRange(SpellEffectType::BENEFICIAL, m);
    CHECK(h.present);
    CHECK_EQ(h.min, 14);
    CHECK_EQ(h.max, 29);
}

// R4 ROUNDDOWN is toward zero, so a negative intermediate becomes 0, not -1.
TEST(RangeRoundsTowardZero) {
    // DmgMod 3.0: min = ROUNDDOWN(0.5) = 0, max = ROUNDDOWN(2.0) = 2
    ValueRange a = damageRange(SpellEffectType::DETRIMENTAL, makeMag(false, 3.0, 0.0));
    CHECK(a.present);
    CHECK_EQ(a.min, 0);
    CHECK_EQ(a.max, 2);

    // DmgMod 1.5: min = ROUNDDOWN(-0.25) = 0 (toward zero), max = ROUNDDOWN(0.5) = 0
    ValueRange b = damageRange(SpellEffectType::DETRIMENTAL, makeMag(false, 1.5, 0.0));
    CHECK(b.present);
    CHECK_EQ(b.min, 0);
    CHECK_EQ(b.max, 0);
}

// R4/R3: a range exists only for the matching type, and never for a blank magnitude.
TEST(RangeGatedByTypeAndBlank) {
    MagnitudeResult m = makeMag(false, 30.0, 30.0);
    CHECK(!damageRange(SpellEffectType::BENEFICIAL, m).present);
    CHECK(!healRange(SpellEffectType::DETRIMENTAL, m).present);
    CHECK(!damageRange(SpellEffectType::BLANK, m).present);

    MagnitudeResult blank = makeMag(true, 30.0, 30.0);
    CHECK(!damageRange(SpellEffectType::DETRIMENTAL, blank).present);
    CHECK(!healRange(SpellEffectType::BENEFICIAL, blank).present);
}

// R5: the HP note needs a damage range and the flag "Y". It is text only.
TEST(HpNoteNeedsRangeAndFlag) {
    ValueRange present{};
    present.present = true;
    present.min = 14;
    present.max = 29;
    ValueRange absent{};

    CHECK(hpNoteShown(present, makeRow(false, 10, 1, true)));
    CHECK(!hpNoteShown(present, makeRow(false, 10, 1, false)));
    CHECK(!hpNoteShown(absent, makeRow(false, 10, 1, true)));
}

// R6: D = floor(Q*R*1.9 - 0.0001). Non-boundary inputs only.
TEST(SheetBaseValueFloorsQRTimes19) {
    CHECK_EQ(sheetBaseValue(1.0, 62.0), 117);  // 117.8
    CHECK_EQ(sheetBaseValue(2.0, 23.0), 87);   // 87.4
    CHECK_EQ(sheetBaseValue(1.0, 76.0), 144);  // 144.4
}

// R6: X = floor(C*Q*R*1.9/SL - 0.0001); E = C when X < C, else X.
TEST(SheetScaledBaseFloorsAndClampsToC) {
    // C*Q*R*1.9 = 8*1*62*1.9 = 942.4
    CHECK_EQ(sheetScaledBase(8.0, 1.0, 62.0, 20), 47);   // 47.12
    CHECK_EQ(sheetScaledBase(8.0, 1.0, 62.0, 100), 9);   // 9.424
    CHECK_EQ(sheetScaledBase(8.0, 1.0, 62.0, 200), 8);   // 4.712 < 8, so E = C
}

// R10 archive minimum: (Base + Base*LN(SL)/1.61 + Int(SL*(Power-1)/10)) / 2.
// Differences cancel the log term, so only the floored power term is tested.
TEST(ArchiveDamagePowerTermHalved) {
    // (25 - 0) / 2 = 12.5
    CHECK(nearly(archiveDamageMinimum(10, 11, 25) - archiveDamageMinimum(10, 1, 25), 12.5, 1e-9));
    // Int(1.5) = 1, so /2 = 0.5
    CHECK(nearly(archiveDamageMinimum(10, 4, 5) - archiveDamageMinimum(10, 1, 5), 0.5, 1e-9));
}

// R13 archive heal minimum at SL 1: LN(1) = 0.
TEST(ArchiveHealMinimumAtSlOne) {
    CHECK(nearly(archiveHealMinimum(10, 1, 1), 5.0, 1e-9));   // (10 + 0 + 0) / 2
    CHECK(nearly(archiveHealMinimum(10, 11, 1), 5.5, 1e-9));  // (10 + 1) / 2
    CHECK(nearly(archiveHealMinimum(10, 4, 1), 5.0, 1e-9));   // Int(0.3) = 0
}

// R13 quirk: archive heal divisor is 2.3, not LN(10).
// (10 + 10*LN(100)/2.3) / 2 = 15.01123953..., while the sheet's LN(10) gives 15.0.
TEST(ArchiveHealUsesDivisorTwoPointThree) {
    double v = archiveHealMinimum(10, 1, 100);
    CHECK(nearly(v, 15.01123953, 1e-6));
    CHECK(std::fabs(v - 15.0) > 1e-3);
}

// R10/R13: roll is Minimum + uniform integer in 1..Minimum.
// Scripted 0.0 gives roll 1; scripted 0.999 gives roll Minimum.
TEST(ArchiveRollsUniformOneToMinimum) {
    tsup::Env env({0.0, 0.999});
    CHECK_EQ(archiveDamageRoll(env.ctx, 5.0), 6);   // 5 + 1
    CHECK_EQ(archiveDamageRoll(env.ctx, 5.0), 10);  // 5 + 5

    tsup::Env henv({0.0});
    CHECK_EQ(archiveHealRoll(henv.ctx, 4.0), 5);    // 4 + 1
}

// Table C, Firebolt SL 15 raw 26: resist 25% -> 20 (19.5 rounds half to even to 20),
// resist 35% -> 17 (16.9 rounds to 17; truncation would give 16), 0% -> 26.
// 10 at 75% is 2.5, which rounds half to even to 2.
TEST(ResistedDamageRoundsHalfToEven) {
    CHECK_EQ(resistedDamage(26, 25), 20);
    CHECK_EQ(resistedDamage(26, 35), 17);
    CHECK_EQ(resistedDamage(26, 0), 26);
    CHECK_EQ(resistedDamage(10, 75), 2);
}

// Protection: LN(1) = 0, so both protection formulas give 0 under any reading of LN(SL)^2.
TEST(ProtectionDefenceZeroAtSlOne) {
    CHECK(nearly(itemProtectionDefence(1), 0.0, 1e-9));
    CHECK(nearly(guildSpellProtectionDefence(1), 0.0, 1e-9));
}

// R21: a resist spell raises the matching resistance to 95, not by +95.
// Starting at 30 everywhere: one entry becomes 95 and the other ten stay 30.
TEST(CastResistSetsMatchingTo95) {
    tsup::Env env({0.5});
    Resistances res{};
    for (u32 i = 0; i < static_cast<u32>(kResistKinds); ++i) {
        res.resist[i] = 30;
    }
    castResistSpell(env.ctx, res, SpellResistKind::FIRE);

    i32 count95 = 0;
    i32 sum = 0;
    for (u32 i = 0; i < static_cast<u32>(kResistKinds); ++i) {
        sum += res.resist[i];
        if (res.resist[i] == 95) {
            ++count95;
        }
    }
    CHECK_EQ(count95, 1);
    CHECK_EQ(sum, 95 + 30 * 10);
}

// R19: Set Sanctuary stores the caster's location.
TEST(SetSanctuaryStoresCasterLocation) {
    tsup::Env env({0.5});
    Location at{};
    at.x = 12;
    at.y = 7;
    at.dungeonLevel = 3;

    SanctuaryStore store{};
    setSanctuary(env.ctx, store, at);
    CHECK_EQ(store.x, 12);
    CHECK_EQ(store.y, 7);
    CHECK_EQ(store.dungeonLevel, 3);
}

// R19: casting Sanctuary ages the caster about one year (365 days, per the port's current value).
// Either ageDays or daysAged may carry the aging, so the combined delta is checked.
TEST(CastSanctuaryAgesCasterAYear) {
    tsup::Env env({0.5});
    std::unique_ptr<Party> party(new Party());
    party->size = 1;
    party->members[0].ageDays = 1000;
    party->members[0].daysAged = 0;

    Location at{};
    at.x = 1;
    at.y = 2;
    at.dungeonLevel = 1;
    SanctuaryStore store{};
    setSanctuary(env.ctx, store, at);

    (void)castSanctuary(env.ctx, *party, 0, store);
    i32 aged = (party->members[0].ageDays - 1000) + party->members[0].daysAged;
    CHECK_EQ(aged, 365);
}

// R17: bind does not work on companions who joined freely, so the bind level is unchanged.
TEST(BindLeavesFreeJoinerUnchanged) {
    tsup::Env env({0.0});
    Companion c{};
    c.joinedFreely = true;
    c.bindingLevel = -10;

    (void)bindCompanion(env.ctx, c, 5);
    CHECK_EQ(c.bindingLevel, -10);
}

// R14: Cure Paralysis is labelled COMBAT in the spell data.
TEST(CureParalysisIsCombatSpell) {
    tsup::Env env({0.5});
    SpellRow row = spellRowByName(env.ctx, Str("Cure Paralysis"));
    CHECK(spellUse(env.ctx, row.index) == SpellUse::Combat);
}
