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

// ---- [c35] Character derived values
// MaxSpellPoints = (INT + WIS) * 5 is maxSpellPoints() in casting2.h (API INDEX); not redeclared here.
// Alignment 0=Good,1=Neutral,2=Evil and Sex 0=Male,1=Female map directly to Align and Sex (state_character.h).








// ---- [c22] Saved field types and sizes (little-endian)
inline constexpr i32 kIntegerBytes = 2;
inline constexpr i32 kLongBytes = 4;
inline constexpr i32 kSingleBytes = 4;
inline constexpr i32 kCurrencyBytes = 8;


inline constexpr i64 kAutomapRowWidth = 30;
inline constexpr i64 kAutomapHeaderSkip = 2;


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

// "Hands" (MDATA3 record 0, unarmed attack) is pinned in pack slot 0 and never moves.
inline constexpr i32 kHandsPackSlot = 0;

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

// Item record words: 7 stats-required, 7 stat-mod; dwords for ability, alignment, resistance, charge.
struct ItemRecordRaw {
    Arr<i16, 7> statsRequired{};
    Arr<i16, 7> statMods{};
    i32 ability = 0;
    i32 alignment = 0;
    i32 resistance = 0;
    i32 charge = 0;
};

// ---- [c96] Data file groups
// Vital, never delete: mdata1, mdata2, mdata3, mdata5, mdata11 (mg11.dll images are also vital).
inline constexpr Arr<i32, 5> kVitalMdataFiles{1, 2, 3, 5, 11};
// Regenerated on next start if deleted (non-vital).
inline constexpr Arr<i32, 10> kRegeneratedMdataFiles{4, 6, 7, 8, 9, 10, 12, 13, 14, 15};

// ---- [c24][c57] Shipped data fixture
inline constexpr i32 kShippedRaces = 9;
inline constexpr i32 kShippedGuilds = 12;
inline constexpr i32 kShippedSpells = 105;
inline constexpr i32 kShippedItemRecords = 366;
inline constexpr i32 kShippedMonsterRows = 401;
inline constexpr i32 kShippedPlayableFloors = 3;
inline constexpr i64 kDemoMdata11Bytes = 68166;


// ---- [c40] Mod save tag
struct ModRef {
    Str id{};
    Str version{};
};


// The port never writes MORDOR.INI.
inline constexpr bool kMayWriteLegacyIni = false;

}  // namespace mordor
