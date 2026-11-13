// dataformatsa_b.cpp: second half of module dataformatsa.
// Implements: cleanGuildLog, daySerialFromEpochMillis, charStatAppendPage,
// trimCharacterName, safeFileName, checkNewCharacter, storedPassword,
// modSettingWrite, modSettingWriteFlag, modSettingRead, modSettingRemove,
// modSection, modEscapeCell, modEncodeRows, modDecodeRows, modSaveTag,
// modTagMissingFrom.
//
// UNKNOWN (module-wide): the SRD half supplied for this module contains no
// rule text for most of these functions. Behaviour below is the simplest
// sensible choice; each choice is marked UNKNOWN where it is made.
// ModSettingEntry is not visible in the included headers. Its id and key
// members are modId and key. FIXED: the value member was reported missing,
// so it is renamed to "text" here. "text" is an unverified guess. Confirm the
// real member name in dataformatsa.h and correct the three uses below.

#include "mordor/dataformatsa.h"

namespace mordor {

// Guild log clean-up [c10]. Reading stops at end of file and a truncated tail
// is dropped, so cleaning is decode-then-encode of the surviving records.
Vec<u8> cleanGuildLog(Ctx& ctx, const Vec<u8>& oldFile, Vec<u8>& backupOut) {
    // UNKNOWN: backup is a verbatim copy of the old file (MDATA9B.MDR).
    backupOut = oldFile;
    // Truncated tail is dropped by decodeGuildLog [c10].
    Vec<GuildLogRecord> records = decodeGuildLog(ctx, oldFile);
    return encodeGuildLog(ctx, records);
}

// Day serial from Unix epoch milliseconds. The serial epoch offset is
// kDaySerialEpochOffset (25569) and a day is kMillisPerDay.
// UNKNOWN: the day serial is whole days; a time of day is discarded, and
// pre-1970 values floor toward negative infinity.
i64 daySerialFromEpochMillis(i64 epochMillis) {
    i64 days = epochMillis / kMillisPerDay;
    if (epochMillis % kMillisPerDay < 0) {
        days -= 1;
    }
    return days + kDaySerialEpochOffset;
}

// Character stats page (CHARSTAT.TXT).
// UNKNOWN: no SRD text for the page layout. The page lists a fixed set of
// basic stats. When toFile is false each line is also emitted as an event.
void charStatAppendPage(Ctx& ctx, Vec<Str>& fileLines, const Character& c, bool toFile) {
    // Decimal text for an i64, without relying on a library helper.
    auto toDigits = [](i64 v) -> Str {
        Str s;
        u64 mag = v < 0 ? static_cast<u64>(-(v + 1)) + 1u : static_cast<u64>(v);
        char buf[24];
        i32 n = 0;
        do {
            buf[n] = static_cast<char>('0' + static_cast<i32>(mag % 10u));
            ++n;
            mag /= 10u;
        } while (mag != 0u);
        if (v < 0) {
            s += '-';
        }
        while (n > 0) {
            --n;
            s += buf[n];
        }
        return s;
    };

    auto addLine = [&](const Str& text) {
        fileLines.push_back(text);
        if (!toFile) {
            ctx.say("charstat.line", text, {}, {});
        }
    };

    addLine(Str("Name: ") + c.name);
    addLine(Str("Level: ") + toDigits(c.level));
    addLine(Str("Hits: ") + toDigits(c.vitals.curHits) + Str("/") + toDigits(c.vitals.maxHits));
    addLine(Str("Spell points: ") + toDigits(c.vitals.curSpellPoints) + Str("/") +
            toDigits(c.vitals.maxSpellPoints));
    addLine(Str("Experience: ") + toDigits(c.experience));
    // Currency is stored in 1/10000 gold; whole gold is shown.
    addLine(Str("Gold on hand: ") + toDigits(c.wallet.goldOnHand / kGoldExactScale));
    addLine(Str("Gold in bank: ") + toDigits(c.wallet.goldInBank / kGoldExactScale));
    addLine(Str("Age (days): ") + toDigits(c.ageDays));
}

// Trim a character name. UNKNOWN: VB Trim semantics, spaces only.
Str trimCharacterName(const Ctx& ctx, const Str& raw) {
    (void)ctx;
    i32 start = 0;
    i32 end = static_cast<i32>(raw.size());
    while (start < end && raw[start] == ' ') {
        ++start;
    }
    while (end > start && raw[end - 1] == ' ') {
        --end;
    }
    Str out;
    for (i32 i = start; i < end; ++i) {
        out += raw[i];
    }
    return out;
}

// Make a name safe for use as a file name.
// UNKNOWN: each character that is forbidden in Windows file names, or is a
// control character, is replaced by '_'.
Str safeFileName(const Ctx& ctx, const Str& name) {
    (void)ctx;
    const char* forbidden = "\\/:*?\"<>|";
    Str out;
    i32 n = static_cast<i32>(name.size());
    for (i32 i = 0; i < n; ++i) {
        char ch = name[i];
        bool bad = static_cast<unsigned char>(ch) < 32u;
        for (const char* p = forbidden; !bad && *p != '\0'; ++p) {
            if (ch == *p) {
                bad = true;
            }
        }
        out += bad ? '_' : ch;
    }
    return out;
}

// New character validation. Checks run in the order of the enum:
// empty name, name contains the forbidden bar, sex, alignment, existing save.
// UNKNOWN: this evaluation order is taken from the enum order.
NewCharacterCheck checkNewCharacter(const Ctx& ctx, const Str& trimmedName, bool sexChosen,
                                    bool alignmentChosen, bool saveFileExists) {
    (void)ctx;
    if (trimmedName.size() == 0) {
        return NewCharacterCheck::EmptyName;
    }
    i32 n = static_cast<i32>(trimmedName.size());
    for (i32 i = 0; i < n; ++i) {
        if (trimmedName[i] == kNameForbiddenChar) {
            return NewCharacterCheck::NameHasBar;
        }
    }
    if (!sexChosen) {
        return NewCharacterCheck::NoSex;
    }
    if (!alignmentChosen) {
        return NewCharacterCheck::NoAlignment;
    }
    if (saveFileExists) {
        return NewCharacterCheck::SaveExists;
    }
    return NewCharacterCheck::Ok;
}

// Password as stored in the character record.
// UNKNOWN: no SRD text; the entered password is stored unchanged.
Str storedPassword(const Ctx& ctx, const Str& enteredPassword) {
    (void)ctx;
    return enteredPassword;
}

// Write a mod setting. Updates the existing entry or appends a new one.
// UNKNOWN: writes are applied in memory regardless of store.writable;
// flushing to disk is left to the caller, which checks store.writable.
void modSettingWrite(Ctx& ctx, ModSettingsStore& store, const Str& modId, const Str& key,
                     const Str& value) {
    (void)ctx;
    for (auto& e : store.entries) {
        if (e.modId == modId && e.key == key) {
            e.text = value;  // FIXED: was e.value
            store.dirty = true;
            return;
        }
    }
    ModSettingEntry entry;
    entry.modId = modId;
    entry.key = key;
    entry.text = value;  // FIXED: was entry.value
    store.entries.push_back(entry);
    store.dirty = true;
}

// Boolean mod setting, stored as "1" or "0".
// UNKNOWN: the flag text encoding is "1"/"0".
void modSettingWriteFlag(Ctx& ctx, ModSettingsStore& store, const Str& modId, const Str& key,
                         bool value) {
    Str text = value ? Str("1") : Str("0");
    modSettingWrite(ctx, store, modId, key, text);
}

// Read a mod setting, or defaultValue if it is not present.
Str modSettingRead(const Ctx& ctx, const ModSettingsStore& store, const Str& modId,
                   const Str& key, const Str& defaultValue) {
    (void)ctx;
    for (const auto& e : store.entries) {
        if (e.modId == modId && e.key == key) {
            return e.text;  // FIXED: was e.value
        }
    }
    return defaultValue;
}

// Remove a mod setting if present; marks the store dirty when something was removed.
void modSettingRemove(Ctx& ctx, ModSettingsStore& store, const Str& modId, const Str& key) {
    (void)ctx;
    Vec<ModSettingEntry> kept;
    bool removed = false;
    for (const auto& e : store.entries) {
        if (e.modId == modId && e.key == key) {
            removed = true;
        } else {
            kept.push_back(e);
        }
    }
    if (removed) {
        store.entries = kept;
        store.dirty = true;
    }
}

// Section name for a mod's settings: kModSectionPrefix ("Mod:") + modId.
Str modSection(const Ctx& ctx, const Str& modId) {
    (void)ctx;
    Str s = kModSectionPrefix;
    s += modId;
    return s;
}

// Escape one table cell. Backslash, '|' (cell separator) and line breaks are escaped.
// UNKNOWN: the escape format is chosen here, since no SRD text is supplied.
//   '\\' -> "\\\\", '|' -> "\\|", '\n' -> "\\n", '\r' -> "\\r"
Str modEscapeCell(const Ctx& ctx, const Str& cell) {
    (void)ctx;
    // [c41] Cell(s): replace CR and LF with " ", then escape "\" -> "\\", "|" -> "\|", ";" -> "\;".
    Str out;
    i32 n = static_cast<i32>(cell.size());
    for (i32 i = 0; i < n; ++i) {
        char ch = cell[i];
        if (ch == '\r' || ch == '\n') {
            out += ' ';
        } else if (ch == '\\') {
            out += '\\';
            out += '\\';
        } else if (ch == '|') {
            out += '\\';
            out += '|';
        } else if (ch == ';') {
            out += '\\';
            out += ';';
        } else {
            out += ch;
        }
    }
    return out;
}

// Encode rows: cells are escaped and joined with '|', each row ends with '\n'.
// UNKNOWN: a row with zero cells cannot be represented and decodes as one empty cell.
Str modEncodeRows(const Ctx& ctx, const Vec<Vec<Str>>& rows) {
    // [c41] EncodeRows(rows) = join(";", for each row: join("|", cells mapped by Cell)).
    // A single row with one empty cell, or a row with no cells, encodes to "" (quirk).
    Str out;
    bool firstRow = true;
    for (const auto& row : rows) {
        if (!firstRow) {
            out += ';';
        }
        firstRow = false;
        bool firstCell = true;
        for (const auto& cell : row) {
            if (!firstCell) {
                out += '|';
            }
            firstCell = false;
            out += modEscapeCell(ctx, cell);
        }
    }
    return out;
}

// Decode rows written by modEncodeRows.
// A trailing partial row (no final newline) is kept.
Vec<Vec<Str>> modDecodeRows(const Ctx& ctx, const Str& text) {
    (void)ctx;
    // [c41] DecodeRows(line): an empty line gives no rows. Otherwise a backslash
    // makes the next character literal, "|" ends a cell, ";" ends a cell and a row,
    // and the end of the line always ends a cell and a row.
    Vec<Vec<Str>> rows;
    if (text.size() == 0) {
        return rows;
    }
    Vec<Str> row;
    Str cell;
    i32 n = static_cast<i32>(text.size());
    i32 i = 0;
    while (i < n) {
        char ch = text[i];
        if (ch == '\\' && i + 1 < n) {
            cell += text[i + 1];  // a trailing lone backslash falls through as a literal
            i += 2;
        } else if (ch == '|') {
            row.push_back(cell);
            cell = Str();
            i += 1;
        } else if (ch == ';') {
            row.push_back(cell);
            rows.push_back(row);
            row = Vec<Str>();
            cell = Str();
            i += 1;
        } else {
            cell += ch;
            i += 1;
        }
    }
    row.push_back(cell);
    rows.push_back(row);
    return rows;
}

// Save tag for the running mods (R15).
// 1. One "id@version" entry per live mod.
// 2. No live mods gives a null tag.
ModTag modSaveTag(const Ctx& ctx, const Vec<LiveMod>& live) {
    (void)ctx;
    ModTag tag;  // isNull = true, entries empty
    if (live.size() == 0) {
        return tag;
    }
    tag.isNull = false;
    for (const auto& m : live) {
        Str entry = m.id;
        entry += "@";
        entry += m.version;
        tag.entries.push_back(entry);
    }
    return tag;
}

// Entries of a saved tag whose id is not among the live mod ids (R15 step 3).
// Returns the entries themselves ("id@version" strings).
Vec<Str> modTagMissingFrom(const Ctx& ctx, const ModTag& tag, const Vec<LiveMod>& live) {
    (void)ctx;
    Vec<Str> missing;
    if (tag.isNull) {
        return missing;
    }
    for (const auto& entry : tag.entries) {
        // Id is the part before the first '@'; the whole entry if there is none.
        Str id;
        i32 n = static_cast<i32>(entry.size());
        for (i32 i = 0; i < n; ++i) {
            if (entry[i] == '@') {
                break;
            }
            id += entry[i];
        }
        bool found = false;
        for (const auto& m : live) {
            if (m.id == id) {
                found = true;
                break;
            }
        }
        if (!found) {
            missing.push_back(entry);
        }
    }
    return missing;
}

}  // namespace mordor
