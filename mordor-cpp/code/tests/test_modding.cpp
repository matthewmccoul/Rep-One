#include "test_support.h"
#include "mordor/modding.h"
#include <cstring>

using namespace mordor;

static AncestorFolder makeAncestor(const char* path, bool hasSln) {
    AncestorFolder f;
    f.path = Str(path);
    f.hasMordorSln = hasSln;
    return f;
}

static AutomapMenuItem makeItem(const char* label, bool separator) {
    AutomapMenuItem item;
    item.label = Str(label);
    item.separator = separator;
    return item;
}

// R1: PortTree.Root is the nearest folder above the build containing Mordor.sln.
TEST(modding_port_root_picks_nearest_sln) {
    Vec<AncestorFolder> chain;
    chain.push_back(makeAncestor("build", false));
    chain.push_back(makeAncestor("src", true));
    chain.push_back(makeAncestor("root", true));
    CHECK_EQ(findPortRootIndex(chain), 1);
}

TEST(modding_port_root_first_entry_matches) {
    Vec<AncestorFolder> chain;
    chain.push_back(makeAncestor("here", true));
    chain.push_back(makeAncestor("up", false));
    CHECK_EQ(findPortRootIndex(chain), 0);
}

// R2.1: MORDOR_MODS set -> use its ';'-separated paths.
TEST(modding_roots_from_env_are_split_on_semicolon) {
    Vec<Str> roots = resolveModRoots(true, Str("a;b;c"), Str("game"), Str("prog"));
    CHECK_EQ(roots.size(), 3);
    CHECK(roots[0] == Str("a"));
    CHECK(roots[1] == Str("b"));
    CHECK(roots[2] == Str("c"));
}

// R2.2: otherwise game-folder mods first, then program-folder mods.
TEST(modding_roots_default_order_game_then_program) {
    Vec<Str> roots = resolveModRoots(false, Str(""), Str("gameMods"), Str("progMods"));
    CHECK_EQ(roots.size(), 2);
    CHECK(roots[0] == Str("gameMods"));
    CHECK(roots[1] == Str("progMods"));
}

// R2 / R3: documented names of the environment variable, separator and manifest file.
TEST(modding_documented_constant_names) {
    CHECK(std::strcmp(kModsEnvVar, "MORDOR_MODS") == 0);
    CHECK(kModsEnvSeparator == ';');
    CHECK(std::strcmp(kManifestFileName, "mod.json") == 0);
}

// R3: a root folder entry without mod.json must not produce a loaded mod.
TEST(modding_discover_skips_folder_without_manifest) {
    tsup::Env env({0.5, 0.9});
    ModHost host;
    ModRootListing root;
    root.rootPath = Str("mods");
    ModFolderEntry entry;
    entry.name = Str("nomanifest");
    entry.hasManifest = false;
    root.subfolders.push_back(entry);
    Vec<ModRootListing> roots;
    roots.push_back(root);
    discoverMods(env.ctx, host, roots);
    CHECK_EQ(host.mods.size(), 0);
}

// F1: exact problem messages for each failing check.
TEST(modding_manifest_problem_texts) {
    CHECK(manifestProblemText(ManifestProblem::NoId) == Str("no id"));
    CHECK(manifestProblemText(ManifestProblem::IdBadChars) == Str("the id may not contain [ ] or ="));
    CHECK(manifestProblemText(ManifestProblem::NoAssembly) == Str("no assembly"));
    CHECK(manifestProblemText(ManifestProblem::NoEntryType) == Str("no entryType"));
}

// F2 IsBlank: empty and whitespace-only text are blank; real text is not.
TEST(modding_is_blank_text) {
    CHECK(isBlankText(Str("")));
    CHECK(isBlankText(Str("   ")));
    CHECK(!isBlankText(Str("x")));
    CHECK(!isBlankText(Str("  y  ")));
}

// Duplicate ids: nothing is a duplicate in an empty host.
TEST(modding_no_duplicate_in_empty_host) {
    ModHost host;
    CHECK(!isDuplicateModId(host, Str("anything")));
}

// R6 / F4: enablement is opt-in; default store has no mod enabled.
TEST(modding_enablement_defaults_off) {
    ModSettingsStore store;
    CHECK(!isModEnabled(store, Str("m")));
}

// F4 / R17: enable then disable round trip; settings are per mod.
TEST(modding_enablement_round_trip_and_per_mod) {
    ModSettingsStore store;
    setModEnabled(store, Str("alpha"), true);
    CHECK(isModEnabled(store, Str("alpha")));
    CHECK(!isModEnabled(store, Str("beta")));
    setModEnabled(store, Str("alpha"), false);
    CHECK(!isModEnabled(store, Str("alpha")));
}

// F4 legend: ReadFlag returns the supplied default when nothing is stored.
TEST(modding_read_flag_returns_default_when_unset) {
    ModSettingsStore store;
    CHECK(readStoredFlag(store, Str("m"), Str("Key"), true));
    CHECK(!readStoredFlag(store, Str("m"), Str("Key"), false));
}

// Stored value wins over the default.
TEST(modding_stored_flag_overrides_default) {
    ModSettingsStore store;
    writeStoredFlag(store, Str("m"), Str("Key"), false);
    CHECK(!readStoredFlag(store, Str("m"), Str("Key"), true));
    writeStoredFlag(store, Str("m"), Str("Key"), true);
    CHECK(readStoredFlag(store, Str("m"), Str("Key"), false));
}

// Quirk (no write check): SetChoice stores text without checking Options.
TEST(modding_set_choice_writes_text_unchecked) {
    ModSettingsStore store;
    setChoice(store, Str("m"), Str("mode"), Str("not-a-declared-option"));
    CHECK(readStoredText(store, Str("m"), Str("mode")) == Str("not-a-declared-option"));
}

// F6: with no spec, ChoiceOf returns the stored text unchanged.
TEST(modding_choice_without_spec_returns_stored) {
    CHECK(resolveChoice(nullptr, Str("stored")) == Str("stored"));
}

// R12.2 / R12.3: HookStatus Ok keeps the mod answer; Threw and NullDecision use the original.
TEST(modding_use_original_answer_by_status) {
    CHECK(!useOriginalAnswer(HookStatus::Ok));
    CHECK(useOriginalAnswer(HookStatus::Threw));
    CHECK(useOriginalAnswer(HookStatus::NullDecision));
}

// R18: a menu made only of separators is discarded; original right-click applies.
TEST(modding_automap_only_separators_gives_original) {
    Vec<AutomapMenuItem> items;
    items.push_back(makeItem("", true));
    items.push_back(makeItem("", true));
    AutomapMenuResult result = resolveAutomapRightClick(items);
    CHECK(result.action == AutomapRightClick::OriginalReplot);
    CHECK_EQ(result.items.size(), 0);
}

// R18.1: real items are collected in load order, separators included.
TEST(modding_automap_keeps_items_in_order) {
    Vec<AutomapMenuItem> items;
    items.push_back(makeItem("A", false));
    items.push_back(makeItem("", true));
    items.push_back(makeItem("B", false));
    AutomapMenuResult result = resolveAutomapRightClick(items);
    CHECK(result.action == AutomapRightClick::ShowMenu);
    CHECK_EQ(result.items.size(), 3);
    CHECK(result.items[0].label == Str("A"));
    CHECK(result.items[1].separator);
    CHECK(result.items[2].label == Str("B"));
}

// Table 2: members tagged with the API version that introduced them.
// Index order follows Table 2; the table/list (UNKNOWN) index is not tested.
TEST(modding_api_member_versions) {
    CHECK_EQ(apiVersionIntroducedBy(1), 8);    // Has / Remove
    CHECK_EQ(apiVersionIntroducedBy(2), 10);   // choices
    CHECK_EQ(apiVersionIntroducedBy(4), 11);   // settings window
    CHECK_EQ(apiVersionIntroducedBy(5), 12);   // OptionDescriptions
}

// R19: with no mod loaded, original town rules apply for every town hook.
TEST(modding_town_hooks_use_original_without_mods) {
    tsup::Env env({0.5, 0.9});
    ModHost host;
    Vec<ModHookReply> replies;
    const Currency original = 12345;
    TownHook hooks[] = {TownHook::PartyPool, TownHook::CoverShortfall, TownHook::WealthiestCheck};
    for (TownHook hook : hooks) {
        TownHookAnswer answer = queryTownHook(env.ctx, host, hook, original, replies);
        CHECK(answer.useOriginal);
        CHECK(answer.amount == original);
    }
}

// R19 with no mod loaded: replies are not consulted, so the original still applies.
TEST(modding_town_hook_ignores_replies_without_mods) {
    tsup::Env env({0.5, 0.9});
    ModHost host;
    Vec<ModHookReply> replies;
    ModHookReply reply;
    reply.status = HookStatus::Ok;
    reply.amount = 99;
    reply.flag = true;
    replies.push_back(reply);
    TownHookAnswer answer = queryTownHook(env.ctx, host, TownHook::PartyPool, 500, replies);
    CHECK(answer.useOriginal);
    CHECK(answer.amount == 500);
}

// No mods in an empty host means no mod is active.
TEST(modding_no_active_mod_in_empty_host) {
    ModHost host;
    CHECK(!anyModActive(host));
}

// F5 / R15 indirectly: an undeclared toggle key with no mods reads false.
TEST(modding_toggle_defaults_false_for_undeclared_key) {
    ModRef manifest;
    CHECK(!declaredToggleDefault(manifest, Str("anything")));
}

// R2: a single MORDOR_MODS entry yields exactly one root.
TEST(modding_roots_single_env_entry) {
    Vec<Str> roots = resolveModRoots(true, Str("only"), Str("g"), Str("p"));
    CHECK_EQ(roots.size(), 1);
    CHECK(roots[0] == Str("only"));
}

// Store-level: writing an enable flag for one mod does not change another mod's flag key.
TEST(modding_flag_keys_are_independent) {
    ModSettingsStore store;
    writeStoredFlag(store, Str("m"), Str("Other"), true);
    CHECK(!isModEnabled(store, Str("m")));
    CHECK(readStoredFlag(store, Str("m"), Str("Other"), false));
}

