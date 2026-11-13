#include <cstring>

#include "test_support.h"
#include "mordor/dataformatsa.h"

using namespace mordor;

namespace {

template <class E>
i32 ev(E e) {
    return static_cast<i32>(e);
}

template <class V>
i32 sz(const V& v) {
    return static_cast<i32>(v.size());
}

IniEntry mkIni(const char* sec, const char* key, const char* val) {
    IniEntry e;
    e.section = sec;
    e.key = key;
    e.value = val;
    return e;
}

LiveMod mkLive(const char* id) {
    LiveMod m;
    m.id = id;
    return m;
}

Character mkChar(const char* name) {
    Character c;
    c.name = name;
    return c;
}

}  // namespace

// SRD: fixed layout and file-name constants (R2, R6, R4, R12, R14, tables).
TEST(dfa_record_layout_and_file_names) {
    CHECK_EQ(kCharRecordLen, 2900);
    CHECK_EQ(kCharRecordFirst, 3);
    CHECK_EQ(kGuildJoinedOffset, 1678);
    CHECK_EQ(kMdata1RecordSize, 260);
    CHECK_EQ(kPackSavedSlots, 41);
    CHECK(kNameForbiddenChar == '|');
    CHECK(std::strcmp(kGuildLogFileName, "MDATA9.MDR") == 0);
    CHECK(std::strcmp(kGuildLogBackupName, "MDATA9B.MDR") == 0);
    CHECK(std::strcmp(kFullFolder, "game\\full") == 0);
    CHECK(std::strcmp(kSharewareFolder, "game\\shareware") == 0);
    CHECK(std::strcmp(kSettingsFileName, "settings.ini") == 0);
    CHECK(std::strcmp(kLegacyIniFileName, "MORDOR.INI") == 0);
    CHECK(std::strcmp(kPreloadKey, "Character") == 0);
}

// SRD R16 and the data file groups table: vital files are never deleted,
// non-vital files may be regenerated.
TEST(dfa_vital_and_nonvital_groups) {
    tsup::Env env({0.5, 0.9});
    const i32 nv = static_cast<i32>(sizeof(kNonVitalDataFiles) / sizeof(kNonVitalDataFiles[0]));
    CHECK_EQ(nv, 10);
    for (i32 i = 0; i < nv; ++i) {
        CHECK(dataFileClass(env.ctx, kNonVitalDataFiles[i]) == DataFileClass::NonVital);
        CHECK(mayDeleteDataFile(env.ctx, kNonVitalDataFiles[i]));
    }
    const i32 vn = static_cast<i32>(sizeof(kVitalDataFiles) / sizeof(kVitalDataFiles[0]));
    CHECK_EQ(vn, 6);
    for (i32 i = 0; i < vn; ++i) {
        CHECK(dataFileClass(env.ctx, kVitalDataFiles[i]) == DataFileClass::Vital);
        CHECK(!mayDeleteDataFile(env.ctx, kVitalDataFiles[i]));
    }
}

// SRD record 0 table: spells and items keep record 0, monsters skip GOBLIE.
TEST(dfa_table_record_zero_handling) {
    tsup::Env env({0.5, 0.9});
    CHECK(keepsTableRecordZero(env.ctx, 2));
    CHECK(keepsTableRecordZero(env.ctx, 3));
    CHECK(!keepsTableRecordZero(env.ctx, 5));
}

// SRD R11: unpacking a file that is not an installer is refused.
TEST(dfa_installer_refuses_non_installer) {
    tsup::Env env({0.5, 0.9});
    Vec<u8> notInstaller;
    UnpackPlan plan = planInstallerUnpack(env.ctx, notInstaller);
    CHECK(plan.outcome == UnpackOutcome::NotAnInstaller);
}

// SRD R14: a whole-gold balance stores no exact field.
TEST(dfa_gold_whole_only_has_no_exact) {
    ExactGold f = goldToSaveFields(250000);  // 25 gold
    CHECK_EQ(f.whole, 25);
    CHECK(!f.hasExact);
    CHECK_EQ(goldFromSaveFields(f), 250000);

    ExactGold zero = goldToSaveFields(0);
    CHECK_EQ(zero.whole, 0);
    CHECK(!zero.hasExact);
}

// SRD R14: a balance with a fraction adds the exact value.
TEST(dfa_gold_fraction_adds_exact) {
    ExactGold f = goldToSaveFields(250005);  // 25.0005 gold
    CHECK_EQ(f.whole, 25);
    CHECK(f.hasExact);
    CHECK_EQ(goldFromSaveFields(f), 250005);
}

// SRD R14: an exact value whose whole part does not match is ignored.
TEST(dfa_gold_mismatched_exact_ignored) {
    ExactGold bad;
    bad.whole = 25;
    bad.hasExact = true;
    bad.exact = 260005;  // whole part 26, does not match 25
    CHECK_EQ(goldFromSaveFields(bad), 250000);

    ExactGold good;
    good.whole = 25;
    good.hasExact = true;
    good.exact = 250005;
    CHECK_EQ(goldFromSaveFields(good), 250005);
}

// SRD R4: settings.ini wins over MORDOR.INI; absent keys use the default.
TEST(dfa_option_text_precedence) {
    tsup::Env env({0.5, 0.9});
    OptionSources src;
    src.settings.push_back(mkIni("Options", "Speed", "New"));
    src.legacyMordorIni.push_back(mkIni("Options", "Speed", "Legacy"));
    src.legacyMordorIni.push_back(mkIni("Options", "Volume", "Legacy"));

    CHECK(readOptionText(env.ctx, src, "Options", "Speed", "Dflt") == "New");
    CHECK(readOptionText(env.ctx, src, "Options", "Volume", "Dflt") == "Legacy");
    CHECK(readOptionText(env.ctx, src, "Options", "Missing", "Dflt") == "Dflt");
}

// SRD R4: a flag is True only when its text equals "True" (case-insensitive).
TEST(dfa_option_flag_truth_and_override) {
    tsup::Env env({0.5, 0.9});
    OptionSources src;
    src.settings.push_back(mkIni("Options", "Sound", "true"));
    src.settings.push_back(mkIni("Options", "Music", "yes"));
    src.settings.push_back(mkIni("Options", "Mouse", "False"));
    src.legacyMordorIni.push_back(mkIni("Options", "Mouse", "True"));
    src.legacyMordorIni.push_back(mkIni("Options", "Fx", "TRUE"));

    CHECK(readOptionFlag(env.ctx, src, "Options", "Sound"));
    CHECK(!readOptionFlag(env.ctx, src, "Options", "Music"));
    // settings.ini key is present, so it wins over the legacy True.
    CHECK(!readOptionFlag(env.ctx, src, "Options", "Mouse"));
    // Absent from settings, falls back to MORDOR.INI.
    CHECK(readOptionFlag(env.ctx, src, "Options", "Fx"));
    CHECK(!readOptionFlag(env.ctx, src, "Options", "Absent"));
}

// SRD R4 save: write only to settings, leave other keys alone, never MORDOR.INI.
TEST(dfa_option_write_touches_settings_only) {
    tsup::Env env({0.5, 0.9});
    Vec<IniEntry> settings;
    settings.push_back(mkIni("Options", "Other", "keep"));

    writeOptionText(env.ctx, settings, "Options", "Speed", "Fast");
    writeOptionFlag(env.ctx, settings, "Options", "Mouse", true);

    OptionSources src;
    src.settings = settings;
    CHECK(readOptionText(env.ctx, src, "Options", "Speed", "x") == "Fast");
    CHECK(readOptionText(env.ctx, src, "Options", "Other", "x") == "keep");
    CHECK(readOptionFlag(env.ctx, src, "Options", "Mouse"));

    // The legacy file was never part of the write; a legacy-only source is unchanged.
    OptionSources legacyOnly;
    legacyOnly.legacyMordorIni.push_back(mkIni("Options", "Speed", "Legacy"));
    CHECK(readOptionText(env.ctx, legacyOnly, "Options", "Speed", "x") == "Legacy");
}

// SRD R8: checks run in order; the first failing check is reported.
TEST(dfa_new_character_check_order) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(ev(checkNewCharacter(env.ctx, "Ann", true, true, false)),
             ev(NewCharacterCheck::Ok));
    CHECK_EQ(ev(checkNewCharacter(env.ctx, "", true, true, false)),
             ev(NewCharacterCheck::EmptyName));
    CHECK_EQ(ev(checkNewCharacter(env.ctx, "A|B", true, true, false)),
             ev(NewCharacterCheck::NameHasBar));
    CHECK_EQ(ev(checkNewCharacter(env.ctx, "Ann", false, true, false)),
             ev(NewCharacterCheck::NoSex));
    CHECK_EQ(ev(checkNewCharacter(env.ctx, "Ann", true, false, false)),
             ev(NewCharacterCheck::NoAlignment));
    CHECK_EQ(ev(checkNewCharacter(env.ctx, "Ann", true, true, true)),
             ev(NewCharacterCheck::SaveExists));
    // Empty name is reported before missing sex and alignment.
    CHECK_EQ(ev(checkNewCharacter(env.ctx, "", false, false, true)),
             ev(NewCharacterCheck::EmptyName));
}

// SRD R8 step 1: the name is trimmed.
TEST(dfa_trim_character_name) {
    tsup::Env env({0.5, 0.9});
    CHECK(trimCharacterName(env.ctx, "  Ann  ") == "Ann");
}

// SRD R3: adding fills the first blank record; otherwise appends at the end.
TEST(dfa_roster_add_placement) {
    tsup::Env env({0.5, 0.9});
    Vec<Character> recs;
    recs.push_back(mkChar("Ann"));
    recs.push_back(Character{});  // blank record
    recs.push_back(mkChar("Bob"));

    i32 idx = rosterAddCharacter(env.ctx, recs, mkChar("Cara"));
    CHECK_EQ(idx, 1);
    CHECK_EQ(sz(recs), 3);
    CHECK(recs[1].name == "Cara");

    // No blank records remain, so the next add goes to the end.
    i32 idx2 = rosterAddCharacter(env.ctx, recs, mkChar("Dee"));
    CHECK_EQ(idx2, 3);
    CHECK_EQ(sz(recs), 4);
}

// SRD R3: deleting blanks the record in place; the list length is kept.
TEST(dfa_roster_delete_blanks_in_place) {
    tsup::Env env({0.5, 0.9});
    Vec<Character> recs;
    recs.push_back(mkChar("Ann"));
    recs.push_back(mkChar("Bob"));

    rosterDeleteRecord(env.ctx, recs, 0);
    CHECK_EQ(sz(recs), 2);
    CHECK(recs[0].name == "");
    CHECK(recs[1].name == "Bob");
}

// SRD R2/R3: roster.json order is used for known names.
TEST(dfa_roster_order_follows_roster) {
    tsup::Env env({0.5, 0.9});
    Vec<Str> roster;
    roster.push_back("Bo");
    roster.push_back("Al");
    Vec<Str> found;
    found.push_back("Al");
    found.push_back("Bo");

    Vec<i32> ord = rosterOrder(env.ctx, roster, found);
    CHECK_EQ(sz(ord), 2);
    // Bo is first in the roster, so the result is {1, 0} for found = {Al, Bo}.
    CHECK_EQ(ord[0], 1);
    CHECK_EQ(ord[1], 0);
}

// SRD R9/R10: in-memory write, overwrite keeps one key, remove restores default.
TEST(dfa_mod_setting_write_read_remove) {
    tsup::Env env({0.5, 0.9});
    ModSettingsStore store;  // not writable: memory only
    CHECK(!store.writable);

    modSettingWrite(env.ctx, store, "m1", "speed", "3");
    CHECK(modSettingRead(env.ctx, store, "m1", "speed", "d") == "3");

    modSettingWrite(env.ctx, store, "m1", "speed", "4");
    CHECK(modSettingRead(env.ctx, store, "m1", "speed", "d") == "4");
    CHECK_EQ(sz(store.entries), 1);
    CHECK(modSettingRead(env.ctx, store, "m1", "other", "d") == "d");

    modSettingRemove(env.ctx, store, "m1", "speed");
    CHECK(modSettingRead(env.ctx, store, "m1", "speed", "d") == "d");
    CHECK_EQ(sz(store.entries), 0);

    // Removing an absent key does nothing.
    modSettingRemove(env.ctx, store, "m1", "speed");
    CHECK_EQ(sz(store.entries), 0);
}

// SRD R9 escaping table and row round trip.
TEST(dfa_mod_escape_and_row_round_trip) {
    tsup::Env env({0.5, 0.9});
    CHECK(modEscapeCell(env.ctx, "a\\b") == "a\\\\b");
    CHECK(modEscapeCell(env.ctx, "a|b") == "a\\|b");
    CHECK(modEscapeCell(env.ctx, "a;b") == "a\\;b");

    Vec<Vec<Str>> rows;
    Vec<Str> r0;
    r0.push_back("a|b");
    r0.push_back("c;d");
    Vec<Str> r1;
    r1.push_back("x\\y");
    rows.push_back(r0);
    rows.push_back(r1);

    Str text = modEncodeRows(env.ctx, rows);
    Vec<Vec<Str>> back = modDecodeRows(env.ctx, text);
    CHECK_EQ(sz(back), 2);
    CHECK_EQ(sz(back[0]), 2);
    CHECK(back[0][0] == "a|b");
    CHECK(back[0][1] == "c;d");
    CHECK(back[1][0] == "x\\y");
}

// SRD R10 quirks: a single empty cell and a row with no cells both encode to ""
// and decode to zero rows.
TEST(dfa_mod_empty_row_quirks) {
    tsup::Env env({0.5, 0.9});
    Vec<Vec<Str>> oneEmptyCell;
    Vec<Str> cell;
    cell.push_back("");
    oneEmptyCell.push_back(cell);
    Str t1 = modEncodeRows(env.ctx, oneEmptyCell);
    CHECK(t1 == "");
    CHECK_EQ(sz(modDecodeRows(env.ctx, t1)), 0);

    Vec<Vec<Str>> noCells;
    noCells.push_back(Vec<Str>{});
    Str t2 = modEncodeRows(env.ctx, noCells);
    CHECK(t2 == "");
    CHECK_EQ(sz(modDecodeRows(env.ctx, t2)), 0);
}

// SRD R15: no running mods gives a null tag; MissingFrom lists entries
// whose id is not live.
TEST(dfa_mod_tag_null_and_missing) {
    tsup::Env env({0.5, 0.9});
    Vec<LiveMod> none;
    ModTag empty = modSaveTag(env.ctx, none);
    CHECK(empty.isNull);

    ModTag tag;
    tag.isNull = false;
    tag.entries.push_back("a@1");
    tag.entries.push_back("b@2");
    Vec<LiveMod> live;
    live.push_back(mkLive("a"));

    Vec<Str> missing = modTagMissingFrom(env.ctx, tag, live);
    CHECK_EQ(sz(missing), 1);
    CHECK(missing[0] == "b@2");
}

// SRD R6 / guild log table: round trip keeps fields; a truncated tail is dropped.
TEST(dfa_guild_log_round_trip_and_truncation) {
    tsup::Env env({0.5, 0.9});
    Vec<GuildLogRecord> log;
    GuildLogRecord r1;
    r1.guildIndex = 3;
    r1.daySerial = 45000;
    r1.text = "Hi there";
    GuildLogRecord r2;
    r2.guildIndex = 7;
    r2.daySerial = 45001;
    r2.text = "Bye";
    log.push_back(r1);
    log.push_back(r2);

    Vec<u8> file = encodeGuildLog(env.ctx, log);
    Vec<GuildLogRecord> back = decodeGuildLog(env.ctx, file);
    CHECK_EQ(sz(back), 2);
    CHECK_EQ(back[0].guildIndex, 3);
    CHECK_EQ(back[0].daySerial, 45000);
    CHECK(back[0].text == "Hi there");
    CHECK(back[1].text == "Bye");

    // Drop the final byte: the truncated second record is dropped.
    Vec<u8> cut;
    for (i32 i = 0; i < sz(file) - 1; ++i) {
        cut.push_back(file[i]);
    }
    Vec<GuildLogRecord> partial = decodeGuildLog(env.ctx, cut);
    CHECK_EQ(sz(partial), 1);
    CHECK_EQ(partial[0].guildIndex, 3);
}

// SRD guild log table: the header count is written and ignored on read.
TEST(dfa_guild_log_header_count_ignored) {
    tsup::Env env({0.5, 0.9});
    Vec<GuildLogRecord> log;
    GuildLogRecord r;
    r.guildIndex = 2;
    r.daySerial = 40000;
    r.text = "Only";
    log.push_back(r);

    Vec<u8> file = encodeGuildLog(env.ctx, log);
    CHECK(sz(file) > 0);
    file[0] = 99;  // header count now claims 99 records
    Vec<GuildLogRecord> back = decodeGuildLog(env.ctx, file);
    CHECK_EQ(sz(back), 1);
    CHECK(back[0].text == "Only");
}

// SRD R6: clean first copies the old file to the backup, byte for byte.
TEST(dfa_guild_log_clean_copies_backup) {
    tsup::Env env({0.5, 0.9});
    Vec<GuildLogRecord> log;
    GuildLogRecord r;
    r.guildIndex = 1;
    r.daySerial = 41000;
    r.text = "Entry";
    log.push_back(r);
    Vec<u8> oldFile = encodeGuildLog(env.ctx, log);

    Vec<u8> backup;
    Vec<u8> cleaned = cleanGuildLog(env.ctx, oldFile, backup);
    CHECK_EQ(sz(backup), sz(oldFile));
    bool same = sz(backup) == sz(oldFile);
    for (i32 i = 0; same && i < sz(oldFile); ++i) {
        same = backup[i] == oldFile[i];
    }
    CHECK(same);
    CHECK(sz(cleaned) > 0);
}

// SRD R7: each request appends another copy of the page, so sizes double.
TEST(dfa_char_stat_each_request_appends_copy) {
    tsup::Env env({0.5, 0.9});
    Vec<Str> lines;
    Character c = mkChar("Ann");
    charStatAppendPage(env.ctx, lines, c, true);
    i32 once = sz(lines);
    CHECK(once > 0);
    charStatAppendPage(env.ctx, lines, c, true);
    CHECK_EQ(sz(lines), 2 * once);
}

// SRD R1 steps 3-4: mods are copied; paralysis without ADShift marks it unsettled.
TEST(dfa_finish_load_adshift_unsettled) {
    tsup::Env env({0.5, 0.9});
    Character c = mkChar("Ann");
    c.ailments.paralysisLevel = 2;
    c.mods.push_back("alpha");
    c.mods.push_back("beta");

    CharacterLoadResult noKey = finishCharacterLoad(env.ctx, c, false, 41);
    CHECK(noKey.adShiftUnsettled);
    CHECK_EQ(sz(noKey.modsWhenLoaded), 2);

    Character c2 = mkChar("Bob");
    c2.ailments.paralysisLevel = 2;
    CharacterLoadResult withKey = finishCharacterLoad(env.ctx, c2, true, 41);
    CHECK(!withKey.adShiftUnsettled);

    Character c3 = mkChar("Cy");
    c3.ailments.paralysisLevel = 0;
    CharacterLoadResult noParalysis = finishCharacterLoad(env.ctx, c3, false, 41);
    CHECK(!noParalysis.adShiftUnsettled);
}

// SRD R13: the older Identified flag is read only from saves without Known.
TEST(dfa_legacy_identified_ignored_when_known_present) {
    tsup::Env env({0.5, 0.9});
    Item it;
    it.idLevel = IdLevel::Mostly;
    applyLegacyIdentified(env.ctx, it, true, true);
    CHECK(it.idLevel == IdLevel::Mostly);
}
