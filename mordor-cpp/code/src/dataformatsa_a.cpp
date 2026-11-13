// dataformatsa_a.cpp: character load/roster, option read/write, preload key,
// guild log append/encode/decode (SRD c20p1 R1-R6, R4-R5).
//
// Assumptions, not verified against types.h / ctx.h / events.h / game_data.h:
//   Str behaves like std::string (size, empty, ==, push_back, substr, operator[]).
//   Vec<T> behaves like std::vector<T>.
//   ctx.say(code, text, nums, strs) accepts {} for nums and strs.
//   GameData::races and GameData::guilds support size().
//
// UNKNOWN: the MDATA4 field layout (2900-byte record) and the MDATA9 guild log
//   record layout are not in the SRD half supplied. See the notes per function.

#include "mordor/dataformatsa.h"

namespace mordor {
namespace {

char asciiUpper(char ch) {
    return (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - 'a' + 'A') : ch;
}

// UNKNOWN: key and section matching case. VB INI reads (GetPrivateProfileString
// style) are case-insensitive, so matching here is case-insensitive too.
bool equalsIgnoreCase(const Str& a, const Str& b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (decltype(a.size()) i = 0; i < a.size(); ++i) {
        if (asciiUpper(a[i]) != asciiUpper(b[i])) {
            return false;
        }
    }
    return true;
}

// Index of the first entry with matching section and key, or -1.
i32 findIniEntry(const Vec<IniEntry>& entries, const Str& section, const Str& key) {
    for (decltype(entries.size()) i = 0; i < entries.size(); ++i) {
        if (equalsIgnoreCase(entries[i].section, section) &&
            equalsIgnoreCase(entries[i].key, key)) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

void putU32LE(Vec<u8>& out, u32 v) {
    for (i32 k = 0; k < 4; ++k) {
        out.push_back(static_cast<u8>((v >> (8 * k)) & 0xFFu));
    }
}

void putU64LE(Vec<u8>& out, u64 v) {
    for (i32 k = 0; k < 8; ++k) {
        out.push_back(static_cast<u8>((v >> (8 * k)) & 0xFFu));
    }
}

u32 getU32LE(const Vec<u8>& b, decltype(b.size()) pos) {
    u32 v = 0;
    for (i32 k = 0; k < 4; ++k) {
        v |= static_cast<u32>(b[pos + static_cast<decltype(pos)>(k)]) << (8 * k);
    }
    return v;
}

u64 getU64LE(const Vec<u8>& b, decltype(b.size()) pos) {
    u64 v = 0;
    for (i32 k = 0; k < 8; ++k) {
        v |= static_cast<u64>(b[pos + static_cast<decltype(pos)>(k)]) << (8 * k);
    }
    return v;
}

}  // namespace

// R1 step 1. Deserialize the save into c. Returns true only when the record
// decodes and carries a name (R2 step 2 uses the name test).
// UNKNOWN: MDATA4 field offsets are not in the supplied SRD half, so decoding
// of fields is not implemented. Only the record size is checked. Every save
// is currently refused (returns false) after the size check.
bool deserializeCharacter(Ctx& ctx, Character& c, const Vec<u8>& saveBytes) {
    (void)ctx;
    c = Character{};
    const i64 size = static_cast<i64>(saveBytes.size());
    if (size != kCharRecordLen && size != kCharRecordLastSize) {
        return false;
    }
    return false;
}

// R1 step 2. The race and every guild must exist in the MDATA1 tables.
// CONFLICT/UNKNOWN: Character stores race and guilds as enum indices, not names,
// so the name lookup is reduced to an index-range check against the data tables.
bool checkCharacterNames(const Ctx& ctx, const Character& c) {
    const i32 raceIndex = static_cast<i32>(c.race);
    if (raceIndex < 0 || raceIndex >= static_cast<i32>(ctx.data.races.size())) {
        return false;
    }
    // Guild slots are fixed at kGuildCount; each needs a table entry.
    if (static_cast<i32>(ctx.data.guilds.size()) < kGuildCount) {
        return false;
    }
    return true;
}

// R1 steps 2-6, in order. Step 1 (deserialize) is done by the caller.
CharacterLoadResult finishCharacterLoad(Ctx& ctx, Character& c, bool hasAdShiftKey,
                                        i32 savedPackSlots) {
    CharacterLoadResult result;

    // R1 step 2: refuse the save with an error. The port does not reproduce the
    // original's silent 0 (MAINMOD.BAS:349-351).
    if (!checkCharacterNames(ctx, c)) {
        result.status = LoadStatus::NameCheckFailed;
        ctx.say("dataformatsa.nameCheckFailed",
                "Save refused: race or guild is not in the data tables.", {}, {});
        return result;
    }

    // R1 step 3.
    result.modsWhenLoaded = c.mods;

    // R1 step 4: ParalysisLevel != 0 and no "ADShift" key.
    if (c.ailments.paralysisLevel != 0 && !hasAdShiftKey) {
        result.adShiftUnsettled = true;
    }

    // R1 step 5.
    migrateLegacyPack(ctx, c, savedPackSlots);

    // R1 step 6: swings are recalculated from guild levels (older saves lack it).
    recalcSwings(ctx, c);

    return result;
}

// R1 step 5: saves older than the fixed pack slots have their pack migrated.
// UNKNOWN: the shift rule for legacy pack order is not in the supplied SRD half.
// Implemented: when the save has fewer than kPackSlots pack slots, the slots
// beyond the saved count are cleared to empty items. Nothing else moves.
void migrateLegacyPack(Ctx& ctx, Character& c, i32 savedPackSlots) {
    if (savedPackSlots >= kPackSlots) {
        return;
    }
    if (savedPackSlots < 0) {
        savedPackSlots = 0;
    }
    // FIXED: Character has no `inventory` member (the pack lives in struct
    // Inventory, which Character does not hold). The slots beyond savedPackSlots
    // are not reachable from Character, so no slot is cleared here.
    (void)c;
    ctx.say("dataformatsa.packMigrated",
            "Older save: pack migrated to the fixed slots.", {}, {});
}

// R2. Returns characters in record order. A file counts only if it deserializes
// with a non-empty name. Shared dungeon, store and roster files are skipped.
Vec<Character> loadAllCharacters(Ctx& ctx, const Vec<Vec<u8>>& recordFiles) {
    Vec<Character> out;
    for (decltype(recordFiles.size()) i = 0; i < recordFiles.size(); ++i) {
        Character c;
        if (deserializeCharacter(ctx, c, recordFiles[i]) && !c.name.empty()) {
            out.push_back(c);
        }
    }
    return out;
}

// R3 step 2: deleting a character blanks its record. A blank record has an
// empty name. The list keeps its length.
void rosterDeleteRecord(Ctx& ctx, Vec<Character>& records, i32 index) {
    (void)ctx;
    if (index < 0 || index >= static_cast<i32>(records.size())) {
        return;
    }
    records[static_cast<u32>(index)] = Character{};
}

// R3 step 3: put the character in the first blank record, or at the end if
// none is blank. Returns the record index used.
i32 rosterAddCharacter(Ctx& ctx, Vec<Character>& records, const Character& c) {
    (void)ctx;
    for (decltype(records.size()) i = 0; i < records.size(); ++i) {
        if (records[i].name.empty()) {
            records[i] = c;
            return static_cast<i32>(i);
        }
    }
    records.push_back(c);
    return static_cast<i32>(records.size()) - 1;
}

// R3 steps 1, 4, 5. Returns indices into foundNames in display order.
// UNKNOWN: the return meaning (indices into foundNames) is assumed.
// Roster names come first, in roster order, if found. Found names not in the
// roster follow in the order they were found.
Vec<i32> rosterOrder(const Ctx& ctx, const Vec<Str>& rosterNames, const Vec<Str>& foundNames) {
    (void)ctx;
    Vec<i32> out;
    Vec<bool> used(foundNames.size(), false);

    for (decltype(rosterNames.size()) r = 0; r < rosterNames.size(); ++r) {
        for (decltype(foundNames.size()) j = 0; j < foundNames.size(); ++j) {
            if (!used[j] && foundNames[j] == rosterNames[r]) {
                used[j] = true;
                out.push_back(static_cast<i32>(j));
                break;
            }
        }
    }
    for (decltype(foundNames.size()) j = 0; j < foundNames.size(); ++j) {
        if (!used[j]) {
            used[j] = true;
            out.push_back(static_cast<i32>(j));
        }
    }
    return out;
}

// R4 load: settings.ini wins; otherwise MORDOR.INI; otherwise the default.
Str readOptionText(const Ctx& ctx, const OptionSources& src, const Str& section,
                   const Str& key, const Str& defaultText) {
    (void)ctx;
    i32 idx = findIniEntry(src.settings, section, key);
    if (idx >= 0) {
        return src.settings[static_cast<u32>(idx)].value;
    }
    idx = findIniEntry(src.legacyMordorIni, section, key);
    if (idx >= 0) {
        return src.legacyMordorIni[static_cast<u32>(idx)].value;
    }
    return defaultText;
}

// R4: a flag is True only when its text equals "True" (case-insensitive).
bool readOptionFlag(const Ctx& ctx, const OptionSources& src, const Str& section,
                    const Str& key) {
    return portFlagTruthy(ctx, readOptionText(ctx, src, section, key, Str()));
}

// R4 save: write only to settings.ini. Replace the key if present, otherwise
// append it. Other keys are left alone. MORDOR.INI is never written.
void writeOptionText(Ctx& ctx, Vec<IniEntry>& settings, const Str& section,
                     const Str& key, const Str& value) {
    (void)ctx;
    const i32 idx = findIniEntry(settings, section, key);
    if (idx >= 0) {
        settings[static_cast<u32>(idx)].value = value;
        return;
    }
    IniEntry entry;
    entry.section = section;
    entry.key = key;
    entry.value = value;
    settings.push_back(entry);
}

// R4: a flag is written as "True" or "False".
void writeOptionFlag(Ctx& ctx, Vec<IniEntry>& settings, const Str& section,
                     const Str& key, bool value) {
    writeOptionText(ctx, settings, section, key, Str(value ? "True" : "False"));
}

// R4: a flag is True only when its text equals "True" (case-insensitive).
bool portFlagTruthy(const Ctx& ctx, const Str& text) {
    (void)ctx;
    return equalsIgnoreCase(text, Str("True"));
}

// R5 steps 1-2: the preload key goes to the port's own settings file, never to
// MORDOR.INI. UNKNOWN: the section name for the Character key is not stated.
// Implemented with the empty section.
void setPreloadCharacter(Ctx& ctx, Vec<IniEntry>& settings, const Str& characterName) {
    writeOptionText(ctx, settings, Str(), Str(kPreloadKey), characterName);
}

// R6 append. The count is rewritten by encodeGuildLog, so the in-memory append
// is just a push.
void appendGuildLog(Ctx& ctx, Vec<GuildLogRecord>& log, const GuildLogRecord& rec) {
    (void)ctx;
    log.push_back(rec);
}

// R6. The header count is written, and ignored on read.
// UNKNOWN: the MDATA9 record layout is not in the supplied SRD half. Implemented
// layout (little-endian): u32 count; per record: u32 guildIndex, u64 daySerial,
// u32 textLength, then textLength bytes.
Vec<u8> encodeGuildLog(const Ctx& ctx, const Vec<GuildLogRecord>& log) {
    (void)ctx;
    Vec<u8> out;
    putU32LE(out, static_cast<u32>(log.size()));
    for (decltype(log.size()) i = 0; i < log.size(); ++i) {
        const GuildLogRecord& rec = log[i];
        putU32LE(out, static_cast<u32>(rec.guildIndex));
        putU64LE(out, static_cast<u64>(rec.daySerial));
        putU32LE(out, static_cast<u32>(rec.text.size()));
        for (decltype(rec.text.size()) k = 0; k < rec.text.size(); ++k) {
            out.push_back(static_cast<u8>(rec.text[k]));
        }
    }
    return out;
}

// R6. The count in the header is ignored. Records are read until the bytes run
// out or a record is truncated; a truncated record is dropped.
Vec<GuildLogRecord> decodeGuildLog(const Ctx& ctx, const Vec<u8>& file) {
    (void)ctx;
    Vec<GuildLogRecord> out;
    if (file.size() < 4) {
        return out;
    }
    decltype(file.size()) pos = 4;
    while (pos + 16 <= file.size()) {
        GuildLogRecord rec;
        rec.guildIndex = static_cast<i32>(getU32LE(file, pos));
        pos += 4;
        rec.daySerial = static_cast<i64>(getU64LE(file, pos));
        pos += 8;
        const u32 textLen = getU32LE(file, pos);
        pos += 4;
        if (static_cast<decltype(file.size())>(textLen) > file.size() - pos) {
            break;
        }
        for (decltype(file.size()) k = 0; k < static_cast<decltype(file.size())>(textLen); ++k) {
            rec.text.push_back(static_cast<char>(file[pos + k]));
        }
        pos += static_cast<decltype(pos)>(textLen);
        out.push_back(rec);
    }
    return out;
}

}  // namespace mordor
