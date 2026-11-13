#pragma once

#include "mordor/types.h"
#include "mordor/state_game.h"
#include "mordor/death1b.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_party.h"

namespace mordor {

struct FloorState;  // full definition in state_dungeon.h

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// R6, R8: teleporter destination search (trap 7)
inline constexpr i32 kTeleportMaxTries = 10000;
inline constexpr i32 kTeleportAxisMin = 1;
inline constexpr i32 kTeleportAxisMax = 30;
inline constexpr i32 kWallBitRock = 19;
inline constexpr i32 kWallBitTeleporter = 14;
inline constexpr u32 kWallMaskRock = u32{1} << 19;
inline constexpr u32 kWallMaskTeleporter = u32{1} << 14;

// R9, R10: chute pit-style roll, fall when 5 + DungeonLevel/4 < Rnd*100
inline constexpr i32 kPitRollBase = 5;
inline constexpr i32 kPitRollDepthDivisor = 4;
inline constexpr i32 kPitRollScale = 100;

// R30: Messages window cap (constant only; the window is a text screen, out of scope)
inline constexpr i32 kMessagesKept = 20;


// R32-R36: quest gating and caps
inline constexpr i32 kQuestMonsterMinGuildLevel = 5;
inline constexpr i32 kQuestItemMinGuildLevel = 15;
inline constexpr i32 kQuestItemFrequencyPercent = 25;
inline constexpr i32 kQuestNoQuestPercent = 6;
inline constexpr i32 kQuestMinAssignPercent = 1;
inline constexpr i32 kQuestRareRerollPercent = 74;
inline constexpr i32 kQuestRareMonsterMaxWeight = 9;
inline constexpr i32 kQuestRareItemMaxWeight = 4;
inline constexpr i32 kQuestGuildLevelDivisor = 25;
inline constexpr i32 kQuestMonsterLevelCap = 10;
inline constexpr i32 kQuestItemLevelCap = 7;
inline constexpr i64 kQuestMinItemValue = 100 * kCurrencyScale;  // 100 gold

// R16-R20: Hall of Records and monster power
inline constexpr i32 kRecordKindCount = 12;
inline constexpr i64 kOriginalRecordDate = 0;  // date 0 displays as "Original RecordHolder"
inline constexpr i32 kPowerSpecialBits = 14;   // special-attack bits 0..13 counted
inline constexpr i32 kPowerSpellBits = 18;     // spell bits 0..17 counted
inline constexpr i64 kPowerSpecialWeight = 75;
inline constexpr i64 kPowerSpellWeight = 100;

// R21-R23: skill indexes
inline constexpr double kFightingWeight = 0.2;
inline constexpr double kFightingDivisor = 160.0;
inline constexpr double kThievingDivisor = 3500.0;
inline constexpr double kSkillPercent = 100.0;
inline constexpr i32 kMagicCapPerSpell = 255;
inline constexpr i32 kMagicLevelScale = 10;

// ---------------------------------------------------------------------------
// Enums and small result structs
// ---------------------------------------------------------------------------

// R16-R19: Hall of Records kinds, in table order
enum class RecordKind : i32 {
    Strength = 0,
    Intelligence,
    Wisdom,
    Constitution,
    Charisma,
    Dexterity,
    Deadliest,
    MostExperience,
    Wealthiest,
    Fighting,
    Magic,
    Thieving
};


// FIXED: HallOfRecords is already defined in an included header (redefinition error).
// The local definition was removed so the existing one is used.

// R16-R19: Hall starting records (new game; missing file loads these)
struct HallSeed {
    RecordKind kind;
    i64 value;
    const char* holder;
};

inline constexpr HallSeed kHallSeeds[kRecordKindCount] = {
    {RecordKind::Strength, 14, "Crashland"},
    {RecordKind::Intelligence, 15, "Mager"},
    {RecordKind::Wisdom, 18, "Theshal"},
    {RecordKind::Constitution, 14, "Orgal"},
    {RecordKind::Charisma, 15, "Alaya"},
    {RecordKind::Dexterity, 17, "Nimblefingers"},
    {RecordKind::Deadliest, 1, "Crashland"},          // value 1 is a monster id
    {RecordKind::MostExperience, 1322451, "Crashland"},
    {RecordKind::Wealthiest, 5466453, "Nimblefingers"},
    {RecordKind::Fighting, 17, "Morgul"},
    {RecordKind::Magic, 6, "Ge'nal"},
    {RecordKind::Thieving, 15, "Nimblefingers"}
};

// R6, R8: result of the teleporter destination search
struct TeleporterDestination {
    bool found = false;
    i32 x = 0;
    i32 y = 0;
};

// R5, R7, R8: result of springing trap 7
struct TeleportTrapResult {
    bool teleported = false;
    TeleporterDestination destination{};
};

// R9-R15: result of a chute fall or take
struct ChuteFallResult {
    Arr<bool, kPartyMaxSize> fell{};
    Arr<i32, kPartyMaxSize> damage{};
    i32 fallCount = 0;
};

// R28: FromDouble result; overflow when NaN or outside +/-2^63
struct CurrencyFromDoubleResult {
    Currency value = 0;
    bool overflow = false;
};

// R29: X87 extended value, value = mantissa * 2^exponent (64-bit mantissa)
struct X87Value {
    u64 mantissa = 0;
    i64 exponent = 0;
    bool negative = false;
};

// ---------------------------------------------------------------------------
// R1-R4: Asmodeus defeat
// ---------------------------------------------------------------------------



// R2: the five form-selection sites inside the ending text are UNKNOWN; no routine declared.



// R4: the vanished body is story only; no routine declared.

// ---------------------------------------------------------------------------
// R5-R8: Teleporter trap (trap 7)
// ---------------------------------------------------------------------------




// ---------------------------------------------------------------------------
// R9-R15: Chute falls
// ---------------------------------------------------------------------------












// ---------------------------------------------------------------------------
// R16-R19: Hall of Records
// ---------------------------------------------------------------------------






// ---------------------------------------------------------------------------
// R20: Monster power
// ---------------------------------------------------------------------------




// ---------------------------------------------------------------------------
// R21-R24: Skill indexes
// ---------------------------------------------------------------------------




// R24: timing of skill evaluation is UNKNOWN; no routine declared.

// ---------------------------------------------------------------------------
// R25-R26: Pluralisation
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// R27-R29: Currency and extended precision
// ---------------------------------------------------------------------------





// R28 RoundToWhole: use currencyWhole from math_util.h (foundation; not redeclared here).

// ---------------------------------------------------------------------------
// R31: Library sort order
// ---------------------------------------------------------------------------


// ---------------------------------------------------------------------------
// R32-R36: Quest gating
// ---------------------------------------------------------------------------










// Quest bug (no routine): T'Mana (11), White Dragons (11), Wizards (11) and Grey Molds (13) can be
// assigned above mLevel 10. T'Mana cannot spawn, so the quest must be forfeited (see Forfeit in guilds2.h).

// ---------------------------------------------------------------------------
// R37-R41: Stored data cross-checks and quirks
// ---------------------------------------------------------------------------
// R37-R41 describe stored data (monster defenses, swing budgets, Nerve Blank, Firebolt #76, MDATA basis).
// They are data checks, not routines; no declarations here.
// CONFLICT (R40): Firebolt #76 base level is 1 in stored data, 2 in the printed manual. Stored data governs.

}  // namespace mordor
