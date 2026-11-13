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

