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

// ---- R49-R54 Library writes ----
// R49 Item identified: grade from known level, file name, grade and item id
void libraryFileItem(Ctx& ctx, LibraryRecord& rec, Str finderName, IdLevel known, i32 itemId);

// R50 Item found: file item (R49), then source monster, today, location, and found count
void libraryFileFind(Ctx& ctx, LibraryRecord& rec, Str finderName, IdLevel known, i32 itemId,
                     i32 fromMonsterId, i64 today, bool lostLevel, bool lostPosition, Str levelText);

// R51 Shelf restock sale: file as fully known (grade 4), MonId -2, location "?", found +1
// CONFLICT: Set 2 [c57] says found = 1 (set, not added); this follows the R51 text (+1).
void libraryFileRestock(Ctx& ctx, LibraryRecord& rec, i64 today);

// R52 Companion identified: grade from ID level; hand-built (Ref -1) not filed (returns false)
bool libraryFileCompanion(Ctx& ctx, LibraryRecord& rec, const Companion& c, i32 defId,
                          bool hasPlace, Str place);

// R53 Dungeon meeting: place (kept if lost), date, NSeen/NDeaths, grade; false = no record written
// CONFLICT: [c12] sets Location to "?" for a lost sighting; [c57] keeps the old place. This follows [c57].
bool libraryDungeonMeeting(Ctx& ctx, LibraryRecord& rec, const Party& party, const MonsterDef& def,
                           i32 groupCount, bool sawGroup, bool killedBy, bool misidentified,
                           bool groupInvisible, i64 today);

// R53 PlaceOf: "x+1,y+1,level" from the party square; false = null place (lost)
bool libraryPlaceOf(const Party& party, Str& outPlace);

// R54 Creature sighting (FileSighting): reference match, invisible gate, grade, counts, place
// UNKNOWN: leadDungeonLevel is taken as the lead character's dungeon level (0 = City).
// UNKNOWN: groupInvisible stands for invisible(def), whose definition is not in this part.
// CONFLICT: Set 2 [c57] differs on invisibility scope, lost case, City case, and whether Known is set.
bool librarySighting(Ctx& ctx, LibraryRecord& rec, i32 refIndex, i32 knownAsIndex,
                     const MonsterDef& def, bool groupInvisible, const Character& reader,
                     const ItemDef* item, bool killed, bool sighting, i32 groupCount,
                     i32 leadDungeonLevel, i64 today);

// R55 Monster grade (Set 2): IdLevelRoll; reader skill 0 vs 15+ levels harder gives grade 1
// UNKNOWN: "harder" compares monsterLevel against the reader's level; exact basis not in this part.
i32 libraryMonsterGrade(Ctx& ctx, const Character& reader, const MonsterDef& def, i32 monsterLevel);

// R56 Item grade: All=4, Mostly=3, else 0 (stored)
i32 libraryItemGrade(IdLevel known);

// R56 Item listed only if grade reaches Mostly or better
bool libraryItemListed(i32 grade);

// T-D Identify roll or ID level to grade: <=0 -> 4, 1 -> 3, 2 -> 2, else 1
i32 libraryGradeFromIdLevel(i32 rollOrIdLevel);

// R57 Library record grade merge on load: name, max grade, capped counts, newer place and date
// CONFLICT: the name rule (take name when there.Known==0) conflicts with Set 2 [c57].
void libraryMergeRecord(Ctx& ctx, LibraryRecord& there, const LibraryRecord& newer);

// R58 Legacy "m:N" key parse; N is the legacy monster Id. False if the key is not legacy.
bool libraryParseLegacyKey(Str key, i32& legacyMonsterId);

// R58 Settle old Id-keyed monster records to table index (SettleMonsterKeys, Merge)
// Port deviation: an Id that no creature has under its old key is left unsettled.
void librarySettleMonsterKeys(Ctx& ctx, LibraryState& lib);

// R58 Port key for a monster record: "mr:index"
Str libraryMonsterKey(i32 monsterIndex);

// R58 Port key for an item record: "i:itemId"
Str libraryItemKey(i32 itemId);

// R58 Import drops found counts (port addition)
// UNKNOWN: "found" is taken to be nSeen; the source does not name the field.
void libraryImportDropFound(Ctx& ctx, LibraryRecord& rec);

// R62 Serial(when): whole days since 1899-12-30, from a UTC millisecond timestamp
i64 libraryDaySerial(i64 utcMillis);

// R62 Days between a sighting or find date and today: abs(seen - today)
i64 libraryDaysAgo(i64 seen, i64 today);

// R63 Shared Id 307: sighting of index 389 files under the first index (278)
i32 libraryCanonicalMonsterIndex(i32 monsterIndex);

// ---- ID roll used by R53-R55 (CombatEngine.IdLevelRoll) ----
// UNKNOWN: owner is outside this part; declared here because R53-R55 depend on it.
// UNKNOWN: meaning of the bool flag (passed false everywhere in this part; fn1272 False -> grade 4).
i32 idLevelRoll(Ctx& ctx, const Character& reader, const MonsterDef& def, const ItemDef* item, bool flag);

// ---- R64 Seer service ----
// R64.2 Level gate: level above DeepestHeCanReach is refused as too deep
bool seerLevelRefused(i32 level, i32 deepestReach);

// R64.3 Seer fee: fee base x Log(level+1) x level, plus up to a quarter, then halved
// UNKNOWN: operand of "halved" is not in this part. UNKNOWN: unit of the result (gold or 1/10000 gold).
Currency seerFee(Ctx& ctx, i32 level);

// R64.5 Locate Character fee: depends on target depth only
// UNKNOWN: formula body is not in this part.
Currency seerLocateCharacterFee(i32 dungeonLevel);

// R64 CanPay: true if the character can cover the fee (purse plus bank)
bool seerCanAfford(const Character& payer, Currency fee);

// R64.3 Charge a fee through CharPay; the bank covers any shortfall. False if it cannot be paid.
bool seerPayFee(Ctx& ctx, Character& payer, Currency fee);

// R64.4 Monster search fails with probability level x 5 percent (fee already charged)
bool seerSearchFails(Ctx& ctx, i32 level);

// R64.4 Frequency line is added with a 75% chance
bool seerFrequencyLineRolled(Ctx& ctx);

// R64.4 Blur a located spot with radius Seer.Blur(..., 5)
// UNKNOWN: exact mapping of the blur argument to squares (comment says up to four each way).
void seerBlurSpot(Ctx& ctx, i32& x, i32& y, i32 radius);

// R64.4 Monster locate: level gate, fee, search, spot, blur, frequency line
SeerMonsterResult seerLocateMonster(Ctx& ctx, Character& payer, i32 levelFound, i32 monsterChance,
                                    i32 deepestReach);

// R64.5 Locate Character roll vs 3 x DungeonLevel
// UNKNOWN: comparison direction and exact expression are not in this part.
bool seerLocateCharacterRoll(Ctx& ctx, i32 dungeonLevel);

// R64.5 Locate a saved character: City refusal, confirmation, fee, roll
// CONFLICT: the port adds saved-character location; the original Seer only locates creatures and items.
SeerCharacterResult seerLocateCharacter(Ctx& ctx, Character& payer, const Character& target,
                                        bool confirmed);

// R64.6 Wiki locate: player pays gold and gives an exact name; returns Z, X, Y on success
// UNKNOWN: success roll is unsourced.
SeerWikiResult seerWikiLocate(Ctx& ctx, Character& payer, Str exactName, Currency fee);

// ---- R65 Guild Log clean ----
// R65 Clean button visible when any guild has past the threshold (entries per guild)
// UNKNOWN: exact comparison inside NeedsCleaning; the text says "past fifty entries per guild".
bool guildLogNeedsCleaning(const Vec<i32>& entriesPerGuild);

// R65 Clean: keep newest kGuildLogKeepOnClean entries across all guilds; returns removed entries
// The host writes the optional export file.
Vec<GuildLogEntry> guildLogClean(Ctx& ctx, Vec<GuildLogEntry>& log);

// ---- R66-R70 Other player inputs ----
// R66 Morgue list: dead characters in record order (file order supplied by host)
// UNKNOWN: exact membership test beyond isDead().
void templeMorgueFill(Ctx& ctx, Party& party, const Vec<Character>& recordsInFileOrder);

// R67 Gold transfer input: VB CLng rules, round half to even; false = refused silently
// "&HFFFF" reads as -1 (16-bit signed). Values outside the Long range are refused.
bool goldTransferParse(Str text, i64& wholeGold);

// R68 Companion number entry: vbValue is VbVal(text); returns 0-based slot, or -1 for none
// UNKNOWN: a null companion is read as !occupied.
i32 companionSlotFromEntry(double vbValue, const Arr<Companion, kCompanionSlots>& companions);

// R69 Realign fee: uses the R26 fee routine declared in its own module; not redeclared here.

// R70 Equip toggle allowed: MaxWorn 0 refuses; MaxWorn > 0 caps at MaxWorn; -1 uncapped
// Port deviation: the port tests != 0 instead of < 1, so -1 is wearable without cap.
bool equipToggleAllowed(i32 maxWorn, i32 wornInClass);

// ---- R72-R73 Price models (wiki transcript; unverified) ----
// R72 Price fit: final = int(max(1000, round(price)))
i64 priceFromFit(const PriceFit& fit, i32 att, i32 def, i32 hits, i32 resSum, i32 guildLevel);

// R73 ECV score: att + 4 x def + 2 x hits + 2 x StatStr
double ecvScore(i32 att, i32 def, i32 hits, i32 strengthStat);

// R73 Standard price approximation: 730 x ECV
// UNKNOWN: M_Type and B are hidden; rounding mode is not stated.
i64 ecvStandardPrice(i32 att, i32 def, i32 hits, i32 strengthStat);

}  // namespace mordor
