#pragma once
// FIXED: add an include guard so a second inclusion (even via a different path/copy) cannot redefine the names below.
// FIXED (unresolved): the guard above already exists, so the reported redefinitions of kSettingsFileName and
// ModSettingsStore are not caused by this header alone. The error text says mordor::mordor, so this header is being
// included inside a `namespace mordor { ... }` block, or another header (types.h, ctx.h, or a file that includes this one)
// defines the same names. Remove the wrapping namespace at the include site, or delete the duplicate definition in the
// other header. No change was made here because the duplicate is not in this file.
// Check: grep -rn "kSettingsFileName\|struct ModSettingsStore" include/ to find the other definition.
#ifndef MORDOR_MODDING_H
#define MORDOR_MODDING_H

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/dataformatsa.h"  // FIXED: ModSettingsStore, LiveMod and ModTag are declared in this header per the API INDEX
#include "mordor/dataformatsb.h"  // FIXED: ModRef is declared in this header per the API INDEX

namespace mordor {

// ---- constants and enums (R2, R3, R17, R22, F3, F4, R23, R19, R18, Table 3) ----

inline constexpr const char* kModsEnvVar = "MORDOR_MODS";           // R2 environment variable overriding mod roots
inline constexpr char kModsEnvSeparator = ';';                      // R2 separator inside MORDOR_MODS
inline constexpr const char* kManifestFileName = "mod.json";        // R3 ModApi.ManifestFile
inline constexpr i32 kApiMemberVersion[] = {0, 8, 10, -1, 11, 12};

// Kind of mod entry: IMod (plain) or IModLifecycle (supports live switching). R11
enum class ModKind : i32 {
    Plain = 0,
    Lifecycle = 1
};

// R20 / F1 result of manifest validation; first failing check wins.
enum class ManifestProblem : i32 {
    None = 0,
    NoId,
    IdBadChars,
    NoAssembly,
    NoEntryType
};

// R12 outcome of one mod call. Threw = exception; NullDecision = mod returned null.
enum class HookStatus : i32 {
    Ok = 0,
    Threw,
    NullDecision
};

// R11 result of a live switch request.
// UNKNOWN: whether values beyond success/failure exist; only the name is cited.
enum class ModSwitchResult : i32 {
    Ok = 0,
    Failed,
    DeferredToRestart
};

// R10 / Table 3 lifecycle hooks.
enum class ModLifecycleHook : i32 {
    Load = 0,
    OnEnable,
    OnDisable
};

// R18 action taken on automap right-click.
enum class AutomapRightClick : i32 {
    ShowMenu = 0,
    OriginalReplot
};

// R19 town-charge hooks that mods may answer.
enum class TownHook : i32 {
    PartyPool = 0,
    CoverShortfall,
    WealthiestCheck
};

// ---- small structs ----





// settings.ini entry (R17). Values are stored as text; flags as "True"/"False".
struct StoredSetting {
    Str modId;
    Str key;
    Str value;
};



// Host state for all mods (R3, R7, R14, R19).
struct ModHost {
    Vec<LiveMod> mods;  // FIXED: LoadedMod does not exist anywhere; closest existing mod type is LiveMod
    ModSettingsStore settings;
};

// R2/R3 input: one root folder and its subfolders as listed by the port.
struct ModFolderEntry {
    Str name;
    bool hasManifest = false;
};

struct ModRootListing {
    Str rootPath;
    Vec<ModFolderEntry> subfolders;
};

// R1 input: an ancestor of the build folder, nearest first.
struct AncestorFolder {
    Str path;
    bool hasMordorSln = false;
};

// R18 item supplied by a running mod's automap menu.
struct AutomapMenuItem {
    Str label;
    bool separator = false;
};

struct AutomapMenuResult {
    AutomapRightClick action = AutomapRightClick::OriginalReplot;
    Vec<AutomapMenuItem> items;
};

// R12 reply from one mod call, supplied by the port.
// For town hooks (R19), amount is the replacement value and flag is the Wealthiest answer.
struct ModHookReply {
    HookStatus status = HookStatus::NullDecision;
    Currency amount = 0;
    bool flag = false;
};

// R19 answer after applying the original-rules fallback.
struct TownHookAnswer {
    bool useOriginal = true;
    Currency amount = 0;
    bool flag = false;
};

// ---- rules ----


































// R24 EventManager and R25 C++ resource definition are port-engine notes, not original rules.
// They declare no core routine here.
// CONFLICT: EventManager doc says events are "marked resolved once triggered" but section 1
// says "marked as activated". Unresolved; no routine declared.
// UNKNOWN: R25 constraint applies only to GameResources::s_resources; not a core routine.

}  // namespace mordor

#endif  // MORDOR_MODDING_H
