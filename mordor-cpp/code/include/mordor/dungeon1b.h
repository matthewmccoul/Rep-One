#pragma once
// dungeon1b.h: SRD part c15p1 (R56-R122).
// Chutes, teleporter squares, teleport/displacement/portal/locate spells,
// area change and forced settle, rotator facing, lead group, look/roster,
// bodies and carrying, rescue, chests and traps, and joining.
// API INDEX items are used here, never redeclared.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_common.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_item.h"
#include "mordor/state_party.h"
#include "mordor/spells1a.h"   // ArrivalOutcome
#include "mordor/spells1b.h"   // PortalOutcome, LocateLine
#include "mordor/spells2.h"    // DisplaceOutcome
#include "mordor/status1b.h"   // sub0930, pruneTheDead, setDead
#include "mordor/status2.h"    // paralyse
#include "mordor/loot1a.h"     // ChestOpenStart
#include "mordor/loot2.h"      // DisarmResult
#include "mordor/death1a.h"    // AskToJoinOutcome // FIXED: was undefined, include its declaring header

namespace mordor {

// ---- Constants ----
inline constexpr i32 kNoRecord = -1;                  // R56 no chute or teleporter record
inline constexpr i32 kLimitedLevelCap = 3;            // R86 limited build clamps level to 3
inline constexpr i32 kTeleportWrapSize = 30;          // R83 coordinates wrap at 30x30
inline constexpr i32 kDisplaceLevelLimit = 3;         // R83 level offset clamped to -3..3

// ---- Chute structures (R56-R66) ----
struct ChuteFloatRolls {                   // R57 one roll per member, leader first
    Arr<bool, kPartyMaxSize> floats{};
};

struct ChuteFallPlan {                     // R57, R58 pure plan from the rolls
    Arr<bool, kPartyMaxSize> falls{};          // member goes down
    Arr<bool, kPartyMaxSize> goesNowhere{};    // floated but did not follow
    bool leaderFloatHint = false;              // R57 hint when the leader floats
    bool askFollow = false;                    // R58 Follow Leader box shown
};

struct ChuteWalkResult {
    bool recordFound = false;              // R56
    bool fell = false;                     // R57/R59 someone changed level
    i32 destLevel = 0;                     // R59
    i32 damage = 0;                        // R60
    bool leaderChangedLevel = false;       // R64
    bool rerun = false;                    // R65
};

enum class ChuteTakeOutcome : i32 {
    Dropped = 0,                           // R67 everyone sent down
    RefusedLimitedDeadEnd = 1              // R67 limited build dead-end level
};

// ---- Teleporter structures (R75-R82) ----
struct TeleporterDest {
    i32 x = 0;                             // 0-based destination square
    i32 y = 0;
    i32 level = 0;
    bool unmapped = false;                 // R77 destination is unmapped
};

// ---- Stairs, pickup, chest and join outcomes ----
enum class StairsOutcome : i32 {
    Taken = 0,                             // R69
    RefusedParalysed = 1,                  // R69 up-climb while paralysed
    RefusedDead = 2                        // R69 up-climb in death state
};

enum class PickUpOutcome : i32 {
    PickedUp = 0,                          // R104
    PickedUpDroppedFirst = 1,              // R104 previous carry dropped and said so
    NoCandidate = 2,                       // R104
    Refused = 3                            // R104 picker dead or paralysed
};

enum class ChestTeleportOutcome : i32 {
    Unmapped = 0,                          // R118 all three lost flags set
    OpenerRemoved = 1,                     // R118 party size > 1
    Teleported = 2                         // R118 party size = 1
};


// ---- Look / roster (R99-R100) ----
struct RosterCounts {
    i32 living = 0;
    i32 dead = 0;
};

// ---- Lead group picture (R98) ----
struct PictureLayout {
    i32 x = 0;
    i32 y = 0;
    i32 w = 0;
    i32 h = 0;
};

// ============================================================
// Map and floor support queries (pure over GameData / BuildConfig)
// ============================================================
i32 dungeonLevelCount(const GameData& data, i32 build);   // FIXED: BuildConfig undefined, now i32 build. R59 dungeon.Count, limited denies past last
i32 mapWidth(const GameData& data, i32 level);                           // map width for a level
i32 mapHeight(const GameData& data, i32 level);                          // map height for a level
i32 squareIndexOf(const GameData& data, i32 level, i32 x, i32 y);       // index into FloorState::squares, -1 if off map
bool isRockSquare(const GameData& data, i32 level, i32 x, i32 y);       // R75, R84 ROCK tile
bool isTeleporterSquare(const GameData& data, i32 level, i32 x, i32 y); // R75 teleporter tile
bool isUnmappedSquare(const GameData& data, i32 build, i32 level, i32 x, i32 y); // FIXED: BuildConfig undefined, now i32 build. R77, R118 Unmapped(level,x,y)  // UNKNOWN: exact test for unmapped
i32 limitedLevelClamp(i32 build, i32 level);             // FIXED: BuildConfig undefined, now i32 build. R86 limited build clamps level to 3
i32 wrapCoordinate(i32 v, i32 size);                                    // R83 Wrap(v,size)

// ============================================================
// Chutes, walked (R56-R66)
// ============================================================
i32 findChuteRecord(const GameData& data, i32 level, i32 x, i32 y);                       // R56 first chute at (x+1,y+1) with Depth > 0, else kNoRecord
ChuteFloatRolls rollChuteFloats(Ctx& ctx, const Party& party);                             // R57 float roll per member, leader first
ChuteFallPlan planChuteFall(const Party& party, const ChuteFloatRolls& rolls, bool followYes); // R57, R58 who falls, goes nowhere, asks follow
i32 chuteDestinationLevel(const GameData& data, i32 build, i32 fromLevel, i32 depth); // FIXED: BuildConfig undefined, now i32 build. R59 clamp(from + max(0,depth), 1, count)
i32 rollChuteDamage(Ctx& ctx, i32 level, bool levitating);                                 // R60 damage on walked chute, 0 if levitating
void reportChuteOnScreen(Ctx& ctx, const Party& party, const ChuteFallPlan& plan);        // R61 "There is a chute here.  Select TAKE to use."
void chuteBreakUp(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                  i32 belowLevel, i32 landX, i32 landY);                                   // R62 sub0908 break-up; members written below
void afterAFall(Ctx& ctx, Party& party);                                                   // R63 lone character at hits <= 0 gets Kill("")
void changeLevel(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                 i32& flags, i32 newLevel);                                                // FIXED: AreaFlags undefined, now i32& flags. R64 ChangeLevel
void settleAfterChute(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                      i32& flags, bool leaderChangedLevel, bool chuteUnderCaster);        // FIXED: AreaFlags undefined, now i32& flags. R64 settle unless caster on chute
bool chuteRerunsSquare(const Party& party, bool takenByTake);                              // R65 walked by party with companion re-runs; TAKE always; lone walker not  // CONFLICT: count of re-runs
void partyListUpdate(Ctx& ctx, Party& party, Vec<Companion>& companions);                  // R66 UPDATE after walked chutes and pit falls
void rerunSquare(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                 i32& flags);                                                              // FIXED: AreaFlags undefined, now i32& flags. R65, R81 RerunSquare (no loop guard)  // CONFLICT: re-run count
ChuteWalkResult walkChute(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                          i32 build, Vec<Companion>& companions,
                          i32& flags, bool followYes);                                     // FIXED: BuildConfig and AreaFlags undefined, now i32 build and i32& flags. R56-R66 orchestrated walked chute

// ============================================================
// Chute TAKE (R67-R68)
// ============================================================
ChuteTakeOutcome takeChute(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                           i32 build, Vec<Companion>& companions,
                           i32& flags, i32 recordIndex);                                    // FIXED: BuildConfig and AreaFlags undefined, now i32 build and i32& flags. R67 TAKE sends everyone down, no roll
void reportLimitedChuteWalkOn(Ctx& ctx);                                                    // R68 "doesn't do anything in the Limited Version."

// ============================================================
// Stairs (R69)
// ============================================================
bool canClimbUp(const Character& who);                                                     // R69 up-climb refused when paralysed or dead
StairsOutcome useStairs(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                        i32 build, i32& flags, bool up);                                   // FIXED: BuildConfig and AreaFlags undefined, now i32 build and i32& flags. R69 UseStairs (down and chutes outside paralysis test)

// ============================================================
// Water, quicksand and other ticks (R70-R73)
// ============================================================
bool rollWaterItemLoss(Ctx& ctx, i32 dungeonLevel);                                        // R71 takes if rng.below(100) < dungeonLevel*4
void applyWaterTick(Ctx& ctx, Character& who, Inventory& inventory, i32 dungeonLevel,
                    bool quicksand, bool levitating);                                      // R70 hits lost each tick; quicksand may take a pack slot  // UNKNOWN: hit-loss scaling (WaterHurts = 1)
i32 identifyChance(const Character& who, bool darkness, bool fog);                         // R72 chance clamped 10..90, quartered in darkness  // UNKNOWN: M4EEE meaning (fog flag)
bool rollIdentify(Ctx& ctx, const Character& who, bool darkness, bool fog);                // R72 per group, per character, per tick
bool fourSecondTickDue(i64 nowMs, i64 lastTickMs);                                         // R73 true when four seconds have passed
void runFourSecondTick(Ctx& ctx, Party& party, Inventory& inventory,
                       i64 nowMs, i64& lastTickMs);                                        // R73 ailment and item tick

// ============================================================
// Teleporter squares (R74-R82)
// ============================================================
i32 teleporterLevel(i32 z2, i32 fromLevel);                                                // R76 Z2 if > 0 and differs, else current level
i32 teleporterLoadedLevel(const Party& party, const GameData& data, i32 build); // FIXED: BuildConfig undefined, now i32 build. R75 loaded level from leader if in 1..count  // UNKNOWN: leader.level read as location.dungeonLevel
Vec<TeleporterDest> rollTeleporterDestinations(Ctx& ctx, const GameData& data, i32 build,
                                               i32 fromLevel, const FloorTeleporter& tp,
                                               i32 memberCount);                           // FIXED: BuildConfig undefined, now i32 build. R75 X2/Y2/Z2 fixed or random, redrawn until not rock or teleporter  // CONFLICT: R76 quirk note
void becomeLost(Ctx& ctx, Character& who);                                                 // R77 BecomeLost for unmapped destination
void splitTeleportedParty(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                          Vec<Companion>& companions, const Vec<TeleporterDest>& dests);   // R78 members i>=1 with dest != dest[0] leave and are saved there
void finishTeleportLeader(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                          i32& flags, const TeleporterDest& leaderDest,
                          bool partyShrank);                                               // FIXED: AreaFlags undefined, now i32& flags. R80 PruneTheDead, leader to dest[0], AfterAFall, RerunSquare, SettleArea last
void walkOntoTeleporter(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                        i32 build, Vec<Companion>& companions,
                        i32& flags, const FloorTeleporter& tp);                            // FIXED: BuildConfig and AreaFlags undefined, now i32 build and i32& flags. R74-R82 walking on a teleporter: no fall, no rock death
void reportTeleportPastLastLevel(Ctx& ctx, i32 destLevel, i32 lastLevel);                  // R82 record past last level only logs

// ============================================================
// Teleport, displacement, Ethereal Portal and Soul Search spells (R83-R88)
// ============================================================
Location displaceTarget(const Location& from, Facing facing, i32 right, i32 forward, i32 down); // R83 pure destination, wrap 30x30, level clamp -3..3, floor 0
ArrivalOutcome teleportArrivalTest(const GameData& data, i32 build,
                                   i32 level, i32 x, i32 y);                                // FIXED: BuildConfig undefined, now i32 build. R84 ForSpell order: FallsFromTheSky, IntoRock, Arrives
DisplaceOutcome displaceParty(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                              i32 build, i32& flags,
                              i32 right, i32 forward, i32 down);                           // FIXED: BuildConfig and AreaFlags undefined, now i32 build and i32& flags. R83-R85 Teleport spell move
void applyFailedTeleport(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                         i32 build, ArrivalOutcome outcome,
                         i32 level, i32 x, i32 y);                                         // FIXED: BuildConfig undefined, now i32 build. R85 level 0 falls and kills; rock marks automap and kills  // UNKNOWN: cause enum value for rock death
PortalOutcome etherealPortalStep(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                                 i32 build, i32& flags);                                   // FIXED: BuildConfig and AreaFlags undefined, now i32 build and i32& flags. R87 StepThrough; party moves, then rock test  // CONFLICT: which square is tested
LocateLine locateCharacter(const Location& me, Facing facing, const Location& other);      // R88 Soul Search, inverse of Teleport; range kLocateLevelRange

// ============================================================
// Area change and forced settle (R89-R95)
// ============================================================
void markForcedSettle(i32& flags, i32 source);                                             // FIXED: AreaFlags and ForcedSettleSource undefined, now i32& flags and i32 source. R89 only the three listed sources set the flag  // CONFLICT: c30 also lists level squares and editor
bool settleArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                i32& flags);                                                               // FIXED: AreaFlags undefined, now i32& flags. R90, R93 takes and clears flag; false when same area and not forced
void afterTurn(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
               i32& flags);                                                                // FIXED: AreaFlags undefined, now i32& flags. R90 AfterTurn: flag still set calls SettleArea
void settleForcedSameArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                          i32& flags);                                                     // FIXED: AreaFlags undefined, now i32& flags. R91, R92 store room, free round, save, put down bodies, report, Arrived; timCombat off  // CONFLICT: c44 on timCombat
void leaveArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data);     // R94 LeaveArea; hostile groups not fighting get leave round
void loadArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
              bool arrivalRound);                                                          // R94 LoadArea
void saveCharacters(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data); // R94 SaveChar
void arrived(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data);       // R91, R94 Arrived()
void changeAreaDifferent(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                         i32& flags, i32 newArea);                                         // FIXED: AreaFlags undefined, now i32& flags. R94 different area path

// ============================================================
// Rotator facing (R96)
// ============================================================
Facing rollRotatorFacing(Ctx& ctx);                                                        // R96 random facing, rng.below(4)

// ============================================================
// Lead group and Asmodeus (R97-R98)
// ============================================================
const Creature* leadGroupCreature(const Encounter& combat, const Vec<Creature>& waiting);   // R97 lead creature or null
bool isAsmodeusLead(const Creature* lead);                                                 // R98 lead is Asmodeus  // UNKNOWN: identification key for Asmodeus
PictureLayout asmodeusPictureLayout(bool noStretch, i32 viewW, i32 viewH);                 // R98 stretch to view, or centre 100x100 when NoStretch

// ============================================================
// Look, visibility and roster (R99-R102)
// ============================================================
bool includeInRoster(const Party& party, const Character& r, bool everywhere);             // R99 same floor and area, or leader, or everywhere
RosterCounts countRoster(const Party& party, const Vec<Character>& roster, bool everywhere); // R100 dead counted apart; dead leader counted as living
Str roomReadout(const Party& party, const Vec<Character>& roster, bool everywhere,
                i32 listedDead);                                                           // R99, R101 readout by state; listedDead is MorgueList copy  // CONFLICT: R101 lost-position text
void reportLookAround(Ctx& ctx, const Party& party, const FloorState& floor);              // R102 bodies and strangers by floor and area

// ============================================================
// Bodies and carrying (R103-R110)
// ============================================================
Vec<i32> pickUpCandidates(const Party& party, const FloorState& floor,
                          const GameData& data, i32 pickerSlot);                           // R104 dead, not self, not carried, same level and area; hidden excluded
PickUpOutcome pickUpBody(Ctx& ctx, Party& party, FloorState& floor, const GameData& data,
                         i32 pickerSlot, i32 bodyIndex);                                   // R104 PickUpBody; picking up while carrying drops first
void putDownWhatIsCarried(Ctx& ctx, Party& party, FloorState& floor, const GameData& data,
                          Vec<Companion>& companions);                                     // R106 companion released from slot; body not left in slot
void dropBody(Ctx& ctx, Party& party, FloorState& floor, const GameData& data,
              Vec<Companion>& companions);                                                 // R105 DropBody; nothing when paralysed
void deliverCarried(Ctx& ctx, Party& party);                                               // R107 carried bodies to city or morgue on walking out
void syncAreas(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data);     // R108 area -1 at level 0; body in rock becomes nought and is dropped
void applyRescueArrival(Character& body, i32 daysWaited);                                  // R109 age rises, level 0, area -1, X and Y unchanged
void loadSavedBodyOnRock(Ctx& ctx, FloorState& floor, const GameData& data,
                         Character& body);                                                 // R110 saved body on rock loads on that rock

// ============================================================
// Rescue, dead screen and corpse checks (R111-R115)
// ============================================================
bool townScreenWanted(const Party& party);                                                 // R112 DungeonLevel = 0 and leader not dead
bool deadScreenOwed(const Party& party);                                                   // R112 dead and alone (UBound(Chars) = 0)
bool waitActsOn(const Character& who);                                                     // R113 cmdWait acts only on a dead character
void showRescueArrival(Ctx& ctx, Party& party, Character& body, i32 daysWaited);           // R111 body brought up; town shows dead screen  // CONFLICT: level 0
double rescueThresholdDays(i32 level);                                                     // R114 ln(L)^2 * 100 + 21  // UNKNOWN: ln assumed natural
double rescueCeiling(i32 level, i32 area, i32 daysWaited);                                 // R114 10000 or 1000; level 1 areas <= 18 give 100
double rescueChance(i32 level, bool easyToFind);                                           // R114 max(16 - L, 0.1), easy-find added after the floor  // CONFLICT: wiki table
bool rollBodyFoundToday(Ctx& ctx, i32 level, i32 area, i32 daysWaited, bool easyToFind);   // R114 found when rng.nextDouble()*ceiling <= chance
i32 waitForRescueDays(Ctx& ctx, i32 level, i32 area, bool easyToFind);                     // R114 days waited, capped at kRescueDayCap
// R115 ordinary rescue is covered by applyRescueArrival and waitForRescueDays.

// ============================================================
// Chests, traps and disarm (R116-R121)
// ============================================================
void stepChestCountdown(Ctx& ctx, Character& opener);                                      // R116 one countdown step before the opener acts
ChestOpenStart beginChestAfterCombat(Ctx& ctx, Party& party, FloorState& floor,
                                     const GameData& data, i32 openerSlot);                // R116 opener opens unlocked chest; clears order and starts countdown if disarm > 10
DisarmResult disarmChestTrap(Ctx& ctx, Party& party, Character& opener, SquareState& chest,
                             i32 bestThiefChance);                                         // R117 1..100 draw at or below chance disarms; failure springs trap first
ChestTeleportOutcome chestTeleporterTrap(Ctx& ctx, Party& party, Vec<FloorState>& floors,
                                         const GameData& data, i32 build,
                                         i32& flags, i32 openerSlot,
                                         i32 level, i32 x, i32 y);                         // FIXED: BuildConfig and AreaFlags undefined, now i32 build and i32& flags. R118 chest teleporter trap, opener only  // CONFLICT: c30 flag sources
i32 chestCountdownRounds(i32 disarm);                                                      // R119 max(2, B(10 - 2*ln(disarm-10))) when disarm > 10  // UNKNOWN: B(...) undefined in dossier
i32 chestDisarmChance(i32 wis, i32 dex, i32 joinedGuildLevel, double thiefValue,
                      i32 guardGuildLevel, i32 depth);                                     // R119 WIS/4 + DEX/2 + (4.5+exp(ln(L+1)/ln 15))*1.4t - 9.375*ln(max(2,M))*ln(d+1), clamp 5..98  // CONFLICT: R120 wiki form
i64 disarmXpBase(double thiefValue, i32 guardGuildLevel, i32 leaderActiveLevel);           // R119 t*(M/2)/(1+active/333)  // UNKNOWN: party-size adjustment
i32 trapIdentifyChance(i32 wis, i32 mLvl, i32 mDex, i32 gLvl, i32 disarm);                 // R121 floor(floor(base + disarm/1.2) + ln(gLvl)^2), clamp 2..98  // UNKNOWN: mLvl, mDex for traps
bool rollTrapIdentify(Ctx& ctx, i32 wis, i32 mLvl, i32 mDex, i32 gLvl, i32 disarm);       // R121 chest trap identification roll; may show a differing guess

// ============================================================
// Ask to Join (R122)
// ============================================================
bool leaderAliveForJoin(const Party& party);                                               // R122 join only if party index 0 is alive
AskToJoinOutcome askToJoin(Ctx& ctx, Party& party, Character& joiner,
                           bool monstersPresent);                                          // R122 name, password, opposed and dead refusals; silent join restores hits and spells

}  // namespace mordor
