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

