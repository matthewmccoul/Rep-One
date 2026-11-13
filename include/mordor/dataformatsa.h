#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"
#include "mordor/combat1a.h"
#include "mordor/guilds3.h"
#include "mordor/items3a.h"

namespace mordor {

// ---- Constants --------------------------------------------------------------

// R6 MDATA4 character record stride (VB3 Len)
inline constexpr i64 kCharRecordLen = 2900;
// R6 size of the last MDATA4 record (size of T5810)
// CONFLICT: wiki says each record is 2900 bytes; docs say last record is 2887. Docs preferred.
inline constexpr i64 kCharRecordLastSize = 2887;
// R6 first character record in MDATA4 (records 1 and 2 are version and count)
inline constexpr i32 kCharRecordFirst = 3;
// R6 GuildsJoined field offset inside the character record
inline constexpr i64 kGuildJoinedOffset = 1678;
// R6 MDATA1 record size
inline constexpr i64 kMdata1RecordSize = 260;
// R6 first MDATA1 table record (records 1-7 are version and counts)
inline constexpr i32 kMdata1FirstTableRecord = 8;
// R6 table index i maps to file record i + kTableToRecordBase
inline constexpr i32 kTableToRecordBase = 4;
// R1 pack slots as saved by the original (Pack(40) = 41 slots)
// CONFLICT: SRD says Pack(40) is 41 slots; state_common kPackSlots is 40. Not reconciled here.
inline constexpr i32 kPackSavedSlots = 41;

// Quick reference: automap index layout
inline constexpr i32 kAutomapLevelStride = 900;
inline constexpr i32 kAutomapRowStride = 30;
inline constexpr i32 kAutomapIndexOffset = 2;
inline constexpr i32 kAutomapBeenHereBit = 23;

// R14 gold exact-value scale (Currency units per gold)
inline constexpr i64 kGoldExactScale = 10000;

// R6 DaySerial: days from 1899-12-30 to 1970-01-01, and milliseconds per day
inline constexpr i64 kDaySerialEpochOffset = 25569;
inline constexpr i64 kMillisPerDay = 86400000;

// R8 forbidden character in names
inline constexpr char kNameForbiddenChar = '|';

// R4 settings file names and preload key
inline constexpr const char* kSettingsFileName = "settings.ini";
inline constexpr const char* kLegacyIniFileName = "MORDOR.INI";
inline constexpr const char* kPreloadKey = "Character";

// R6 guild log files
inline constexpr const char* kGuildLogFileName = "MDATA9.MDR";
inline constexpr const char* kGuildLogBackupName = "MDATA9B.MDR";

// R7 character statistics export
inline constexpr const char* kCharStatFileName = "CHARSTAT.TXT";

// R12 store and R15 roster files
inline constexpr const char* kStoreFileName = "store.json";
inline constexpr const char* kRosterFileName = "roster.json";

// R9 mod settings section prefix
inline constexpr const char* kModSectionPrefix = "Mod:";

// R11 installer target folders
inline constexpr const char* kFullFolder = "game\\full";
inline constexpr const char* kSharewareFolder = "game\\shareware";

// R16 vital files: must never be deleted
inline constexpr const char* kVitalDataFiles[] = {
    "MDATA1", "MDATA2", "MDATA3", "MDATA5", "MDATA11", "MG11.DLL"};

// R16 non-vital files: regenerated on next start if deleted
inline constexpr const char* kNonVitalDataFiles[] = {
    "MDATA4", "MDATA6", "MDATA7", "MDATA8", "MDATA9", "MDATA10",
    "MDATA12", "MDATA13", "MDATA14", "MDATA15"};

// ---- Enums ------------------------------------------------------------------

// R1 outcome of finishing a character load
enum class LoadStatus : i32 {
    Ok = 0,
    NameCheckFailed,
    DecodeFailed
};

// R8 outcome of validating new character input (screen flow excluded)
enum class NewCharacterCheck : i32 {
    Ok = 0,
    EmptyName,
    NameHasBar,
    NoSex,
    NoAlignment,
    SaveExists
};

// R11 outcome of planning an installer unpack
enum class UnpackOutcome : i32 {
    Unpacked = 0,
    NotAnInstaller
};

// R11 edition an installer unpacks to
enum class UnpackEdition : i32 {
    Full = 0,
    Shareware
};

// R16 classification of a data file
enum class DataFileClass : i32 {
    Vital = 0,
    NonVital,
    // UNKNOWN: names not listed in either group; SRD gives no rule for them.
    Unlisted
};

// ---- Small result structs ---------------------------------------------------

struct CharacterLoadResult {
    LoadStatus status = LoadStatus::Ok;
    // R1 step 4: set when paralysed save has no ADShift key
    bool adShiftUnsettled = false;
    // R1 step 3: ModsWhenLoaded = c.Mods
    Vec<Str> modsWhenLoaded{};
};

// R4 one key of an INI-style store
struct IniEntry {
    Str section{};
    Str key{};
    Str value{};
};

// R4 the two option sources: settings.ini (writable) and MORDOR.INI (read only)
struct OptionSources {
    Vec<IniEntry> settings{};
    Vec<IniEntry> legacyMordorIni{};
};

// R6 one guild log record
struct GuildLogRecord {
    i32 guildIndex = 0;
    i64 daySerial = 0;
    Str text{};
};


// R9/R10 in-memory mod settings copy; shell flushes to disk when dirty and writable
struct ModSettingsStore {
    Vec<ModSettingEntry> entries{};
    bool writable = false;
    bool dirty = false;
};

// R15 a running mod
struct LiveMod {
    Str id{};
    Str version{};
  ModManifest manifest = ModManifest{};  // ADDED
  i32 kind = 0;  // FIXED: ModKind is not declared in any header; plain i32 used. // ADDED
  bool loaded = false;  // ADDED
  bool active = false;  // ADDED
  bool faulted = false;  // ADDED
  Str faultReason = "";  // ADDED
};

// R15 mod save tag; isNull when no mods are running
struct ModTag {
    bool isNull = true;
    Vec<Str> entries{};
};

// R11 installer unpack plan
struct UnpackPlan {
    UnpackOutcome outcome = UnpackOutcome::NotAnInstaller;
    UnpackEdition edition = UnpackEdition::Full;
    Str folder{};
};

// R14 gold saved as integer plus optional exact value
struct ExactGold {
    // integer part, stored in GoldOnHand / GoldInBank / Value
    i64 whole = 0;
    // true only when the balance has a fraction
    bool hasExact = false;
    // UNKNOWN: on-disk type of the *Exact field; held here in Currency units (1/10000)
    Currency exact = 0;
};

// ---- Rules ------------------------------------------------------------------

// R1 step 1: deserialize one character record; false on read failure (no silent 0)
bool deserializeCharacter(Ctx& ctx, Character& c, const Vec<u8>& saveBytes);

// R1 step 2: race and every guild name must exist in the MDATA1 tables
bool checkCharacterNames(const Ctx& ctx, const Character& c);

// R1 steps 2-6: name check, ModsWhenLoaded, ADShift settling, legacy pack, swing recalc
CharacterLoadResult finishCharacterLoad(Ctx& ctx, Character& c, bool hasAdShiftKey, i32 savedPackSlots);

// R1 step 5: migrate a pack saved with fewer slots than the fixed pack
// UNKNOWN: migration details (which slots move and where) are not in the SRD excerpt.
void migrateLegacyPack(Ctx& ctx, Character& c, i32 savedPackSlots);

// R2 steps 1-2: keep records (in given record order) that deserialize with a non-blank name
Vec<Character> loadAllCharacters(Ctx& ctx, const Vec<Vec<u8>>& recordFiles);

// R3 step 2: blank a record in place
void rosterDeleteRecord(Ctx& ctx, Vec<Character>& records, i32 index);

// R3 step 3: place into the first blank record, else append; returns record index
i32 rosterAddCharacter(Ctx& ctx, Vec<Character>& records, const Character& c);

// R3 steps 4-5: order from roster.json names; unknown names keep found order; returns indices into foundNames
Vec<i32> rosterOrder(const Ctx& ctx, const Vec<Str>& rosterNames, const Vec<Str>& foundNames);

// R4 steps 1-3: read text; settings.ini wins, then MORDOR.INI, then default
Str readOptionText(const Ctx& ctx, const OptionSources& src, const Str& section, const Str& key, const Str& defaultText);

// R4 step 3: True only when text equals "True" (case-insensitive)
bool readOptionFlag(const Ctx& ctx, const OptionSources& src, const Str& section, const Str& key);

// R4 step 4: write to settings only; other keys untouched; MORDOR.INI never written
void writeOptionText(Ctx& ctx, Vec<IniEntry>& settings, const Str& section, const Str& key, const Str& value);

// R4 step 4: flag written as "True" or "False"
void writeOptionFlag(Ctx& ctx, Vec<IniEntry>& settings, const Str& section, const Str& key, bool value);

// R4 quirk: port flags Welcomed and LimitationNoticeSeen are set if text contains a capital T
bool portFlagTruthy(const Ctx& ctx, const Str& text);

// R5 steps 1-2: preload character name goes to the port's settings only, never MORDOR.INI
// UNKNOWN: the section that original sub16CC uses for the Character key is not given.
void setPreloadCharacter(Ctx& ctx, Vec<IniEntry>& settings, const Str& characterName);

// R6 step 1: append a record to the in-memory guild log (count is written on encode)
void appendGuildLog(Ctx& ctx, Vec<GuildLogRecord>& log, const GuildLogRecord& rec);

// R6 step 1: encode the whole file, header count first
Vec<u8> encodeGuildLog(const Ctx& ctx, const Vec<GuildLogRecord>& log);

// R6 format: decode; header count ignored; stops at end of file, drops truncated tail
Vec<GuildLogRecord> decodeGuildLog(const Ctx& ctx, const Vec<u8>& file);

// R6 step 2: copy old file to backupOut (MDATA9B), then return cleaned file
// UNKNOWN: SRD does not say what "clean" removes; simplest sensible: drop truncated tail and rewrite.
Vec<u8> cleanGuildLog(Ctx& ctx, const Vec<u8>& oldFile, Vec<u8>& backupOut);

// R6 DaySerial: days since 1899-12-30 from epoch milliseconds (pure arithmetic)
i64 daySerialFromEpochMillis(i64 epochMillis);

// R7 steps 1-3: append one CHARSTAT page; header and item lists only when toFile; no footer
// UNKNOWN: port's footer behaviour is not stated; the original footer is dead code and is not reproduced.
void charStatAppendPage(Ctx& ctx, Vec<Str>& fileLines, const Character& c, bool toFile);

// R8 step 1: trim the entered name
Str trimCharacterName(const Ctx& ctx, const Str& raw);

// R8 step 6: sanitised file name used for the duplicate-save check
// UNKNOWN: sanitising rules are not given in the SRD excerpt.


// R8 steps 2-6: validate trimmed name, sex, alignment and existing save (screen flow out of scope)
NewCharacterCheck checkNewCharacter(const Ctx& ctx, const Str& trimmedName, bool sexChosen, bool alignmentChosen, bool saveFileExists);

// R8 step 9: stored form of a password; empty becomes the NoPassword sentinel
// UNKNOWN: the sentinel text is not given in the SRD excerpt.
Str storedPassword(const Ctx& ctx, const Str& enteredPassword);

// R9 steps 1-3: update in-memory value, mark dirty for shell flush, raise Changed on every call
void modSettingWrite(Ctx& ctx, ModSettingsStore& store, const Str& modId, const Str& key, const Str& value);

// R9 Quick reference: WriteFlag(mod, key, b) = Write(mod, key, b ? "True" : "False")
void modSettingWriteFlag(Ctx& ctx, ModSettingsStore& store, const Str& modId, const Str& key, bool value);

// R9 quirk: reads come from the in-memory copy only
Str modSettingRead(const Ctx& ctx, const ModSettingsStore& store, const Str& modId, const Str& key, const Str& defaultValue);

// R10 steps 1-3: no-op when absent; otherwise remove, mark dirty if writable, raise Changed
void modSettingRemove(Ctx& ctx, ModSettingsStore& store, const Str& modId, const Str& key);

// R9 Quick reference: section = "Mod:" + modId
Str modSection(const Ctx& ctx, const Str& modId);

// R9 Table: escape one cell (\ to \\, | to \|, ; to \;; CR and LF become spaces)
Str modEscapeCell(const Ctx& ctx, const Str& cell);

// R9 Quick reference: EncodeRows(rows) = join(";", each row: join("|", escaped cells))
// Quirk: one row with one empty cell encodes to ""; a row with no cells is lost.
Str modEncodeRows(const Ctx& ctx, const Vec<Vec<Str>>& rows);

// R9 decode: empty text decodes to zero rows; a trailing lone backslash is kept as literal
// UNKNOWN: exact decode rules beyond the escape table are not given in the SRD excerpt.
Vec<Vec<Str>> modDecodeRows(const Ctx& ctx, const Str& text);

// R15 steps 1-2: tag = id@version for each live mod; null when none
ModTag modSaveTag(const Ctx& ctx, const Vec<LiveMod>& live);

// R15 step 3: tag entries whose id is not among live mod ids
Vec<Str> modTagMissingFrom(const Ctx& ctx, const ModTag& tag, const Vec<LiveMod>& live);

// R11 steps 1-4: plan an unpack; installer is read, never run; non-installer refused
// UNKNOWN: how the installer's edition is detected is not given in the SRD excerpt.
UnpackPlan planInstallerUnpack(const Ctx& ctx, const Vec<u8>& installerFile);

// R12 step 1: encode store shelf to store.json text
Str encodeStoreShelf(const Ctx& ctx, const Shelf& shelf);

// R12 steps 2-3: empty text gives a fresh store; missing file builds initial stock when item table exists
Shelf loadStoreShelf(Ctx& ctx, const Str& storeJson, bool fileMissing);

// R12 Quick reference: Key(id, a) = "{id}:{(int)a}"; Unaligned is 0
Str storeKey(const Ctx& ctx, i32 itemId, ItemAlign alignment);

// R13 step 2: legacy Identified flag, read only from saves without Known
// UNKNOWN: mapping of legacy flag to IdLevel is not stated; simplest: true -> Barely, false -> Unknown.
void applyLegacyIdentified(Ctx& ctx, Item& item, bool hasKnownField, bool identifiedFlag);

// R14 steps 1-2: split a balance into whole part and optional exact value (pure arithmetic)
ExactGold goldToSaveFields(Currency balance);

// R14 steps 3-4: rebuild balance; exact value ignored when its whole part does not match
// Hall records use the same routine for Value and ValueExact.
Currency goldFromSaveFields(const ExactGold& fields);

// Guild and experience (port): stored Att/Def are not used; a differing stored value is warned about
void warnStoredGuildStatsDiffer(Ctx& ctx, i32 guildIndex, i32 storedAtt, i32 storedDef, const GuildAttDefResult& recomputed);

// R16 steps 1-2: classify a data file by the group tables
DataFileClass dataFileClass(const Ctx& ctx, const Str& fileName);

// R16 step 2: vital files must never be deleted
bool mayDeleteDataFile(const Ctx& ctx, const Str& fileName);

// Record 0 of each table: MDATA2 and MDATA3 keep it; MDATA5 skips it (duplicate GOBLIE)
// UNKNOWN: whether table numbering uses base record 4 for MDATA5 is not resolved.
bool keepsTableRecordZero(const Ctx& ctx, i32 mdataNumber);

// Quick reference: offset = recordLength * (recnum - 1), recnum 1-based (pure arithmetic)
i64 recordOffset(i64 recordLength, i64 recnum);

// Quick reference: MDATA4 record offset = (n - 1) * kCharRecordLen (pure arithmetic)
i64 charRecordOffset(i32 n);

// Quick reference: automap index = (L-1)*900 + x + 30*(y-1) + 2 (pure arithmetic)
i32 automapIndex(i32 level, i32 x, i32 y);

// Quick reference: been_here = word & (1 << 23) (pure arithmetic)
bool automapBeenHere(u32 word);

}  // namespace mordor
