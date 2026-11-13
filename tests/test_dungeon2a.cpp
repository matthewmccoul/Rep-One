#include "test_support.h"
#include "mordor/dungeon2a.h"

using namespace mordor;

// R137 / automap: Square = X + 30*(Y-1), with X and Y starting at 1.
TEST(squareIndexOf_rowMajorFromOne) {
    CHECK_EQ(squareIndexOf(1, 1), 1);
    CHECK_EQ(squareIndexOf(30, 1), 30);
    CHECK_EQ(squareIndexOf(1, 2), 31);
    CHECK_EQ(squareIndexOf(30, 30), 900);
    CHECK_EQ(kFloorWidth, 30);
}

// R128: full build limit 15, limited build limit 3.
TEST(clampLevelRequest_fullBuildLimitIs15) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(buildLevelLimit(ReleaseKind::Full), 15);
    CHECK_EQ(clampLevelRequest(env.ctx, ReleaseKind::Full, 15), 15);
    CHECK_EQ(clampLevelRequest(env.ctx, ReleaseKind::Full, 9), 9);
}

// R128: a request above the limit is set to the limit (limited build).
TEST(clampLevelRequest_limitedBuildClampsToThree) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(buildLevelLimit(ReleaseKind::Limited), 3);
    CHECK_EQ(clampLevelRequest(env.ctx, ReleaseKind::Limited, 4), 3);
    CHECK_EQ(clampLevelRequest(env.ctx, ReleaseKind::Limited, 5), 3);
    CHECK_EQ(clampLevelRequest(env.ctx, ReleaseKind::Limited, 3), 3);
    CHECK_EQ(clampLevelRequest(env.ctx, ReleaseKind::Limited, 1), 1);
}

// R131: teleporter and chute destinations past the limit are denied.
TEST(isDestinationDenied_pastLimitOnly) {
    CHECK(!isDestinationDenied(15, ReleaseKind::Full));
    CHECK(isDestinationDenied(16, ReleaseKind::Full));
    CHECK(!isDestinationDenied(3, ReleaseKind::Limited));
    CHECK(isDestinationDenied(4, ReleaseKind::Limited));
}

// R129: Full requires levels >= 15 plus both monster and item conditions.
TEST(classifyRelease_levelCountAndBothFlags) {
    CHECK(classifyRelease(15, true, true) == ReleaseKind::Full);
    CHECK(classifyRelease(14, true, true) == ReleaseKind::Limited);
    CHECK(classifyRelease(3, true, true) == ReleaseKind::Limited);
}

// R132: nothing below dungeon level 12 (DeepestHeCanReach = 12).
TEST(seerCanReach_stopsAtLevel12) {
    CHECK_EQ(kSeerDeepestReach, 12);
    CHECK(seerCanReach(1));
    CHECK(seerCanReach(12));
    CHECK(!seerCanReach(13));
}

// R133: fail if Int(Rnd*100)+1 < level*5. Level 4 gives threshold 20.
// Roll 0.0 -> 1 < 20 fails. Second roll 0.99 -> 99 >= 20 names the level.
TEST(seerEarlyFailure_failureNamesLevel) {
    tsup::Env env({0.0, 0.99});
    SeerEarlyResult r = rollSeerEarlyFailure(env.ctx, 4);
    CHECK(r.failed);
    CHECK(r.namesLevel);
}

// Second roll 0.0 -> 0 >= 20 is false, so the failure says nothing.
// Level 1 (threshold 5) with roll 0.5 -> 51 < 5 is false, so no failure.
TEST(seerEarlyFailure_silentFailureOrPass) {
    tsup::Env silent({0.0, 0.0});
    SeerEarlyResult r = rollSeerEarlyFailure(silent.ctx, 4);
    CHECK(r.failed);
    CHECK(!r.namesLevel);

    tsup::Env pass({0.5, 0.5});
    SeerEarlyResult r2 = rollSeerEarlyFailure(pass.ctx, 1);
    CHECK(!r2.failed);
}

// R134: case-insensitive prefix; the typed text cannot be longer than the name.
TEST(isSeerNamePrefix_caseInsensitivePrefix) {
    CHECK(isSeerNamePrefix(Str("kob"), Str("Kobold")));
    CHECK(isSeerNamePrefix(Str("KOBOLD"), Str("kobold")));
    CHECK(!isSeerNamePrefix(Str("kobolds"), Str("kobold")));
    CHECK(!isSeerNamePrefix(Str("bold"), Str("kobold")));
}

// R134: two or more partial matches gives "several" (count >= 2).
TEST(matchSeerNames_severalPartialMatches) {
    Vec<Str> names;
    names.push_back(Str("Kobold"));
    names.push_back(Str("Kobold Lord"));
    names.push_back(Str("Orc"));

    SeerNameMatch several = matchSeerNames(Str("kob"), names);
    CHECK_EQ(several.count, 2);
    CHECK(!several.exact);

    SeerNameMatch one = matchSeerNames(Str("or"), names);
    CHECK_EQ(one.count, 1);
    CHECK_EQ(one.index, 2);
    CHECK(!one.exact);
}

// R134: an exact match wins at once, even when partial matches also exist.
TEST(matchSeerNames_exactMatchWins) {
    Vec<Str> names;
    names.push_back(Str("Kobold Lord"));
    names.push_back(Str("Kobold"));

    SeerNameMatch m = matchSeerNames(Str("kobold"), names);
    CHECK(m.exact);
    CHECK_EQ(m.index, 1);
}

// Quirk (c59): "Kobolds" is longer than "Kobold", so the plain match fails.
// Only the plural branch (monsterNameFor with IdLevel 0) finds it.
TEST(matchSeerNames_pluralFormNotMatchedPlain) {
    Vec<Str> names;
    names.push_back(Str("Kobold"));

    SeerNameMatch m = matchSeerNames(Str("Kobolds"), names);
    CHECK_EQ(m.count, 0);
    CHECK(!m.exact);
}

// Commonality boundaries: <=2, <=4, <=7, <=11, else.
TEST(seerCommonality_boundaries) {
    CHECK(seerCommonality(2) == SeerCommonality::VeryCommon);
    CHECK(seerCommonality(3) == SeerCommonality::FairlyCommon);
    CHECK(seerCommonality(4) == SeerCommonality::FairlyCommon);
    CHECK(seerCommonality(5) == SeerCommonality::Uncommon);
    CHECK(seerCommonality(7) == SeerCommonality::Uncommon);
    CHECK(seerCommonality(8) == SeerCommonality::Rare);
    CHECK(seerCommonality(11) == SeerCommonality::Rare);
    CHECK(seerCommonality(12) == SeerCommonality::AlmostUnheard);
}

// Frequency boundaries: <=2, <=8, <=15, <=35, <=50, else.
TEST(seerFrequency_boundaries) {
    CHECK(seerFrequency(2) == SeerFrequency::AlmostNever);
    CHECK(seerFrequency(3) == SeerFrequency::VeryRarely);
    CHECK(seerFrequency(8) == SeerFrequency::VeryRarely);
    CHECK(seerFrequency(9) == SeerFrequency::Rarely);
    CHECK(seerFrequency(15) == SeerFrequency::Rarely);
    CHECK(seerFrequency(16) == SeerFrequency::Uncommonly);
    CHECK(seerFrequency(35) == SeerFrequency::Uncommonly);
    CHECK(seerFrequency(36) == SeerFrequency::Commonly);
    CHECK(seerFrequency(50) == SeerFrequency::Commonly);
    CHECK(seerFrequency(51) == SeerFrequency::VeryCommonly);
}

// R135: a lair is returned when Rnd*100 < 75. Roll 0.74 -> 74 < 75 gives the lair.
TEST(locateMonster_lairReturnedUnder75Percent) {
    FloorState floor;
    floor.areas[3].lairId = 7;
    tsup::Env env({0.74, 0.74});
    SeerMonsterPick pick = locateMonster(env.ctx, floor, 7, 2);
    CHECK(pick.lairResult);
    CHECK_EQ(pick.areaIndex, 3);
}

// R135: roll 0.75 -> 75, not < 75, so no lair. The only area whose spawn
// mask admits type 2 is area 5 (mask bit 2 = 4), so it is the candidate.
TEST(locateMonster_candidateAreaWhenNotLair) {
    FloorState floor;
    floor.areas[5].spawnMask = 4;
    tsup::Env env({0.75, 0.75});
    SeerMonsterPick pick = locateMonster(env.ctx, floor, 7, 2);
    CHECK(!pick.lairResult);
    CHECK_EQ(pick.areaIndex, 5);
}

// R136: a character in the city cannot be located.
TEST(locateCharacter_cityRefused) {
    Character subject;
    subject.location.x = 7;
    subject.location.y = 9;
    tsup::Env env({0.5, 0.5});
    SeerCharacterReport r = locateCharacter(env.ctx, subject, 5, true);
    CHECK(r.outcome == SeerCharacterOutcome::CityRefused);
}

// R136: need = level*3 = 15 at level 5. Roll 0.5 -> 51, not < 15, so the
// location is reported. A dead subject is reported "among the dead".
TEST(locateCharacter_reportsPositionAndDeadFlag) {
    Character subject;
    subject.location.x = 7;
    subject.location.y = 9;
    subject.location.dungeonLevel = 5;
    tsup::Env env({0.5, 0.5});
    SeerCharacterReport r = locateCharacter(env.ctx, subject, 5, false);
    CHECK(r.outcome == SeerCharacterOutcome::Located);
    CHECK_EQ(r.x, 7);
    CHECK_EQ(r.y, 9);
    CHECK(!r.amongDead);

    subject.deathState = DeathState::Dead;
    tsup::Env env2({0.5, 0.5});
    SeerCharacterReport dead = locateCharacter(env2.ctx, subject, 5, false);
    CHECK(dead.amongDead);
}

// R137: blur is up to four squares either way and clamped to 1..width / 1..height.
// Zero spread leaves the point unchanged.
TEST(blurPoint_zeroSpreadAndBounds) {
    tsup::Env env({0.5, 0.9});
    BlurredPoint z = blurPoint(env.ctx, 10, 20, 0);
    CHECK_EQ(z.x, 10);
    CHECK_EQ(z.y, 20);

    tsup::Env seeded(777);
    for (int i = 0; i < 200; ++i) {
        BlurredPoint p = blurPoint(seeded.ctx, 1, 30, kSeerMonsterBlurSpread);
        CHECK(p.x >= 1 && p.x <= 5);
        CHECK(p.y >= 26 && p.y <= kFloorHeight);
    }
}

// NoteDepth: returns true only when the level is deeper than the record.
TEST(noteDepth_recordsOnlyNewDepths) {
    i32 deepest = 5;
    CHECK(!noteDepth(5, deepest));
    CHECK_EQ(deepest, 5);
    CHECK(noteDepth(6, deepest));
    CHECK_EQ(deepest, 6);
    CHECK(!noteDepth(3, deepest));
    CHECK_EQ(deepest, 6);
}

// Preserved formula: deepest = Max(dungeon, map); if 0 then 1.
TEST(effectiveDeepest_maxWithFloorOfOne) {
    CHECK_EQ(effectiveDeepest(0, 0), 1);
    CHECK_EQ(effectiveDeepest(4, 2), 4);
    CHECK_EQ(effectiveDeepest(2, 7), 7);
}

// nextHookedMod: next index after afterIndex that is live, has a hook and is
// not disabled. disableModAfterHookFailure marks a failing hook disabled.
TEST(nextHookedMod_skipsUnhookedAndDisabled) {
    Vec<ModHookSlot> mods;
    ModHookSlot a;
    a.live = true;
    a.hasHook = true;
    ModHookSlot b;
    b.live = true;
    b.hasHook = false;
    ModHookSlot c;
    c.live = true;
    c.hasHook = true;
    c.disabled = true;
    ModHookSlot d;
    d.live = true;
    d.hasHook = true;
    mods.push_back(a);
    mods.push_back(b);
    mods.push_back(c);
    mods.push_back(d);

    CHECK_EQ(nextHookedMod(mods, -1), 0);
    CHECK_EQ(nextHookedMod(mods, 0), 3);
    CHECK_EQ(nextHookedMod(mods, 3), -1);

    tsup::Env env({0.5, 0.9});
    disableModAfterHookFailure(env.ctx, mods[0]);
    CHECK(mods[0].disabled);
    CHECK_EQ(nextHookedMod(mods, -1), 3);
}

// R123: an active run refuses the new request. No hooked mod completes with
// NoHooks. A hooked mod starts RunHooks.
TEST(beginActionRun_refusedNoHooksRunHooks) {
    tsup::Env env({0.5, 0.9});

    Vec<ModHookSlot> hooked;
    ModHookSlot h;
    h.live = true;
    h.hasHook = true;
    hooked.push_back(h);

    ActionRunState busy;
    busy.active = true;
    CHECK(beginActionRun(env.ctx, busy, ActionTrigger::EnteredDungeonFromTown, hooked)
          == ActionRunStart::Refused);

    Vec<ModHookSlot> none;
    ActionRunState idle;
    CHECK(beginActionRun(env.ctx, idle, ActionTrigger::LeavingByStairs, none)
          == ActionRunStart::NoHooks);

    ActionRunState idle2;
    CHECK(beginActionRun(env.ctx, idle2, ActionTrigger::LeavingByStairs, hooked)
          == ActionRunStart::RunHooks);
}

// R124: the climb goes on only if the party is still on the stairs and in the dungeon.
TEST(stairsClimbStillValid_needsStairsAndDungeon) {
    Party p;
    p.onStairs = true;
    p.inDungeon = true;
    CHECK(stairsClimbStillValid(p));

    p.inDungeon = false;
    CHECK(!stairsClimbStillValid(p));

    p.inDungeon = true;
    p.onStairs = false;
    CHECK(!stairsClimbStillValid(p));
}

