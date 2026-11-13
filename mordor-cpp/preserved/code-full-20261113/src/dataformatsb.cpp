// dataformatsb.cpp: implements include/mordor/dataformatsb.h
// Source references are to SRD c-numbers, e.g. [c35].
// GuildLogEntry members used here: guild (guild index) and stamp (day serial; newest = largest stamp).
// city2a.h is unchanged.
#include "mordor/dataformatsb.h"
#include "mordor/city2a.h"  // FIXED: GuildLogEntry is declared in city2a.h

#include <cstddef>
#include <cstring>

namespace mordor {

namespace {

// [c13] table index i maps to file record i+4.
constexpr i64 kMonsterFileBase = 4;
// [c28] library record for monster index Ref is read at Ref + 6.
// UNKNOWN: relation between library Ref+6 and monster file record i+4 is not stated.
constexpr i64 kLibraryRefBase = 6;
// [c62] MDATA5 is the monster table; its record 0 is a duplicate and is skipped.
constexpr i32 kMonsterMdataFile = 5;
// [c28] Monster Id 307 is shared by the Morgul (278) and the Shadow Ninja (389).
constexpr i32 kAmbiguousMonsterId = 307;
// [c35] been_here = word & (1 << 23), gc1D50 = 23.
// UNKNOWN: whether automap indexes are 0- or 1-based is not stated.
constexpr u32 kAutomapBeenHereBit = 23;
// [c13] MDATA1 holds 7 header records before the tables start.
constexpr i64 kMdata1HeaderRecords = 7;

bool isAsciiSpace(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

char lowerAscii(char c) {
    return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
}

i64 sizeI64(const Vec<u8>& b) {
    return static_cast<i64>(b.size());
}

// True when [offset, offset+n) lies entirely inside buf.
bool rangeOk(const Vec<u8>& buf, i64 offset, i64 n) {
    return offset >= 0 && n >= 0 && offset <= sizeI64(buf) && n <= sizeI64(buf) - offset;
}

void ensureSize(Vec<u8>& buf, i64 needed) {
    if (sizeI64(buf) < needed) {
        buf.resize(static_cast<std::size_t>(needed), static_cast<u8>(0));
    }
}

// Decimal text of a signed integer.
Str i64ToStr(i64 v) {
    if (v == 0) {
        return Str("0");
    }
    const bool neg = v < 0;
    u64 mag = neg ? (static_cast<u64>(0) - static_cast<u64>(v)) : static_cast<u64>(v);
    Str digits;
    while (mag > 0) {
        digits += static_cast<char>('0' + static_cast<int>(mag % 10));
        mag /= 10;
    }
    Str out;
    if (neg) {
        out += '-';
    }
    for (i64 i = static_cast<i64>(digits.size()) - 1; i >= 0; --i) {
        out += digits[i];
    }
    return out;
}

// Flag text equals "True", ignoring ASCII case, with no trimming.
bool equalsIgnoreCaseTrue(const Str& s) {
    const char* lit = "true";
    if (s.size() != 4) {
        return false;
    }
    for (i64 i = 0; i < 4; ++i) {
        if (lowerAscii(s[i]) != lit[i]) {
            return false;
        }
    }
    return true;
}

// Ordinal, case-sensitive search for 'T'.
bool containsUpperT(const Str& s) {
    for (i64 i = 0; i < static_cast<i64>(s.size()); ++i) {
        if (s[i] == 'T') {
            return true;
        }
    }
    return false;
}

// Trims leading and trailing ASCII whitespace.
Str trimWs(const Str& s) {
    i64 b = 0;
    i64 e = static_cast<i64>(s.size());
    while (b < e && isAsciiSpace(s[b])) {
        ++b;
    }
    while (e > b && isAsciiSpace(s[e - 1])) {
        --e;
    }
    Str out;
    for (i64 i = b; i < e; ++i) {
        out += s[i];
    }
    return out;
}

// Reads `count` little-endian Integer (2-byte) values into dst, advancing pos.
bool readInt16Run(const Vec<u8>& buf, i64& pos, i16* dst, i32 count) {
    for (i32 i = 0; i < count; ++i) {
        if (!readInteger(buf, pos, dst[i])) {
            return false;
        }
        pos += kIntegerBytes;
    }
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// Option and flag values
// ---------------------------------------------------------------------------

// Flag(k) = (Text(section, k, "False") equalsIgnoreCase "True")  [c10]
bool optionFlagValue(const Str& text) {
    return equalsIgnoreCaseTrue(text);
}

// Welcomed = NotFirstTime.Contains('T'), case-sensitive ordinal  [c10]
// Quirk kept: any value containing a capital T counts as set.
bool welcomedFlagValue(const Str& notFirstTime) {
    return containsUpperT(notFirstTime);
}

// LimitationNoticeSeen = Limited.Contains('T'), case-sensitive ordinal  [c10]
bool limitationNoticeSeenValue(const Str& limited) {
    return containsUpperT(limited);
}

// ---------------------------------------------------------------------------
// Record addressing
// ---------------------------------------------------------------------------

// offset = recordLength * (recnum - 1); recnum is 1-based  [c17][c35]
i64 recordOffset(i64 recordLength, i64 recnum) {
    // CONFLICT: dataformatsa.h also declares recordOffset and automapBeenHere with the
    // same signatures. They are defined here because this header declares them. The
    // integrator must link exactly one definition.
    return recordLength * (recnum - 1);
}

// MDATA4 character record offset(n) = (n - 1) * 2900  [c35]
i64 characterRecordOffset(i64 n) {
    return recordOffset(kCharacterRecordLen, n);
}

// ---------------------------------------------------------------------------
// Identification level
// ---------------------------------------------------------------------------

// Saved IdLevel: 0=All, 3=Barely, counting down  [c35]
IdLevel idLevelFromSavedCode(i32 code) {
    switch (code) {
        case 0:
            return IdLevel::All;
        case 1:
            // UNKNOWN: codes 1 and 2 are inferred from "counts down"; the source does not number them.
            return IdLevel::Mostly;
        case 2:
            // UNKNOWN: see above.
            return IdLevel::Partial;
        case 3:
            return IdLevel::Barely;
        default:
            return IdLevel::Unknown;
    }
}

// Legacy boolean Identified, read only for old saves  [c32]
// UNKNOWN: the source does not give the mapping for the legacy flag. Chosen: true -> All,
// false -> Unknown.
IdLevel idLevelFromLegacyIdentified(bool identified) {
    return identified ? IdLevel::All : IdLevel::Unknown;
}

// ---------------------------------------------------------------------------
// Guild switching and checks
// ---------------------------------------------------------------------------

// Experience is stored in the character only for the current guild. It is copied into the
// guild slot only when the guild changes  [c35] (FRMCTYG.BAS:148-150).
void switchActiveGuild(Character& c, Guild next) {
    if (c.activeGuild == next) {
        return;  // no change, no copy
    }
    GuildMembership& from = c.guilds[static_cast<u32>(c.activeGuild)];
    from.experience = c.experience;
    // UNKNOWN: the source gives only the store direction. Loading the incoming guild's
    // experience into the character is assumed from "Experience is for the current guild".
    const GuildMembership& to = c.guilds[static_cast<u32>(next)];
    c.experience = to.experience;
    c.activeGuild = next;
}

// Stored guild Att/Def are not used; the port recomputes them from level  [c35].
// The original warns when a stored value differs.
// UNKNOWN: this function gets no recomputed values, so it warns unconditionally.
void warnStoredGuildAttDef(Ctx& ctx, Guild guild, i32 storedAtt, i32 storedDef) {
    ctx.say("dataformats.guild_att_def_stored",
            "Stored guild Att/Def ignored; recomputed from guild level",
            {static_cast<i64>(static_cast<i32>(guild)), static_cast<i64>(storedAtt),
             static_cast<i64>(storedDef)},
            {});
}

// Save loading refuses unknown race or guild names  [c3].
// UNKNOWN: the GameData field names RaceDef::name and GuildDef::name are assumed.
bool validateSaveNames(Ctx& ctx, const Str& raceName, const Str& guildName) {
    bool raceOk = false;
    for (const auto& r : ctx.data.races) {
        if (r.name == raceName) {
            raceOk = true;
            break;
        }
    }
    bool guildOk = false;
    for (const auto& g : ctx.data.guilds) {
        if (g.name == guildName) {
            guildOk = true;
            break;
        }
    }
    if (!raceOk) {
        ctx.say("dataformats.unknown_race", "Save refused: unknown race name", {}, {raceName});
    }
    if (!guildOk) {
        ctx.say("dataformats.unknown_guild", "Save refused: unknown guild name", {}, {guildName});
    }
    return raceOk && guildOk;
}

// Legacy monster Id to table index  [c28].
// Id 307 is ambiguous (Morgul 278 and Shadow Ninja 389). The port copies to neither index.
// Otherwise the first match wins, as in the original.
// UNKNOWN: MonsterDef::id is the assumed field name.
bool resolveLegacyMonsterId(Ctx& ctx, i32 monsterId, i32& outIndex) {
    if (monsterId == kAmbiguousMonsterId) {
        ctx.say("dataformats.monster_id_ambiguous",
                "Legacy monster Id is shared by two creatures; not resolved",
                {static_cast<i64>(monsterId)}, {});
        return false;
    }
    for (std::size_t i = 0; i < ctx.data.monsters.size(); ++i) {
        if (ctx.data.monsters[i].id == monsterId) {
            outIndex = static_cast<i32>(i);
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// File names and duplicate check
// ---------------------------------------------------------------------------

// UNKNOWN: the source does not give the sanitise rule. Chosen: replace characters
// that are invalid in Windows file names, and control characters, with '_'.
Str sanitiseFileName(const Str& name) {
    Str out;
    for (i64 i = 0; i < static_cast<i64>(name.size()); ++i) {
        const char c = name[i];
        const bool bad = c == '\\' || c == '/' || c == ':' || c == '*' || c == '?' ||
                         c == '"' || c == '<' || c == '>' || c == '|' ||
                         static_cast<unsigned char>(c) < 32;
        out += bad ? '_' : c;
    }
    return out;
}

// Character creation compares sanitised file names  [c53].
// UNKNOWN: the case sensitivity of the comparison is not stated. Ordinal is used.
bool characterNameCollides(const Vec<Str>& existingNames, const Str& candidate) {
    const Str target = sanitiseFileName(candidate);
    for (const auto& n : existingNames) {
        if (sanitiseFileName(n) == target) {
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// Little-endian field readers and writers  [c17][c35][c91]
// Integer and Word = 2 bytes, Long and Single = 4, Currency = 8.
// On a failed read the out value is set to 0 and false is returned.
// The port refuses instead of silently using 0 (see [c3]).
// ---------------------------------------------------------------------------

bool readLittleEndian(const Vec<u8>& buf, i64 offset, i32 nbytes, u64& out) {
    out = 0;
    if (nbytes < 1 || nbytes > 8 || !rangeOk(buf, offset, nbytes)) {
        return false;
    }
    for (i32 i = nbytes - 1; i >= 0; --i) {
        out = (out << 8) | static_cast<u64>(buf[offset + i]);
    }
    return true;
}

bool readInteger(const Vec<u8>& buf, i64 offset, i16& out) {
    u64 v = 0;
    if (!readLittleEndian(buf, offset, kIntegerBytes, v)) {
        out = 0;
        return false;
    }
    out = static_cast<i16>(static_cast<u16>(v));
    return true;
}

bool readWord(const Vec<u8>& buf, i64 offset, u16& out) {
    u64 v = 0;
    if (!readLittleEndian(buf, offset, kIntegerBytes, v)) {
        out = 0;
        return false;
    }
    out = static_cast<u16>(v);
    return true;
}

bool readLong(const Vec<u8>& buf, i64 offset, i32& out) {
    u64 v = 0;
    if (!readLittleEndian(buf, offset, kLongBytes, v)) {
        out = 0;
        return false;
    }
    out = static_cast<i32>(static_cast<u32>(v));
    return true;
}

bool readLongWord(const Vec<u8>& buf, i64 offset, u32& out) {
    u64 v = 0;
    if (!readLittleEndian(buf, offset, kLongBytes, v)) {
        out = 0;
        return false;
    }
    out = static_cast<u32>(v);
    return true;
}

bool readSingle(const Vec<u8>& buf, i64 offset, f32& out) {
    u64 v = 0;
    out = 0.0f;
    if (!readLittleEndian(buf, offset, kSingleBytes, v)) {
        return false;
    }
    const u32 bits = static_cast<u32>(v);
    static_assert(sizeof(f32) == sizeof(u32), "Single must be 4 bytes");
    std::memcpy(&out, &bits, sizeof(out));
    return true;
}

// Currency: 64-bit integer in 1/10000 gold  [c17][c35]
bool readCurrency(const Vec<u8>& buf, i64 offset, Currency& out) {
    u64 v = 0;
    out = 0;
    if (!readLittleEndian(buf, offset, kCurrencyBytes, v)) {
        return false;
    }
    out = static_cast<Currency>(static_cast<i64>(v));
    return true;
}

// String*n: n bytes, space-padded  [c35].
// UNKNOWN: the source does not say whether padding is stripped on read. Chosen: strip
// trailing spaces.
bool readStringN(const Vec<u8>& buf, i64 offset, i64 n, Str& out) {
    out = Str();
    if (n < 0 || !rangeOk(buf, offset, n)) {
        return false;
    }
    Str s;
    for (i64 i = 0; i < n; ++i) {
        s += static_cast<char>(buf[offset + i]);
    }
    std::size_t e = s.size();
    while (e > 0 && s[e - 1] == ' ') {
        --e;
    }
    s.resize(e);
    out = s;
    return true;
}

void writeLittleEndian(Vec<u8>& buf, i64 offset, u64 value, i32 nbytes) {
    if (offset < 0 || nbytes < 1 || nbytes > 8) {
        return;
    }
    ensureSize(buf, offset + nbytes);
    for (i32 i = 0; i < nbytes; ++i) {
        buf[offset + i] = static_cast<u8>((value >> (8 * i)) & 0xFFu);
    }
}

// Writes n bytes, truncating a longer string and space-padding a shorter one  [c35].
void writeStringN(Vec<u8>& buf, i64 offset, const Str& s, i64 n) {
    if (offset < 0 || n < 0) {
        return;
    }
    ensureSize(buf, offset + n);
    for (i64 i = 0; i < n; ++i) {
        const char c = (i < static_cast<i64>(s.size())) ? s[i] : ' ';
        buf[offset + i] = static_cast<u8>(c);
    }
}

// ---------------------------------------------------------------------------
// Automap
// ---------------------------------------------------------------------------

// been_here = word & (1 << 23)  [c35]
// UNKNOWN: bit numbering as stated in the source. Verified only as the word/bit rule.
bool automapBeenHere(u32 word) {
    // CONFLICT: also declared in dataformatsa.h. See recordOffset.
    return (word & (static_cast<u32>(1) << kAutomapBeenHereBit)) != 0u;
}

// ---------------------------------------------------------------------------
// Guild log (MDATA9.MDR)  [c10]
// Layout: header count u32 (written, ignored on read); then per entry:
//   guild index u32, day serial i64, byte length u32, text UTF-8.
// Reading stops at end of file. A truncated tail is dropped.
// ---------------------------------------------------------------------------

// GuildLogEntry mapping: guild index is GuildLogEntry::guild; the file's day serial is
// stored in GuildLogEntry::stamp (the header orders entries by largest stamp, newest first).
// UNKNOWN: the day serial is assumed to be the ordering key that the header calls stamp.
Vec<GuildLogEntry> readGuildLog(const Vec<u8>& bytes) {
    Vec<GuildLogEntry> out;
    if (!rangeOk(bytes, 0, kLongBytes)) {
        return out;  // no header count
    }
    i64 pos = kLongBytes;  // header count is skipped
    while (true) {
        if (!rangeOk(bytes, pos, 4 + 8 + 4)) {
            break;  // truncated fixed part: drop the tail
        }
        u32 guildIdx = 0;
        u64 daySerial = 0;
        u32 byteLen = 0;
        readLongWord(bytes, pos, guildIdx);
        readLittleEndian(bytes, pos + 4, kCurrencyBytes, daySerial);
        readLongWord(bytes, pos + 12, byteLen);
        pos += 16;
        if (!rangeOk(bytes, pos, static_cast<i64>(byteLen))) {
            break;  // truncated text: drop the tail
        }
        Str text;
        for (i64 i = 0; i < static_cast<i64>(byteLen); ++i) {
            text += static_cast<char>(bytes[pos + i]);
        }
        pos += static_cast<i64>(byteLen);
        GuildLogEntry e;
        e.guild = static_cast<i32>(guildIdx);
        e.stamp = static_cast<i64>(daySerial);
        e.text = text;
        out.push_back(e);
    }
    return out;
}

Vec<u8> writeGuildLog(const Vec<GuildLogEntry>& entries) {
    Vec<u8> buf;
    writeLittleEndian(buf, 0, static_cast<u64>(entries.size()), kLongBytes);
    i64 pos = kLongBytes;
    for (const auto& e : entries) {
        writeLittleEndian(buf, pos, static_cast<u64>(static_cast<u32>(e.guild)), kLongBytes);
        writeLittleEndian(buf, pos + 4, static_cast<u64>(e.stamp), kCurrencyBytes);
        const i64 len = static_cast<i64>(e.text.size());
        writeLittleEndian(buf, pos + 12, static_cast<u64>(len), kLongBytes);
        pos += 16;
        for (i64 i = 0; i < len; ++i) {
            buf.push_back(static_cast<u8>(e.text[i]));
        }
        pos += len;
    }
    return buf;
}

// ---------------------------------------------------------------------------
// MDATA1 header and table layout  [c13]
// Record size 260 bytes. Record 1 is the version. Records 2..7 are counts, in order:
// races, guilds, item types, item descriptions, monster classes, monster groups.
// ---------------------------------------------------------------------------

bool parseMdata1Header(const Vec<u8>& bytes, Mdata1Header& out) {
    out = Mdata1Header{};
    // Record-less Gets in the original; the port reads record k at 260*(k-1).
    if (!rangeOk(bytes, 0, kMdata1HeaderRecords * kMdata1RecordBytes)) {
        return false;
    }
    // UNKNOWN: value type of the version record. Read as Long.
    if (!readLong(bytes, recordOffset(kMdata1RecordBytes, 1), out.version)) return false;
    // UNKNOWN: count value width. Read as Long (4 bytes).
    if (!readLong(bytes, recordOffset(kMdata1RecordBytes, 2), out.raceCount)) return false;
    if (!readLong(bytes, recordOffset(kMdata1RecordBytes, 3), out.guildCount)) return false;
    if (!readLong(bytes, recordOffset(kMdata1RecordBytes, 4), out.itemTypeCount)) return false;
    if (!readLong(bytes, recordOffset(kMdata1RecordBytes, 5), out.itemDescriptionCount)) return false;
    if (!readLong(bytes, recordOffset(kMdata1RecordBytes, 6), out.monsterClassCount)) return false;
    if (!readLong(bytes, recordOffset(kMdata1RecordBytes, 7), out.monsterGroupCount)) return false;
    return true;
}

// Table index i maps to file record i+4  [c13].
i64 monsterFileRecord(i64 tableIndex) {
    return tableIndex + kMonsterFileBase;
}

// Library record for monster index Ref is read at Ref + 6  [c28].
// Ref is -1 for a hand-built monster, which has no library record (returned as -1).
// UNKNOWN: the relation between Ref+6 and monster file record i+4 is not stated.
i64 libraryRecordForMonster(i64 monsterRef) {
    if (monsterRef < 0) {
        return -1;
    }
    return monsterRef + kLibraryRefBase;
}

// Record 0 handling  [c62]:
//   MDATA2 (spells): record 0 is real and is not skipped.
//   MDATA3 (items):  record 0 is "Hands", real and not skipped.
//   MDATA5 (monsters): record 0 is a duplicate "GOBLIE" and is skipped.
bool skipsRecordZero(i32 mdataFile) {
    return mdataFile == kMonsterMdataFile;
}

// [c62] "Hands" is pinned in pack slot 0 and never moves.
bool isPinnedPackSlot(i32 packSlot) {
    return packSlot == kHandsPackSlot;
}

// Monster record: 12 resistance words, 11 item-slot words, 4 box-chance words, 7 stat words,
// then a dword each for special property, special attack and spell flag  [c13].
bool readMonsterRecordRaw(const Vec<u8>& buf, i64 offset, MonsterRecordRaw& out) {
    i64 pos = offset;
    if (!readInt16Run(buf, pos, out.resistWords.data(), 12)) return false;
    if (!readInt16Run(buf, pos, out.itemSlotWords.data(), 11)) return false;
    if (!readInt16Run(buf, pos, out.boxChanceWords.data(), 4)) return false;
    if (!readInt16Run(buf, pos, out.statWords.data(), 7)) return false;
    if (!readLongWord(buf, pos, out.specialProperty)) return false;
    pos += kLongBytes;
    if (!readLongWord(buf, pos, out.specialAttack)) return false;
    pos += kLongBytes;
    if (!readLongWord(buf, pos, out.spellFlag)) return false;
    return true;
}

// Item record: 7 stats-required words, 7 stat-mod words, then dwords for ability,
// alignment, resistance and charge  [c13].
bool readItemRecordRaw(const Vec<u8>& buf, i64 offset, ItemRecordRaw& out) {
    i64 pos = offset;
    if (!readInt16Run(buf, pos, out.statsRequired.data(), 7)) return false;
    if (!readInt16Run(buf, pos, out.statMods.data(), 7)) return false;
    if (!readLong(buf, pos, out.ability)) return false;
    pos += kLongBytes;
    if (!readLong(buf, pos, out.alignment)) return false;
    pos += kLongBytes;
    if (!readLong(buf, pos, out.resistance)) return false;
    pos += kLongBytes;
    if (!readLong(buf, pos, out.charge)) return false;
    return true;
}

// ---------------------------------------------------------------------------
// MDATA file groups and names  [c96][c57]
// ---------------------------------------------------------------------------

bool isVitalMdata(i32 fileNo) {
    for (i32 f : kVitalMdataFiles) {
        if (f == fileNo) {
            return true;
        }
    }
    return false;
}

bool isRegeneratedMdata(i32 fileNo) {
    for (i32 f : kRegeneratedMdataFiles) {
        if (f == fileNo) {
            return true;
        }
    }
    return false;
}

// UNKNOWN: the source uses both "mdata" and "MDATA" names. Chosen: uppercase, with the
// .MDR extension seen in [c57] (MDATA11.MDR).
Str mdataFileName(i32 fileNo) {
    Str s("MDATA");
    s += i64ToStr(static_cast<i64>(fileNo));
    s += ".MDR";
    return s;
}

// ---------------------------------------------------------------------------
// Mod settings  [c41]
// ---------------------------------------------------------------------------

Str modSettingsSection(const Str& modId) {
    Str s("Mod:");
    s += modId;
    return s;
}

// Cell(s): CR and LF become spaces first. Then "\" -> "\\", "|" -> "\|", ";" -> "\;".
Str escapeModCell(const Str& raw) {
    Str out;
    for (i64 i = 0; i < static_cast<i64>(raw.size()); ++i) {
        const char c = raw[i];
        if (c == '\r' || c == '\n') {
            out += ' ';
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c == '|') {
            out += "\\|";
        } else if (c == ';') {
            out += "\\;";
        } else {
            out += c;
        }
    }
    return out;
}

// EncodeRows: rows joined by ";", cells in a row joined by "|", each cell escaped.
// Quirk kept: one row with one empty cell encodes to "", which decodes to zero rows.
// A row with no cells also encodes to "" and is lost.
Str encodeModRows(const Vec<Vec<Str>>& rows) {
    Str out;
    for (std::size_t r = 0; r < rows.size(); ++r) {
        if (r > 0) {
            out += ';';
        }
        const Vec<Str>& row = rows[r];
        for (std::size_t c = 0; c < row.size(); ++c) {
            if (c > 0) {
                out += '|';
            }
            out += escapeModCell(row[c]);
        }
    }
    return out;
}

// DecodeRows, as in the pseudo-code [c41].
// A trailing lone backslash is kept as a literal backslash.
Vec<Vec<Str>> decodeModRows(const Str& line) {
    Vec<Vec<Str>> rows;
    if (line.empty()) {
        return rows;
    }
    Vec<Str> row;
    Str cell;
    const i64 n = static_cast<i64>(line.size());
    for (i64 i = 0; i < n; ++i) {
        const char c = line[i];
        if (c == '\\' && i + 1 < n) {
            cell += line[i + 1];
            ++i;  // skip the escaped character
        } else if (c == '|') {
            row.push_back(cell);  // end cell
            cell.clear();
        } else if (c == ';') {
            row.push_back(cell);  // end cell
            cell.clear();
            rows.push_back(row);  // end row
            row.clear();
        } else {
            cell += c;
        }
    }
    row.push_back(cell);  // end cell
    rows.push_back(row);  // end row
    return rows;
}

// ReadFlag(mod, key, fallback): null gives fallback; otherwise trim(v) equals "True", ignoring case.
// Here `present` is (value != null) and `text` is the stored value.
bool readModFlag(bool present, const Str& text, bool fallback) {
    if (!present) {
        return fallback;
    }
    return equalsIgnoreCaseTrue(trimWs(text));
}

// WriteFlag writes "True" or "False".
Str modFlagText(bool value) {
    return value ? Str("True") : Str("False");
}

// Has(key) = Read(id, key) != null. Here `readWasNull` is true when the read returned null.
bool modHasKey(bool readWasNull) {
    return !readWasNull;
}

// Write normalisation: null becomes ""; CR and LF become spaces.
Str normaliseModValue(bool isNull, const Str& value) {
    if (isNull) {
        return Str();
    }
    Str out;
    for (i64 i = 0; i < static_cast<i64>(value.size()); ++i) {
        const char c = value[i];
        out += (c == '\r' || c == '\n') ? ' ' : c;
    }
    return out;
}

// tag = [ m.Id + "@" + m.Manifest.Version for m in Live ]; an empty Vec stands for null  [c40]
Vec<Str> buildModSaveTag(const Vec<ModRef>& live) {
    Vec<Str> tag;
    for (const auto& m : live) {
        Str t = m.id;
        t += "@";
        t += m.version;
        tag.push_back(t);
    }
    return tag;
}

// Split('@')[0]: the text before the first '@', or the whole text if there is none  [c40]
Str modIdOfTag(const Str& tag) {
    Str id;
    for (i64 i = 0; i < static_cast<i64>(tag.size()); ++i) {
        if (tag[i] == '@') {
            break;
        }
        id += tag[i];
    }
    return id;
}

// MissingFrom(tag): tags whose mod id is not among the live ids  [c40]
Vec<Str> missingModTags(const Vec<Str>& tag, const Vec<Str>& liveIds) {
    Vec<Str> missing;
    for (const auto& t : tag) {
        const Str id = modIdOfTag(t);
        bool live = false;
        for (const auto& l : liveIds) {
            if (l == id) {
                live = true;
                break;
            }
        }
        if (!live) {
            missing.push_back(t);
        }
    }
    return missing;
}

// ---------------------------------------------------------------------------
// Store key and settings file  [c22][c34][c10]
// ---------------------------------------------------------------------------

// Key(id, a) = "{id}:{(int)a}"; alignment 0 = Unaligned  [c22]
// The key uses the enum order (see the UNKNOWN in [c22]).
Str storeKey(i32 itemId, ItemAlign alignment) {
    Str key = i64ToStr(static_cast<i64>(itemId));
    key += ':';
    key += i64ToStr(static_cast<i64>(alignment));
    return key;
}

// The port maps MORDOR.INI to settings.ini and never writes MORDOR.INI  [c34].
Str preloadSettingsFile() {
    return Str("settings.ini");
}

// The port reads MORDOR.INI only as a fallback for keys absent from settings.ini  [c10].
bool legacyIniFallbackApplies(bool keyInSettings) {
    return !keyInSettings;
}

}  // namespace mordor
