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



















// R6 DaySerial: days since 1899-12-30 from epoch milliseconds (pure arithmetic)




// R8 step 6: sanitised file name used for the duplicate-save check
// UNKNOWN: sanitising rules are not given in the SRD excerpt.





























}  // namespace mordor
