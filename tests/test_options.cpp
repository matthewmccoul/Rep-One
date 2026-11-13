#include "test_support.h"
#include "mordor/options.h"

#include <cstring>

using namespace mordor;

static AutomapLevel makeLevel(i32 w, i32 h) {
    AutomapLevel lv;
    lv.width = w;
    lv.height = h;
    for (i32 i = 0; i < w * h; ++i) lv.explored.push_back(false);
    return lv;
}

// R1, R2, R7: flag counts and the documented constant values.
TEST(OptionConstantsMatchSource) {
    CHECK_EQ(kOptionFlagCount, 12);
    CHECK_EQ(kNamedOptionFlagCount, 8);
    CHECK_EQ(kUnnamedOptionFlagCount, 4);
    CHECK_EQ(kNamedOptionFlagCount + kUnnamedOptionFlagCount, kOptionFlagCount);
    CHECK_EQ(kMusicVolumeDefault, 0);
    CHECK_EQ(kWaveVolumeDefault, -65538);
    CHECK_EQ(kBrightnessDefault, 0);
    CHECK_EQ(kProFontDefaultSize, 9);
    CHECK_EQ(kNonProFontDefaultSize, 8);
    CHECK(std::strcmp(kProFontDefaultFace, "Courier New") == 0);
    CHECK(std::strcmp(kNonProFontDefaultFace, "MS Sans Serif") == 0);
    CHECK_EQ(kFullBuildLevelCap, 15);
    CHECK(std::strcmp(kImportVersion, "1.1") == 0);
}

// R7: the eight named flag keys, in table order.
TEST(NamedFlagKeysMatchTable) {
    const char* expected[kNamedOptionFlagCount] = {
        "NoStretch", "SoundCanvas", "NoPartyFX", "NoAutoSave",
        "NoLoadAutomap", "NoBlinkCursor", "NoDing", "NoToolbar"};
    for (i32 i = 0; i < kNamedOptionFlagCount; ++i) {
        CHECK(std::strcmp(kNamedFlagKeys[i], expected[i]) == 0);
    }
    // R5: the True branch writes NoToolBar, the False branch writes NoToolbar.
    CHECK(std::strcmp(kNoToolbarKeyWhenTrue, "NoToolBar") == 0);
    CHECK(std::strcmp(kNoToolbarKeyWhenFalse, "NoToolbar") == 0);
}

// R2, R3: missing keys take defaults; flags read as False when absent.
TEST(LoadEmptyIniGivesDefaults) {
    tsup::Env env({0.5, 0.9});
    IniDocument ini;
    GameOptions opts;
    opts.flags.noStretch = true;
    opts.flags.noDing = true;
    opts.musicVolume = 42;
    opts.fxVolume = 5;
    opts.brightness = 9;

    loadOptions(env.ctx, ini, opts);

    CHECK(!opts.flags.noStretch);
    CHECK(!opts.flags.noDing);
    CHECK(!opts.flags.soundCanvas);
    CHECK(!opts.flags.noToolbar);
    CHECK_EQ(opts.musicVolume, 0);
    CHECK_EQ(opts.fxVolume, -65538);
    CHECK_EQ(opts.brightness, 0);
    CHECK(!iniFlagValue(ini, kOptionsSection, "NoStretch"));
}

// R2: the font options have stated defaults when nothing is read.
TEST(LoadEmptyIniGivesFontDefaults) {
    tsup::Env env({0.5, 0.9});
    IniDocument ini;
    GameOptions opts;
    opts.proFont.face = "Arial";
    opts.proFont.size = 20;
    opts.nonProFont.face = "Times";
    opts.nonProFont.size = 3;

    loadOptions(env.ctx, ini, opts);

    CHECK(opts.proFont.face == kProFontDefaultFace);
    CHECK_EQ(opts.proFont.size, kProFontDefaultSize);
    CHECK(opts.nonProFont.face == kNonProFontDefaultFace);
    CHECK_EQ(opts.nonProFont.size, kNonProFontDefaultSize);
}

// R1, R3: a flag present in the INI is read as True; others stay False.
TEST(LoadReadsPresentFlag) {
    tsup::Env env({0.5, 0.9});
    IniDocument ini;
    iniSet(ini, kOptionsSection, "NoStretch", "True");
    GameOptions opts;

    loadOptions(env.ctx, ini, opts);

    CHECK(opts.flags.noStretch);
    CHECK(!opts.flags.soundCanvas);
    CHECK(!opts.flags.noPartyFx);
}

// R5 + R3: a saved flag set reloads to the same values.
TEST(SaveThenLoadRoundTripsFlags) {
    tsup::Env env({0.5, 0.9});
    GameOptions src;
    src.flags.noDing = true;
    src.flags.noToolbar = true;
    src.flags.soundCanvas = false;

    IniDocument ini;
    saveOptions(env.ctx, src, ini);

    GameOptions dst;
    loadOptions(env.ctx, ini, dst);
    CHECK(dst.flags.noDing);
    CHECK(dst.flags.noToolbar);
    CHECK(!dst.flags.soundCanvas);
    CHECK(!dst.flags.noStretch);
}

// R5: True branch writes the NoToolBar key.
TEST(SaveWritesNoToolBarWhenTrue) {
    tsup::Env env({0.5, 0.9});
    GameOptions opts;
    opts.flags.noToolbar = true;
    IniDocument ini;

    saveOptions(env.ctx, opts, ini);

    CHECK(iniFind(ini, kOptionsSection, "NoToolBar") != nullptr);
    CHECK(iniFlagValue(ini, kOptionsSection, "NoToolBar"));
}

// R5: False branch writes the NoToolbar key, and it reads back as False.
TEST(SaveWritesNoToolbarWhenFalse) {
    tsup::Env env({0.5, 0.9});
    GameOptions opts;
    opts.flags.noToolbar = false;
    IniDocument ini;

    saveOptions(env.ctx, opts, ini);

    CHECK(iniFind(ini, kOptionsSection, "NoToolbar") != nullptr);
    CHECK(!iniFlagValue(ini, kOptionsSection, "NoToolbar"));
}

// R5: keys are case-insensitive, so the two casings name the same key.
TEST(IniKeysAreCaseInsensitive) {
    IniDocument ini;
    iniSet(ini, kOptionsSection, "NoToolBar", "True");

    CHECK(iniFind(ini, kOptionsSection, "notoolbar") != nullptr);
    CHECK(iniFind(ini, kOptionsSection, "NOTOOLBAR") != nullptr);
    CHECK(iniFlagValue(ini, kOptionsSection, "NoToolbar"));
}

// R6: restoreOptionDefaults resets volumes, brightness and both fonts.
TEST(RestoreDefaultsResetsValuesAndFonts) {
    tsup::Env env({0.5, 0.9});
    GameOptions opts;
    opts.musicVolume = 7;
    opts.fxVolume = 1;
    opts.brightness = 11;
    opts.proFont.face = "Arial";
    opts.proFont.size = 20;
    opts.nonProFont.face = "Times";
    opts.nonProFont.size = 3;

    restoreOptionDefaults(env.ctx, opts);

    CHECK_EQ(opts.musicVolume, kMusicVolumeDefault);
    CHECK_EQ(opts.fxVolume, kWaveVolumeDefault);
    CHECK_EQ(opts.brightness, kBrightnessDefault);
    CHECK(opts.proFont.face == kProFontDefaultFace);
    CHECK_EQ(opts.proFont.size, kProFontDefaultSize);
    CHECK(opts.nonProFont.face == kNonProFontDefaultFace);
    CHECK_EQ(opts.nonProFont.size, kNonProFontDefaultSize);
}

// R9 / R10 index formula: explored cell index = y * width + x.
TEST(ExploredIndexIsRowMajor) {
    CHECK_EQ(exploredIndex(0, 0, 7), 0);
    CHECK_EQ(exploredIndex(6, 0, 7), 6);
    CHECK_EQ(exploredIndex(0, 1, 7), 7);
    CHECK_EQ(exploredIndex(3, 2, 10), 23);
}

// R9: explored cells per level only grow when merging.
TEST(MergeOnlyGrowsExploredCells) {
    tsup::Env env({0.5, 0.9});
    SharedMap target;
    target.levels.push_back(makeLevel(3, 2));
    target.levels[0].explored[1] = true;

    SharedMap incoming;
    incoming.levels.push_back(makeLevel(3, 2));
    incoming.levels[0].explored[0] = true;
    incoming.levels[0].explored[5] = true;
    incoming.levels[0].explored[1] = false;

    mergeSharedMap(env.ctx, target, incoming);

    CHECK(target.levels[0].explored[0]);  // gained from incoming
    CHECK(target.levels[0].explored[1]);  // kept although incoming is false
    CHECK(target.levels[0].explored[5]);  // gained from incoming
    CHECK(!target.levels[0].explored[2]); // nobody explored it
}

// R10: a level with no explored cells is not shown.
TEST(LevelWithNoExploredCellsIsHidden) {
    tsup::Env env({0.5, 0.9});
    SharedMap map;
    map.levels.push_back(makeLevel(3, 3));
    map.levels.push_back(makeLevel(3, 3));
    map.levels[0].explored[0] = true;

    CHECK(levelShownOnAutomap(env.ctx, map, 0));
    CHECK(!levelShownOnAutomap(env.ctx, map, 1));
}

// R10: DeepestVisited is the highest level with any explored cell.
TEST(DeepestVisitedIsHighestExploredLevel) {
    tsup::Env env({0.5, 0.9});
    SharedMap map;
    for (i32 i = 0; i < 4; ++i) map.levels.push_back(makeLevel(2, 2));
    map.levels[0].explored[3] = true;
    map.levels[2].explored[0] = true;
    // level 1 and level 3 have nothing explored

    CHECK_EQ(deepestVisited(env.ctx, map), 2);
}

// R10: a single explored deeper level is found even when shallower ones are empty.
TEST(DeepestVisitedWithOnlyLevelOneExplored) {
    tsup::Env env({0.5, 0.9});
    SharedMap map;
    for (i32 i = 0; i < 3; ++i) map.levels.push_back(makeLevel(2, 2));
    map.levels[1].explored[2] = true;

    CHECK_EQ(deepestVisited(env.ctx, map), 1);
}

// R11 step 1: MORDOR_SAVES wins when set, even if the legacy folder has files.
TEST(SaveFolderOverrideWins) {
    tsup::Env env({0.5, 0.9});
    SaveFolderProbe probe;
    probe.overrideSet = true;
    probe.overridePath = "D:\\custom";
    probe.legacyNonEmpty = true;
    probe.legacyPath = "legacy";
    probe.appDataPath = "appdata";

    SaveFolderChoice c = resolveSaveFolder(env.ctx, probe);
    CHECK(c.source == SaveFolderSource::Override);
    CHECK(c.path == probe.overridePath);
}

// R11 step 2: legacy folder used when non-empty and no override is set.
// An override path with overrideSet false must be ignored.
TEST(SaveFolderLegacyWhenNoOverride) {
    tsup::Env env({0.5, 0.9});
    SaveFolderProbe probe;
    probe.overrideSet = false;
    probe.overridePath = "ignored";
    probe.legacyNonEmpty = true;
    probe.legacyPath = "legacy";
    probe.appDataPath = "appdata";

    SaveFolderChoice c = resolveSaveFolder(env.ctx, probe);
    CHECK(c.source == SaveFolderSource::Legacy);
    CHECK(c.path == probe.legacyPath);
}

// R11 step 3: AppData is the last resort.
TEST(SaveFolderFallsBackToAppData) {
    tsup::Env env({0.5, 0.9});
    SaveFolderProbe probe;
    probe.overrideSet = false;
    probe.legacyNonEmpty = false;
    probe.legacyPath = "legacy";
    probe.appDataPath = "appdata";

    SaveFolderChoice c = resolveSaveFolder(env.ctx, probe);
    CHECK(c.source == SaveFolderSource::AppData);
    CHECK(c.path == probe.appDataPath);
}

// R12: every stamp must be exactly "1.1"; anything else is refused.
TEST(ImportAllowedOnlyForVersion11) {
    tsup::Env env({0.5, 0.9});
    Vec<Str> allGood;
    allGood.push_back("1.1");
    allGood.push_back("1.1");
    CHECK(importAllowed(env.ctx, allGood));

    Vec<Str> oneBad;
    oneBad.push_back("1.1");
    oneBad.push_back("1.0");
    CHECK(!importAllowed(env.ctx, oneBad));

    Vec<Str> prefixOnly;
    prefixOnly.push_back("1.10");
    CHECK(!importAllowed(env.ctx, prefixOnly));
}

// R13 / table: full build switch values.
TEST(BuildSwitchesFullBuild) {
    BuildSwitches sw = buildSwitchesFor(BuildVariant::Full);
    CHECK(sw.goodbyeBox);            // m009C = 0 (False) shows goodbye box
    CHECK(!sw.limitedTitle);
    CHECK(!sw.takeDenied);
    CHECK(!sw.deadChuteLines);
    CHECK(!sw.aboutOrderingInfo);
    CHECK(!sw.aboutOrdering);
    CHECK(!sw.limitationNotice);     // gv0180 never set in full
    CHECK(!sw.limitedSplashPrefix);  // m002E never set in full
    CHECK(!sw.writesLimitedFlag);
}

// R13 / table: limited build switch values.
TEST(BuildSwitchesLimitedBuild) {
    BuildSwitches sw = buildSwitchesFor(BuildVariant::Limited);
    CHECK(!sw.goodbyeBox);           // goodbye box only shown when m009C is False
    CHECK(sw.limitedTitle);
    CHECK(sw.takeDenied);
    CHECK(sw.deadChuteLines);
    CHECK(sw.aboutOrderingInfo);
    CHECK(sw.aboutOrdering);
    CHECK(sw.limitationNotice);
    CHECK(sw.limitedSplashPrefix);
    CHECK(sw.writesLimitedFlag);
}

// R15: the full build shows 15 levels in the Limitation Notice.
TEST(LimitationNoticeFullBuildShows15) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(limitationNoticeLevels(env.ctx, BuildVariant::Full), 15);
    CHECK_EQ(limitationNoticeLevels(env.ctx, BuildVariant::Full), kFullBuildLevelCap);
}

// R14: the limited splash shows only when Limited is set in [Loading Information].
TEST(LimitedSplashNeedsLimitedFlag) {
    tsup::Env env({0.5, 0.9});
    IniDocument ini;
    CHECK(!limitedSplashShown(env.ctx, BuildVariant::Limited, ini));

    iniSet(ini, kLimitedFlagSection, kLimitedFlagKey, "True");
    CHECK(limitedSplashShown(env.ctx, BuildVariant::Limited, ini));
}

// R14 / table: the limited build writes Limited=True to MORDOR.INI.
TEST(LimitationNoticeLimitedWritesFlag) {
    tsup::Env env({0.5, 0.9});
    IniDocument ini;
    showLimitationNotice(env.ctx, BuildVariant::Limited, ini);

    CHECK(iniFind(ini, kLimitedFlagSection, kLimitedFlagKey) != nullptr);
    CHECK(iniFlagValue(ini, kLimitedFlagSection, kLimitedFlagKey));
}

// Table: gv0180 is never set in the full build, so the notice leaves INI alone.
TEST(LimitationNoticeFullBuildLeavesIniAlone) {
    tsup::Env env({0.5, 0.9});
    IniDocument ini;
    showLimitationNotice(env.ctx, BuildVariant::Full, ini);

    CHECK(iniFind(ini, kLimitedFlagSection, kLimitedFlagKey) == nullptr);
}
