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

i32 countChars(const Str& s) {
    return static_cast<i32>(s.size());
}

template <typename T>
i32 countOf(const Vec<T>& v) {
    return static_cast<i32>(v.size());
}

bool validIndex(const ModHost& host, i32 idx) {
    return idx >= 0 && idx < countOf(host.mods);
}

char lowerAscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

bool isBlankChar(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\v' || c == '\f';
}

Str trimText(const Str& s) {
    i32 begin = 0;
    i32 end = countChars(s);
    while (begin < end && isBlankChar(s[begin])) {
        ++begin;
    }
    while (end > begin && isBlankChar(s[end - 1])) {
        --end;
    }
    Str out;
    for (i32 i = begin; i < end; ++i) {
        out += s[i];
    }
    return out;
}

// Case-insensitive (ASCII) equality of two strings.
bool equalsCi(const Str& a, const Str& b) {
    const i32 n = countChars(a);
    if (n != countChars(b)) {
        return false;
    }
    for (i32 i = 0; i < n; ++i) {
        if (lowerAscii(a[i]) != lowerAscii(b[i])) {
            return false;
        }
    }
    return true;
}

// Case-insensitive equality against a C string literal.
bool equalsCiLiteral(const Str& a, const char* b) {
    i32 i = 0;
    while (b[i] != '\0') {
        if (i >= countChars(a)) {
            return false;
        }
        if (lowerAscii(a[i]) != lowerAscii(b[i])) {
            return false;
        }
        ++i;
    }
    return i == countChars(a);
}

// R3: case-insensitive name order.
bool lessCi(const Str& a, const Str& b) {
    const i32 na = countChars(a);
    const i32 nb = countChars(b);
    for (i32 i = 0; i < na && i < nb; ++i) {
        const char ca = lowerAscii(a[i]);
        const char cb = lowerAscii(b[i]);
        if (ca != cb) {
            return ca < cb;
        }
    }
    return na < nb;
}

bool containsAny(const Str& s, const char* set) {
    for (i32 i = 0; i < countChars(s); ++i) {
        for (i32 k = 0; set[k] != '\0'; ++k) {
            if (s[i] == set[k]) {
                return true;
            }
        }
    }
    return false;
}

// ---------- settings store helpers ----------

// Exact match on modId and key (storage lookup). UNKNOWN: storage key case sensitivity is not stated.
i32 findStored(const ModSettingsStore& store, const Str& modId, const Str& key) {
    for (i32 i = 0; i < countOf(store.entries); ++i) {
        if (store.entries[i].modId == modId && store.entries[i].key == key) {
            return i;
        }
    }
    return -1;
}

void writeStoredText(ModSettingsStore& store, const Str& modId, const Str& key, const Str& value) {
    const i32 idx = findStored(store, modId, key);
    if (idx >= 0) {
        store.entries[idx].text = value;
        return;
    }
    ModSettingEntry entry;  // ModSettingEntry is the element type of ModSettingsStore::entries
    entry.modId = modId;
    entry.key = key;
    entry.text = value;
    store.entries.push_back(entry);
}

// F5 DefaultFor: first toggle whose key matches case-insensitively, else -1.
i32 findToggleIndexCi(const ModManifest& manifest, const Str& key) {
    for (i32 i = 0; i < countOf(manifest.settings); ++i) {
        if (equalsCi(manifest.settings[i].key, key)) {
            return i;
        }
    }
    return -1;
}

// F6 match(x): case-insensitive lookup returning the declared option index, or -1.
// An empty input never matches (R16 quirk: an empty Default never matches).
i32 matchOption(const Vec<Str>& options, const Str& x) {
    if (x.empty()) {
        return -1;
    }
    for (i32 i = 0; i < countOf(options); ++i) {
        if (equalsCi(options[i], x)) {
            return i;
        }
    }
    return -1;
}

// ---------- minimal tolerant JSON reader for mod.json (R4, Table 1) ----------
// Comments (// and /* */) are skipped, trailing commas are allowed, keys are case-insensitive.
// UNKNOWN: structured members (settings, tables, choices, settingsWindow) are skipped and not
//   populated here; the SRD does not give their JSON layout.
// UNKNOWN: comments nested inside skipped structured values are not stripped.

bool isJsonWs(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

void jsonSkipWs(const Str& t, i32& pos) {
    const i32 n = countChars(t);
    bool moved = true;
    while (moved) {
        moved = false;
        while (pos < n && isJsonWs(t[pos])) {
            ++pos;
            moved = true;
        }
        if (pos + 1 < n && t[pos] == '/' && t[pos + 1] == '/') {
            while (pos < n && t[pos] != '\n') {
                ++pos;
            }
            moved = true;
        } else if (pos + 1 < n && t[pos] == '/' && t[pos + 1] == '*') {
            pos += 2;
            while (pos + 1 < n && !(t[pos] == '*' && t[pos + 1] == '/')) {
                ++pos;
            }
            pos += 2;
            if (pos > n) {
                pos = n;
            }
            moved = true;
        }
    }
}

bool jsonParseString(const Str& t, i32& pos, Str& out) {
    const i32 n = countChars(t);
    out = Str();
    if (pos >= n || t[pos] != '"') {
        return false;
    }
    ++pos;
    while (pos < n) {
        const char c = t[pos];
        if (c == '"') {
            ++pos;
            return true;
        }
        if (c == '\\') {
            if (pos + 1 >= n) {
                return false;
            }
            const char e = t[pos + 1];
            pos += 2;
            if (e == 'n') {
                out += '\n';
            } else if (e == 't') {
                out += '\t';
            } else if (e == 'r') {
                out += '\r';
            } else if (e == 'b') {
                out += '\b';
            } else if (e == 'f') {
                out += '\f';
            } else if (e == 'u') {
                // UNKNOWN: \uXXXX escapes are replaced by '?'; the SRD does not say.
                if (pos + 4 > n) {
                    return false;
                }
                pos += 4;
                out += '?';
            } else {
                out += e;  // covers \" \\ and \/
            }
            continue;
        }
        out += c;
        ++pos;
    }
    return false;
}

// Skips one JSON value starting at pos (pos must already be at the value start).
bool jsonSkipValue(const Str& t, i32& pos) {
    const i32 n = countChars(t);
    if (pos >= n) {
        return false;
    }
    const char c = t[pos];
    if (c == '"') {
        Str discard;
        return jsonParseString(t, pos, discard);
    }
    if (c == '{' || c == '[') {
        i32 depth = 0;
        while (pos < n) {
            const char d = t[pos];
            if (d == '"') {
                Str discard;
                if (!jsonParseString(t, pos, discard)) {
                    return false;
                }
                continue;
            }
            if (d == '{' || d == '[') {
                ++depth;
            } else if (d == '}' || d == ']') {
                --depth;
                if (depth == 0) {
                    ++pos;
                    return true;
                }
            }
            ++pos;
        }
        return false;
    }
    // Bare token: number, true, false, null.
    const i32 start = pos;
    while (pos < n && !isJsonWs(t[pos]) && t[pos] != ',' && t[pos] != '}' && t[pos] != ']' &&
           t[pos] != ':') {
        ++pos;
    }
    return pos > start;
}

// Manifest string fields in Table 1 order.
const char* const kManifestFieldNames[] = {
    "id", "name", "version", "author", "gameVersion", "assembly", "entryType", "description"};
const i32 kManifestFieldCount = 8;

i32 manifestFieldIndex(const Str& key) {
    for (i32 i = 0; i < kManifestFieldCount; ++i) {
        if (equalsCiLiteral(key, kManifestFieldNames[i])) {
            return i;
        }
    }
    return -1;
}

void assignManifestField(ModManifest& m, i32 field, const Str& value) {
    switch (field) {
        case 0: m.id = value; break;
        case 1: m.name = value; break;
        case 2: m.version = value; break;
        case 3: m.author = value; break;
        case 4: m.gameVersion = value; break;
        case 5: m.assembly = value; break;
        case 6: m.entryType = value; break;
        case 7: m.description = value; break;
        default: break;
    }
}

bool manifestBad(Ctx& ctx) {
    ctx.say("mod.manifest_bad", "mod.json could not be parsed", {}, {});
    return false;
}

// FIXED: stands in for the missing LiveMod::kind member (see header comment above).
// UNKNOWN: the real kind is not available in this file. Every mod is treated as Lifecycle.
ModKind modKindOf(const LiveMod& mod) {
    (void)mod;
    return ModKind::Lifecycle;
}

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

i32 findPortRootIndex(const Vec<AncestorFolder>& chainNearestFirst) {
    // R1: PortTree.Root is the nearest ancestor folder that contains Mordor.sln.
    // UNKNOWN: no-match result is -1; the SRD does not say what the caller does then.
    for (i32 i = 0; i < countOf(chainNearestFirst); ++i) {
        if (chainNearestFirst[i].hasMordorSln) {
            return i;
        }
    }
    return -1;
}

// ---------- R2: mod roots ----------

Vec<Str> resolveModRoots(bool mordorModsSet,
                         const Str& mordorModsValue,
                         const Str& gameFolderModsDir,
                         const Str& programFolderModsDir) {
    Vec<Str> roots;
    if (mordorModsSet) {
        // R2.1: use the ';'-separated paths in MORDOR_MODS.
        // UNKNOWN: empty and blank segments are skipped and segments are trimmed; the SRD does
        //   not say. A blank MORDOR_MODS value therefore yields no roots.
        Str current;
        const i32 n = countChars(mordorModsValue);
        for (i32 i = 0; i <= n; ++i) {
            const bool atEnd = (i == n);
            if (atEnd || mordorModsValue[i] == kModsEnvSeparator) {
                const Str piece = trimText(current);
                if (!piece.empty()) {
                    roots.push_back(piece);
                }
                current.clear();
            } else {
                current += mordorModsValue[i];
            }
        }
        return roots;
    }
    // R2.2: mods folder beside the game folder, then beside the program.
    roots.push_back(gameFolderModsDir);
    roots.push_back(programFolderModsDir);
    return roots;
}

// ---------- R3: discovery ----------

void discoverMods(Ctx& ctx, ModHost& host, const Vec<ModRootListing>& roots) {
    // R3: scan each root's subfolders in case-insensitive name order for mod.json.
    // UNKNOWN: ModRootListing carries only folder names and a hasManifest flag, not the text of
    //   mod.json. This function fixes the discovery order and reports each candidate. The host
    //   reads the file and then calls readManifestText, validateManifest and isDuplicateModId
    //   (R5) before adding the mod to host.mods.
    (void)host;
    for (i32 r = 0; r < countOf(roots); ++r) {
        // Copy, then insertion-sort the copy by case-insensitive name.
        Vec<ModFolderEntry> folders = roots[r].subfolders;
        const i32 n = countOf(folders);
        for (i32 i = 1; i < n; ++i) {
            ModFolderEntry key = folders[i];
            i32 j = i - 1;
            while (j >= 0 && lessCi(key.name, folders[j].name)) {
                folders[j + 1] = folders[j];
                --j;
            }
            folders[j + 1] = key;
        }
        for (i32 i = 0; i < n; ++i) {
            if (folders[i].hasManifest) {
                ctx.say("mod.discovered", "Mod folder with mod.json found",
                        {static_cast<i64>(r)}, {folders[i].name, roots[r].rootPath});
            }
        }
    }
}

// ---------- R4: manifest read ----------

bool readManifestText(Ctx& ctx, const Str& jsonText, ModManifest& out) {
    // R4: an empty file is an error. The SRD throws InvalidDataException; this port has no
    // exceptions, so it reports an event and returns false.
    if (countChars(jsonText) == 0) {
        ctx.say("mod.manifest_empty", "mod.json is empty", {}, {});
        return false;
    }
    const i32 n = countChars(jsonText);
    ModManifest parsed;
    i32 pos = 0;
    jsonSkipWs(jsonText, pos);
    if (pos >= n || jsonText[pos] != '{') {
        return manifestBad(ctx);
    }
    ++pos;
    while (true) {
        jsonSkipWs(jsonText, pos);
        if (pos < n && jsonText[pos] == '}') {  // also covers a trailing comma
            ++pos;
            break;
        }
        Str key;
        if (!jsonParseString(jsonText, pos, key)) {
            return manifestBad(ctx);
        }
        jsonSkipWs(jsonText, pos);
        if (pos >= n || jsonText[pos] != ':') {
            return manifestBad(ctx);
        }
        ++pos;
        jsonSkipWs(jsonText, pos);
        const i32 field = manifestFieldIndex(key);
        if (field >= 0 && pos < n && jsonText[pos] == '"') {
            Str value;
            if (!jsonParseString(jsonText, pos, value)) {
                return manifestBad(ctx);
            }
            assignManifestField(parsed, field, value);
        } else {
            if (!jsonSkipValue(jsonText, pos)) {
                return manifestBad(ctx);
            }
        }
        jsonSkipWs(jsonText, pos);
        if (pos < n && jsonText[pos] == ',') {
            ++pos;
            continue;
        }
        if (pos < n && jsonText[pos] == '}') {
            ++pos;
            break;
        }
        return manifestBad(ctx);
    }
    out = parsed;
    return true;
}

// ---------- R5 / R20 / F1: validation ----------

ManifestProblem validateManifest(const ModManifest& manifest) {
    // F1, first failure wins.
    if (isBlankText(manifest.id)) {
        return ManifestProblem::NoId;
    }
    if (containsAny(manifest.id, "[]=\r\n")) {
        return ManifestProblem::IdBadChars;
    }
    if (isBlankText(manifest.assembly)) {
        return ManifestProblem::NoAssembly;
    }
    if (isBlankText(manifest.entryType)) {
        return ManifestProblem::NoEntryType;
    }
    return ManifestProblem::None;
}

Str manifestProblemText(ManifestProblem problem) {
    // F1 messages. UNKNOWN: None is the F1 null result; it is returned here as "".
    switch (problem) {
        case ManifestProblem::NoId: return Str("no id");
        case ManifestProblem::IdBadChars: return Str("the id may not contain [ ] or =");
        case ManifestProblem::NoAssembly: return Str("no assembly");
        case ManifestProblem::NoEntryType: return Str("no entryType");
        case ManifestProblem::None: return Str("");
    }
    return Str("");
}

// ---------- duplicate and lookup ----------

bool isDuplicateModId(const ModHost& host, const Str& id) {
    // R5: a duplicate id is skipped and the first one found is kept.
    return findModIndexById(host, id) >= 0;
}

i32 findModIndexById(const ModHost& host, const Str& id) {
    // UNKNOWN: id comparison is exact (case-sensitive); the SRD does not say.
    for (i32 i = 0; i < countOf(host.mods); ++i) {
        if (host.mods[i].id == id) {
            return i;
        }
    }
    return -1;
}

// ---------- F2: display title ----------

Str modDisplayTitle(const ModManifest& manifest) {
    // F2: Title = (IsBlank(Name) ? Id : Name) + (Version == "" ? "" : " " + Version)
    Str title = isBlankText(manifest.name) ? manifest.id : manifest.name;
    if (!manifest.version.empty()) {
        title += " ";
        title += manifest.version;
    }
    return title;
}

bool isBlankText(const Str& text) {
    // F1/F2 IsBlank: empty or whitespace only.
    for (i32 i = 0; i < countChars(text); ++i) {
        if (!isBlankChar(text[i])) {
            return false;
        }
    }
    return true;
}

// ---------- F4: enablement ----------

bool isModEnabled(const ModSettingsStore& store, const Str& modId) {
    // F4: IsEnabled(id) = ReadFlag(id, "Enabled", false)
    return readStoredFlag(store, modId, Str("Enabled"), false);
}

void setModEnabled(ModSettingsStore& store, const Str& modId, bool value) {
    // F4: SetEnabled writes "Enabled" as True or False.
    writeStoredFlag(store, modId, Str("Enabled"), value);
}

// ---------- R7: startup ----------

void loadEnabledMods(Ctx& ctx, ModHost& host) {
    // R6: only mods whose Enabled flag is true are loaded (opt-in).
    // R5: a mod with a validation problem is listed but not loaded.
    // R7: loaded in discovery order.
    // UNKNOWN: whether startup calls Load then OnEnable along the same path as a live switch-on.
    //   Chosen: yes, the same path as firstSwitchOn (R8).
    for (i32 i = 0; i < countOf(host.mods); ++i) {
        const LiveMod& mod = host.mods[i];
        if (!isModEnabled(host.settings, mod.id)) {
            continue;
        }
        if (validateManifest(mod.manifest) != ManifestProblem::None) {
            continue;
        }
        if (mod.faulted) {
            continue;
        }
        firstSwitchOn(ctx, host, i);
    }
}

// ---------- R8 / R9 / R10: lifecycle ----------

void firstSwitchOn(Ctx& ctx, ModHost& host, i32 modIndex) {
    // R8: the host calls Load once, then OnEnable.
    // UNKNOWN: plain (non-lifecycle) mods have no Load or OnEnable hook in this port. Chosen: they
    //   only become active, and no hook is requested for them.
    if (!validIndex(host, modIndex)) {
        return;
    }
    LiveMod& mod = host.mods[modIndex];
    if (modKindOf(mod) == ModKind::Lifecycle) {
        if (!mod.loaded) {
            mod.loaded = true;
            ctx.say("mod.load", "Host calls Load", {}, {mod.id});
        }
        ctx.say("mod.on_enable", "Host calls OnEnable", {}, {mod.id});
    }
    mod.active = true;
}

void reactivateMod(Ctx& ctx, ModHost& host, i32 modIndex) {
    // R9: later activations call OnEnable and re-note patches.
    if (!validIndex(host, modIndex)) {
        return;
    }
    LiveMod& mod = host.mods[modIndex];
    if (modKindOf(mod) == ModKind::Lifecycle) {
        ctx.say("mod.on_enable", "Host calls OnEnable", {}, {mod.id});
        ctx.say("mod.repatch", "Host re-notes Harmony patches", {}, {mod.id});
    }
    mod.active = true;
}

void deactivateMod(Ctx& ctx, ModHost& host, i32 modIndex) {
    // R10: stop calls immediately, run OnDisable, then remove Harmony patches (Table 3).
    // UNKNOWN: UnpatchAll behaviour beyond removing the mod's patches is not stated.
    if (!validIndex(host, modIndex)) {
        return;
    }
    LiveMod& mod = host.mods[modIndex];
    mod.active = false;
    if (modKindOf(mod) == ModKind::Lifecycle) {
        ctx.say("mod.on_disable", "Host calls OnDisable", {}, {mod.id});
    }
    ctx.say("mod.unpatch", "Host removes Harmony patches", {}, {mod.id});
}

// ---------- R11: live switch ----------

ModSwitchResult switchModLive(Ctx& ctx, ModHost& host, i32 modIndex, bool switchOn) {
    if (!validIndex(host, modIndex)) {
        return ModSwitchResult::Failed;
    }
    LiveMod& mod = host.mods[modIndex];
    // R11.4: settings are saved in every case.
    setModEnabled(host.settings, mod.id, switchOn);
    // R11.1 / R11.3: only lifecycle mods switch live. Faulted mods change at next start.
    if (modKindOf(mod) != ModKind::Lifecycle || mod.faulted) {
        ctx.say("mod.switch_deferred", "Change applies at next start", {}, {mod.id});
        return ModSwitchResult::DeferredToRestart;
    }
    if (switchOn) {
        // UNKNOWN: a switch-on of a mod with a manifest problem is refused as Failed. The SRD
        //   says such mods are not loaded but does not name the live-switch result.
        if (validateManifest(mod.manifest) != ManifestProblem::None) {
            return ModSwitchResult::Failed;
        }
        // R11.2: load if not yet loaded, otherwise reactivate.
        if (!mod.loaded) {
            firstSwitchOn(ctx, host, modIndex);
        } else {
            reactivateMod(ctx, host, modIndex);
        }
    } else {
        deactivateMod(ctx, host, modIndex);
    }
    return ModSwitchResult::Ok;
}

// ---------- R12: guarded call ----------

bool useOriginalAnswer(HookStatus status) {
    // R12.3: a null decision or a throw gives the original's answer. Only Ok uses the mod.
    return status != HookStatus::Ok;
}

bool applyHookReply(Ctx& ctx, LiveMod& mod, const ModHookReply& reply) {
    // R12.1-2: a throw disables the mod for the session and the original answer is used.
    if (reply.status == HookStatus::Threw) {
        faultMod(ctx, mod, Str("hook threw an exception"));
        return false;
    }
    // R12.3: a null decision gives the original's answer.
    if (reply.status == HookStatus::NullDecision) {
        return false;
    }
    return true;
}

void faultMod(Ctx& ctx, LiveMod& mod, const Str& reason) {
    // R12.1-2, R13: disable for the rest of the session, remove patches, log the reason.
    // UNKNOWN: whether the fault persists across sessions. Chosen: the enabled setting is left
    //   unchanged, so the mod is tried again at next start.
    mod.faulted = true;
    mod.faultReason = reason;
    mod.active = false;
    ctx.say("mod.unpatch", "Host removes Harmony patches of faulted mod", {}, {mod.id});
    ctx.say("mod.fault", "Mod disabled for this session; original rule answer used",
            {}, {mod.id, reason});
}

// ---------- R14: settings change ----------

void raiseSettingChanged(Ctx& ctx, ModHost& host, const Str& modId, const Str& key) {
    // R14: the store raises one Changed(modId, key) event for all mods.
    ctx.say("mod.settings_changed", "Setting changed", {}, {modId, key});
    // ModSettings forwards only matching ids. Each forward is an independent call. Exceptions
    // cannot occur in this port, so the per-handler try block has no counterpart here.
    for (i32 i = 0; i < countOf(host.mods); ++i) {
        if (host.mods[i].id == modId) {
            ctx.say("mod.settings_forward", "Setting forwarded to mod", {}, {modId, key});
        }
    }
}

// ---------- R15 / F5: toggles ----------

bool toggleValue(const ModHost& host, i32 modIndex, const Str& key) {
    // F5: Get(key) = ReadFlag(id, key, DefaultFor(key)).
    if (!validIndex(host, modIndex)) {
        return false;
    }
    const LiveMod& mod = host.mods[modIndex];
    return readStoredFlag(host.settings, mod.id, key,
                          declaredToggleDefault(mod.manifest, key));
}

bool declaredToggleDefault(const ModManifest& manifest, const Str& key) {
    // F5 DefaultFor: default of the toggle whose key matches case-insensitively, else false.
    const i32 idx = findToggleIndexCi(manifest, key);
    if (idx < 0) {
        return false;
    }
    return manifest.settings[idx].defaultValue;  // UNVERIFIED: ModToggleSpec::defaultValue not confirmed
}

// ---------- R16 / F6: choices ----------

Str resolveChoice(const ModChoiceSpec* spec, const Str& stored) {
    // F6: ChoiceOf(spec, stored).
    if (spec == nullptr || spec->options.empty()) {
        // stored ?? "": stored is never null in this port.
        return stored;
    }
    i32 idx = matchOption(spec->options, trimText(stored));
    if (idx < 0) {
        idx = matchOption(spec->options, spec->defaultOption);
    }
    if (idx < 0) {
        // Options[0]: the first declared option is the fallback.
        idx = 0;
    }
    return spec->options[idx];
}

// ---------- settings storage (R17) ----------

bool readStoredFlag(const ModSettingsStore& store, const Str& modId, const Str& key, bool defaultValue) {
    // F4 ReadFlag: the stored boolean, or defaultValue when none is stored.
    const i32 idx = findStored(store, modId, key);
    if (idx < 0) {
        return defaultValue;
    }
    const Str text = trimText(store.entries[idx].text);
    if (equalsCiLiteral(text, "True")) {
        return true;
    }
    if (equalsCiLiteral(text, "False")) {
        return false;
    }
    // UNKNOWN: an unparsable stored flag gives defaultValue; the SRD does not say.
    return defaultValue;
}

void writeStoredFlag(ModSettingsStore& store, const Str& modId, const Str& key, bool value) {
    // F4: written as True or False.
    writeStoredText(store, modId, key, value ? Str("True") : Str("False"));
}

Str readStoredText(const ModSettingsStore& store, const Str& modId, const Str& key) {
    // Stored text, or "" when none is stored.
    const i32 idx = findStored(store, modId, key);
    if (idx < 0) {
        return Str("");
    }
    return store.entries[idx].text;
}

// ---------- R17: code-declared toggles ----------

bool declareToggle(ModManifest& manifest, const ModToggleSpec& spec) {
    // R17 quirk (manifest wins): DeclareToggle does not replace an existing toggle with the same
    // key. The existing (manifest) toggle stays and the code default is silently ignored.
    // UNKNOWN: key match here is case-insensitive, following F5; the SRD does not say.
    if (findToggleIndexCi(manifest, spec.key) >= 0) {
        return false;
    }
    manifest.settings.push_back(spec);
    return true;
}

// R17 quirk (no write check): option text is written directly, without checking Options.
void setChoice(ModSettingsStore& store, const Str& modId, const Str& key, const Str& text) {
    writeStoredText(store, modId, key, text);
}

// ---------- R18: automap menu ----------

AutomapMenuResult resolveAutomapRightClick(const Vec<AutomapMenuItem>& collectedInLoadOrder) {
    // R18.2: if the collected items are only separators (or none), discard them and apply the
    // original right-click (instant re-plot).
    AutomapMenuResult result;
    bool anyRealItem = false;
    for (i32 i = 0; i < countOf(collectedInLoadOrder); ++i) {
        if (!collectedInLoadOrder[i].separator) {
            anyRealItem = true;
            break;
        }
    }
    if (!anyRealItem) {
        result.action = AutomapRightClick::OriginalReplot;
        result.items = Vec<AutomapMenuItem>();
        return result;
    }
    result.action = AutomapRightClick::ShowMenu;
    result.items = collectedInLoadOrder;
    return result;
}

// ---------- R19: town hooks ----------

bool anyModActive(const ModHost& host) {
    for (i32 i = 0; i < countOf(host.mods); ++i) {
        if (host.mods[i].active) {
            return true;
        }
    }
    return false;
}

TownHookAnswer queryTownHook(Ctx& ctx,
                             ModHost& host,
                             TownHook hook,
                             Currency original,
                             const Vec<ModHookReply>& replies) {
    // R19: with no mod loaded, the original rules apply.
    TownHookAnswer answer;
    answer.useOriginal = true;
    answer.amount = original;
    answer.flag = false;
    if (!anyModActive(host)) {
        return answer;
    }
    // UNKNOWN: which mod's answer wins when several answer is not stated. Chosen: the first Ok
    //   reply in load order. Replies that threw or gave a null decision are skipped here; the host
    //   faults the mod through applyHookReply.
    for (i32 i = 0; i < countOf(replies); ++i) {
        if (replies[i].status == HookStatus::Ok) {
            answer.useOriginal = false;
            answer.amount = replies[i].amount;
            answer.flag = replies[i].flag;
            ctx.say("mod.town_hook_answered", "Mod answered town hook",
                    {static_cast<i64>(hook)}, {});
            return answer;
        }
    }
    ctx.say("mod.town_hook_original", "No mod answered town hook; original rule used",
            {static_cast<i64>(hook)}, {});
    return answer;
}

// ---------- R23: API version tags ----------

i32 apiVersionIntroducedBy(i32 member) {
    // Table 2 order: 0 toggle (pre-8), 1 Has/Remove (8), 2 choices (10), 3 table/list settings
    // (-1, Unknown), 4 settings window (11), 5 OptionDescriptions (12).
    // UNKNOWN: out-of-range members return -1; the SRD does not say.
    const i32 count = static_cast<i32>(sizeof(kApiMemberVersion) / sizeof(kApiMemberVersion[0]));
    if (member < 0 || member >= count) {
        return -1;
    }
    return kApiMemberVersion[member];
}

}  // namespace mordor
