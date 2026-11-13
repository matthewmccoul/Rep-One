#include "test_support.h"
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"
#include "mordor/death1a.h"

using namespace mordor;

// SKIP B_0066#20: RoomRoster.Lines has no equivalent function in the headers.
// SKIP B_0066#21: RoomRoster.Lines has no equivalent function in the headers.
// SKIP B_0066#69: Party.RemoveTheDead has no equivalent; removePartyMember does not model the same messages/marking.
// SKIP B_0066#136: CombatEngine.MonsterAttacks has no equivalent in the headers; needs combat simulation.
// SKIP B_0066#137: Combat caption ordering needs CombatEngine simulation, no equivalent function.
// SKIP B_0066#138: Combat death message ordering needs CombatEngine simulation, no equivalent function.

TEST(engine_B_0066_132_new_character_area_is_unset) {
    Character ch;
    CHECK_EQ(ch.location.area, -1);
}

TEST(engine_B_0066_133_rock_death_below_ground_area_zero) {
    Character ch;
    ch.location.dungeonLevel = 3;
    ch.location.area = 17;
    setAreaForRockDeath(ch);
    CHECK_EQ(ch.location.area, 0);
}

TEST(engine_B_0066_134_carrier_puts_down_body_sets_area) {
    tsup::Env env({0.5, 0.9});
    Character body;
    body.deathState = DeathState::Dead;
    body.location.area = 0;
    Character raiser;
    raiser.location.dungeonLevel = 2;
    raiser.location.area = 9;
    initiatePendingRaise(env.ctx, body, raiser, 100, RaiseKind::RaiseDead);
    CHECK_EQ(body.location.area, 9);
}

TEST(engine_B_0066_135_rest_in_city_area_unset) {
    tsup::Env env({0.5, 0.9});
    Character body;
    body.deathState = DeathState::Dead;
    body.location.area = 9;
    restAtCity(env.ctx, body);
    CHECK_EQ(body.location.area, -1);
}
