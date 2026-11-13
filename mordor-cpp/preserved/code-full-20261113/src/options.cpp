// options.cpp: implements include/mordor/options.h (SRD chapter 19, part c19p1).
// Rule numbers (R<n>) refer to the SRD. Every function declared in options.h is defined once here.
// BLOCKED: the six errors (lines 56, 58, 199, 224, 225, 231) all come from IniSection::entries being
// declared as int in dataformatsa.h. IniSection is not in the API INDEX, so its definition could not be
// checked. The fix belongs in that header (make entries a Vec<IniEntry>), not in this file. This file is
// otherwise unchanged.
#include "mordor/options.h"
#include "mordor/dataformatsa.h"  // IniEntry. UNKNOWN: field names `key` and `value` are assumed.

namespace mordor {
namespace {

// UNKNOWN: INI key names for MusicVolume, FXVolume, Brightness and the two font settings.
// SRD R6 gives only the defaults, so the SRD setting names are used as keys.
constexpr const char* kKeyMusicVolume = "MusicVolume";
constexpr const char* kKeyFxVolume = "FXVolume";
constexpr const char* kKeyBrightness = "Brightness";
constexpr const char* kKeyProFace = "ProFontFace";
constexpr const char* kKeyProSize = "ProFontSize";
constexpr const char* kKeyNonProFace = "NonProFontFace";
constexpr const char* kKeyNonProSize = "NonProFontSize";

constexpr const char* kTextTrue = "True";
constexpr const char* kTextFalse = "False";

// R5: NoToolbar is the eighth named flag, kNamedFlagKeys[7].
constexpr i32 kNoToolbarFlagIndex = 7;

char asciiLower(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

// INI keys and section names are case-insensitive (R5 quirk note).
bool equalsIgnoreCase(const Str& a, const Str& b) {
    const i64 n = static_cast<i64>(a.size());
    if (n != static_cast<i64>(b.size())) {
        return false;
    }
    for (i64 i = 0; i < n; ++i) {
        if (asciiLower(a[i]) != asciiLower(b[i])) {
            return false;
        }
    }
    return true;
}

// Returns the index of the section with this name (case-insensitive), or -1.
i64 findSectionIndex(const IniDocument& ini, const Str& name) {
    const i64 count = static_cast<i64>(ini.sections.size());
    for (i64 i = 0; i < count; ++i) {
        if (equalsIgnoreCase(ini.sections[i].name, name)) {
            return i;
        }
    }
    return -1;
}

// Returns the index of the entry with this key (case-insensitive), or -1.
i64 findEntryIndex(const IniSection& section, const Str& key) {
    const i64 count = static_cast<i64>(section.entries.size());
    for (i64 i = 0; i < count; ++i) {
        if (equalsIgnoreCase(section.entries[i].key, key)) {
            return i;
        }
    }
    return -1;
}

// Parses an optionally signed decimal integer (surrounding spaces allowed).
// UNKNOWN: the SRD does not say how malformed numbers are read. Callers fall back to the default.
bool parseI32(const Str& text, i32& out) {
    const i64 n = static_cast<i64>(text.size());
    i64 p = 0;
    while (p < n && text[p] == ' ') {
        ++p;
    }
    i64 e = n;
    while (e > p && text[e - 1] == ' ') {
        --e;
    }
    bool negative = false;
    if (p < e && (text[p] == '-' || text[p] == '+')) {
        negative = (text[p] == '-');
        ++p;
    }
    if (p >= e) {
        return false;
    }
    i64 value = 0;
    for (i64 i = p; i < e; ++i) {
        const char c = text[i];
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10 + (c - '0');
        if (value > 2147483648LL) {
            return false;
        }
    }
    if (negative) {
        value = -value;
    }
    if (value < -2147483648LL || value > 2147483647LL) {
        return false;
    }
    out = static_cast<i32>(value);
    return true;
}

// R2/R5: flags are stored as True/False.
// UNKNOWN: accepted spellings beyond "True"/"False". This takes "True", "1" and "-1" as on.
bool parseFlagText(const Str& text) {
    if (equalsIgnoreCase(text, Str(kTextTrue))) {
        return true;
    }
    if (equalsIgnoreCase(text, Str("1")) || equalsIgnoreCase(text, Str("-1"))) {
        return true;
    }
    return false;
}

// R1/R7: flag i in kNamedFlagKeys order.
bool flagGet(const OptionFlags& f, i32 idx) {
    switch (idx) {
        case 0: return f.noStretch;
        case 1: return f.soundCanvas;
        case 2: return f.noPartyFx;
        case 3: return f.noAutoSave;
        case 4: return f.noLoadAutomap;
        case 5: return f.noBlinkCursor;
        case 6: return f.noDing;
        case 7: return f.noToolbar;
        default: return false;
    }
}

void flagSet(OptionFlags& f, i32 idx, bool value) {
    switch (idx) {
        case 0: f.noStretch = value; break;
        case 1: f.soundCanvas = value; break;
        case 2: f.noPartyFx = value; break;
        case 3: f.noAutoSave = value; break;
        case 4: f.noLoadAutomap = value; break;
        case 5: f.noBlinkCursor = value; break;
        case 6: f.noDing = value; break;
        case 7: f.noToolbar = value; break;
        default: break;
    }
}

// Reads an integer option from [Options]. A missing or malformed key takes the default (R3.1).
i32 readIntOr(const IniDocument& ini, const char* key, i32 fallback) {
    const Str* text = iniFind(ini, Str(kOptionsSection), Str(key));
    i32 value = 0;
    if (text != nullptr && parseI32(*text, value)) {
        return value;
    }
    return fallback;
}

// R4 for one font. R4.1: an empty face resets to the default face.
// R4.2: the size keeps the value read. A missing size takes the default (R3.1).
void readOneFont(const IniDocument& ini,
                 const char* faceKey,
                 const char* sizeKey,
                 const char* defaultFace,
                 i32 defaultSize,
                 FontSetting& out) {
    const Str* face = iniFind(ini, Str(kOptionsSection), Str(faceKey));
    if (face != nullptr && !face->empty()) {
        out.face = *face;
    } else {
        out.face = Str(defaultFace);  // R4.1
    }

    i32 size = defaultSize;
    const Str* sizeText = iniFind(ini, Str(kOptionsSection), Str(sizeKey));
    if (sizeText != nullptr) {
        i32 parsed = 0;
        if (parseI32(*sizeText, parsed)) {
            size = parsed;  // R4.2
        }
    }
    out.size = size;
}

}  // namespace

// ---------------------------------------------------------------------------
// INI helpers (SRD R2, R5)
// ---------------------------------------------------------------------------

const Str* iniFind(const IniDocument& ini, const Str& section, const Str& key) {
    const i64 s = findSectionIndex(ini, section);
    if (s < 0) {
        return nullptr;
    }
    const IniSection& sec = ini.sections[s];
    const i64 e = findEntryIndex(sec, key);
    if (e < 0) {
        return nullptr;
    }
    return &sec.entries[e].value;
}

// R2: absent keys read as False.
bool iniFlagValue(const IniDocument& ini, const Str& section, const Str& key) {
    const Str* text = iniFind(ini, section, key);
    if (text == nullptr) {
        return false;
    }
    return parseFlagText(*text);
}

// Writes key=value. Creates the section if needed. Replaces the key text and value of a
// case-insensitively matching entry. R5 quirk: the key casing written is the last one saved.
void iniSet(IniDocument& ini, const Str& section, const Str& key, const Str& value) {
    i64 s = findSectionIndex(ini, section);
    if (s < 0) {
        IniSection created;
        created.name = section;
        ini.sections.push_back(created);
        s = static_cast<i64>(ini.sections.size()) - 1;
    }
    IniSection& sec = ini.sections[s];
    const i64 e = findEntryIndex(sec, key);
    if (e >= 0) {
        sec.entries[e].key = key;
        sec.entries[e].value = value;
        return;
    }
    IniEntry entry;
    entry.key = key;
    entry.value = value;
    sec.entries.push_back(entry);
}

// ---------------------------------------------------------------------------
// Options load, fonts, save, defaults (SRD R1-R6)
// ---------------------------------------------------------------------------

// R3: read each key, missing keys take their defaults. Fonts are read last via readFonts (R3.6).
void loadOptions(Ctx& ctx, const IniDocument& ini, GameOptions& opts) {
    // R3.2: flags read as False when absent.
    for (i32 i = 0; i < kNamedOptionFlagCount; ++i) {
        flagSet(opts.flags, i, iniFlagValue(ini, Str(kOptionsSection), Str(kNamedFlagKeys[i])));
    }
    // UNKNOWN: the four unnamed flags have no known key names, so they read as False (R2).
    for (i32 i = 0; i < kUnnamedOptionFlagCount; ++i) {
        opts.flags.unnamed[i] = false;
    }
    // R3.3: MusicVolume defaults to 0.
    opts.musicVolume = readIntOr(ini, kKeyMusicVolume, kMusicVolumeDefault);
    // R3.4: FXVolume defaults to WaveVolume.Default.
    opts.fxVolume = readIntOr(ini, kKeyFxVolume, kWaveVolumeDefault);
    // R3.5: Brightness defaults to 0.
    opts.brightness = readIntOr(ini, kKeyBrightness, kBrightnessDefault);
    // R3.6: fonts via ReadFonts.
    // UNKNOWN: whether Load runs before or after ReadFonts. The SRD listing order is used.
    readFonts(ctx, ini, opts);
}

// R4: font read.
// QUIRK (kept by the port): the "Pro" font is shown under the "Non-Proportional Font" caption,
// and the names are swapped relative to their captions. This code stores each setting under
// its own name and does not swap them.
void readFonts(Ctx& /*ctx*/, const IniDocument& ini, GameOptions& opts) {
    readOneFont(ini, kKeyProFace, kKeyProSize, kProFontDefaultFace, kProFontDefaultSize, opts.proFont);
    readOneFont(ini, kKeyNonProFace, kKeyNonProSize, kNonProFontDefaultFace, kNonProFontDefaultSize,
                opts.nonProFont);
}

// R5: write the flags under their keys.
// UNKNOWN: R5 lists only the flags as saved. The numeric values and fonts are not written, and
// neither are the four unnamed flags, since their keys are unknown.
void saveOptions(Ctx& /*ctx*/, const GameOptions& opts, IniDocument& ini) {
    for (i32 i = 0; i < kNamedOptionFlagCount; ++i) {
        const bool on = flagGet(opts.flags, i);
        Str key;
        if (i == kNoToolbarFlagIndex) {
            // R5.2 QUIRK (kept by the port): True writes "NoToolBar", False writes "NoToolbar".
            // Keys are case-insensitive, so both refer to the same entry.
            key = on ? Str(kNoToolbarKeyWhenTrue) : Str(kNoToolbarKeyWhenFalse);
        } else {
            key = Str(kNamedFlagKeys[i]);
        }
        iniSet(ini, Str(kOptionsSection), key, Str(on ? kTextTrue : kTextFalse));
    }
}

// R6: the Default button restores MusicVolume, FXVolume, Brightness and the two fonts.
// The flags are not touched by R6.
void restoreOptionDefaults(Ctx& /*ctx*/, GameOptions& opts) {
    opts.musicVolume = kMusicVolumeDefault;
    opts.fxVolume = kWaveVolumeDefault;
    opts.brightness = kBrightnessDefault;
    opts.proFont.face = Str(kProFontDefaultFace);
    opts.proFont.size = kProFontDefaultSize;
    opts.nonProFont.face = Str(kNonProFontDefaultFace);
    opts.nonProFont.size = kNonProFontDefaultSize;
}

// ---------------------------------------------------------------------------
// Automap (SRD R8-R10)
// ---------------------------------------------------------------------------

// Explored cell index = y * width + x (SRD, automap formula).
i32 exploredIndex(i32 x, i32 y, i32 width) {
    return y * width + x;
}

// R9.1: explored cells per level only grow. Cells are OR-ed in and never cleared.
// UNKNOWN: the SRD gives no merge rule for levels whose shapes differ. Those levels are left
// unmerged. A level with no shape in the target adopts the incoming shape.
void mergeSharedMap(Ctx& /*ctx*/, SharedMap& target, const SharedMap& incoming) {
    const i64 count = static_cast<i64>(incoming.levels.size());
    for (i64 lv = 0; lv < count; ++lv) {
        const AutomapLevel& src = incoming.levels[lv];
        if (src.width <= 0 || src.height <= 0) {
            continue;  // nothing to merge for this level
        }
        while (static_cast<i64>(target.levels.size()) <= lv) {
            target.levels.push_back(AutomapLevel{});
        }
        AutomapLevel& dst = target.levels[lv];
        if (dst.width <= 0 || dst.height <= 0) {
            dst.width = src.width;
            dst.height = src.height;
            dst.explored.resize(src.explored.size(), false);
        }
        if (dst.width != src.width || dst.height != src.height) {
            continue;  // UNKNOWN: differing level shapes are not merged
        }
        const i64 dstCells = static_cast<i64>(dst.explored.size());
        const i64 srcCells = static_cast<i64>(src.explored.size());
        const i64 cells = dstCells < srcCells ? dstCells : srcCells;
        for (i64 i = 0; i < cells; ++i) {
            if (src.explored[i]) {
                dst.explored[i] = true;  // R9.1: only grow
            }
        }
    }
}

// R9.2: a legacy Character.Explored map is absorbed for older saves.
// UNKNOWN: the SRD does not say how Character.Explored is absorbed in detail. It is assumed that
// each legacy level uses the same y * width + x indexing, and that its height is derived as
// cells / width, rounded up.
void absorbLegacyExplored(Ctx& ctx,
                          SharedMap& map,
                          const Vec<Vec<bool>>& legacyExplored,
                          const Vec<i32>& legacyLevelWidths) {
    SharedMap incoming;
    const i64 levelCount = static_cast<i64>(legacyExplored.size());
    const i64 widthCount = static_cast<i64>(legacyLevelWidths.size());
    const i64 count = levelCount < widthCount ? levelCount : widthCount;
    for (i64 lv = 0; lv < count; ++lv) {
        const i32 width = legacyLevelWidths[lv];
        if (width <= 0) {
            continue;
        }
        const Vec<bool>& cells = legacyExplored[lv];
        const i64 cellCount = static_cast<i64>(cells.size());
        AutomapLevel level;
        level.width = width;
        level.height = static_cast<i32>((cellCount + width - 1) / width);
        level.explored = cells;
        while (static_cast<i64>(incoming.levels.size()) <= lv) {
            incoming.levels.push_back(AutomapLevel{});
        }
        incoming.levels[lv] = level;
    }
    mergeSharedMap(ctx, map, incoming);  // R9.1 rule applies
}

// R10.1: levels with no explored cells are not shown. QUIRK (kept by the port): the original
// does not show levels nobody has reached.
bool levelShownOnAutomap(Ctx& /*ctx*/, const SharedMap& map, i32 level) {
    if (level < 0 || static_cast<i64>(level) >= static_cast<i64>(map.levels.size())) {
        return false;
    }
    const AutomapLevel& lv = map.levels[level];
    const i64 cells = static_cast<i64>(lv.explored.size());
    for (i64 i = 0; i < cells; ++i) {
        if (lv.explored[i]) {
            return true;
        }
    }
    return false;
}

// R10.2: DeepestVisited is the maximum level with any explored cells.
// UNKNOWN: the SRD does not say what is returned when no level is explored. This returns -1.
i32 deepestVisited(Ctx& ctx, const SharedMap& map) {
    for (i64 level = static_cast<i64>(map.levels.size()) - 1; level >= 0; --level) {
        if (levelShownOnAutomap(ctx, map, static_cast<i32>(level))) {
            return static_cast<i32>(level);
        }
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Save folder (SRD R11)
// ---------------------------------------------------------------------------

// R11: MORDOR_SAVES if set, else the legacy gameDir\..\saves if it is non-empty,
// else %APPDATA%\OpenDejenol\saves.
SaveFolderChoice resolveSaveFolder(Ctx& ctx, const SaveFolderProbe& probe) {
    SaveFolderChoice choice;
    if (probe.overrideSet) {  // R11.1
        choice.source = SaveFolderSource::Override;
        choice.path = probe.overridePath;
    } else if (probe.legacyNonEmpty) {  // R11.2
        choice.source = SaveFolderSource::Legacy;
        choice.path = probe.legacyPath;
    } else {  // R11.3
        choice.source = SaveFolderSource::AppData;
        choice.path = probe.appDataPath;
    }
    ctx.say("options.save_folder", "Save folder in use", {}, {choice.path});
    return choice;
}

// ---------------------------------------------------------------------------
// Save import (SRD R12)
// ---------------------------------------------------------------------------

// R12: every MDATA file and MDATA4 record 1 must carry version "1.1". Any other stamp refuses.
// UNKNOWN: the SRD does not say what an empty list of stamps means. This returns true (every
// stamp is checked, and none fails).
bool importAllowed(Ctx& ctx, const Vec<Str>& stamps) {
    const Str required(kImportVersion);
    const i64 count = static_cast<i64>(stamps.size());
    for (i64 i = 0; i < count; ++i) {
        if (!(stamps[i] == required)) {
            ctx.say("options.import_refused", "Import refused: version stamp is not 1.1", {}, {stamps[i]});
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Limited (shareware) build (SRD R13-R15 and the switch table)
// ---------------------------------------------------------------------------

// Switch values per the SRD table. Full build: m009C = 0 (goodbye box shown, title not),
// everything else unset or never set. Limited build: the switches are on.
BuildSwitches buildSwitchesFor(BuildVariant variant) {
    BuildSwitches sw;
    if (variant == BuildVariant::Full) {
        sw.goodbyeBox = true;          // m009C = 0: goodbye box asking to pass on shareware
        sw.limitedTitle = false;
        sw.takeDenied = false;
        sw.deadChuteLines = false;
        sw.aboutOrderingInfo = false;
        sw.aboutOrdering = false;
        sw.limitationNotice = false;   // gv0180 never set in the full build
        sw.limitedSplashPrefix = false;  // m002E never set in the full build
        sw.writesLimitedFlag = false;  // gv0180 never set in the full build
    } else {
        sw.goodbyeBox = false;         // m009C True: goodbox box shown only when False
        sw.limitedTitle = true;        // m009C True: title " (Limited Version)"
        sw.takeDenied = true;          // m0622 True
        sw.deadChuteLines = true;      // m02A6 True
        sw.aboutOrderingInfo = true;   // m0032 True
        sw.aboutOrdering = true;       // m00A8 True
        sw.limitationNotice = true;    // gv0180 set
        sw.limitedSplashPrefix = true; // m002E: "Limited " prefix on splash
        sw.writesLimitedFlag = true;   // gv0180 writes Limited=True
    }
    return sw;
}

// R14: the limited release reads Limited from MORDOR.INI [Loading Information] and, if set,
// shows the limited splash.
// UNKNOWN: whether the full build reads Limited at run time. The SRD notes are in tension (see
// the Tension note). The full build is treated as never showing the limited splash (m002E is never
// set there). Order of this check against other startup steps is also UNKNOWN.
bool limitedSplashShown(Ctx& /*ctx*/, BuildVariant variant, const IniDocument& ini) {
    if (!buildSwitchesFor(variant).limitedSplashPrefix) {
        return false;
    }
    return iniFlagValue(ini, Str(kLimitedFlagSection), Str(kLimitedFlagKey));
}

// R15: the Limitation Notice shows the number of levels from gc0182. The full build value is 15.
// UNKNOWN: the limited build value of gc0182 is not given. The full value is used for both builds.
i32 limitationNoticeLevels(Ctx& /*ctx*/, BuildVariant /*variant*/) {
    return kFullBuildLevelCap;
}

// gv0180: the Limitation Notice ("PUBLIC BETA ... only n dungeon levels") shows in the limited
// build only, and it writes Limited=True to MORDOR.INI. The full build never clears Limited=True
// (SRD quirk), and this function does not clear it either.
void showLimitationNotice(Ctx& ctx, BuildVariant variant, IniDocument& ini) {
    const BuildSwitches sw = buildSwitchesFor(variant);
    if (!sw.limitationNotice) {
        return;
    }
    const i32 levels = limitationNoticeLevels(ctx, variant);
    // Assumes ctx.say's nums parameter is a Vec<i64>.
    ctx.say("options.limitation_notice",
            "PUBLIC BETA ... only n dungeon levels, where n is nums[0]",
            {static_cast<i64>(levels)},
            {});
    if (sw.writesLimitedFlag) {
        iniSet(ini, Str(kLimitedFlagSection), Str(kLimitedFlagKey), Str(kTextTrue));
    }
}

// Options grouping: flags, volumes and fonts. Assumed: Vec and Arr behave as std-like containers.
// Dead-code helper removed; all declared functions are defined above.

}  // namespace mordor
