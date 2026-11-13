#include "test_support.h"
#include "mordor/items3a.h"

#include <cstring>

using namespace mordor;
using tsup::Env;

static bool sameText(const char* a, const char* b) {
    return std::strcmp(a, b) == 0;
}

// ---- Store fee: round_half_even(0.8 * Price / 0.5) = round_half_even(1.6 * Price)

// ---- Shelf counts: Align(0..3) = counts for {U, G, N, E}

// ---- Floor price line: only floor 0 with unaligned stock shows list price

// ---- Free will: BindingLevel < 0 cannot be bound

TEST(isFreeWillOnlyWhenBindingLevelIsNegative) {
    Companion c;             // default bindingLevel is -10
    CHECK(isFreeWill(c));
    c.bindingLevel = -1;
    CHECK(isFreeWill(c));
    c.bindingLevel = 0;
    CHECK(!isFreeWill(c));
    c.bindingLevel = 1;
    CHECK(!isFreeWill(c));
}

// ---- bindingTarget: min(level + step, kBindingMax), kBindingMax = 100

// ---- Stat items: need 10 Wisdom and 10 Intelligence

// ---- payFromPurse: hand first, rest from bank; refusal if hand + bank < amount

// ---- drawTheftSlot: Int(Rnd * 39) + 2, so slots 2..40; slot 40 is one past the pack

// ---- Guild crest tables (crest rows indexed by Guild)

TEST(crestPaladinRowMatchesSrdTable) {
    const i32 p = static_cast<i32>(Guild::Paladin);
    const i32 expectMods[kStatCount] = {2, 0, 0, 1, 2, 1};
    CHECK_EQ(kCrestAttack[p], 6);
    CHECK_EQ(kCrestDefence[p], 12);
    CHECK_EQ(kCrestAbilityMask[p], 4u);
    for (i32 s = 0; s < kStatCount; ++s) {
        CHECK_EQ(kCrestMods[p][s], expectMods[s]);
    }
    CHECK(sameText(kCrestSpellName[p], "Minor Heal"));
}

TEST(crestMageAndWizardRowsMatchSrdTable) {
    const i32 m = static_cast<i32>(Guild::Mage);
    const i32 mageMods[kStatCount] = {0, 2, 2, 0, 2, 0};
    CHECK_EQ(kCrestAttack[m], 0);              // attack 0 is a real value here
    CHECK_EQ(kCrestDefence[m], 12);
    CHECK_EQ(kCrestAbilityMask[m], 0xAu);
    for (i32 s = 0; s < kStatCount; ++s) {
        CHECK_EQ(kCrestMods[m][s], mageMods[s]);
    }
    CHECK(sameText(kCrestSpellName[m], "Soul Entrapment"));

    const i32 w = static_cast<i32>(Guild::Wizard);
    const i32 wizardMods[kStatCount] = {0, 4, 2, 0, 0, 0};
    CHECK_EQ(kCrestAttack[w], 3);
    CHECK_EQ(kCrestDefence[w], 12);
    CHECK_EQ(kCrestAbilityMask[w], 0xEu);
    for (i32 s = 0; s < kStatCount; ++s) {
        CHECK_EQ(kCrestMods[w][s], wizardMods[s]);
    }
    CHECK(sameText(kCrestSpellName[w], "Power"));
}

TEST(crestNinjaHasHighestDefenceAndNoSpell) {
    const i32 n = static_cast<i32>(Guild::Ninja);
    CHECK_EQ(kCrestDefence[n], 18);
    for (i32 g = 0; g < kGuildCount; ++g) {
        CHECK(kCrestDefence[g] <= 18);
    }
    CHECK(sameText(kCrestSpellName[n], ""));
}

TEST(crestSpellNamesBlankForNomadWarriorNinjaAndShareCharm) {
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Nomad)], ""));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Warrior)], ""));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Ninja)], ""));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Thief)], "Charm of Opening"));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Scavenger)], "Charm of Opening"));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Healer)], "Restoration"));
    CHECK(sameText(kCrestSpellName[static_cast<i32>(Guild::Seeker)], "Displacement"));
}

// ---- bindCompanion: refusals leave state and purse untouched

// ---- Item helpers: guild marker boundary and no-spell index

TEST(guildItemMarkerBoundaryAtFiveHundred) {
    Item it;
    it.ailments = 500;
    CHECK(it.isGuildItem());
    CHECK_EQ(it.guildIndex(), 0);

    it.ailments = 499;
    CHECK(!it.isGuildItem());

    it.ailments = 503;
    CHECK(it.isGuildItem());
    CHECK_EQ(it.guildIndex(), 3);
}

// ---- Alignment bitmask constants (wiki 0x28 field)

TEST(alignmentBitConstantsMatchWikiMask) {
    CHECK_EQ(static_cast<int>(kAlignBitUnaligned), 0x01);
    CHECK_EQ(static_cast<int>(kAlignBitGood), 0x02);
    CHECK_EQ(static_cast<int>(kAlignBitNeutral), 0x04);
    CHECK_EQ(static_cast<int>(kAlignBitEvil), 0x08);
    const int all = kAlignBitUnaligned | kAlignBitGood | kAlignBitNeutral | kAlignBitEvil;
    CHECK_EQ(all, 0x0F);
}
