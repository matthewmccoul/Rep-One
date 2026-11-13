#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/dataformatsa.h" // FIXED: IniEntry is declared in dataformatsa.h (API INDEX); it was not visible from types.h/ctx.h

namespace mordor {

// ---- constants -------------------------------------------------------------

// R1 twelve on/off flags: eight named in the findings, four not yet identified.
inline constexpr i32 kOptionFlagCount = 12;
inline constexpr i32 kNamedOptionFlagCount = 8;
inline constexpr i32 kUnnamedOptionFlagCount = 4; // UNKNOWN: names of the four unnamed flags (R7)

// R1/R3 numeric defaults.
inline constexpr i32 kMusicVolumeDefault = 0;     // R3 MusicVolume defaults to 0
inline constexpr i32 kWaveVolumeDefault = -65538; // R3 WaveVolume.Default; UNKNOWN: definition of WaveVolume (listed as "Needs")
inline constexpr i32 kBrightnessDefault = 0;      // R3 Brightness defaults to 0

// R2/R4 font defaults.
inline constexpr const char* kProFontDefaultFace = "Courier New";    // "Pro" font default
inline constexpr i32 kProFontDefaultSize = 9;
inline constexpr const char* kNonProFontDefaultFace = "MS Sans Serif"; // "NonPro" font default
inline constexpr i32 kNonProFontDefaultSize = 8;

// R1 INI keys of the eight named flags, in the table order (read case-insensitively).
// UNKNOWN: section that holds the flag keys (not given in the findings).
inline constexpr const char* kOptionsSection = "Options";
inline constexpr const char* kNamedFlagKeys[kNamedOptionFlagCount] = {
    "NoStretch",
    "SoundCanvas",
    "NoPartyFX",
    "NoAutoSave",
    "NoLoadAutomap",
    "NoBlinkCursor",
    "NoDing",
    "NoToolbar"};

// R5 the two casings written for the NoToolbar option (keys are case-insensitive).
inline constexpr const char* kNoToolbarKeyWhenTrue = "NoToolBar";
inline constexpr const char* kNoToolbarKeyWhenFalse = "NoToolbar";

// R12 save-version stamp accepted on import.
inline constexpr const char* kImportVersion = "1.1";

// R14/R15 limited-build flag and notice values.
inline constexpr const char* kLimitedFlagSection = "Loading Information";
inline constexpr const char* kLimitedFlagKey = "Limited";
inline constexpr i32 kFullBuildLevelCap = 15; // R15 gc0182 full build value; mc015E = mc015C = 15

// ---- plain structs ---------------------------------------------------------

// R1/R2 the twelve on/off flags. Member order follows the table.
struct OptionFlags {
    bool noStretch = false;    // draws pictures at native size instead of stretching
    bool soundCanvas = false;  // the only flag that turns a feature on
    bool noPartyFx = false;    // silences background combat sounds from other party members
    bool noAutoSave = false;   // autosave behaviour
    bool noLoadAutomap = false; // automap behaviour
    bool noBlinkCursor = false; // message/cursor behaviour
    bool noDing = false;       // message-sound behaviour
    bool noToolbar = false;    // toolbar behaviour (see R5 casing quirk)
    Arr<bool, kUnnamedOptionFlagCount> unnamed{}; // UNKNOWN: four flags not named in the findings
};

// R4 one font setting: face name and point size.
struct FontSetting {
    Str face{};
    i32 size = 0;
};

// R1/R3 full options state of the options window.
struct GameOptions {
    OptionFlags flags{};
    i32 musicVolume = kMusicVolumeDefault;
    i32 fxVolume = kWaveVolumeDefault;
    i32 brightness = kBrightnessDefault;
    // R4 original quirk: the "Pro" font is shown under the "Non-Proportional Font" caption.
    FontSetting proFont{};
    FontSetting nonProFont{};
};

// FIXED: IniEntry (key/value pair) was defined twice (redefinition of mordor::mordor::IniEntry).
// The local definition was removed; the existing IniEntry declared in dataformatsa.h
// is used instead (included above). Its fields must provide Str key and Str value.

// INI document model. The core never reads files; the caller parses MORDOR.INI into this.
struct IniSection {
    Str name{};
    Vec<IniEntry> entries{};
};

struct IniDocument {
    Vec<IniSection> sections{};
};

// R11 inputs for save folder resolution. The caller reads the environment and the file system.
// UNKNOWN: the precise definition of gameDir (named in the findings only).
struct SaveFolderProbe {
    bool overrideSet = false;   // MORDOR_SAVES is set
    Str overridePath{};         // value of MORDOR_SAVES, when set
    bool legacyNonEmpty = false; // gameDir\..\saves contains anything
    Str legacyPath{};           // gameDir\..\saves
    Str appDataPath{};          // %APPDATA%\OpenDejenol\saves
};

enum class SaveFolderSource : i32 {
    Override = 0,
    Legacy = 1,
    AppData = 2
};

struct SaveFolderChoice {
    SaveFolderSource source = SaveFolderSource::AppData;
    Str path{};
};

// R8-R10 automap shared by all characters in one save folder.
// UNKNOWN: exact layout of the legacy Character.Explored map (per level, per cell assumed).
struct AutomapLevel {
    i32 width = 0;
    i32 height = 0;
    Vec<bool> explored{}; // indexed y * width + x
};

struct SharedMap {
    Vec<AutomapLevel> levels{}; // indexed by dungeon level
};

// R13-R15 build variants and their switch table.
enum class BuildVariant : i32 {
    Full = 0,
    Limited = 1
};

// R13 switch table. Field names describe the original variables (m009C, m0622, m02A6,
// m0032, m00A8, gv0180, m002E). Values follow the table rows for each build.
struct BuildSwitches {
    bool goodbyeBox = false;        // m009C: goodbye box asking to pass on shareware (full build only)
    bool limitedTitle = false;      // m009C: title " (Limited Version)"
    bool takeDenied = false;        // m0622: TAKE denial and "Public Version" box
    bool deadChuteLines = false;    // m02A6: dead chute / teleporter lines
    bool aboutOrderingInfo = false; // m0032: About "Ordering Info" button
    bool aboutOrdering = false;     // m00A8: About captioned "Ordering/&About"
    bool limitationNotice = false;  // gv0180: "Limitation Notice" (limited build only)
    bool limitedSplashPrefix = false; // m002E: "Limited " prefix on splash
    bool writesLimitedFlag = false; // gv0180: writes Limited=True to MORDOR.INI
};

// ---- INI helpers (pure queries) --------------------------------------------

// Pure lookup. Returns nullptr when the section or key is absent. Key match is case-insensitive.
const Str* iniFind(const IniDocument& ini, const Str& section, const Str& key);

// Pure. Flag value: absent key reads as false (R2). Parses "True"/"False" values.
bool iniFlagValue(const IniDocument& ini, const Str& section, const Str& key);

// Mutating helper. Sets or creates the key in the section.
void iniSet(IniDocument& ini, const Str& section, const Str& key, const Str& value);

// ---- R1-R7 options: flags, values, fonts -----------------------------------

// R3 options load (GameOptions.Load). Missing keys take defaults; flags read false when absent.
// UNKNOWN: whether Load runs before or after ReadFonts; this header calls readFonts from loadOptions.
void loadOptions(Ctx& ctx, const IniDocument& ini, GameOptions& opts);

// R4 font read (ReadFonts). An empty face resets to its default face; the size keeps its read value.
// UNKNOWN: INI key names of the font face and size sub-keys (only "Pro" and "NonPro" are given).
void readFonts(Ctx& ctx, const IniDocument& ini, GameOptions& opts);

// R5 options save (GameOptions.Save). Writes flags under their keys into ini.
// The True branch writes NoToolBar; the False branch writes NoToolbar.
void saveOptions(Ctx& ctx, const GameOptions& opts, IniDocument& ini);

// R6 default button: restores MusicVolume, FXVolume, Brightness and both fonts to defaults.
void restoreOptionDefaults(Ctx& ctx, GameOptions& opts);

// ---- R8-R10 automap --------------------------------------------------------

// R10 explored cell index = y * width + x (pure arithmetic).
i32 exploredIndex(i32 x, i32 y, i32 width);

// R9 automap merge (SharedMap.Merge). Explored cells only grow.
// UNKNOWN: merge rule for conflicting cells beyond "only grow".
void mergeSharedMap(Ctx& ctx, SharedMap& target, const SharedMap& incoming);

// R9 legacy absorption (SharedMap.AbsorbLegacy) for a Character.Explored map from older saves.
// legacyLevelWidths gives the width of each legacy level.
// UNKNOWN: detailed absorption rule for Character.Explored.
void absorbLegacyExplored(Ctx& ctx,
                          SharedMap& map,
                          const Vec<Vec<bool>>& legacyExplored,
                          const Vec<i32>& legacyLevelWidths);

// R10 automap display: a level with no explored cells is not shown.
bool levelShownOnAutomap(Ctx& ctx, const SharedMap& map, i32 level);

// R10 DeepestVisited: maximum level number that has any explored cell (0 if none).
i32 deepestVisited(Ctx& ctx, const SharedMap& map);

// ---- R11 save folder -------------------------------------------------------

// R11 save folder: override if set; else legacy if non-empty; else AppData.
SaveFolderChoice resolveSaveFolder(Ctx& ctx, const SaveFolderProbe& probe);

// ---- R12 save import -------------------------------------------------------

// R12 import check: every MDATA file version stamp and MDATA4 record 1 stamp must equal "1.1".
// Any other stamp refuses the import. Refusal is reported through ctx.say.
// stamps holds every stamp that must be checked, including MDATA4 record 1.
bool importAllowed(Ctx& ctx, const Vec<Str>& stamps);

// ---- R13-R15 limited (shareware) build -------------------------------------

// R13 switch table for a build variant (pure table lookup).
BuildSwitches buildSwitchesFor(BuildVariant variant);

// R14 limited splash check. True when the limited build finds Limited=True in
// [Loading Information] of MORDOR.INI.
// UNKNOWN: order of the limited checks against other startup steps.
// CONFLICT: the limited-build note says no run-time flag marks the full release, yet the full build
// never clears Limited=True, which implies it reads the key at run time. Only the limited build is
// modelled here; the author must confirm whether the full build reads Limited from MORDOR.INI.
bool limitedSplashShown(Ctx& ctx, BuildVariant variant, const IniDocument& ini);

// R15 number of dungeon levels shown in the Limitation Notice (gc0182).
// Full build value is kFullBuildLevelCap (15).
// UNKNOWN: limited build value of gc0182 and its ordering beyond "15 in full".
i32 limitationNoticeLevels(Ctx& ctx, BuildVariant variant);

// R15 shows the Limitation Notice on the limited build and writes Limited=True into ini (gv0180).
// The full build never clears Limited=True once written (persistence quirk; no routine here).
void showLimitationNotice(Ctx& ctx, BuildVariant variant, IniDocument& ini);

}  // namespace mordor
