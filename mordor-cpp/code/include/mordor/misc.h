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

// R1: pure query, whether the player is in a party at the moment of defeat
bool inPartyAtDefeat(const Party& party);

// R1: shows Asmodeus.Ending in party or lone wording
void sayAsmodeusEnding(Ctx& ctx, bool inAParty);

// R2: the five form-selection sites inside the ending text are UNKNOWN; no routine declared.

// R3: LogLine = inAParty ? PartyBeatAsmodeus(name) : AloneBeatAsmodeus(name)
// Original quirk: lone wording "single handedly defeated Asmodeus"; party wording "was part of the party that defeated" him.
Str asmodeusLogLine(const Str& name, bool inAParty);

// R3: writes one guild-log line per surviving party member (not dead)
void writeAsmodeusGuildLogs(Ctx& ctx, const Party& party, bool inAParty);

// R4: the vanished body is story only; no routine declared.

// ---------------------------------------------------------------------------
// R5-R8: Teleporter trap (trap 7)
// ---------------------------------------------------------------------------

// R5, R7, R8: opener springs trap 7; only the opener moves, and only if a destination is found
TeleportTrapResult springTeleporterTrap(Ctx& ctx, Party& party, const FloorState& floor);

// R6, R8: random search up to kTeleportMaxTries; found = false after exhaustion (no teleport)
// UNKNOWN: square indexing for walls(x,y); assumed to be SquareState::bits on the floor
TeleporterDestination findTeleporterDestination(Ctx& ctx, const FloorState& floor);

// R6: pure query, true if a wall word has neither Rock (bit 19) nor Teleporter (bit 14) set
bool isValidTeleportSquare(u32 wallWord);

// ---------------------------------------------------------------------------
// R9-R15: Chute falls
// ---------------------------------------------------------------------------

// R9: pit-style roll, true when 5 + DungeonLevel/4 < Rnd*100
// UNKNOWN: whether this matches the engine code body; formula taken from the source doc
bool pitStyleRoll(Ctx& ctx, i32 dungeonLevel);

// R10: Rules.HoversOverPit helper
// UNKNOWN: body not in source chunks; assumed to be pitStyleRoll(ctx, dungeonLevel)
bool hoversOverPit(Ctx& ctx, i32 dungeonLevel);

// R10: fall iff not (Levitating and HoversOverPit)
bool chuteMemberFalls(Ctx& ctx, const Character& ch, i32 dungeonLevel);

// R13: Rules.ChuteDamage helper (raceSize is RaceData.Size)
// UNKNOWN: body not in source chunks
i32 chuteDamage(Ctx& ctx, i32 landedLevel, i32 raceSize);

// R13: landing damage, 0 if Levitating, else chuteDamage
i32 chuteLandingDamage(Ctx& ctx, const Character& ch, i32 landedLevel, i32 raceSize);

// R12: pure query, area id of square (x,y) on the landed floor
// CONFLICT: original kept the area of the floor above; port uses the lower floor's area (per source)
i32 landedAreaId(const GameData& data, i32 landedLevel, i32 x, i32 y);

// R12: moves a fallen member to landedLevel, keeping x and y; sets area from the landed floor
void moveFallenMember(Ctx& ctx, Character& ch, i32 landedLevel);

// R14: emits FALL then HIT for a member who fell
void announceChuteFall(Ctx& ctx, const Character& ch, i32 damage);

// R9-R14: walks party onto a chute, leader first. followAnswer is the caller's answer to the
// R11 prompt (asked only when the leader falls and the party has more than one member);
// "yes" sends all later members down regardless of their roll. The core never prompts.
ChuteFallResult walkOntoChute(Ctx& ctx, Party& party, const FloorChute& chute,
                              i32 fromLevel, bool followAnswer);

// R15: TAKE on a chute square lands all members with no roll (landing damage still applies per R13)
ChuteFallResult takeChuteLanding(Ctx& ctx, Party& party, const FloorChute& chute, i32 fromLevel);

// Quirk (no rule number): members not on the leader's square/floor after sub0908 disappear from view
// and are dead if hits are gone. UNKNOWN: sub0908 mechanics; recorded with the lower floor's area.
void resolveChuteLeftBehind(Ctx& ctx, Party& party);

// ---------------------------------------------------------------------------
// R16-R19: Hall of Records
// ---------------------------------------------------------------------------

// R16: pure query, whether value beats stored. Deadliest compares monster power, not id.
// UNKNOWN: effect of "stored = -1" in the Deadliest pseudocode; handled in checkAndLogHallRecord.
bool hallRecordBeaten(const GameData& data, RecordKind kind, const HallRecord& stored, i64 value);

// R16: check, then write and announce as needed. Returns true if a new record was written.
bool checkAndLogHallRecord(Ctx& ctx, HallOfRecords& hall, RecordKind kind, i64 value,
                           const Str& holder, bool holderIsPlayer, i64 nowMs);

// R18: writes the new record with holder, value and date (nowMs)
void writeHallRecord(HallOfRecords& hall, RecordKind kind, i64 value, const Str& holder,
                     bool holderIsPlayer, i64 nowMs);

// R19: announces unless the holder was the player or the kind is Deadliest
void announceHallRecord(Ctx& ctx, RecordKind kind, const HallRecord& rec);

// R16-R19: pure, builds the new-game Hall from kHallSeeds
HallOfRecords defaultHallOfRecords();

// ---------------------------------------------------------------------------
// R20: Monster power
// ---------------------------------------------------------------------------

// R20: count of set bits among the lowest bitCount bits of flags
i32 countLowBits(u32 flags, i32 bitCount);

// R20: monster power for a creature.
// UNKNOWN: which Creature fields hold the special-attack and spell bits; spell bits assumed in specialBits
i64 monsterPower(const Creature& m);

// R20: monster power for a monster id from static data
i64 monsterPowerById(const GameData& data, i32 monsterId);

// ---------------------------------------------------------------------------
// R21-R24: Skill indexes
// ---------------------------------------------------------------------------

// R21: fighting index. abilityRating indexes 0..5 match rating(a). Rounds at each step.
i32 fightingSkillIndex(const Arr<f32, 6>& ratings);

// R22: thieving index from rating 0 and effective Dexterity, value floored at 1 before scaling
i32 thievingSkillIndex(f32 rating0, i32 effectiveDex);

// R23: magic index from static spell data and guild level.
// UNKNOWN: spell set counted (assumed all of data.spells); guild rate lookup per spell.
i32 magicSkillIndex(const GameData& data, i32 guildLevel);

// R24: timing of skill evaluation is UNKNOWN; no routine declared.

// ---------------------------------------------------------------------------
// R25-R26: Pluralisation
// ---------------------------------------------------------------------------

// R25, R26: pluralises name for count; unchanged when count <= 1; first matching rule wins
Str pluraliseName(const Str& name, i32 count);

// ---------------------------------------------------------------------------
// R27-R29: Currency and extended precision
// ---------------------------------------------------------------------------

// R28: FromDouble(d) = round_half_even(d * kCurrencyScale); overflow if NaN or outside +/-2^63
CurrencyFromDoubleResult currencyFromDoubleChecked(double value);

// R29: converts a double to X87 (exact, double mantissa fits in 64 bits)
X87Value x87FromDouble(double value);

// R29: each X87 operation rounds the mantissa to 64 bits, half to even
X87Value x87Add(const X87Value& a, const X87Value& b);
X87Value x87Mul(const X87Value& a, const X87Value& b);
X87Value x87Div(const X87Value& a, const X87Value& b);

// R29: stores to Long, rounding to nearest, half to even.
// UNKNOWN: saturation at call sites; this returns the unsaturated value clamped to i64.
i64 x87StoreToLong(const X87Value& v);

// R28 RoundToWhole: use currencyWhole from math_util.h (foundation; not redeclared here).

// ---------------------------------------------------------------------------
// R31: Library sort order
// ---------------------------------------------------------------------------

// R31: WordSort comparison, returns <0, 0 or >0.
// UNKNOWN: exact WordSort rules; examples imply case-insensitive, with -, ' and spaces ignored.
i32 wordSortCompare(const Str& a, const Str& b);

// ---------------------------------------------------------------------------
// R32-R36: Quest gating
// ---------------------------------------------------------------------------

// R32: Defeat (monster) quests from guild level 5; Fetch (item) quests from guild level 15.
// UNKNOWN: eligibility for Companion and None kinds; assumed false.
bool questKindEligible(QuestKind kind, i32 guildLevel);

// R33: monster level cap, min(floor(gLevel/25), 10)
i32 questMonsterLevelCap(i32 guildLevel);

// R33: item level cap, iMax = min(7, floor(min(gLevel/25 floored, 10)) / 1.5 + 1)
// UNKNOWN: whether iMax is floored after the division; assumed floored (vbInt).
i32 questItemLevelCap(i32 guildLevel);

// R33: draws a quest level in 1..levelCap
i32 drawQuestLevel(Ctx& ctx, i32 levelCap);

// R34: pure query, rare monster when spawn weight <= 9
bool isRareQuestMonster(i32 spawnWeight);

// R34: pure query, rare item when spawn weight <= 4
bool isRareQuestItem(i32 spawnWeight);

// R34: true 74% of the time, meaning a rare pick is rerolled
bool rareQuestReroll(Ctx& ctx);

// R35: pure query, true if the item is never assigned (cursed, iLevel 0, or worth under 100g)
bool questItemExcluded(const Item& item, i32 itemLevel, Currency itemValue);

// R36: rolls the quest kind (6% none, 1% minimum assignment chance, 25% item frequency).
// UNKNOWN: how the 1%, 6% and 25% values combine; assumed: none 6%, else item at 25% if eligible.
QuestKind rollQuestKind(Ctx& ctx, i32 guildLevel);

// Quest bug (no routine): T'Mana (11), White Dragons (11), Wizards (11) and Grey Molds (13) can be
// assigned above mLevel 10. T'Mana cannot spawn, so the quest must be forfeited (see Forfeit in guilds2.h).

// ---------------------------------------------------------------------------
// R37-R41: Stored data cross-checks and quirks
// ---------------------------------------------------------------------------
// R37-R41 describe stored data (monster defenses, swing budgets, Nerve Blank, Firebolt #76, MDATA basis).
// They are data checks, not routines; no declarations here.
// CONFLICT (R40): Firebolt #76 base level is 1 in stored data, 2 in the printed manual. Stored data governs.

}  // namespace mordor
