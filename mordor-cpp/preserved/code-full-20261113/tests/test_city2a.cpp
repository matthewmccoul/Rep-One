#include "test_support.h"
#include "mordor/city2a.h"

using namespace mordor;

// R49 / R56 / T-E: identify state to item grade, and the listing threshold.
TEST(city2a_item_grade_table) {
    CHECK_EQ(libraryItemGrade(IdLevel::All), 4);
    CHECK_EQ(libraryItemGrade(IdLevel::Mostly), 3);
    CHECK_EQ(libraryItemGrade(IdLevel::Partial), 0);
    CHECK_EQ(libraryItemGrade(IdLevel::Barely), 0);
    CHECK_EQ(libraryItemGrade(IdLevel::Unknown), 0);
    CHECK(libraryItemListed(4));
    CHECK(libraryItemListed(3));
    CHECK(!libraryItemListed(2));
    CHECK(!libraryItemListed(0));
}

// T-D: identify level or roll to grade.
TEST(city2a_roll_to_grade_table) {
    CHECK_EQ(libraryGradeFromIdLevel(-3), 4);
    CHECK_EQ(libraryGradeFromIdLevel(0), 4);
    CHECK_EQ(libraryGradeFromIdLevel(1), 3);
    CHECK_EQ(libraryGradeFromIdLevel(2), 2);
    CHECK_EQ(libraryGradeFromIdLevel(3), 1);
    CHECK_EQ(libraryGradeFromIdLevel(9), 1);
}

// R49 FileItem: name kept once graded; Partial stays grade 0 and is hidden.
TEST(city2a_file_item_grade_name_and_partial) {
    tsup::Env env({0.5, 0.9});
    LibraryRecord rec;
    libraryFileItem(env.ctx, rec, "Bob", IdLevel::All, 42);
    CHECK_EQ(rec.known, 4);
    CHECK(rec.name == "Bob");
    CHECK_EQ(rec.id, 42);

    libraryFileItem(env.ctx, rec, "Ann", IdLevel::Partial, 43);
    CHECK_EQ(rec.known, 4);
    CHECK(rec.name == "Bob");
    CHECK_EQ(rec.id, 43);

    LibraryRecord fresh;
    libraryFileItem(env.ctx, fresh, "Cy", IdLevel::Partial, 7);
    CHECK_EQ(fresh.known, 0);
    CHECK(fresh.name == "Cy");
    CHECK_EQ(fresh.id, 7);
    CHECK(!libraryItemListed(fresh.known));

    LibraryRecord mostly;
    libraryFileItem(env.ctx, mostly, "Dee", IdLevel::Mostly, 5);
    CHECK_EQ(mostly.known, 3);
    CHECK(libraryItemListed(mostly.known));
}

// R51 FileRestock: fully known, monster Id -2, location "?", found +1; name rules.
TEST(city2a_restock_fills_known_and_name_rules) {
    tsup::Env env({0.5, 0.9});
    LibraryRecord rec;
    libraryFileRestock(env.ctx, rec, 500);
    CHECK_EQ(rec.known, 4);
    CHECK_EQ(rec.monId, -2);
    CHECK_EQ(rec.lastSeen, 500);
    CHECK(rec.location == "?");
    CHECK_EQ(rec.nSeen, 1);
    CHECK(rec.name == "");

    LibraryRecord kept;
    kept.known = 2;
    kept.name = "Alice";
    kept.nSeen = 3;
    libraryFileRestock(env.ctx, kept, 9);
    CHECK_EQ(kept.known, 4);
    CHECK(kept.name == "Alice");
    CHECK_EQ(kept.nSeen, 4);

    LibraryRecord blank;
    blank.known = 0;
    blank.name = "Old";
    libraryFileRestock(env.ctx, blank, 9);
    CHECK(blank.name == "");
}

// R50 FileFind: location is the level text unless lost; found count goes up.
TEST(city2a_file_find_location_rules) {
    tsup::Env env({0.5, 0.9});
    LibraryRecord rec;
    libraryFileFind(env.ctx, rec, "Eve", IdLevel::All, 88, 9, 1234, false, false, "3,5,2");
    CHECK_EQ(rec.known, 4);
    CHECK_EQ(rec.id, 88);
    CHECK_EQ(rec.monId, 9);
    CHECK_EQ(rec.lastSeen, 1234);
    CHECK(rec.location == "3,5,2");
    CHECK_EQ(rec.nSeen, 1);

    LibraryRecord lostLevel;
    libraryFileFind(env.ctx, lostLevel, "Eve", IdLevel::Mostly, 88, 9, 1234, true, false, "3,5,2");
    CHECK_EQ(lostLevel.known, 3);
    CHECK(lostLevel.location == "?");

    LibraryRecord lostPos;
    libraryFileFind(env.ctx, lostPos, "Eve", IdLevel::Mostly, 88, 9, 1234, false, true, "3,5,2");
    CHECK(lostPos.location == "?");
}

// R52 FileCompanion: grade from ID level, place only when present, hand-built not filed.
TEST(city2a_file_companion_grade_place_and_hand_built) {
    tsup::Env env({0.5, 0.9});
    Companion c;
    c.ref = 4;
    c.name = "Imp";
    c.idLevel = OwnerIdLevel::Partially;
    LibraryRecord rec;
    libraryFileCompanion(env.ctx, rec, c, 33, true, "1,2,0");
    CHECK_EQ(rec.known, 2);
    CHECK(rec.name == "Imp");
    CHECK(rec.location == "1,2,0");
    CHECK_EQ(rec.id, 33);

    c.idLevel = OwnerIdLevel::Barely;
    LibraryRecord low;
    libraryFileCompanion(env.ctx, low, c, 33, false, "");
    CHECK_EQ(low.known, 1);

    LibraryRecord kept;
    kept.location = "old";
    c.idLevel = OwnerIdLevel::Completely;
    libraryFileCompanion(env.ctx, kept, c, 33, false, "");
    CHECK_EQ(kept.known, 4);
    CHECK(kept.location == "old");

    Companion hand;
    hand.ref = -1;
    hand.name = "Hand";
    hand.idLevel = OwnerIdLevel::Completely;
    LibraryRecord none;
    libraryFileCompanion(env.ctx, none, hand, 33, true, "1,1,1");
    CHECK_EQ(none.known, 0);
    CHECK_EQ(none.id, 0);
    CHECK(none.name == "");
}

// R53 dungeon meeting: found and death counts, date written.
TEST(city2a_dungeon_meeting_counts_and_deaths) {
    tsup::Env env({0.5, 0.9});
    Party party;
    MonsterDef def{};
    LibraryRecord rec;
    libraryDungeonMeeting(env.ctx, rec, party, def, 3, true, true, false, false, 99);
    CHECK_EQ(rec.lastSeen, 99);
    CHECK_EQ(rec.nSeen, 3);
    CHECK_EQ(rec.nDeaths, 1);
}

// R53 step 5: a misidentified group writes no record.
TEST(city2a_dungeon_meeting_misidentified_writes_nothing) {
    tsup::Env env({0.5, 0.9});
    Party party;
    MonsterDef def{};
    LibraryRecord rec;
    libraryDungeonMeeting(env.ctx, rec, party, def, 2, true, true, true, false, 99);
    CHECK_EQ(rec.lastSeen, 0);
    CHECK_EQ(rec.nSeen, 0);
    CHECK_EQ(rec.nDeaths, 0);
}

// R53 PlaceOf: "x+1,y+1,level" from the party square.
TEST(city2a_place_of_party_square) {
    Party party;
    party.x = 2;
    party.y = 4;
    party.dungeonLevel = 3;
    Str place = "";
    CHECK(libraryPlaceOf(party, place));
    CHECK(place == "3,5,3");
}

// R57 merge: name taken when there is grade 0, grade is max, counts add with cap at short.MaxValue.
TEST(city2a_merge_record_grade_counts_and_caps) {
    tsup::Env env({0.5, 0.5});
    LibraryRecord there;
    there.known = 0;
    there.name = "A";
    there.nSeen = 2;
    there.nDeaths = 32760;
    there.location = "";
    there.lastSeen = 30;
    LibraryRecord newer;
    newer.known = 3;
    newer.name = "B";
    newer.nSeen = 5;
    newer.nDeaths = 10;
    newer.location = "1,1,2";
    newer.lastSeen = 50;
    libraryMergeRecord(env.ctx, there, newer);
    CHECK_EQ(there.known, 3);
    CHECK(there.name == "B");
    CHECK_EQ(there.nSeen, 7);
    CHECK_EQ(there.nDeaths, 32767);
    CHECK(there.location == "1,1,2");
    CHECK_EQ(there.lastSeen, 50);

    LibraryRecord a;
    a.known = 4;
    a.name = "Keep";
    a.nSeen = 1;
    a.location = "9,9,1";
    a.lastSeen = 30;
    LibraryRecord b;
    b.known = 2;
    b.name = "Other";
    b.nSeen = 1;
    b.nDeaths = 1;
    b.location = "";
    b.lastSeen = 10;
    libraryMergeRecord(env.ctx, a, b);
    CHECK_EQ(a.known, 4);
    CHECK(a.name == "Keep");
    CHECK_EQ(a.nSeen, 2);
    CHECK_EQ(a.nDeaths, 1);
    CHECK(a.location == "9,9,1");
    CHECK_EQ(a.lastSeen, 30);
}

// R63 shared Id 307 and R58 legacy "m:N" keys.
TEST(city2a_shared_id_307_and_legacy_keys) {
    CHECK_EQ(kSharedMonsterId, 307);
    CHECK_EQ(kMorgulMonsterIndex, 278);
    CHECK_EQ(kShadowNinjaMonsterIndex, 389);
    CHECK_EQ(libraryCanonicalMonsterIndex(kShadowNinjaMonsterIndex), kMorgulMonsterIndex);
    CHECK_EQ(libraryCanonicalMonsterIndex(kMorgulMonsterIndex), kMorgulMonsterIndex);
    i32 legacy = -1;
    CHECK(libraryParseLegacyKey("m:307", legacy));
    CHECK_EQ(legacy, 307);
}

// R62 / serial: days are |seen - today|; serial counts whole days from 1899-12-30.
TEST(city2a_day_serial_and_days_ago) {
    CHECK_EQ(libraryDaySerial(0), 25569);
    CHECK_EQ(libraryDaySerial(86400000LL), 25570);
    CHECK_EQ(libraryDaySerial(86399999LL), 25569);
    CHECK_EQ(libraryDaysAgo(100, 103), 3);
    CHECK_EQ(libraryDaysAgo(103, 100), 3);
    // Quirk: an undated record counts from 0, giving the full serial.
    CHECK_EQ(libraryDaysAgo(0, 25600), 25600);
}

// R64 level gate: refused only when level exceeds the deepest reach.
TEST(city2a_seer_level_gate) {
    CHECK(seerLevelRefused(13, 12));
    CHECK(!seerLevelRefused(12, 12));
    CHECK(!seerLevelRefused(0, 12));
}

// R64 fee affordability (bank covers shortfall) and payment through the purse.
TEST(city2a_seer_fee_affordability_and_payment) {
    Character payer;
    payer.wallet.goldOnHand = 100000;
    payer.wallet.goldInBank = 500000;
    CHECK(seerCanAfford(payer, 600000));
    CHECK(!seerCanAfford(payer, 600001));

    tsup::Env env({0.5, 0.5});
    CHECK(seerPayFee(env.ctx, payer, 300000));
    CHECK_EQ(payer.purse(), 300000);
}

// R64 monster locate: too-deep refusal is free; a failed search still charges the fee.
TEST(city2a_seer_locate_monster_gate_and_failed_search) {
    {
        tsup::Env env({0.5, 0.5});
        Character payer;
        payer.wallet.goldInBank = 1000000000000000LL;
        Currency before = payer.purse();
        SeerMonsterResult r = seerLocateMonster(env.ctx, payer, 13, 50, 12);
        CHECK(r.outcome == SeerMonsterOutcome::TooDeep);
        CHECK_EQ(r.feeCharged, 0);
        CHECK_EQ(payer.purse(), before);
    }
    {
        tsup::Env env({0.1, 0.1});
        Character payer;
        payer.wallet.goldInBank = 1000000000000000LL;
        Currency before = payer.purse();
        SeerMonsterResult r = seerLocateMonster(env.ctx, payer, 4, 50, 12);
        CHECK(r.outcome == SeerMonsterOutcome::LevelOnly || r.outcome == SeerMonsterOutcome::NoPlace);
        CHECK(!r.frequencyLine);
        CHECK(r.feeCharged > 0);
        CHECK_EQ(payer.purse(), before - r.feeCharged);
    }
}

// R64 search fails at level x 5 in a hundred; frequency line at 75% (strict <).
TEST(city2a_seer_search_and_frequency_rolls) {
    {
        tsup::Env env({0.1, 0.1});
        CHECK(seerSearchFails(env.ctx, 4));   // 10 < 20
    }
    {
        tsup::Env env({0.5, 0.5});
        CHECK(!seerSearchFails(env.ctx, 4));  // 50 is not < 20
    }
    {
        tsup::Env env({0.74, 0.74});
        CHECK(seerFrequencyLineRolled(env.ctx));   // 74 < 75
    }
    {
        tsup::Env env({0.75, 0.75});
        CHECK(!seerFrequencyLineRolled(env.ctx));  // 75 is not < 75
    }
}

// R64 locate character: city refusal and confirmation both cost nothing.
TEST(city2a_seer_locate_character_refusals) {
    tsup::Env env({0.5, 0.5});
    Character payer;
    payer.wallet.goldOnHand = 500000000LL;
    Currency before = payer.purse();

    Character target;
    target.location.dungeonLevel = kCityDungeonLevel;
    SeerCharacterResult r = seerLocateCharacter(env.ctx, payer, target, true);
    CHECK(r.outcome == SeerCharacterOutcome::CityRefused);
    CHECK_EQ(r.feeCharged, 0);
    CHECK_EQ(payer.purse(), before);

    target.location.dungeonLevel = 5;
    r = seerLocateCharacter(env.ctx, payer, target, false);
    CHECK(r.outcome == SeerCharacterOutcome::NotConfirmed);
    CHECK_EQ(r.feeCharged, 0);
    CHECK_EQ(payer.purse(), before);
}

// R65: cleaning is offered when any guild has more than fifty entries.
TEST(city2a_guild_log_needs_cleaning_threshold) {
    Vec<i32> few;
    few.push_back(10);
    few.push_back(20);
    few.push_back(49);
    CHECK(!guildLogNeedsCleaning(few));

    Vec<i32> many;
    many.push_back(10);
    many.push_back(51);
    many.push_back(3);
    CHECK(guildLogNeedsCleaning(many));
}

// R65 clean: keeps the newest KeepOnClean entries (largest stamps) across all guilds.
TEST(city2a_guild_log_clean_keeps_newest_fifty) {
    tsup::Env env({0.5, 0.5});
    Vec<GuildLogEntry> log;
    for (i32 i = 1; i <= 55; ++i) {
        GuildLogEntry e{};
        e.guild = i % 4;
        e.stamp = i;
        log.push_back(e);
    }
    guildLogClean(env.ctx, log);
    CHECK_EQ(static_cast<i64>(log.size()), static_cast<i64>(kGuildLogKeepOnClean));
    i64 minStamp = 1000;
    for (const GuildLogEntry& e : log) {
        if (e.stamp < minStamp) minStamp = e.stamp;
    }
    CHECK_EQ(minStamp, 6);

    Vec<GuildLogEntry> small;
    for (i32 i = 1; i <= 10; ++i) {
        GuildLogEntry e{};
        e.guild = 0;
        e.stamp = i;
        small.push_back(e);
    }
    guildLogClean(env.ctx, small);
    CHECK_EQ(static_cast<i64>(small.size()), 10);
}

// R67: VB CLng rules: round half to even; "&HFFFF" reads as -1; past Long range refused.
TEST(city2a_gold_transfer_vb_clng_rules) {
    i64 w = 0;
    CHECK(goldTransferParse("2.5", w));
    CHECK_EQ(w, 2);
    CHECK(goldTransferParse("3.5", w));
    CHECK_EQ(w, 4);
    CHECK(goldTransferParse("&HFFFF", w));
    CHECK_EQ(w, -1);
    CHECK(!goldTransferParse("3000000000", w));
}

// R68: entry number minus one, round half to even, must be an occupied slot.
TEST(city2a_companion_slot_entry_rounding) {
    Arr<Companion, kCompanionSlots> comps{};
    comps[0].occupied = true;
    comps[2].occupied = true;
    comps[3].occupied = true;
    CHECK_EQ(companionSlotFromEntry(1.5, comps), 0);   // v = 0.5 -> 0
    CHECK_EQ(companionSlotFromEntry(2.5, comps), 2);   // v = 1.5 -> 2
    CHECK_EQ(companionSlotFromEntry(3.5, comps), 2);   // v = 2.5 -> 2
    CHECK_EQ(companionSlotFromEntry(4.0, comps), 3);   // v = 3
    CHECK_EQ(companionSlotFromEntry(2.0, comps), -1);  // v = 1, empty slot
    CHECK_EQ(companionSlotFromEntry(5.0, comps), -1);  // v = 4, out of range
    CHECK_EQ(companionSlotFromEntry(0.0, comps), -1);  // v = -1
}

// R70: MaxWorn 0 refuses, -1 is uncapped, positive caps at worn count.
TEST(city2a_equip_toggle_wear_caps) {
    CHECK(!equipToggleAllowed(0, 0));
    CHECK(!equipToggleAllowed(0, 2));
    CHECK(equipToggleAllowed(-1, 99));
    CHECK(equipToggleAllowed(1, 0));
    CHECK(!equipToggleAllowed(1, 1));
    CHECK(equipToggleAllowed(3, 2));
    CHECK(!equipToggleAllowed(3, 3));
}

// R72 (unverified transcript): Fit A linear form, rounded. 241520.95 -> 241521.
TEST(city2a_price_fit_a_linear_form) {
    CHECK_EQ(priceFromFit(kPriceFitA, 30, 20, 40, 10, 0), 241521);
}

// R73 (unverified transcript): ECV = att + 4*def + 2*hits + 2*Str; standard = 730 * ECV.
TEST(city2a_ecv_score_and_standard_price) {
    CHECK_EQ(ecvScore(10, 5, 7, 3), 50.0);
    CHECK_EQ(ecvStandardPrice(10, 5, 7, 3), 36500);
}
