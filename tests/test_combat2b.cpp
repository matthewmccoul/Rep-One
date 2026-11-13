#include <cmath>

#include "test_support.h"
#include "mordor/combat2b.h"

using mordor::Character;
using mordor::CombatProc;
using mordor::Guild;
using mordor::PlayerBlowInput;
using mordor::PlayerBlowRolls;
using mordor::PlayerChanceParams;

namespace {

bool approx(double a, double b, double tol) { return std::fabs(a - b) <= tol; }

void setLevel(Character& c, Guild g, int lvl) {
    c.guilds[static_cast<unsigned>(g)].level = lvl;
}

PlayerChanceParams makeParams(int gl, int coef, int mon, bool weapon, int floorF) {
    PlayerChanceParams p;
    p.guildLevel = gl;
    p.coefficient = coef;
    p.monLevel = mon;
    p.weaponHasAbility = weapon;
    p.weaponFloor = floorF;
    return p;
}

}  // namespace

// R10 / constants from the header and SRD tables.
TEST(combat2b_proc_rule_constants) {
    CHECK_EQ(mordor::kCritRule.divisor, 3);
    CHECK_EQ(mordor::kCritRule.weaponLog, 10);
    CHECK_EQ(mordor::kCritRule.floor, 1);
    CHECK_EQ(mordor::kCritRule.cap, 50);
    CHECK_EQ(mordor::kCritRule.increment, 5);
    CHECK_EQ(mordor::kCritRule.effectBit, 4);
    CHECK_EQ(mordor::kBackstabRule.divisor, 2);
    CHECK_EQ(mordor::kBackstabRule.weaponLog, 100);
    CHECK_EQ(mordor::kBackstabRule.floor, 2);
    CHECK_EQ(mordor::kBackstabRule.cap, 75);
    CHECK_EQ(mordor::kBackstabRule.increment, 2);
    CHECK_EQ(mordor::kBackstabRule.effectBit, 5);
    CHECK_EQ(mordor::kMonsterCritDivisor, 650);
    CHECK_EQ(mordor::kMonsterBackstabDivisor, 500);
    CHECK_EQ(mordor::kMonsterCritFloor, 2);
    CHECK_EQ(mordor::kMonsterCritCap, 50);
    CHECK_EQ(mordor::kMonsterBackstabFloor, 4);
    CHECK_EQ(mordor::kMonsterBackstabCap, 75);
    CHECK_EQ(mordor::kCritCoefficients[0].coefficient, 9);
    CHECK_EQ(mordor::kCritCoefficients[1].coefficient, 3);
    CHECK_EQ(mordor::kBackstabCoefficients[0].coefficient, 9);
    CHECK_EQ(mordor::kBackstabCoefficients[1].coefficient, 2);
}

