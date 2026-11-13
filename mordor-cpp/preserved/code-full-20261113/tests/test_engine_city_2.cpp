#include "test_support.h"
#include "mordor/city1.h"

using namespace mordor;

// Pen alignment indices per city1.h (kPenAlignCount): 0 = Good, 1 = Neutral, 2 = Evil.
namespace {

constexpr i32 kPenGood = 0;
constexpr i32 kPenNeutral = 1;
constexpr i32 kPenEvil = 2;

// Table indices used by the reference cases.
constexpr i32 kTestMorgulRef = 278;        // kMorgulMonsterIndex
constexpr i32 kTestShadowNinjaRef = 389;   // kShadowNinjaMonsterIndex
constexpr i32 kTestGenericRef = 100;       // arbitrary ref for generic cases
constexpr i32 kTestMorlochRef = 1;         // arbitrary ref for the Morloch companions

void padTo(PenBook& pens, i32 ref) {
    while (static_cast<i32>(pens.byRef.size()) <= ref) {
        pens.byRef.push_back(PenCounts{});
    }
}

// Confinement "Add": one penned companion of this ref at this alignment.
void addPen(PenBook& pens, i32 ref, i32 align, i32 levelFound) {
    padTo(pens, ref);
    static_cast<void>(penOne(pens, ref, align, levelFound));
}

// Confinement "Take": one companion of this ref at this alignment leaves its pen.
void takePen(PenBook& pens, i32 ref, i32 align) {
    padTo(pens, ref);
    releaseOne(pens, ref, align);
}

const PenCounts& rowOf(const PenBook& pens, i32 ref) {
    return pens.byRef[static_cast<u32>(ref)];
}

}  // namespace

// B_0055#11: Take(creature, Neutral) done once after two Neutral companions.
TEST(engine_B_0055_11_take_once_neutral_count) {
    PenBook pens{};
    addPen(pens, kTestGenericRef, kPenNeutral, 1);
    addPen(pens, kTestGenericRef, kPenNeutral, 1);
    takePen(pens, kTestGenericRef, kPenNeutral);
    const PenCounts& row = rowOf(pens, kTestGenericRef);
    CHECK_EQ(row[static_cast<u32>(kPenNeutral)], 1);
    CHECK_EQ(row[static_cast<u32>(kPenGood)], 0);
}

// B_0055#22: two Neutral Morloch companions sold into the pen.
TEST(engine_B_0055_22_two_morloch_neutral_row) {
    PenBook pens{};
    addPen(pens, kTestMorlochRef, kPenNeutral, 1);
    addPen(pens, kTestMorlochRef, kPenNeutral, 1);
    const PenCounts& row = rowOf(pens, kTestMorlochRef);
    CHECK_EQ(row[static_cast<u32>(kPenGood)], 0);
    CHECK_EQ(row[static_cast<u32>(kPenNeutral)], 2);
    CHECK_EQ(row[static_cast<u32>(kPenEvil)], 0);
}

// B_0055#45: Morgul gets one Neutral; Shadow Ninja gets one Evil and one Neutral.
TEST(engine_B_0055_45_morgul_and_shadow_ninja_rows) {
    PenBook pens{};
    addPen(pens, kTestMorgulRef, kPenNeutral, 1);
    addPen(pens, kTestShadowNinjaRef, kPenEvil, 1);
    addPen(pens, kTestShadowNinjaRef, kPenNeutral, 1);

    const PenCounts& morgul = rowOf(pens, kTestMorgulRef);
    CHECK_EQ(morgul[static_cast<u32>(kPenGood)], 0);
    CHECK_EQ(morgul[static_cast<u32>(kPenNeutral)], 1);
    CHECK_EQ(morgul[static_cast<u32>(kPenEvil)], 0);

    const PenCounts& ninja = rowOf(pens, kTestShadowNinjaRef);
    CHECK_EQ(ninja[static_cast<u32>(kPenGood)], 0);
    CHECK_EQ(ninja[static_cast<u32>(kPenNeutral)], 1);
    CHECK_EQ(ninja[static_cast<u32>(kPenEvil)], 1);
}

// B_0055#49: Shadow Ninja Neutral bought out of its Neutral pen.
TEST(engine_B_0055_49_buy_out_shadow_ninja_neutral) {
    PenBook pens{};
    addPen(pens, kTestMorgulRef, kPenNeutral, 1);
    addPen(pens, kTestShadowNinjaRef, kPenEvil, 1);
    addPen(pens, kTestShadowNinjaRef, kPenNeutral, 1);
    takePen(pens, kTestShadowNinjaRef, kPenNeutral);

    const PenCounts& morgul = rowOf(pens, kTestMorgulRef);
    CHECK_EQ(morgul[static_cast<u32>(kPenGood)], 0);
    CHECK_EQ(morgul[static_cast<u32>(kPenNeutral)], 1);
    CHECK_EQ(morgul[static_cast<u32>(kPenEvil)], 0);

    const PenCounts& ninja = rowOf(pens, kTestShadowNinjaRef);
    CHECK_EQ(ninja[static_cast<u32>(kPenGood)], 0);
    CHECK_EQ(ninja[static_cast<u32>(kPenNeutral)], 0);
    CHECK_EQ(ninja[static_cast<u32>(kPenEvil)], 1);
    // The bought companion's name ("Shadow Ninja") is not stored in PenBook; not asserted here.
}

// B_0055#50: the bought Shadow Ninja (Neutral) is sold back into its Neutral pen.
TEST(engine_B_0055_50_resell_shadow_ninja_neutral) {
    PenBook pens{};
    addPen(pens, kTestMorgulRef, kPenNeutral, 1);
    addPen(pens, kTestShadowNinjaRef, kPenEvil, 1);
    addPen(pens, kTestShadowNinjaRef, kPenNeutral, 1);
    takePen(pens, kTestShadowNinjaRef, kPenNeutral);
    addPen(pens, kTestShadowNinjaRef, kPenNeutral, 1);

    const PenCounts& morgul = rowOf(pens, kTestMorgulRef);
    CHECK_EQ(morgul[static_cast<u32>(kPenGood)], 0);
    CHECK_EQ(morgul[static_cast<u32>(kPenNeutral)], 1);
    CHECK_EQ(morgul[static_cast<u32>(kPenEvil)], 0);

    const PenCounts& ninja = rowOf(pens, kTestShadowNinjaRef);
    CHECK_EQ(ninja[static_cast<u32>(kPenGood)], 0);
    CHECK_EQ(ninja[static_cast<u32>(kPenNeutral)], 1);
    CHECK_EQ(ninja[static_cast<u32>(kPenEvil)], 1);
}

// SKIP B_0055#23: Confinement keyed by table index with alignment values 2, 9, 3 needs a
//   "repair" step that has no equivalent function in the headers.
// SKIP B_0055#25: Confinement.Count after the repair; the repair step has no equivalent function.
// SKIP B_0055#47: Morgul BaseValue is not given and creatureBaseWorth needs MonsterDef data;
//   the PriceToBuy formula is not a direct header function.
// SKIP B_0055#48: hand-made monster with no table index; PenBook is keyed by ref and no
//   equivalent of pen key counting exists.
// SKIP B_0055#77: Party.BankTyped has no equivalent function in the headers.
