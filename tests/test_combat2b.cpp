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

// R10: crit +5, backstab +2, severed adds both.
TEST(combat2b_blow_damage_modifier_increments) {
    CHECK_EQ(mordor::blowDamageModifier(false, false), 0);
    CHECK_EQ(mordor::blowDamageModifier(true, false), 5);
    CHECK_EQ(mordor::blowDamageModifier(false, true), 2);
    CHECK_EQ(mordor::blowDamageModifier(true, true), 7);
}

// Player formula with no guild and no weapon ability: p = 0, no floor applies.
TEST(combat2b_player_chance_zero_without_ability) {
    PlayerChanceParams p = makeParams(0, 9, 0, false, 0);
    CHECK(approx(mordor::playerProcChance(p, mordor::kCritRule), 0.0, 1e-9));
    CHECK(approx(mordor::playerProcChance(p, mordor::kBackstabRule), 0.0, 1e-9));
}

// Crit, GLvl 999, CritMod 9, divisor 3:
// (100*ln2*ln10)/3/2 = 159.60304/6 = 26.60051.
TEST(combat2b_player_crit_level_999) {
    PlayerChanceParams p = makeParams(999, 9, 0, false, 0);
    CHECK(approx(mordor::playerProcChance(p, mordor::kCritRule), 26.6005, 0.001));
}

// Backstab uses divisor 2 (engine): 159.60304/2/2 = 39.90076.
TEST(combat2b_player_backstab_divisor_two) {
    PlayerChanceParams p = makeParams(999, 9, 0, false, 0);
    CHECK(approx(mordor::playerProcChance(p, mordor::kBackstabRule), 39.9008, 0.001));
}

// Caps: crit 50, backstab 75, reached with huge guild level.
TEST(combat2b_player_caps) {
    PlayerChanceParams p = makeParams(999999, 9, 0, false, 0);
    CHECK(approx(mordor::playerProcChance(p, mordor::kCritRule), 50.0, 1e-9));
    CHECK(approx(mordor::playerProcChance(p, mordor::kBackstabRule), 75.0, 1e-9));
}

// Floors: applies when guildLevel > 0 or weapon has the ability.
TEST(combat2b_player_floors) {
    PlayerChanceParams guildOnly = makeParams(1, 0, 0, false, 0);
    CHECK(approx(mordor::playerProcChance(guildOnly, mordor::kCritRule), 1.0, 1e-9));
    CHECK(approx(mordor::playerProcChance(guildOnly, mordor::kBackstabRule), 2.0, 1e-9));

    PlayerChanceParams weaponOnly = makeParams(0, 0, 0, true, 0);
    CHECK(approx(mordor::playerProcChance(weaponOnly, mordor::kCritRule), 1.0, 1e-9));
    CHECK(approx(mordor::playerProcChance(weaponOnly, mordor::kBackstabRule), 2.0, 1e-9));
}

// Monster level subtraction drives p negative; floor then applies (GLvl > 0).
TEST(combat2b_player_monster_level_floor) {
    PlayerChanceParams p = makeParams(999, 9, 999, false, 0);
    CHECK(approx(mordor::playerProcChance(p, mordor::kCritRule), 1.0, 1e-9));
}

// Weapon term: w = ln(F+1)*ln(weaponLog).
// F=99, crit: w = ln100*ln10 = 10.60380. With GLvl 0, p = max(0+w, w) = w.
// With GLvl 999: p = 26.60051 + 10.60380 = 37.20431.
// Backstab F=99: w = ln100*ln100 = 21.20759 (below cap 75).
TEST(combat2b_player_weapon_term_max_rule) {
    PlayerChanceParams noGuild = makeParams(0, 0, 0, true, 99);
    CHECK(approx(mordor::playerProcChance(noGuild, mordor::kCritRule), 10.6038, 0.001));
    CHECK(approx(mordor::playerProcChance(noGuild, mordor::kBackstabRule), 21.2076, 0.001));

    PlayerChanceParams withGuild = makeParams(999, 9, 0, true, 99);
    CHECK(approx(mordor::playerProcChance(withGuild, mordor::kCritRule), 37.2043, 0.001));
}

// Player form: integer draw k = Int(Rnd*100), wins if k < chance.
TEST(combat2b_roll_player_proc_boundaries) {
    tsup::Env env({0.0, 0.999});
    CHECK(!mordor::rollPlayerProc(env.ctx, 0.0));
    CHECK(mordor::rollPlayerProc(env.ctx, 100.0));
}

// Chance 5.4 wins for k = 0..5: six of the hundred draws.
TEST(combat2b_roll_player_proc_fractional_six_wins) {
    int wins = 0;
    for (int i = 0; i < 100; ++i) {
        tsup::Env env({(i + 0.5) / 100.0, 0.9});
        if (mordor::rollPlayerProc(env.ctx, 5.4)) ++wins;
    }
    CHECK_EQ(wins, 6);
}

// Monster form: Int(Rnd*100)+1 <= chance. Chance 5.4 wins for k = 0..4: five wins.
TEST(combat2b_roll_monster_proc_fractional_five_wins) {
    int wins = 0;
    for (int i = 0; i < 100; ++i) {
        tsup::Env env({(i + 0.5) / 100.0, 0.9});
        if (mordor::rollMonsterProc(env.ctx, 5.4)) ++wins;
    }
    CHECK_EQ(wins, 5);
}

// Monster clamps: p = 0 gives floor 2 (crit) and 4 (backstab);
// a huge defender level drives p negative and gives the same floors.
TEST(combat2b_monster_chance_floor_clamp) {
    CHECK(approx(mordor::monsterProcChance(0, 0, 0, false), 2.0, 1e-9));
    CHECK(approx(mordor::monsterProcChance(0, 0, 0, true), 4.0, 1e-9));
    CHECK(approx(mordor::monsterProcChance(0, 0, 9990, false), 2.0, 1e-9));
    CHECK(approx(mordor::monsterProcChance(0, 0, 9990, true), 4.0, 1e-9));
}

// Monster caps: 650000/650 = 1000 and 500000/500 = 1000; ln(1001)*100*ln(1001) is far above caps.
TEST(combat2b_monster_chance_caps) {
    CHECK(approx(mordor::monsterProcChance(650000, 1000, 0, false), 50.0, 1e-9));
    CHECK(approx(mordor::monsterProcChance(500000, 1000, 0, true), 75.0, 1e-9));
}

// Divisor 650 for crit, 500 for backstab, with GLvl 500 and F = 1 (ln2):
// crit: ln(500/650+1)*100*ln2 = 39.5472; backstab: ln2*100*ln2 = 48.0453.
// Backstab with defender 500: 48.0453 - ln(500/999+1)*100 = 48.0453 - 40.5799 = 7.4654.
TEST(combat2b_monster_chance_divisor_and_subtraction) {
    CHECK(approx(mordor::monsterProcChance(500, 1, 0, false), 39.5472, 0.002));
    CHECK(approx(mordor::monsterProcChance(500, 1, 0, true), 48.0453, 0.001));
    CHECK(approx(mordor::monsterProcChance(500, 1, 500, true), 7.4654, 0.002));
}

// R11: a zero score wins no guild.
TEST(combat2b_select_guild_zero_wins_none) {
    Character ch;
    CHECK_EQ(mordor::selectGuildForProc(ch, CombatProc::Critical), mordor::kNoGuild);
    CHECK_EQ(mordor::selectGuildForProc(ch, CombatProc::Backstab), mordor::kNoGuild);
    setLevel(ch, Guild::Mage, 999);  // not a crit guild
    CHECK_EQ(mordor::selectGuildForProc(ch, CombatProc::Critical), mordor::kNoGuild);
}

// R11 crit: maximize CritMod * GLvl. Warrior 10 -> 90; Seeker 31 -> 93, Seeker wins.
// Warrior 20 -> 180 beats Seeker 31 -> 93.
TEST(combat2b_select_guild_crit_max_score) {
    Character ch;
    setLevel(ch, Guild::Warrior, 10);
    setLevel(ch, Guild::Seeker, 31);
    CHECK_EQ(mordor::selectGuildForProc(ch, CombatProc::Critical), static_cast<int>(Guild::Seeker));

    Character ch2;
    setLevel(ch2, Guild::Warrior, 20);
    setLevel(ch2, Guild::Seeker, 31);
    CHECK_EQ(mordor::selectGuildForProc(ch2, CombatProc::Critical), static_cast<int>(Guild::Warrior));
}

// R11 backstab: BSMod * GLvl. Thief 10 -> 90; Ninja 46 -> 92, Ninja wins. Thief alone wins.
TEST(combat2b_select_guild_backstab_max_score) {
    Character ch;
    setLevel(ch, Guild::Thief, 10);
    setLevel(ch, Guild::Ninja, 46);
    CHECK_EQ(mordor::selectGuildForProc(ch, CombatProc::Backstab), static_cast<int>(Guild::Ninja));

    Character ch2;
    setLevel(ch2, Guild::Thief, 10);
    CHECK_EQ(mordor::selectGuildForProc(ch2, CombatProc::Backstab), static_cast<int>(Guild::Thief));
}

// Warrior 999: crit chance 26.6005. Draw 0.10 -> k = 10 < 26.6, crit wins.
// No backstab guild: chance 0, backstab loses.
TEST(combat2b_blow_crit_only) {
    Character ch;
    setLevel(ch, Guild::Warrior, 999);
    PlayerBlowInput in;
    tsup::Env env({0.10, 0.30});
    PlayerBlowRolls r = mordor::rollPlayerBlow(env.ctx, ch, in);
    CHECK(r.crit);
    CHECK(!r.backstab);
    CHECK(approx(r.critChance, 26.6005, 0.001));
    CHECK(approx(r.backstabChance, 0.0, 1e-9));
    CHECK_EQ(r.damageModifier, 5);
    CHECK_EQ(r.critGuild, static_cast<int>(Guild::Warrior));
    CHECK_EQ(r.backstabGuild, mordor::kNoGuild);
}

// Severed: Warrior 999 and Thief 999, both weapon bits, F = 0 so weapon term is 0.
// Crit 26.6005 and backstab 39.9008; draws 0.0 win both. Modifier 5 + 2 = 7.
TEST(combat2b_blow_severed_both_procs) {
    Character ch;
    setLevel(ch, Guild::Warrior, 999);
    setLevel(ch, Guild::Thief, 999);
    PlayerBlowInput in;
    in.weaponCrit = true;
    in.weaponBackstab = true;
    in.weaponFloor = 0;
    tsup::Env env({0.0, 0.0});
    PlayerBlowRolls r = mordor::rollPlayerBlow(env.ctx, ch, in);
    CHECK(r.crit);
    CHECK(r.backstab);
    CHECK_EQ(r.damageModifier, 7);
    CHECK_EQ(r.critGuild, static_cast<int>(Guild::Warrior));
    CHECK_EQ(r.backstabGuild, static_cast<int>(Guild::Thief));
}

// Thief 999 only: crit chance 0 (draw 0.5 fails); backstab 39.9 with draw 0.1 wins.
TEST(combat2b_blow_backstab_only) {
    Character ch;
    setLevel(ch, Guild::Thief, 999);
    PlayerBlowInput in;
    tsup::Env env({0.5, 0.1});
    PlayerBlowRolls r = mordor::rollPlayerBlow(env.ctx, ch, in);
    CHECK(!r.crit);
    CHECK(r.backstab);
    CHECK_EQ(r.damageModifier, 2);
    CHECK_EQ(r.critGuild, mordor::kNoGuild);
    CHECK_EQ(r.backstabGuild, static_cast<int>(Guild::Thief));
}

// No guild, weapon crit bit, F = 0: floor gives crit chance 1.0 and draw 0.0 wins.
TEST(combat2b_blow_weapon_alone_floor_crits) {
    Character ch;
    PlayerBlowInput in;
    in.weaponCrit = true;
    in.weaponFloor = 0;
    tsup::Env env({0.0, 0.0});
    PlayerBlowRolls r = mordor::rollPlayerBlow(env.ctx, ch, in);
    CHECK(approx(r.critChance, 1.0, 1e-9));
    CHECK(r.crit);
    CHECK(!r.backstab);
    CHECK_EQ(r.damageModifier, 5);
    CHECK_EQ(r.critGuild, mordor::kNoGuild);
}

// Monster level 999 floors crit chance at 1.0. Only k = 0 wins, so draw 0.02 loses.
TEST(combat2b_blow_monster_level_floor_blocks_crit) {
    Character ch;
    setLevel(ch, Guild::Warrior, 999);
    PlayerBlowInput in;
    in.monLevel = 999;
    tsup::Env env({0.02, 0.0});
    PlayerBlowRolls r = mordor::rollPlayerBlow(env.ctx, ch, in);
    CHECK(approx(r.critChance, 1.0, 1e-9));
    CHECK(!r.crit);
    CHECK_EQ(r.damageModifier, 0);
}

// No guilds, no weapon bits: both chances 0; draws 0.0 do not win (0 < 0 is false).
TEST(combat2b_blow_no_guilds_zero_chance) {
    Character ch;
    PlayerBlowInput in;
    tsup::Env env({0.0, 0.0});
    PlayerBlowRolls r = mordor::rollPlayerBlow(env.ctx, ch, in);
    CHECK(approx(r.critChance, 0.0, 1e-9));
    CHECK(approx(r.backstabChance, 0.0, 1e-9));
    CHECK(!r.crit);
    CHECK(!r.backstab);
    CHECK_EQ(r.damageModifier, 0);
    CHECK_EQ(r.critGuild, mordor::kNoGuild);
    CHECK_EQ(r.backstabGuild, mordor::kNoGuild);
}
