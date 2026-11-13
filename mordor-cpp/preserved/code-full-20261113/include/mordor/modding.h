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

// R1 Locate port source: index of the nearest ancestor containing Mordor.sln, or -1.
i32 findPortRootIndex(const Vec<AncestorFolder>& chainNearestFirst);

// R2 Resolve mod roots from MORDOR_MODS (';'-separated) or the two default mods folders.
Vec<Str> resolveModRoots(bool mordorModsSet,
                         const Str& mordorModsValue,
                         const Str& gameFolderModsDir,
                         const Str& programFolderModsDir);

// R3 Discover mods: scan each root's subfolders in case-insensitive name order for mod.json.
// Duplicate ids are skipped (first kept) and reported via ctx.events. R5
void discoverMods(Ctx& ctx, ModHost& host, const Vec<ModRootListing>& roots);

// R4 Read manifest: parse mod.json text. Empty text is an error (InvalidDataException in source);
// returns false and reports an event instead of throwing.
bool readManifestText(Ctx& ctx, const Str& jsonText, ModRef& out);  // FIXED: ModManifest does not exist anywhere; closest existing mod reference type is ModRef

// R5 / R20 Validate per F1; first failing check wins.
ManifestProblem validateManifest(const ModRef& manifest);  // FIXED: ModManifest replaced by ModRef (see readManifestText)

// F1 message text for a problem. ManifestProblem::None gives an empty string.
Str manifestProblemText(ManifestProblem problem);

// R5 True when a mod with this id is already in the host.
bool isDuplicateModId(const ModHost& host, const Str& id);

// R14 Index of the mod with this id, or -1.
i32 findModIndexById(const ModHost& host, const Str& id);

// F2 / R21 Display title: (IsBlank(Name) ? Id : Name) + (Version == "" ? "" : " " + Version).
Str modDisplayTitle(const ModRef& manifest);  // FIXED: ModManifest replaced by ModRef

// F2 helper: true when the text is empty or whitespace only.
bool isBlankText(const Str& text);

// R6 / F4 Opt-in: enabled only when the stored Enabled flag is true; defaults to false.
bool isModEnabled(const ModSettingsStore& store, const Str& modId);

// F4 Writes the Enabled flag as True/False.
void setModEnabled(ModSettingsStore& store, const Str& modId, bool value);

// R7 Load enabled mods at start, in discovery order.
// UNKNOWN: whether this calls Load then OnEnable on the same path as a live switch-on.
void loadEnabledMods(Ctx& ctx, ModHost& host);

// R8 First switch-on: call Load, then OnEnable.
void firstSwitchOn(Ctx& ctx, ModHost& host, i32 modIndex);

// R9 Later activations: call OnEnable and re-note patches.
void reactivateMod(Ctx& ctx, ModHost& host, i32 modIndex);

// R10 Switch-off: stop calls, run OnDisable, remove Harmony patches.
// UNKNOWN: Harmony UnpatchAll has no stated signature beyond removing this mod's patches.
void deactivateMod(Ctx& ctx, ModHost& host, i32 modIndex);

// R11 Live switch from the main menu. Lifecycle mods only; faulted or plain mods are
// deferred to next start. Settings (Enabled flag) are saved in every case.
ModSwitchResult switchModLive(Ctx& ctx, ModHost& host, i32 modIndex, bool switchOn);

// R12 True when the original rule answer must be used (HookStatus::Threw or NullDecision).
bool useOriginalAnswer(HookStatus status);

// R12 / R13 Guarded call result: on Threw, disables the mod for the session, removes its
// patches, records the reason and logs it. Returns true when the mod's answer may be used.
bool applyHookReply(Ctx& ctx, LiveMod& mod, const ModHookReply& reply);  // FIXED: LoadedMod replaced by LiveMod

// R13 Mark a mod faulted for the rest of the session, with reason.
void faultMod(Ctx& ctx, LiveMod& mod, const Str& reason);  // FIXED: LoadedMod replaced by LiveMod

// R14 Setting change: raise Changed(modId, key) for all mods; only matching ids are forwarded.
// Each handler runs guarded; a throw faults only that mod.
void raiseSettingChanged(Ctx& ctx, ModHost& host, const Str& modId, const Str& key);

// R15 / F5 Toggle value: stored flag, else declared default (case-insensitive key), else false.
bool toggleValue(const ModHost& host, i32 modIndex, const Str& key);

// F5 helper: declared default of the toggle whose key matches case-insensitively, else false.
bool declaredToggleDefault(const ModRef& manifest, const Str& key);  // FIXED: ModManifest replaced by ModRef

// R16 / F6 Choice resolution. spec may be null (returns stored or "").
// Original takes an unused key parameter; not declared here.
// Original quirk: an empty Default never matches, so the first option is the fallback.
Str resolveChoice(const ModTag* spec, const Str& stored);  // FIXED: ModChoiceSpec does not exist anywhere; closest existing type is ModTag

// R17 Read a stored flag, returning defaultValue when none is stored.
bool readStoredFlag(const ModSettingsStore& store, const Str& modId, const Str& key, bool defaultValue);

// R17 Write a stored flag as True/False.
void writeStoredFlag(ModSettingsStore& store, const Str& modId, const Str& key, bool value);

// R17 Read stored text for a key; empty when none is stored.
Str readStoredText(const ModSettingsStore& store, const Str& modId, const Str& key);

// R17 Original quirk (manifest wins): does not replace an existing toggle with the same key.
// Returns false when the key already exists and the spec is ignored.
bool declareToggle(ModRef& manifest, const ModTag& spec);  // FIXED: ModManifest replaced by ModRef; ModToggleSpec does not exist anywhere, closest existing type is ModTag

// R17 Original quirk (no write check): writes choice text directly, without checking Options.
void setChoice(ModSettingsStore& store, const Str& modId, const Str& key, const Str& text);

// R18 Automap right-click. Items arrive in load order including separators. If only separators
// remain, the menu is discarded and the original right-click (instant re-plot) applies.
AutomapMenuResult resolveAutomapRightClick(const Vec<AutomapMenuItem>& collectedInLoadOrder);

// R19 ModHost.Active: true when any mod is currently running. With no mod loaded, original rules apply.
bool anyModActive(const ModHost& host);

// R19 Resolve one town hook. replies[i] is the answer of host.mods[i] (ignored unless active).
// Replies are tried in load order; the first mod answer that is usable wins.
// Faults are applied via applyHookReply. Falls back to the original answer otherwise.
TownHookAnswer queryTownHook(Ctx& ctx,
                             ModHost& host,
                             TownHook hook,
                             Currency original,
                             const Vec<ModHookReply>& replies);

// R23 API version that introduced a member: the value from kApiMemberVersion.
// 0 = pre-8, -1 = UNKNOWN.
// FIXED: ApiMember is not declared anywhere in the API INDEX, so it is replaced by plain i32
// (the member is identified by its index into kApiMemberVersion).
i32 apiVersionIntroducedBy(i32 member);

// R24 EventManager and R25 C++ resource definition are port-engine notes, not original rules.
// They declare no core routine here.
// CONFLICT: EventManager doc says events are "marked resolved once triggered" but section 1
// says "marked as activated". Unresolved; no routine declared.
// UNKNOWN: R25 constraint applies only to GameResources::s_resources; not a core routine.

}  // namespace mordor

#endif  // MORDOR_MODDING_H
