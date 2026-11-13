#include "test_support.h"
#include "mordor/dataformatsb.h"

using namespace mordor;

// ---- option and port flag text parsing (SRD c10) ----

TEST(OptionFlagIsCaseInsensitiveTrueOnly) {
    CHECK(optionFlagValue(Str("True")));
    CHECK(optionFlagValue(Str("true")));
    CHECK(optionFlagValue(Str("TRUE")));
    CHECK(!optionFlagValue(Str("False")));
    CHECK(!optionFlagValue(Str("")));
    CHECK(!optionFlagValue(Str("Yes")));
}

TEST(CapitalTFlagsWelcomedAndLimitationNotice) {
    // Case-sensitive ordinal Contains('T'); "True" counts as set (documented quirk).
    CHECK(welcomedFlagValue(Str("T")));
    CHECK(welcomedFlagValue(Str("True")));
    CHECK(!welcomedFlagValue(Str("false")));
    CHECK(!welcomedFlagValue(Str("t")));
    CHECK(!welcomedFlagValue(Str("")));

    CHECK(limitationNoticeSeenValue(Str("T")));
    CHECK(limitationNoticeSeenValue(Str("True")));
    CHECK(!limitationNoticeSeenValue(Str("true")));
    CHECK(!limitationNoticeSeenValue(Str("False")));
}

// ---- record addressing (SRD c17, c35) ----

TEST(RecordOffsetsAndFixtureConstants) {
    // offset = recordLength * (recnum - 1), recnum 1-based
    CHECK_EQ(recordOffset(2900, 1), 0);
    CHECK_EQ(recordOffset(2900, 3), 5800);
    CHECK_EQ(recordOffset(16, 5), 64);
    CHECK_EQ(recordOffset(kMdata1RecordBytes, 8), 1820);
    // offset(n) = (n - 1) * Len, Len = 2900
    CHECK_EQ(characterRecordOffset(1), 0);
    CHECK_EQ(characterRecordOffset(4), 8700);
    CHECK_EQ(kCharacterRecordLen, 2900);
    CHECK_EQ(kCharacterTypeBytes, 2887);
    CHECK_EQ(kGuildsJoinedOffset, 1678);
    CHECK_EQ(kPackFileSlots, 41);
    CHECK_EQ(kGuildStatusRecordCount, 16);
    CHECK_EQ(kShippedRaces, 9);
    CHECK_EQ(kShippedGuilds, 12);
    CHECK_EQ(kShippedItemRecords, 366);
    CHECK_EQ(kShippedMonsterRows, 401);
    CHECK_EQ(kDemoMdata11Bytes, 68166);
}

// ---- fixed-width little-endian reads (SRD c17, c91) ----

TEST(FixedWidthReadsAreLittleEndian) {
    // Integer is signed, Word is unsigned.
    Vec<u8> ff = {0xFF, 0xFF};
    i16 s = 0;
    u16 w = 0;
    CHECK(readInteger(ff, 0, s));
    CHECK_EQ(s, -1);
    CHECK(readWord(ff, 0, w));
    CHECK_EQ(w, 65535);

    // Long is signed, LongWord is unsigned, both 4 bytes.
    Vec<u8> longs = {0x78, 0x56, 0x34, 0x12, 0xFF, 0xFF, 0xFF, 0xFF};
    i32 l = 0;
    u32 ul = 0;
    CHECK(readLong(longs, 0, l));
    CHECK_EQ(l, 0x12345678);
    CHECK(readLongWord(longs, 4, ul));
    CHECK_EQ(ul, 0xFFFFFFFFu);

    // Arbitrary width: three bytes starting at offset 1.
    Vec<u8> three = {0xAA, 0x01, 0x02, 0x03};
    u64 v = 0;
    CHECK(readLittleEndian(three, 1, 3, v));
    CHECK_EQ(v, 0x030201);

    // Single: IEEE 754 binary32, 1.0f is 0x3F800000.
    Vec<u8> one = {0x00, 0x00, 0x80, 0x3F};
    f32 f = 0.0f;
    CHECK(readSingle(one, 0, f));
    CHECK(f == 1.0f);
}

// ---- writers and string fields (SRD c17, c91) ----

TEST(WriteHelpersAndStringRoundTrip) {
    Vec<u8> buf(12, u8{0});
    writeLittleEndian(buf, 0, 0x1234, 2);
    CHECK_EQ(buf[0], 0x34);
    CHECK_EQ(buf[1], 0x12);
    writeLittleEndian(buf, 2, 0xABCDEF01, 4);
    CHECK_EQ(buf[2], 0x01);
    CHECK_EQ(buf[3], 0xEF);
    CHECK_EQ(buf[4], 0xCD);
    CHECK_EQ(buf[5], 0xAB);

    // String*n is n bytes, space-padded.
    writeStringN(buf, 6, Str("AB"), 4);
    CHECK_EQ(buf[6], 'A');
    CHECK_EQ(buf[7], 'B');
    CHECK_EQ(buf[8], ' ');
    CHECK_EQ(buf[9], ' ');
    CHECK_EQ(buf[10], 0);

    Vec<u8> hello(5, u8{0});
    writeStringN(hello, 0, Str("HELLO"), 5);
    Str out;
    CHECK(readStringN(hello, 0, 5, out));
    CHECK_EQ(out, Str("HELLO"));
}

// ---- raw record layouts (SRD c13) ----

TEST(MonsterRecordRawFieldOffsets) {
    // 12 resist words (0-23), 11 item-slot words (24-45), 4 box-chance words (46-53),
    // 7 stat words (54-67), dwords: special property (68), special attack (72), spell flag (76).
    Vec<u8> buf(80, u8{0});
    buf[0] = 0x02;
    buf[1] = 0x01;     // resistWords[0] = 0x0102
    buf[24] = 0x05;    // itemSlotWords[0] = 5
    buf[66] = 0xFF;
    buf[67] = 0xFF;    // statWords[6] = -1
    buf[72] = 0x07;    // specialAttack = 7
    buf[76] = 0x00;
    buf[77] = 0x00;
    buf[78] = 0x01;
    buf[79] = 0x00;    // spellFlag = 0x00010000

    MonsterRecordRaw m;
    CHECK(readMonsterRecordRaw(buf, 0, m));
    CHECK_EQ(m.resistWords[0], 0x0102);
    CHECK_EQ(m.itemSlotWords[0], 5);
    CHECK_EQ(m.statWords[6], -1);
    CHECK_EQ(m.specialProperty, 0u);
    CHECK_EQ(m.specialAttack, 7u);
    CHECK_EQ(m.spellFlag, 0x00010000u);
}

TEST(ItemRecordRawFieldOffsets) {
    // 7 stats-required words (0-13), 7 stat-mod words (14-27),
    // dwords: ability (28), alignment (32), resistance (36), charge (40).
    Vec<u8> buf(44, u8{0});
    buf[0] = 0x0A;     // statsRequired[0] = 10
    buf[26] = 0xFF;
    buf[27] = 0xFF;    // statMods[6] = -1
    buf[28] = 0x03;    // ability = 3
    buf[32] = 0x02;    // alignment = 2
    buf[40] = 0x07;    // charge = 7

    ItemRecordRaw r;
    CHECK(readItemRecordRaw(buf, 0, r));
    CHECK_EQ(r.statsRequired[0], 10);
    CHECK_EQ(r.statMods[6], -1);
    CHECK_EQ(r.ability, 3);
    CHECK_EQ(r.alignment, 2);
    CHECK_EQ(r.resistance, 0);
    CHECK_EQ(r.charge, 7);
}

// ---- table and pack addressing (SRD c13, c62) ----

TEST(MonsterFileRecordSkipZeroAndPinnedSlot) {
    CHECK_EQ(monsterFileRecord(0), 4);     // table index i -> file record i+4
    CHECK_EQ(monsterFileRecord(10), 14);
    CHECK(skipsRecordZero(5));             // MDATA5 record 0 is a duplicate GOBLIE
    CHECK(!skipsRecordZero(2));            // MDATA2 record 0 is a real spell
    CHECK(!skipsRecordZero(3));            // MDATA3 record 0 is a real item (Hands)
    CHECK_EQ(kHandsPackSlot, 0);
    CHECK(isPinnedPackSlot(0));
    CHECK(!isPinnedPackSlot(1));
    CHECK(!isPinnedPackSlot(40));
}

// ---- vital and regenerated data files (SRD c96) ----

TEST(VitalAndRegeneratedMdataSets) {
    CHECK(isVitalMdata(1));
    CHECK(isVitalMdata(5));
    CHECK(isVitalMdata(11));
    CHECK(!isVitalMdata(4));
    CHECK(!isVitalMdata(10));       // mdata10 is non-vital
    CHECK(isRegeneratedMdata(4));
    CHECK(isRegeneratedMdata(10));
    CHECK(isRegeneratedMdata(15));
    CHECK(!isRegeneratedMdata(1));
    CHECK(!isRegeneratedMdata(11));
}

// ---- automap flag bit (SRD c35) ----

TEST(AutomapBeenHereTestsBitTwentyThree) {
    // been_here = word & (1 << 23)
    CHECK(automapBeenHere(1u << 23));
    CHECK(!automapBeenHere(1u << 22));
    CHECK(!automapBeenHere(1u << 24));
    CHECK(!automapBeenHere(0u));
    CHECK(automapBeenHere(0xFFFFFFFFu));
}

// ---- IdLevel saved codes (SRD c35) ----

TEST(SavedIdLevelCodesZeroIsAllThreeIsBarely) {
    CHECK(idLevelFromSavedCode(0) == All);
    CHECK(idLevelFromSavedCode(3) == Barely);
}

// ---- guild log MDATA9 (SRD c10) ----

TEST(GuildLogIgnoresHeaderCountAndDropsTruncatedTail) {
    Vec<u8> bytes = {
        0x63, 0x00, 0x00, 0x00,                          // header count 99, ignored on read
        0x02, 0x00, 0x00, 0x00,                          // guild index 2
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // day serial 1 (64-bit)
        0x02, 0x00, 0x00, 0x00,                          // byte length 2
        0x68, 0x69,                                      // "hi"
        0x05, 0x00, 0x00                                 // truncated tail, dropped
    };
    Vec<GuildLogEntry> entries = readGuildLog(bytes);
    CHECK_EQ(static_cast<i32>(entries.size()), 1);

    Vec<u8> none;
    CHECK_EQ(static_cast<i32>(readGuildLog(none).size()), 0);
}

// ---- mod settings escaping and encoding (SRD c41) ----

TEST(ModSectionNameAndCellEscaping) {
    CHECK_EQ(modSettingsSection(Str("Demo")), Str("Mod:Demo"));
    // Cell: CR and LF become spaces, then \ | ; are escaped.
    CHECK_EQ(escapeModCell(Str("a|b;c\\d")), Str("a\\|b\\;c\\\\d"));
    CHECK_EQ(escapeModCell(Str("a\r\nb")), Str("a  b"));
    CHECK_EQ(escapeModCell(Str("x\\")), Str("x\\\\"));
}

TEST(EncodeRowsAndRoundTripKeepsDelimiters) {
    Vec<Vec<Str>> rows;
    rows.push_back(Vec<Str>{Str("a"), Str("b")});
    rows.push_back(Vec<Str>{Str("c")});
    CHECK_EQ(encodeModRows(rows), Str("a|b;c"));

    // Documented lossy case: one row with one empty cell encodes to "" and decodes to zero rows.
    Vec<Vec<Str>> lossy;
    lossy.push_back(Vec<Str>{Str("")});
    CHECK_EQ(encodeModRows(lossy), Str(""));
    CHECK_EQ(static_cast<i32>(decodeModRows(Str("")).size()), 0);

    Vec<Vec<Str>> pq;
    pq.push_back(Vec<Str>{Str("p|q"), Str("r;s")});
    pq.push_back(Vec<Str>{Str("t")});
    Str line = encodeModRows(pq);
    CHECK_EQ(line, Str("p\\|q|r\\;s;t"));
    Vec<Vec<Str>> back = decodeModRows(line);
    CHECK_EQ(static_cast<i32>(back.size()), 2);
    CHECK_EQ(back[0][0], Str("p|q"));
    CHECK_EQ(back[0][1], Str("r;s"));
    CHECK_EQ(back[1][0], Str("t"));
}

TEST(DecodeRowsUnescapesAndKeepsTrailingBackslash) {
    Vec<Vec<Str>> one = decodeModRows(Str("a\\|b"));
    CHECK_EQ(static_cast<i32>(one.size()), 1);
    CHECK_EQ(one[0][0], Str("a|b"));

    Vec<Vec<Str>> esc = decodeModRows(Str("a\\\\b"));   // text: a\\b
    CHECK_EQ(esc[0][0], Str("a\\b"));

    // A trailing lone backslash is kept as a literal backslash.
    Vec<Vec<Str>> tr = decodeModRows(Str("a\\"));
    CHECK_EQ(tr[0][0], Str("a\\"));
}

// ---- mod flags, has-key and value normalisation (SRD c41) ----

TEST(ModFlagReadWriteHasKeyAndNormalise) {
    // Missing value (present == false) returns the fallback.
    CHECK(readModFlag(false, Str("True"), true));
    CHECK(!readModFlag(false, Str("True"), false));
    // Present value is trimmed and compared ignoring case to "True".
    CHECK(readModFlag(true, Str(" true "), false));
    CHECK(!readModFlag(true, Str("yes"), true));

    CHECK_EQ(modFlagText(true), Str("True"));
    CHECK_EQ(modFlagText(false), Str("False"));

    CHECK(modHasKey(false));    // read was not null, so the key exists
    CHECK(!modHasKey(true));    // read was null

    // Null becomes empty; CR and LF become spaces.
    CHECK_EQ(normaliseModValue(true, Str("abc")), Str(""));
    CHECK_EQ(normaliseModValue(false, Str("a\rb\nc")), Str("a b c"));
}

// ---- mod save tags (SRD c40) ----

TEST(ModSaveTagFromLiveModsAndMissingDetection) {
    Vec<ModRef> none;
    CHECK_EQ(static_cast<i32>(buildModSaveTag(none).size()), 0);

    ModRef a;
    a.id = Str("mod1");
    a.version = Str("2.0");
    ModRef b;
    b.id = Str("mod2");
    b.version = Str("1.5");
    Vec<ModRef> live = {a, b};
    Vec<Str> tag = buildModSaveTag(live);
    CHECK_EQ(static_cast<i32>(tag.size()), 2);
    CHECK_EQ(tag[0], Str("mod1@2.0"));
    CHECK_EQ(tag[1], Str("mod2@1.5"));

    // Id is the part before the first '@'.
    CHECK_EQ(modIdOfTag(Str("a@b@c")), Str("a"));

    Vec<Str> liveIds = {Str("mod1")};
    Vec<Str> missing = missingModTags(tag, liveIds);
    CHECK_EQ(static_cast<i32>(missing.size()), 1);
    CHECK_EQ(missing[0], Str("mod2@1.5"));
}

// ---- store key and legacy INI (SRD c22, c34) ----

TEST(StoreKeyFormatsIdAndAlignmentIndex) {
    // Key(id, a) = "{id}:{(int)a}"; alignment 0 = Unaligned.
    CHECK_EQ(storeKey(7, Unaligned), Str("7:0"));
    CHECK_EQ(storeKey(12, Unaligned), Str("12:0"));
}

TEST(LegacyIniFallbackOnlyForAbsentKeys) {
    CHECK(!legacyIniFallbackApplies(true));   // key present in settings.ini
    CHECK(legacyIniFallbackApplies(false));   // key absent, fall back to MORDOR.INI
    CHECK(!kMayWriteLegacyIni);               // port never writes MORDOR.INI
}

// ---- name collision and save refusal (SRD c3, c53) ----

TEST(CharacterNameCollisionDetection) {
    Vec<Str> names;
    names.push_back(Str("Bob"));
    names.push_back(Str("Alice"));
    CHECK(characterNameCollides(names, Str("Bob")));
    CHECK(!characterNameCollides(names, Str("Carol")));
}

TEST(SaveWithUnknownRaceOrGuildIsRefused) {
    tsup::Env env({0.5, 0.9});
    CHECK(!validateSaveNames(env.ctx, Str("NoSuchRace"), Str("Warrior")));
    CHECK(!validateSaveNames(env.ctx, Str("Human"), Str("NoSuchGuild")));
}

// ---- active guild switching (SRD c35) ----

TEST(SwitchActiveGuildSetsGuildAndCopiesOnlyOnChange) {
    Character c;
    c.activeGuild = Guild::Warrior;
    switchActiveGuild(c, Guild::Mage);
    CHECK(c.activeGuild == Guild::Mage);

    // Same guild: experience is not copied into the guild slot.
    Character same;
    same.activeGuild = Guild::Warrior;
    same.experience = 500;
    same.guilds[static_cast<u32>(Guild::Warrior)].experience = 100;
    switchActiveGuild(same, Guild::Warrior);
    CHECK(same.guilds[static_cast<u32>(Guild::Warrior)].experience == 100);
}

// ---- enum values (SRD c35) ----

TEST(AlignAndSexEnumValues) {
    CHECK(static_cast<i32>(Align::Good) == 0);
    CHECK(static_cast<i32>(Align::Neutral) == 1);
    CHECK(static_cast<i32>(Align::Evil) == 2);
    CHECK(static_cast<i32>(Sex::Male) == 0);
    CHECK(static_cast<i32>(Sex::Female) == 1);
}
