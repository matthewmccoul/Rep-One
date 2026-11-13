#pragma once
// city2a.h: SRD part c16p2.
// Covers Library R48-R58 and R63, Seer R64, Guild Log R65, other inputs R66-R70, and price models R72-R73.
// Declarations only. Bodies live in src/city2a_*.cpp.
// Out of scope (player text screens): R59 listing text and plurals, R60/R61 page text,
// R62 day wording, R66 temple text, R67/R68 prompts (parsing only is declared).
// R71 is not declared (unverified wiki claim that conflicts with R1 step 5; the engine rule governs).
// File IO (library.json, MDATA13/MDATA15, guild log export, folder scans) belongs to the host, not the core.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- constants ----
inline constexpr i32 kLibraryCountCap = 32767;          // R57 NDeaths cap (short.MaxValue)
inline constexpr i32 kMorgulMonsterIndex = 278;         // R63 first index of shared Id 307
inline constexpr i32 kShadowNinjaMonsterIndex = 389;    // R63 second index of shared Id 307
inline constexpr i32 kSharedMonsterId = 307;            // R63 shared monster Id
inline constexpr i32 kLibraryRestockMonId = -2;         // R51 MonId for shelf restock filing
inline constexpr i32 kCityDungeonLevel = 0;             // R54/R64 dungeon level of the City

inline constexpr double kSeerFeeBase = 132567.52;       // R64 Seer fee base (T-L)
inline constexpr i32 kSeerSearchFailPerLevel = 5;       // R64 search fails level x 5 in a hundred
inline constexpr i32 kSeerFrequencyPercent = 75;        // R64 frequency line chance
inline constexpr i32 kSeerBlurArg = 5;                  // R64 Seer.Blur argument
inline constexpr i32 kSeerLocateCharacterFactor = 3;    // R64 locate roll uses 3 x DungeonLevel

inline constexpr i32 kGuildLogCleanThreshold = 50;      // R65 entries per guild before cleaning is offered
// UNKNOWN: KeepOnClean value is not in this part; 50 is assumed to match the threshold.

inline constexpr i32 kMaxWornNone = 0;                  // R70 MaxWorn 0: wear refused
inline constexpr i32 kMaxWornUncapped = -1;             // R70 MaxWorn -1: wearable without cap

inline constexpr i64 kVbLongMin = -2147483648LL;        // R67 VB Long range
inline constexpr i64 kVbLongMax = 2147483647LL;         // R67 VB Long range
inline constexpr double kVbIntegerMin = -32768.0;       // R68 VB Integer range
inline constexpr double kVbIntegerMax = 32767.0;        // R68 VB Integer range

inline constexpr i64 kPriceFloor = 1000;                // R72 price floor
inline constexpr double kEcvStandardMultiplier = 730.0; // R73 standard price multiplier

// ---- R72 price fit coefficients (wiki transcript; unverified) ----
struct PriceFit {
    double att;
    double def;
    double hits;
    double resSum;
    double guildLevel;
    double intercept;
};
// R72 Fit A (guildLevel weight 0)
inline constexpr PriceFit kPriceFitA{3733.328, 840.686, 4593.044, 423.633, 0.0, -75250.700};
// R72 Fit B
inline constexpr PriceFit kPriceFitB{2223.2, 1635.5, 4418.3, 433.1, 308.3, -49301.7};
// R72 Fit C (guildLevel weight 0)
inline constexpr PriceFit kPriceFitC{2432.42, 1622.18, 4530.62, 426.23, 0.0, -51048.67};

// ---- Library state (R49-R58) ----
// Monster records are keyed by monster table index. Item records are keyed by item id.
struct LibraryRecord {
    i32 known = 0;          // grade 0..4 (R49, R52, R53, R54, R56)
    Str name{};             // first recorder's name
    i32 id = 0;             // item id, or monster Id (R49, R52)
    i32 monId = 0;          // source monster Id; -2 for shelf restock (R50, R51)
    i64 lastSeen = 0;       // day serial; 0 = not known
    Str location{};         // "x+1,y+1,level", "?", "the City", or ""
    i32 nSeen = 0;          // found / seen count (R50, R51, R53, R54)
    i32 nDeaths = 0;        // deaths recorded (R53, R54, R57)
};

struct LibraryState {
    Vec<LibraryRecord> monsters{};  // by monster table index
    Vec<LibraryRecord> items{};     // by item id
};

// ---- Seer and Guild Log result types ----
enum class SeerMonsterOutcome : i32 { TooDeep = 0, NoPlace, LevelOnly, Located };
enum class SeerCharacterOutcome : i32 { CityRefused = 0, NotConfirmed, NotEnoughGold, Failed, Located };

struct SeerMonsterResult {
    SeerMonsterOutcome outcome;
    Currency feeCharged;
    bool frequencyLine;
    i32 dungeonLevel;
    i32 x;
    i32 y;
};

struct SeerCharacterResult {
    SeerCharacterOutcome outcome;
    Currency feeCharged;
    i32 dungeonLevel;
    i32 x;
    i32 y;
};

struct SeerWikiResult {
    bool found;
    Currency feeCharged;
    i32 x;
    i32 y;
    i32 z;
};

struct GuildLogEntry {
    i32 guild;
    i64 stamp;              // newest = largest stamp
    Str text;
};

// ---- Screen reveal tables (R60, R61): which fields a grade unlocks ----
struct CreatureReveal {
    bool sizeShown;             // grade >= 1
    bool classShown;            // grade >= 2
    bool alignmentAndAd;        // grade >= 3
    bool specialAbilitiesUnknown; // grade 3
    bool specialProperties;     // grade >= 4
    bool attacks;               // grade >= 4
    bool spellSchools;          // grade >= 4
    bool resistances;           // grade >= 4
};

struct ItemReveal {
    bool damageModifier;        // weapons only (wear class 0), grade 4
    bool curse;                 // grade 4
    bool castSpellAndCharges;   // grade 4
    bool specialEffects;        // grade 4
    bool useEffect;             // grade 4
    bool classRestrictionFlag;  // grade 4
    bool guildUseLevels;        // grade 4
};

// ---- R48 Library writes ----
// R48 Writes occur only in R49-R54 events. Screen reads never write (no function).







































// R69 Realign fee: uses the R26 fee routine declared in its own module; not redeclared here.





}  // namespace mordor
