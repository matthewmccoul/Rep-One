#pragma once
// Module death1b: raise resolution, teleport/fall deaths, chest-opener death,
// old-age death, character creation/deletion hooks, kills and quests,
// Hall of Records, Retrieve Soul, morgue input. SRD part c18p1 (R12-R24).

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"
#include "mordor/state_dungeon.h"
#include "mordor/death1a.h"

namespace mordor {

// ---- Constants (R12-R24; Table 6) ----
inline constexpr i32 kAgeOrdinaryDays = 54;          // R24 mc00B4: ordinary age added, days
inline constexpr i32 kAgeStonedDays = 84;            // R24 mc00C4: age after petrification, days
inline constexpr i32 kAgeRockDays = 124;             // R24 mc00B8: age after rock, days
inline constexpr i32 kComplicationAgeDays = 5475;    // R24 mc02EA: complication age, days (15 years)
inline constexpr i32 kOrdinaryDeathValue = 1;        // R24 mc022C; CONFLICT 30: R17 gives DeathOrdinary cause 0, R18/R24 give 1
inline constexpr i32 kFirstMemberIndex = 0;          // R24 m008C: first party member index
inline constexpr i32 kDaysPerYear = 365;             // R19: days per year
inline constexpr i32 kComplicationStatLoss = 5;      // R12.8: each stat loss on complication
inline constexpr double kComplicationDivisor = 1.2;  // R12.8: MaxHits divisor (archive 5/6)
inline constexpr i32 kComplicationMinMaxHits = 1;    // R12.8: MaxHits floor after complication
inline constexpr i32 kSpellTierMultiplier = 10;      // R13: tier multiplier
inline constexpr double kSpellChanceConFactor = 35.0;    // R13: CON term constant
inline constexpr double kSpellChanceConDivisor = 4.5;    // R13: CON term divisor

// R17 teleport into rock outcome
enum class TeleportRockOutcome : i32 {
    LandedInRock = 0,    // level in 1..Count and x,y within floor; rock recorded, party moved
    BodiesOffMap = 1,    // level above 0 but off the map; bodies stored off-map
    LevelZeroDeath = 2   // R17.4 separate ordinary death at level 0
};

struct TeleportRockResult {
    TeleportRockOutcome outcome = TeleportRockOutcome::LandedInRock;
    i32 area = 0;        // R17.3: -1 at level 0, 0 otherwise
};

// R17 off-map body storage at (level, x, y)
// CONFLICT 32: R17.3 says everyone dies with DeathRock and R17.4 says level 0 is a separate
// ordinary death; the header follows R17.4 for level 0 (ordinary death, area -1).
struct OffMapStore {
    i32 level = 0;
    i32 x = 0;
    i32 y = 0;
    Vec<Character> bodies{};
};

// R18 chest opener outcome
enum class OpenerTrapOutcome : i32 {
    LeaderStanding = 0,  // Members[0] has 1 or more hits; trap rule does not apply
    OpenerKilled = 1,    // opener in slot 0 and not dead; killed ordinary, no cleanup
    NobodyKilled = 2     // leader down but no one set dead (deliberate deviation, quirk)
};

// R23 Retrieve Soul check, typed errors
enum class SoulCheck : i32 {
    Ok = 0,
    NotOnThisLevel = 1,   // "Character is not on this level!"
    NotInRock = 2         // "Character is not in ROCK!"
};

// R22.4 guild log kinds
// UNKNOWN: Table 10 text and kinds are not in the chunk; Join is added for R20.1.
enum class GuildLogKind : i32 {
    Death = 0,
    Resurrection = 1,
    Demotion = 2,
    Join = 3
};


// R22.4 guild log line (MDATA9 entry)
struct GuildLogLine {
    Guild guild = Guild::Nomad;
    i64 day = 0;
    Str text{};
};

// R22 Hall of Records state (MDATA9 log, MDATA12 names, Wealthiest value)
struct HallOfRecords {
    Vec<i32> records{};   // FIXED: HallRecord is declared nowhere; Vec<i32> used instead (kHallCategories is declared nowhere too, so no fixed-size Arr)
    Vec<GuildLogLine> log{};
    Arr<Str, kPartyMaxSize> partyNames{};   // four names, leader first
    Currency wealthiest = 0;                 // M73F5
    bool dirty = false;                      // R22.3 file to save after any offer
};

// ---- R12 Raise resolution ----












// ---- R13 Spell raise chance ----



// ---- R14 Morgue raise chance ----


// ---- R15 Morgue fee ----




// ---- R16 Companion raise (dungeon, cast on Who?) ----








// ---- R17 Teleported into rock, or fall from the sky ----




// ---- R18 Trap death of the chest opener ----


// ---- R19 Old-age death ----



// ---- R20 Character creation and deletion ----




// ---- R21 Kills, chest finds and quests ----





// ---- R22 Hall of Records checks ----







// ---- R23 Retrieve Soul targets ----


// ---- Morgue input (no rule number in the SRD) ----



}  // namespace mordor
