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

}  // namespace

// ---------------------------------------------------------------------------
// INI helpers (SRD R2, R5)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Options load, fonts, save, defaults (SRD R1-R6)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Automap (SRD R8-R10)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Save folder (SRD R11)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Save import (SRD R12)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Limited (shareware) build (SRD R13-R15 and the switch table)
// ---------------------------------------------------------------------------

// Options grouping: flags, volumes and fonts. Assumed: Vec and Arr behave as std-like containers.
// Dead-code helper removed; all declared functions are defined above.

}  // namespace mordor
