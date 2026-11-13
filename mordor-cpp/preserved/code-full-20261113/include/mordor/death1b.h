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

// R12.2 refusal checked by cause of death (Table 3)
i32 raiseRefusalFor(const Character& body);  // FIXED: RaiseRefusal is declared nowhere; plain i32 used for the refusal code

// R12.7 / R12.11 age added on a failed or stoned/rock raise: 84 stoned, 124 rock, else 54
i32 raiseFailAgeDays(DeathState state);

// R12.10 / Table 2: CurHits = (kind in {22, 23}) ? MaxHits : 1
i32 raisedCurHits(RaiseKind kind, i32 maxHits);

// R12.5 roll: fail if rng.below(100) > roll, roll = 0 if rock else chance
// CONFLICT 33: R12.4 says rock "can only fail"; with roll 0 a draw of 0 succeeds (1%). Implemented as stated.
bool raiseRollFails(Ctx& ctx, i32 chance, bool rock);

// R12.6 complication roll against the complication chance
// UNKNOWN: complication chance formula is not in the chunk; caller passes it.
bool rollRaiseComplication(Ctx& ctx, i32 complicationChance);

// R12.8 complication effects, in order (count up, stat loss, age, MaxHits, CurHits, unequip)
// UNKNOWN: Character has no complication counter field in state_character.h; to be added.
void applyRaiseComplication(Ctx& ctx, Character& body);

// R12.9 success effects: revive, cause becomes ordinary, age (Table 4), hits per kind
// UNKNOWN: Table 4 age values are not in the chunk; age added is taken from Table 4 by the caller's data.
void applyRaiseSuccess(Ctx& ctx, Character& body);

// R12.9 CON loss for stone, applied regardless of outcome
void applyStoneConLoss(Character& body);

// R12.9 / R12.11 morgue raise (kind 23 only) extra CON-loss roll
// UNKNOWN: morgue CON-loss roll formula not in the chunk.
void applyMorgueConRoll(Ctx& ctx, Character& body);

// R12.9 own-body raises reset CON to the racial minimum
void resetConToRacialMinimum(Ctx& ctx, Character& body);

// R12 full resolution, in order (pending body, chance and kind taken from body fields)
// Points spent first (R12.1) is the caller's duty; UNKNOWN which pool.
// Failure emits ctx.say("raise.failed", "Raise Failed!", ...) (R12.7).
RaiseResult resolveRaise(Ctx& ctx, Character& body);  // FIXED: RaiseResolution is declared nowhere; RaiseResult (suggested by compiler) used

// ---- R13 Spell raise chance ----

// R13 tier = max(1, effect - 19): 20 -> 1, 21 -> 2, 22 -> 3
i32 spellRaiseTier(RaiseKind effect);

// R13 chance = clamp(round_half_even(35 * (CON / 4.5) + tier * 10), 10, 98); rock forces 10
i32 spellRaiseChance(i32 naturalCon, RaiseKind effect, bool causeIsRock);

// ---- R14 Morgue raise chance ----

// R14 own-body (+40) or counter (+10) morgue chance
// UNKNOWN: base formula not given; this module uses 35 * (CON / 4.5) as the base (spell CON term).
// CONFLICT 1: own body +40 vs counter +10.
i32 morgueRaiseChance(i32 naturalCon, bool ownBody);

// ---- R15 Morgue fee ----

// R15 counter fee: round_half_even(2912.32 * (MaxHits / 1.5)); returned in 1/10000 gold
// (SRD formula gives whole gold; convert with currencyFromWhole).
Currency morgueFeeCounter(i32 maxHits);

// R15 own-body offer fee: Int(2912.32 * (MaxHits / 1.5)); returned in 1/10000 gold.
// Differs from morgueFeeCounter by at most one gold (quirk).
Currency morgueFeeOwnBody(i32 maxHits);

// R15 guild pays when TopGuildLevel <= 5 and purse < cost
// UNKNOWN: "Purse" taken as Character::purse() (on hand + bank).
bool morgueGuildPays(const Character& ch, Currency cost);

// ---- R16 Companion raise (dungeon, cast on Who?) ----

// R16.1 refused if the caster is carrying a character (CarryingSlot::DeadCharacter)
bool companionRaiseRefused(const Party& party);

// R16.2 candidate companion indices: dead, not pending raise, not carried by any party member,
// on party.dungeonLevel and in area. Listed by slot.
// UNKNOWN: "not the caster" is applied as companion index != casterSlot.
Vec<i32> companionRaiseCandidates(const Party& party, const Vec<Companion>& companions,
                                  i32 casterSlot, i32 area);

// R16.4 the carried companion is put down silently, on success or failure
void putDownCarriedCompanion(Party& party);

// R16 fallback CON: MonsterOf(comp).Stats[3] ?? 10
// UNKNOWN: Stats[3] index convention is from the port, not the original.
i32 companionRaiseCon(const GameData& data, const Companion& comp);

// R16.5 morgue companion cost: round_half_even(MaxHits * 1244.34 * Floor), in 1/10000 gold
Currency companionMorgueCost(i32 maxHits, i32 floor);

// R16.5 morgue companion chance: min(98, 75 + CON)
i32 companionMorgueChance(i32 naturalCon);

// R16.5 morgue companion roll: success unless Int(Rnd*100) > chance
bool rollMorgueCompanion(Ctx& ctx, i32 naturalCon);

// ---- R17 Teleported into rock, or fall from the sky ----

// R17 kill routine used by R17 and R18.
// When runCleanup is true the caller's cleanup (sub0930, status1b.h) runs for the slot.
void killPartyMember(Ctx& ctx, Party& party, Vec<Companion>& companions, i32 slot,
                     DeathState state, bool runCleanup);

// R17.1-R17.4 teleport into rock (rock death for members, or level 0 ordinary death)
// Writes the rock square to the automap of levels[level - 1] when on the map.
TeleportRockResult teleportIntoRock(Ctx& ctx, Party& party, Vec<Companion>& companions,
                                    Vec<FloorState>& levels, OffMapStore& offMap,
                                    i32 level, i32 x, i32 y);

// R17.5 fall from the sky: each member dies ordinary and a tell box is shown (ctx.say)
void fellOutOfTheSky(Ctx& ctx, Party& party, Vec<Companion>& companions);

// ---- R18 Trap death of the chest opener ----

// R18 chest trap leader check; the chest stays untrapped with its gold.
// Display and save routines in the SRD are out of scope for this module.
OpenerTrapOutcome chestOpenerDeathCheck(Ctx& ctx, Party& party, Vec<Companion>& companions,
                                        SquareState& chest, i32 openerSlot);

// ---- R19 Old-age death ----

// R19.1 pct = (ageDays / 365) / raceLifespanYears * 100
double oldAgePercent(i32 ageDays, i32 raceLifespanYears);

// R19.2 death roll: only when pct > 98 (CONFLICT 13 resolves "passes 98%" over "pct >= 98");
// threshold 5 + Ln(pct - 98) is clamped to 0 at the low end (UNKNOWN; no clamp in the chunk).
bool rollOldAgeDeath(Ctx& ctx, double pct);

// ---- R20 Character creation and deletion ----

// R20.1 registers a new character in the roster, marks it saved, logs the guild join
void registerNewCharacter(Ctx& ctx, Vec<Character>& roster, Character& ch,
                          HallOfRecords& hall, i64 day);

// R20.2 offers all six natural stats to the Hall of Records, each alone
void offerNaturalStatsToHall(Ctx& ctx, HallOfRecords& hall, const Character& ch, i64 day);

// R20.3 deletion is by main-menu Delete Character only; this query checks save is gone
bool characterDeletable(const Character& ch, bool saveFileExists);

// ---- R21 Kills, chest finds and quests ----

// R21.1 a kill is recordable only when the group is identified; invisible monster needs
// the killer to see invisible.
bool isKillRecordable(bool groupIdentified, bool monsterInvisible, const Character& killer);

// R21.1 records the kill for the killer (kills and monsterKills) when recordable
// CONFLICT 16: engine says killer only; owner credit (FIDELITY) is not applied.
bool recordKill(Ctx& ctx, Character& killer, const Creature& monster,
                bool groupIdentified, bool monsterInvisible);

// R21.2 chest find credit: lead.Id, or -2 (kChestFindNoMonster)
// UNKNOWN: value -2 beyond "unknown monster" is not in the chunk.
i32 chestFindCredit(i32 leadIndex, i32 foes0KnownAs, i32 leadId,
                    bool leadInvisible, const Character& taker);

// R21.3 monster quest settled at the kill, current guild only, win or lose; names the character
void settleMonsterQuestAtKill(Ctx& ctx, Character& killer, i32 monsterId);

// ---- R22 Hall of Records checks ----

// R22.1 offer a record for a category; returns true when taken. Sets hall.dirty when offered.
// UNKNOWN: taken semantics (CheckAndLog) are not in the chunk; this module uses value > stored or no holder.
bool offerHallRecord(Ctx& ctx, HallOfRecords& hall, i32 category, const Character& ch,
                     i64 value, bool alone, i64 day);

// R22.1 after a level, the Fighting, Magic and Thieving skill records are checked
// UNKNOWN: mapping of these skills to category indices is not in the chunk.
void checkLevelRecords(Ctx& ctx, HallOfRecords& hall, const Character& ch, i64 day);

// R22.2 Wealthiest record, checked each time the Bank status line is drawn (exact comparison)
// CONFLICT 15: half-gold behaviour is ambiguous; exact comparison is used.
void checkWealthiestRecord(Ctx& ctx, HallOfRecords& hall, const Character& ch,
                           Currency wealth, i64 day);

// R22.4 guild log line for death, resurrection, demotion or join
void appendGuildLog(Ctx& ctx, HallOfRecords& hall, GuildLogKind kind, Guild guild,
                    i64 day, const Character& ch);

// R22 stored stat field: value * 10000 (written as hex, reverse byte order by the adapter)
// CONFLICT 28: archive field layout may not match MDATA9/12 fields.
i64 hallStatField(i64 value);

// R22 stored date field: days since 1900-01-01 + 2 (written as hex by the adapter)
i64 hallDateField(i64 daysSince1900);

// ---- R23 Retrieve Soul targets ----

// R23.1-R23.2 dead in rock (DeathState::Rock), on the current level, in area 0.
// UNKNOWN: check order and handling of a non-zero area are not in the chunk; level is checked first.
SoulCheck retrieveSoulStatus(const Character& ch, i32 currentDungeonLevel);

// ---- Morgue input (no rule number in the SRD) ----

// Coordinates split on commas; a trailing comma is ignored.
Vec<Str> splitCoordinateInput(const Str& text);

// Val: skips spaces, rounds (3.7 -> 4), stops at the first non-numeric character
// (1-2 -> 1; x -> 0). UNKNOWN: tie rule for rounding; roundHalfEven is used.
i64 valLikeOriginal(const Str& text);

}  // namespace mordor
