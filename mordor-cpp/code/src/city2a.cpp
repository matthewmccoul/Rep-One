// city2a.cpp - Library writes and grades, Seer service, Guild Log clean,
// temple/morgue fill, gold transfer parse, companion number entry,
// equip toggle, price models. Rules cited as R-numbers from SRD c16p2.

#include "mordor/city1.h"   // FIXED: ChargeResult type for charPay prototype
#include "mordor/city2a.h"
#include "mordor/city2b.h"  // charPay (R64 step 3)

#include <algorithm>
#include <cmath>
#include <string>

namespace mordor {

// FIXED: charPay is declared in city2b.h per the API index, but the compiler
// reports it as not declared here. Restated with the same signature so this TU
// sees it; it is defined elsewhere, so it is not static.
ChargeResult charPay(Ctx& ctx, Character& payer, Currency cost);

namespace {

// UNKNOWN: floor dimensions are not in this chunk; spot range is a placeholder.
constexpr i32 kSeerSpotSpan = 20;
// UNKNOWN: wiki success roll is unsourced (R64 step 6); 50% is a placeholder.
constexpr i32 kWikiSuccessPercent = 50;
// R55: skill-0 readers facing a creature fifteen or more levels harder get grade 1.
constexpr i32 kMonsterGradeLevelGate = 15;
// R62 / R49-R56 style constants.
constexpr i64 kMillisPerDay = 86400000LL;
// Days from 1899-12-30 (serial 0) to 1970-01-01 (Unix day 0).
constexpr i64 kUnixToSerialDays = 25569LL;
// UNKNOWN: CombatEngine.IdLevelRoll die size is not in this chunk.
constexpr i32 kIdRollDie = 4;
// R49 / R56: grades for the item grade mapping.
constexpr i32 kGradeFull = 4;
constexpr i32 kGradeMostly = 3;

bool isDigitCh(char c) { return c >= '0' && c <= '9'; }

bool isSpaceCh(char c) { return c == ' ' || c == '\t'; }

// Returns 0..15 for a hex digit, -1 otherwise.
i32 hexDigitValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

}  // namespace

// ---------------------------------------------------------------------------
// Library grade mapping
// ---------------------------------------------------------------------------

// R49 / R56 / T-E: item grade from identify state. All=4, Mostly=3, else 0.
// Also declared in city3a.h (read param); defined here once.
i32 libraryItemGrade(IdLevel known) {
    if (known == All) return kGradeFull;
    if (known == Mostly) return kGradeMostly;
    return 0;  // Partial stored as 0 (R56, quirk: hides item from listing)
}

// R56: an item is listed only if its grade reaches Mostly or better.
bool libraryItemListed(i32 grade) {
    return grade >= kGradeMostly;
}

// T-D (R54, R52): roll or ID level to grade. <=0 -> 4, 1 -> 3, 2 -> 2, else 1.
i32 libraryGradeFromIdLevel(i32 rollOrIdLevel) {
    if (rollOrIdLevel <= 0) return 4;
    if (rollOrIdLevel <= 1) return 3;
    if (rollOrIdLevel <= 2) return 2;
    return 1;
}

// ---------------------------------------------------------------------------
// Library writes
// ---------------------------------------------------------------------------

// R49 FileItem.
void libraryFileItem(Ctx& ctx, LibraryRecord& rec, Str finderName, IdLevel known, i32 itemId) {
    (void)ctx;  // no output from this write
    const i32 grade = libraryItemGrade(known);
    if (rec.known == 0) rec.name = finderName;
    if (rec.known < grade) rec.known = grade;
    rec.id = itemId;
}

// R50 FileFind: apply FileItem, then source, date, location and found count.
void libraryFileFind(Ctx& ctx, LibraryRecord& rec, Str finderName, IdLevel known, i32 itemId,
                     i32 fromMonsterId, i64 today, bool lostLevel, bool lostPosition, Str levelText) {
    libraryFileItem(ctx, rec, finderName, known, itemId);
    rec.monId = fromMonsterId;
    rec.lastSeen = today;
    rec.location = (!lostLevel && !lostPosition) ? levelText : Str("?");
    rec.nSeen += 1;
}

// R51 FileRestock: filed as fully known, no random draw.
// Set 2 [c57]: grade 4, no finder name, found +1.
void libraryFileRestock(Ctx& ctx, LibraryRecord& rec, i64 today) {
    (void)ctx;
    if (rec.known == 0) rec.name = Str{};
    if (rec.known < kGradeFull) rec.known = kGradeFull;
    rec.monId = kLibraryRestockMonId;  // -2
    rec.lastSeen = today;
    rec.location = Str("?");
    rec.nSeen += 1;
}

// R52 FileCompanion. Hand-built companion (Ref -1) is not filed.
bool libraryFileCompanion(Ctx& ctx, LibraryRecord& rec, const Companion& c, i32 defId,
                          bool hasPlace, Str place) {
    (void)ctx;
    if (c.ref == -1) return false;
    const i32 grade = libraryGradeFromIdLevel(static_cast<i32>(c.idLevel));
    if (rec.known == 0) rec.name = c.name;
    if (rec.known < grade) rec.known = grade;
    if (hasPlace) rec.location = place;
    rec.id = defId;
    return true;
}

// R64-related helper, R53 step 1 (PlaceOf). Returns false (null) when lost.
// Format "x+1,y+1,level" from the party square.
bool libraryPlaceOf(const Party& party, Str& outPlace) {
    if (party.lostLevel || party.lostPosition) return false;
    outPlace = std::to_string(party.x + 1) + "," + std::to_string(party.y + 1) + "," +
               std::to_string(party.dungeonLevel);
    return true;
}

// R53 DungeonMeeting (sub06FE).
// CONFLICT: c57 keeps the old place when lost; c12 writes "?". c57 is used.
// CONFLICT: the signature has no reader, so "reader lacks See Invisible" is
//           assumed to be resolved by the caller into groupInvisible.
// CONFLICT: the formula computes grade via IdLevelRoll, which needs a reader.
//           Without a reader the fn1272-False branch (grade 4) is used.
bool libraryDungeonMeeting(Ctx& ctx, LibraryRecord& rec, const Party& party, const MonsterDef& def,
                           i32 groupCount, bool sawGroup, bool killedBy, bool misidentified,
                           bool groupInvisible, i64 today) {
    (void)ctx;
    (void)def;  // no record field is set from def in this formula
    if (misidentified) return false;   // R53 step 5
    if (groupInvisible) return false;  // R53 step 5 (caller resolves See Invisible)

    Str place{};
    const bool hasPlace = libraryPlaceOf(party, place);  // false when lost: keep old place
    if (hasPlace) rec.location = place;                  // R53 step 2
    rec.lastSeen = today;                                // date written regardless
    if (sawGroup) rec.nSeen += groupCount;               // R53 step 3
    if (killedBy) rec.nDeaths += 1;
    rec.known = kGradeFull;                              // R53 step 4 (literal assignment)
    return true;
}

// R54 FileSighting (c12 formula; c57 differences noted below).
// CONFLICT: the c12 formula computes grade but never stores Known. Set 2 [c57]
//           differs on whether Known is set. The c12 literal is kept; the
//           roll is still drawn so RNG use matches the original.
// Quirk: rec.LastSeen is set to today before the "LastSeen == 0" test, so the
//        test only succeeds when today == 0 (as the SRD order implies).
bool librarySighting(Ctx& ctx, LibraryRecord& rec, i32 refIndex, i32 knownAsIndex,
                     const MonsterDef& def, bool groupInvisible, const Character& reader,
                     const ItemDef* item, bool killed, bool sighting, i32 groupCount,
                     i32 leadDungeonLevel, i64 today) {
    bool lost = false;
    if (refIndex < 0) {  // R54 step 1: lost reader
        refIndex = -refIndex;
        lost = true;
    } else if (refIndex != knownAsIndex) {
        return false;
    }
    // R54 step 2: invisible(def) && !reader.SeeingInvisible && reader.DungeonLevel > 0
    if (groupInvisible && !reader.ailments.seeInvisible && reader.location.dungeonLevel > 0) {
        return false;
    }
    const i32 roll = idLevelRoll(ctx, reader, def, item, false);
    const i32 grade = libraryGradeFromIdLevel(roll);
    (void)grade;  // see CONFLICT above
    if (killed) rec.nDeaths += 1;
    if (sighting) rec.nSeen += groupCount;
    rec.lastSeen = today;
    if (sighting && lost) {
        rec.location = Str("?");
    } else if (sighting && leadDungeonLevel == 0 &&
               (rec.location.empty() || rec.lastSeen == 0)) {
        rec.location = Str("the City");
    }
    return true;
}

// R55 Monster grade (Set 2). Skill 0 and creature 15+ levels harder -> grade 1.
// UNKNOWN: whether the roll is mapped through T-D or used raw is not explicit in
//          R55; the R54 mapping is used.
i32 libraryMonsterGrade(Ctx& ctx, const Character& reader, const MonsterDef& def, i32 monsterLevel) {
    const i32 skill = reader.activeLevel();
    if (skill == 0 && monsterLevel - skill >= kMonsterGradeLevelGate) return 1;
    return libraryGradeFromIdLevel(idLevelRoll(ctx, reader, def, nullptr, false));
}

// ---------------------------------------------------------------------------
// Library merge, keys, settling
// ---------------------------------------------------------------------------

// R57 merge. NDeaths capped at short.MaxValue (kLibraryCountCap).
// UNKNOWN: whether the cap also applies to NSeen is not stated; NSeen is uncapped.
void libraryMergeRecord(Ctx& ctx, LibraryRecord& there, const LibraryRecord& newer) {
    (void)ctx;
    if (there.known == 0) there.name = newer.name;
    there.known = maxv(there.known, newer.known);
    there.nSeen += newer.nSeen;
    there.nDeaths = minv(there.nDeaths + newer.nDeaths, kLibraryCountCap);
    if (!newer.location.empty()) there.location = newer.location;
    if (newer.lastSeen > there.lastSeen) there.lastSeen = newer.lastSeen;
}

// R58 step 1: legacy "m:N" keys.
bool libraryParseLegacyKey(Str key, i32& legacyMonsterId) {
    if (key.size() < 3 || key[0] != 'm' || key[1] != ':') return false;
    i64 value = 0;
    for (size_t p = 2; p < key.size(); ++p) {
        if (!isDigitCh(key[p])) return false;
        value = value * 10 + (key[p] - '0');
        if (value > kVbLongMax) return false;
    }
    legacyMonsterId = static_cast<i32>(value);
    return true;
}

// R63 shared Id 307: the second index (Shadow Ninja, 389) maps to the first (Morgul, 278).
i32 libraryCanonicalMonsterIndex(i32 monsterIndex) {
    if (monsterIndex == kShadowNinjaMonsterIndex) return kMorgulMonsterIndex;
    return monsterIndex;
}

// R58 step 2 (SettleMonsterKeys). Records for non-canonical indices are merged
// into their canonical record and cleared.
// UNKNOWN: the Id-to-index move from old Id-keyed records is not fully specified
//          here; only the shared-Id merge (R63) is applied.
void librarySettleMonsterKeys(Ctx& ctx, LibraryState& lib) {
    const i32 n = static_cast<i32>(lib.monsters.size());
    for (i32 i = 0; i < n; ++i) {
        const i32 canonical = libraryCanonicalMonsterIndex(i);
        if (canonical == i || canonical < 0 || canonical >= n) continue;
        libraryMergeRecord(ctx, lib.monsters[static_cast<u32>(canonical)],
                           lib.monsters[static_cast<u32>(i)]);
        lib.monsters[static_cast<u32>(i)] = LibraryRecord{};
    }
}

// R58 step 4: port key for monsters.
Str libraryMonsterKey(i32 monsterIndex) {
    return Str("mr:") + std::to_string(monsterIndex);
}

// R58 step 4: port key for items.
Str libraryItemKey(i32 itemId) {
    return Str("i:") + std::to_string(itemId);
}

// c35 port addition: Library "found" counts are dropped on import.
void libraryImportDropFound(Ctx& ctx, LibraryRecord& rec) {
    (void)ctx;
    rec.nSeen = 0;
}

// R62 Serial(when): whole days since 1899-12-30 for a UTC millisecond stamp.
i64 libraryDaySerial(i64 utcMillis) {
    const i64 days = vbInt(static_cast<double>(utcMillis) / static_cast<double>(kMillisPerDay));
    return days + kUnixToSerialDays;
}

// R62: days = Abs(seen - today). A record dated 0 reports the full serial (quirk).
// Also declared in city2b.h (seenSerial, todaySerial); defined here once.
i64 libraryDaysAgo(i64 seen, i64 today) {
    return seen > today ? seen - today : today - seen;
}

// ---------------------------------------------------------------------------
// ID level roll
// ---------------------------------------------------------------------------

// UNKNOWN: CombatEngine.IdLevelRoll body is not in this chunk (R54, R55 cite it).
// Placeholder: draws a die and subtracts reader skill (guild level, clamped 0..3),
// so higher skill gives lower (better) ID levels. def, item and flag are unused.
i32 idLevelRoll(Ctx& ctx, const Character& reader, const MonsterDef& def, const ItemDef* item,
                bool flag) {
    (void)def;
    (void)item;
    (void)flag;
    const i32 skill = clampv(reader.activeLevel(), 0, 3);
    const i32 roll = static_cast<i32>(ctx.rng.below(kIdRollDie));
    return roll - skill;
}

// ---------------------------------------------------------------------------
// Seer
// ---------------------------------------------------------------------------

// R64 step 2: a target with level > DeepestHeCanReach is refused as too deep.
bool seerLevelRefused(i32 level, i32 deepestReach) {
    return level > deepestReach;
}

// R64 step 3 and Seer fee: fee = base * Log(level+1) * level, plus up to a quarter
// of itself (random), then halved.
// UNKNOWN: the operand of "halved" is not stated; halving the whole value is used.
// UNKNOWN: units. The raw value is taken directly as Currency (1/10000 gold).
// Level <= 0 gives zero without a random draw (log(1) = 0).
Currency seerFee(Ctx& ctx, i32 level) {
    if (level <= 0) return 0;
    const double lvl = static_cast<double>(level);
    double fee = kSeerFeeBase * std::log(lvl + 1.0) * lvl;
    fee += fee * 0.25 * ctx.rng.nextDouble();  // plus up to a quarter of itself
    fee /= 2.0;                                 // UNKNOWN: "halved" operand
    return roundHalfEven(fee);
}

// R64 step 5: LocateCharacter fee depends on depth only (no random draw).
// UNKNOWN: the formula is not in this chunk; the Seer fee shape is reused without
//          the random quarter, then halved.
Currency seerLocateCharacterFee(i32 dungeonLevel) {
    if (dungeonLevel <= 0) return 0;
    const double lvl = static_cast<double>(dungeonLevel);
    const double fee = kSeerFeeBase * std::log(lvl + 1.0) * lvl / 2.0;
    return roundHalfEven(fee);
}

// R64 step 5: "Not enough gold!" if CanPay fails. Total purse covers the fee.
bool seerCanAfford(const Character& payer, Currency fee) {
    return payer.purse() >= fee;
}

// R64 step 3: paid through CharPay (the bank covers any shortfall).
// Returns false, without charging, when the payer cannot afford the fee.
bool seerPayFee(Ctx& ctx, Character& payer, Currency fee) {
    if (!seerCanAfford(payer, fee)) return false;
    (void)charPay(ctx, payer, fee);
    return true;
}

// R64 step 4: search fails level x 5 in a hundred. Chance is drawn as
// NextDouble() * 100 < level*5, the same pattern as the 75% frequency line.
bool seerSearchFails(Ctx& ctx, i32 level) {
    return ctx.rng.nextDouble() * 100.0 < static_cast<double>(level * kSeerSearchFailPerLevel);
}

// R64 step 4: frequency line chance, NextDouble() * 100 < 75.
bool seerFrequencyLineRolled(Ctx& ctx) {
    return ctx.rng.nextDouble() * 100.0 < static_cast<double>(kSeerFrequencyPercent);
}

// R64 step 4: Seer.Blur(..., radius). Doc comment: radius 5 is up to four squares
// each way. UNKNOWN: the blur mapping is not specified; offset in [-(r-1), r-1].
void seerBlurSpot(Ctx& ctx, i32& x, i32& y, i32 radius) {
    if (radius < 1) return;
    const i32 span = radius - 1;
    x += static_cast<i32>(ctx.rng.below(2 * span + 1)) - span;
    y += static_cast<i32>(ctx.rng.below(2 * span + 1)) - span;
}

// R64 step 4: Seer locates a monster.
// Order: level gate, fee (charged even if search fails), search, spot, blur,
// frequency line. The frequency text is built by the caller from monsterChance.
// UNKNOWN: an unaffordable fee returns NoPlace with nothing charged (no outcome
//          for refused-for-gold exists in SeerMonsterOutcome).
// UNKNOWN: the spot is picked uniformly over kSeerSpotSpan (floor size not in chunk).
SeerMonsterResult seerLocateMonster(Ctx& ctx, Character& payer, i32 levelFound, i32 monsterChance,
                                    i32 deepestReach) {
    SeerMonsterResult r{};
    (void)monsterChance;
    if (seerLevelRefused(levelFound, deepestReach)) {  // R64 step 2
        r.outcome = SeerMonsterOutcome::TooDeep;
        return r;
    }
    const Currency fee = seerFee(ctx, levelFound);     // R64 step 3
    if (!seerPayFee(ctx, payer, fee)) {
        r.outcome = SeerMonsterOutcome::NoPlace;
        return r;
    }
    r.feeCharged = fee;
    r.dungeonLevel = levelFound;
    if (seerSearchFails(ctx, levelFound)) {            // R64 step 4
        r.outcome = SeerMonsterOutcome::LevelOnly;
        return r;
    }
    // Successful search: level data loaded, spot picked, then blurred.
    r.x = static_cast<i32>(ctx.rng.below(kSeerSpotSpan));
    r.y = static_cast<i32>(ctx.rng.below(kSeerSpotSpan));
    seerBlurSpot(ctx, r.x, r.y, kSeerBlurArg);         // Seer.Blur(..., 5)
    r.frequencyLine = seerFrequencyLineRolled(ctx);    // 75% frequency line
    r.outcome = SeerMonsterOutcome::Located;
    return r;
}

// R64 doc comment: roll vs 3 x DungeonLevel.
// UNKNOWN: the exact expression is not in this chunk. Success when a percent roll
//          falls below 3 x DungeonLevel, matching the frequency-line pattern.
bool seerLocateCharacterRoll(Ctx& ctx, i32 dungeonLevel) {
    return ctx.rng.nextDouble() * 100.0 <
           static_cast<double>(kSeerLocateCharacterFactor * dungeonLevel);
}

// R64 step 5: Seer locates a saved character.
// Order: refused in city, confirmation, affordability, pay, roll.
// Port addition (c50): scope conflict noted in the SRD; implemented as specified.
SeerCharacterResult seerLocateCharacter(Ctx& ctx, Character& payer, const Character& target,
                                        bool confirmed) {
    SeerCharacterResult r{};
    const i32 depth = target.location.dungeonLevel;
    r.dungeonLevel = depth;
    if (depth == kCityDungeonLevel) {  // target is in the City
        r.outcome = SeerCharacterOutcome::CityRefused;
        return r;
    }
    if (!confirmed) {
        r.outcome = SeerCharacterOutcome::NotConfirmed;
        return r;
    }
    const Currency fee = seerLocateCharacterFee(depth);
    if (!seerCanAfford(payer, fee)) {
        r.outcome = SeerCharacterOutcome::NotEnoughGold;
        return r;
    }
    (void)seerPayFee(ctx, payer, fee);
    r.feeCharged = fee;
    if (!seerLocateCharacterRoll(ctx, depth)) {
        r.outcome = SeerCharacterOutcome::Failed;
        return r;
    }
    r.x = target.location.x;
    r.y = target.location.y;
    r.outcome = SeerCharacterOutcome::Located;
    return r;
}

// R64 step 6: wiki locate. The player pays and gives an exact name.
// The success roll is unsourced; kWikiSuccessPercent is a placeholder.
// UNKNOWN: no target table is among the inputs, so coordinates stay 0 on success.
// Results are a snapshot (R64 step 6).
SeerWikiResult seerWikiLocate(Ctx& ctx, Character& payer, Str exactName, Currency fee) {
    SeerWikiResult r{};
    (void)exactName;
    if (!seerPayFee(ctx, payer, fee)) return r;
    r.feeCharged = fee;
    r.found = ctx.rng.nextDouble() * 100.0 < static_cast<double>(kWikiSuccessPercent);
    return r;
}

// ---------------------------------------------------------------------------
// Guild Log
// ---------------------------------------------------------------------------

// R65 step 2: Clean is shown only when some guild has more than the threshold.
// UNKNOWN: the exact comparison; "past fifty entries per guild" is used as strictly greater.
bool guildLogNeedsCleaning(const Vec<i32>& entriesPerGuild) {
    for (i32 n : entriesPerGuild) {
        if (n > kGuildLogCleanThreshold) return true;
    }
    return false;
}

// R65 step 3: keep the newest KeepOnClean entries across all guilds (largest stamp
// is newest). Kept entries keep their original relative order. Returns the removed
// entries so the caller can write them to a text file if asked.
Vec<GuildLogEntry> guildLogClean(Ctx& ctx, Vec<GuildLogEntry>& log) {
    Vec<GuildLogEntry> removed{};
    const i32 n = static_cast<i32>(log.size());
    if (n <= kGuildLogKeepOnClean) return removed;

    Vec<i32> order{};
    order.reserve(static_cast<size_t>(n));
    for (i32 i = 0; i < n; ++i) order.push_back(i);
    std::sort(order.begin(), order.end(), [&log](i32 a, i32 b) {
        const GuildLogEntry& ea = log[static_cast<u32>(a)];
        const GuildLogEntry& eb = log[static_cast<u32>(b)];
        if (ea.stamp != eb.stamp) return ea.stamp > eb.stamp;  // newest first
        return a < b;
    });

    Vec<bool> keep(static_cast<size_t>(n), false);
    for (i32 k = 0; k < kGuildLogKeepOnClean; ++k) {
        keep[static_cast<size_t>(order[static_cast<size_t>(k)])] = true;
    }

    Vec<GuildLogEntry> kept{};
    for (i32 i = 0; i < n; ++i) {
        if (keep[static_cast<size_t>(i)]) {
            kept.push_back(log[static_cast<u32>(i)]);
        } else {
            removed.push_back(log[static_cast<u32>(i)]);
        }
    }
    log = kept;
    ctx.say("guildlog.clean", "Guild Log cleaned.",
            {static_cast<i64>(removed.size()), static_cast<i64>(kept.size())}, {});
    return removed;
}

// ---------------------------------------------------------------------------
// Temple and Morgue
// ---------------------------------------------------------------------------

// R66 step 2: MorgueList fills by walking character-file records in record order.
// UNKNOWN: the morgue is assumed to hold dead characters only (isDead()).
void templeMorgueFill(Ctx& ctx, Party& party, const Vec<Character>& recordsInFileOrder) {
    (void)ctx;
    party.morgueList.clear();
    for (const Character& c : recordsInFileOrder) {
        if (c.isDead()) party.morgueList.push_back(c);
    }
}

// ---------------------------------------------------------------------------
// Gold transfer input (R67)
// ---------------------------------------------------------------------------

// R67: VB CLng rules. Round half to even. Values past Long range are refused.
// Quirk [c62]: "&HFFFF" reads as -1 (an Integer). Hex literals with up to four
// digits are 16-bit signed; up to eight digits are 32-bit signed.
bool goldTransferParse(Str text, i64& wholeGold) {
    size_t b = 0;
    size_t e = text.size();
    while (b < e && isSpaceCh(text[b])) ++b;
    while (e > b && isSpaceCh(text[e - 1])) --e;
    if (b >= e) return false;

    // Hex literal: &H...
    if (e - b >= 2 && text[b] == '&' && (text[b + 1] == 'H' || text[b + 1] == 'h')) {
        size_t p = b + 2;
        size_t digits = 0;
        u64 v = 0;
        while (p < e) {
            const i32 d = hexDigitValue(text[p]);
            if (d < 0) return false;
            if (digits >= 8) return false;
            v = v * 16ULL + static_cast<u64>(d);
            ++digits;
            ++p;
        }
        if (digits == 0) return false;
        i64 val = 0;
        if (digits <= 4) {
            val = (v >= 0x8000ULL) ? static_cast<i64>(v) - 0x10000LL : static_cast<i64>(v);
        } else {
            val = (v >= 0x80000000ULL) ? static_cast<i64>(v) - 0x100000000LL
                                       : static_cast<i64>(v);
        }
        if (val < kVbLongMin || val > kVbLongMax) return false;
        wholeGold = val;
        return true;
    }

    // Decimal literal with optional sign and fraction.
    size_t p = b;
    bool neg = false;
    if (text[p] == '+' || text[p] == '-') {
        neg = (text[p] == '-');
        ++p;
    }
    i64 ip = 0;
    size_t intDigits = 0;
    bool tooBig = false;
    while (p < e && isDigitCh(text[p])) {
        if (ip > 100000000000000LL) {
            tooBig = true;
        } else {
            ip = ip * 10 + static_cast<i64>(text[p] - '0');
        }
        ++intDigits;
        ++p;
    }

    bool above = false;      // fraction > .5
    bool exactHalf = false;  // fraction == .5
    size_t fracDigits = 0;
    if (p < e && text[p] == '.') {
        ++p;
        bool first = true;
        bool restNonZero = false;
        while (p < e && isDigitCh(text[p])) {
            const char c = text[p];
            if (first) {
                first = false;
                if (c > '5') {
                    above = true;
                } else if (c == '5') {
                    exactHalf = true;
                }
            } else if (c != '0') {
                restNonZero = true;
            }
            ++fracDigits;
            ++p;
        }
        if (exactHalf && restNonZero) {
            exactHalf = false;
            above = true;
        }
    }
    if (p != e) return false;
    if (intDigits + fracDigits == 0) return false;
    if (tooBig) return false;

    i64 mag = ip;
    if (above || (exactHalf && (ip % 2 == 1))) mag += 1;  // round half to even
    const i64 val = neg ? -mag : mag;
    if (val < kVbLongMin || val > kVbLongMax) return false;
    wholeGold = val;
    return true;
}

// ---------------------------------------------------------------------------
// Companion number entry (R68)
// ---------------------------------------------------------------------------

// R68: v = VbVal(text) - 1 (vbValue already parsed); out-of-Integer range -> -1;
// round half to even; outside 0..slots-1 -> -1; occupied slot -> n, else -1.
i32 companionSlotFromEntry(double vbValue, const Arr<Companion, kCompanionSlots>& companions) {
    const double v = vbValue - 1.0;
    if (v > kVbIntegerMax || v < kVbIntegerMin) return -1;
    const i64 n = roundHalfEven(v);
    if (n < 0 || n > kCompanionSlots - 1) return -1;
    return companions[static_cast<u32>(n)].occupied ? static_cast<i32>(n) : -1;
}

// ---------------------------------------------------------------------------
// Equip toggle (R70)
// ---------------------------------------------------------------------------

// R70: MaxWorn 0 refuses. MaxWorn > 0 caps at worn >= MaxWorn. MaxWorn -1 (or any
// other non-positive nonzero value) is wearable without cap.
// Port deviation [c63]: the port tests against zero, so -1 is wearable without cap.
bool equipToggleAllowed(i32 maxWorn, i32 wornInClass) {
    if (maxWorn == kMaxWornNone) return false;                    // R70 step 1
    if (maxWorn > 0 && wornInClass >= maxWorn) return false;      // R70 step 2
    return true;
}

// ---------------------------------------------------------------------------
// Price models (R72, R73; wiki transcript, unverified)
// ---------------------------------------------------------------------------

// R72: price = W_ATT*att + W_DEF*def + W_HITS*hits + W_RES*ResSum + W_GL*guildLevel
// + INTERCEPT, then int(max(1000, round(price))). Transcript Python; unverified.
i64 priceFromFit(const PriceFit& fit, i32 att, i32 def, i32 hits, i32 resSum, i32 guildLevel) {
    const double price = fit.att * att + fit.def * def + fit.hits * hits + fit.resSum * resSum +
                         fit.guildLevel * guildLevel + fit.intercept;
    const i64 rounded = roundHalfEven(price);
    return maxv(kPriceFloor, rounded);
}

// R73: ECV = 1*att + 4*def + 2*hits + 2*StatStr (transcript; unverified).
double ecvScore(i32 att, i32 def, i32 hits, i32 strengthStat) {
    return 1.0 * att + 4.0 * def + 2.0 * hits + 2.0 * strengthStat;
}

// R73: Price_Standard ≈ 730 * ECV (transcript; unverified). No intercept, no floor
// (none stated).
i64 ecvStandardPrice(i32 att, i32 def, i32 hits, i32 strengthStat) {
    return roundHalfEven(kEcvStandardMultiplier * ecvScore(att, def, hits, strengthStat));
}

}  // namespace mordor
