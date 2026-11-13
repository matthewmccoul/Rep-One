#pragma once
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_item.h"
#include "mordor/state_character.h"
// FIXED: GuildLogEntry is declared in city2a.h (API INDEX); it was used here without an include.
#include "mordor/city2a.h"

// return type (i64) in the header that declares it; this file cannot fix that.

namespace mordor {

// ---- [c10] Option flags (port settings.ini)
// Flag(k): true iff text equalsIgnoreCase "True" (caller supplies default "False"; no trim).
bool optionFlagValue(const Str& text);
// Welcomed: NotFirstTime contains a capital 'T' (case-sensitive ordinal), so "True" also counts.
bool welcomedFlagValue(const Str& notFirstTime);
// LimitationNoticeSeen: Limited contains a capital 'T' (case-sensitive ordinal).
bool limitationNoticeSeenValue(const Str& limited);

// ---- [c17][c35] Record addressing
// offset = recordLength * (recnum - 1); recnum is 1-based; returns -1 if recnum < 1.
i64 recordOffset(i64 recordLength, i64 recnum);

// ---- [c35] MDATA4 character records
inline constexpr i64 kCharacterRecordLen = 2900;
// CONFLICT: last MDATA4 record is 2887 bytes (T5810) vs Len 2900 [c91]; padding UNKNOWN.
inline constexpr i64 kCharacterTypeBytes = 2887;
// Guilds joined block starts at this offset in a character record (confirmed against an edited copy).
inline constexpr i64 kGuildsJoinedOffset = 1678;
// CONFLICT: 16 guild status records [c88][c91] vs kGuildCount 12; purpose of extra 4 UNKNOWN.
inline constexpr i32 kGuildStatusRecordCount = 16;
// CONFLICT: Pack(40) gives 41 slots; kPackSlots in state_common is 40.
inline constexpr i32 kPackFileSlots = 41;
// offset(n) = (n-1) * kCharacterRecordLen; n is the 1-based MDATA4 record number.
i64 characterRecordOffset(i64 n);

// ---- [c35] Character derived values
// MaxSpellPoints = (INT + WIS) * 5 is maxSpellPoints() in casting2.h (API INDEX); not redeclared here.
// Alignment 0=Good,1=Neutral,2=Evil and Sex 0=Male,1=Female map directly to Align and Sex (state_character.h).

// [c35] IdLevel saved code: 0=All, 3=Barely (counts down).
// UNKNOWN: meaning of codes 1 and 2 is not stated.
// CONFLICT: state_common IdLevel uses Unknown=0..All=4, which disagrees with saved codes.
IdLevel idLevelFromSavedCode(i32 code);

// [c32] Legacy Identified flag, read only for old saves; the port never writes it.
// The original turned Partial/Mostly into Barely; Identified=true maps to Barely here.
// UNKNOWN: which level "Known" maps to is not stated.
IdLevel idLevelFromLegacyIdentified(bool identified);

// [c35] Experience is stored only for the active guild; it is copied into the guild slot only when the guild changes (FRMCTYG.BAS:148-150).
// UNKNOWN: which slot receives the copy; chosen: the slot of the guild being left, then load the slot of the guild entered.
void switchActiveGuild(Character& c, Guild next);

// [c35] Port does not store guild Att/Def and recomputes them from level; a differing stored value is warned about, not used.
void warnStoredGuildAttDef(Ctx& ctx, Guild guild, i32 storedAtt, i32 storedDef);

// [c3] Save load refuses an unknown race or guild name (no silent 0 as in MAINMOD.BAS:349-351).
bool validateSaveNames(Ctx& ctx, const Str& raceName, const Str& guildName);

// [c28] Legacy monster Id 307 is two creatures (index 278 and 389); returns false with no index for it.
// Other ids take the first match in the monster table.
bool resolveLegacyMonsterId(Ctx& ctx, i32 monsterId, i32& outIndex);

// [c53] Duplicate-name check compares sanitised file names.
// UNKNOWN: sanitiser character rules are not stated in the source.
Str sanitiseFileName(const Str& name);
bool characterNameCollides(const Vec<Str>& existingNames, const Str& candidate);

// ---- [c22] Saved field types and sizes (little-endian)
inline constexpr i32 kIntegerBytes = 2;
inline constexpr i32 kLongBytes = 4;
inline constexpr i32 kSingleBytes = 4;
inline constexpr i32 kCurrencyBytes = 8;

// Reads nbytes unsigned little-endian at offset; false if out of range or nbytes not in 1..8.
bool readLittleEndian(const Vec<u8>& buf, i64 offset, i32 nbytes, u64& out);
// Integer: 2 bytes, signed.
bool readInteger(const Vec<u8>& buf, i64 offset, i16& out);
// Word: 2 bytes, unsigned.
bool readWord(const Vec<u8>& buf, i64 offset, u16& out);
// Long: 4 bytes, signed.
bool readLong(const Vec<u8>& buf, i64 offset, i32& out);
// LongWord: 4 bytes, unsigned.
bool readLongWord(const Vec<u8>& buf, i64 offset, u32& out);
// Single: 4-byte IEEE 754 float, little-endian.
bool readSingle(const Vec<u8>& buf, i64 offset, f32& out);
// Currency: 8-byte integer in 1/10000 gold; whole gold is currencyWhole() in math_util.h.
bool readCurrency(const Vec<u8>& buf, i64 offset, Currency& out);
// String*n: n bytes, trailing spaces removed on read.
// UNKNOWN: whether padding is trimmed on read is not stated; chosen: yes.
bool readStringN(const Vec<u8>& buf, i64 offset, i64 n, Str& out);
// Writes nbytes little-endian; does nothing if the range is outside buf.
void writeLittleEndian(Vec<u8>& buf, i64 offset, u64 value, i32 nbytes);
// Writes s into n bytes, space-padded; truncated if longer than n.
void writeStringN(Vec<u8>& buf, i64 offset, const Str& s, i64 n);

inline constexpr i64 kAutomapRowWidth = 30;
inline constexpr i64 kAutomapHeaderSkip = 2;
// index = (L-1)*900 + x + 30*(y-1) + 2; the +2 skips the version and deepest-level header words.
// UNKNOWN: whether indexes are 0-based or 1-based is not stated.
// been_here = word & (1 << 23); word is treated as 32-bit (UNKNOWN: word width in source).
bool automapBeenHere(u32 word);

// Reads entries until end of file; the header count is ignored; a truncated tail is dropped.
Vec<GuildLogEntry> readGuildLog(const Vec<u8>& bytes);
// Writes the 32-bit count header followed by each entry (guild index, day serial, byte length, UTF-8 text).
Vec<u8> writeGuildLog(const Vec<GuildLogEntry>& entries);

// ---- [c13] MDATA1 header
inline constexpr i32 kMdata1RecordBytes = 260;

struct Mdata1Header {
    i32 version = 0;               // record 1; UNKNOWN: value type
    i32 raceCount = 0;             // record 2
    i32 guildCount = 0;            // record 3
    i32 itemTypeCount = 0;         // record 4
    i32 itemDescriptionCount = 0;  // record 5
    i32 monsterClassCount = 0;     // record 6
    i32 monsterGroupCount = 0;     // record 7
};
// Reads records 1-7 with record-less sequential reads, as the original does.
// UNKNOWN: position of each count within its 260-byte record; assumed first Integer.
// False if buf is too short.
bool parseMdata1Header(const Vec<u8>& bytes, Mdata1Header& out);

// ---- [c13][c28][c62] Table and record numbering
// Table index i is file record i+4.
// UNKNOWN: whether this applies to MDATA5 given its record 0 handling [c62].
i64 monsterFileRecord(i64 tableIndex);
// Library record for monster index Ref is read at Ref+6.
// UNKNOWN: relation to monster file records is not stated.
i64 libraryRecordForMonster(i64 monsterRef);
// Record 0 is skipped for MDATA5 (duplicate GOBLIE); MDATA2 (spells) and MDATA3 (items) record 0 are real.
bool skipsRecordZero(i32 mdataFile);
// "Hands" (MDATA3 record 0, unarmed attack) is pinned in pack slot 0 and never moves.
inline constexpr i32 kHandsPackSlot = 0;
bool isPinnedPackSlot(i32 packSlot);

// Monster record words: 12 resistance, 11 item-slot, 4 box-chance, 7 stat; dwords for special property, special attack, spell flag.
struct MonsterRecordRaw {
    Arr<i16, 12> resistWords{};
    Arr<i16, 11> itemSlotWords{};
    Arr<i16, 4> boxChanceWords{};
    Arr<i16, 7> statWords{};
    u32 specialProperty = 0;
    u32 specialAttack = 0;
    u32 spellFlag = 0;
};
// UNKNOWN: signedness of the words is not stated.
bool readMonsterRecordRaw(const Vec<u8>& buf, i64 offset, MonsterRecordRaw& out);

// Item record words: 7 stats-required, 7 stat-mod; dwords for ability, alignment, resistance, charge.
struct ItemRecordRaw {
    Arr<i16, 7> statsRequired{};
    Arr<i16, 7> statMods{};
    i32 ability = 0;
    i32 alignment = 0;
    i32 resistance = 0;
    i32 charge = 0;
};
// UNKNOWN: signedness of the words is not stated.
bool readItemRecordRaw(const Vec<u8>& buf, i64 offset, ItemRecordRaw& out);

// ---- [c96] Data file groups
// Vital, never delete: mdata1, mdata2, mdata3, mdata5, mdata11 (mg11.dll images are also vital).
inline constexpr Arr<i32, 5> kVitalMdataFiles{1, 2, 3, 5, 11};
// Regenerated on next start if deleted (non-vital).
inline constexpr Arr<i32, 10> kRegeneratedMdataFiles{4, 6, 7, 8, 9, 10, 12, 13, 14, 15};
// UNKNOWN: mdata16 to mdata20 are referenced in [c87] but never described; mdata1 annotated "gamedata ?"; the "V" markers are not explained.
bool isVitalMdata(i32 fileNo);
bool isRegeneratedMdata(i32 fileNo);
// Returns "MDATA<n>.MDR".
// UNKNOWN: the wiki uses lowercase mdata; the port's casing is not stated.
Str mdataFileName(i32 fileNo);

// ---- [c24][c57] Shipped data fixture
inline constexpr i32 kShippedRaces = 9;
inline constexpr i32 kShippedGuilds = 12;
inline constexpr i32 kShippedSpells = 105;
inline constexpr i32 kShippedItemRecords = 366;
inline constexpr i32 kShippedMonsterRows = 401;
inline constexpr i32 kShippedPlayableFloors = 3;
inline constexpr i64 kDemoMdata11Bytes = 68166;

// ---- [c41] Mod settings storage (escaping and flags)
// SectionFor: "Mod:" + modId.
Str modSettingsSection(const Str& modId);
// Cell: CR and LF become spaces; then "\" -> "\\", "|" -> "\|", ";" -> "\;".
// A trailing lone backslash is kept as a literal backslash when decoded.
Str escapeModCell(const Str& raw);
// EncodeRows: cells joined by "|", rows joined by ";".
// Lossy: one row with one empty cell, or a row with no cells, encodes to "" and decodes to zero rows.
Str encodeModRows(const Vec<Vec<Str>>& rows);
// DecodeRows: inverse of encodeModRows; "" gives zero rows.
Vec<Vec<Str>> decodeModRows(const Str& line);
// ReadFlag: fallback when the key is absent (present == false); otherwise trimmed text equalsIgnoreCase "True".
bool readModFlag(bool present, const Str& text, bool fallback);
// WriteFlag: "True" or "False".
Str modFlagText(bool value);
// Has(key): true iff the read did not return null.
bool modHasKey(bool readWasNull);
// Write normalisation: null becomes ""; CR and LF become spaces.
Str normaliseModValue(bool isNull, const Str& value);

// ---- [c40] Mod save tag
struct ModRef {
    Str id{};
    Str version{};
};
// tag = [ id + "@" + version for each live mod ]; an empty result stands for null.
Vec<Str> buildModSaveTag(const Vec<ModRef>& live);
// Id part of a tag: Split('@')[0].
Str modIdOfTag(const Str& tag);
// Tags whose id is not in liveIds.
Vec<Str> missingModTags(const Vec<Str>& tag, const Vec<Str>& liveIds);

// ---- [c22] Store key (port)
// Key(id, a) = "{id}:{(int)a}".
// UNKNOWN: ItemAlign enum order beyond Unaligned=0 is not stated in the source.
Str storeKey(i32 itemId, ItemAlign alignment);

// ---- [c34][c10] Preload Character settings path
// The port maps MORDOR.INI to settings.ini.
Str preloadSettingsFile();
// The port never writes MORDOR.INI.
inline constexpr bool kMayWriteLegacyIni = false;
// MORDOR.INI is read only as a fallback for keys absent from settings.ini.
// UNKNOWN: whether any write path to MORDOR.INI remains is not stated.
bool legacyIniFallbackApplies(bool keyInSettings);

}  // namespace mordor
