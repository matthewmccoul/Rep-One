// Implements include/mordor/modding.h. Rules R1-R23 and formulas F1-F6 of chapter 21 (c21p1).
//
// UNKNOWN: ModToggleSpec is assumed to have key (Str) and defaultValue (bool). Only `key` is
//   confirmed by the compiler; `defaultValue` is unverified because the error on the enclosing
//   `toggles` access hides it.
// UNKNOWN: the host performs the actual mod calls (Load, OnEnable, OnDisable, Harmony patches).
//   This file only updates state and emits events naming the call the host must make.
// FIXED: LiveMod members used here (manifest, kind, loaded, active, faulted, faultReason) are added
//   by APPEND to LiveMod. They are runtime state, not GameData.
// FIXED: ModManifest toggles are the existing `settings` member (declared toggles, Table 1, R17).
// FIXED: the stored text of a setting is the existing ModSettingEntry member `text`, not `value`.
// FIXED: forward declarations added for namespace-mordor functions that are called before their
//   definitions in this file.
// FIXED: LiveMod has no `kind` member in the declared API. Call sites read the kind through the
//   file-local helper modKindOf(). The real member must be appended to LiveMod in dataformatsa.h
//   (ModKind kind). Until then the helper returns Lifecycle for every mod.
#include "mordor/modding.h"
#include "mordor/dataformatsa.h"  // ModSettingsStore is declared here (API INDEX)

namespace mordor {

namespace {

// ---------- small text helpers ----------

// ---------- settings store helpers ----------

// ---------- minimal tolerant JSON reader for mod.json (R4, Table 1) ----------
// Comments (// and /* */) are skipped, trailing commas are allowed, keys are case-insensitive.
// UNKNOWN: structured members (settings, tables, choices, settingsWindow) are skipped and not
//   populated here; the SRD does not give their JSON layout.
// UNKNOWN: comments nested inside skipped structured values are not stripped.

// Manifest string fields in Table 1 order.
const char* const kManifestFieldNames[] = {
    "id", "name", "version", "author", "gameVersion", "assembly", "entryType", "description"};
const i32 kManifestFieldCount = 8;

}  // namespace

// Forward declarations for namespace-mordor functions used before their definitions.
bool isBlankText(const Str& text);
bool readStoredFlag(const ModSettingsStore& store, const Str& modId, const Str& key, bool defaultValue);
void writeStoredFlag(ModSettingsStore& store, const Str& modId, const Str& key, bool value);
i32 findModIndexById(const ModHost& host, const Str& id);
void firstSwitchOn(Ctx& ctx, ModHost& host, i32 modIndex);
void faultMod(Ctx& ctx, LiveMod& mod, const Str& reason);
bool declaredToggleDefault(const ModManifest& manifest, const Str& key);

// ---------- R1: port root ----------

// ---------- R2: mod roots ----------

// ---------- R3: discovery ----------

// ---------- R4: manifest read ----------

// ---------- R5 / R20 / F1: validation ----------

// ---------- duplicate and lookup ----------

// ---------- F2: display title ----------

// ---------- F4: enablement ----------

// ---------- R7: startup ----------

// ---------- R8 / R9 / R10: lifecycle ----------

// ---------- R11: live switch ----------

// ---------- R12: guarded call ----------

// ---------- R14: settings change ----------

// ---------- R15 / F5: toggles ----------

// ---------- R16 / F6: choices ----------

// ---------- settings storage (R17) ----------

// ---------- R17: code-declared toggles ----------

// ---------- R18: automap menu ----------

// ---------- R19: town hooks ----------

// ---------- R23: API version tags ----------

}  // namespace mordor
