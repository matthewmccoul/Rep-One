// state_game.h
// Top-level session/save state. Plain data only; no logic beyond tiny inline helpers.
// Merged from the SRD state census. Grep for CONFLICT: and UNKNOWN: to find open items.
// Encounter-owned census lines (Game.Combat.*, Foes, Round, Fight, Order, Phase, Spawn*,
// Companion*, TurnDamage, TargetWeights, WaitingCount, GroupCount, JoinOffer, Breath*,
// MonstersPresent, InCombat sub-state) live in state_encounter.h, not here.
// state_character.h and state_floor.h.
// UNKNOWN: the exact names of the other state_*.h headers are inferred from the census types;
// confirm them against the tree before building.
#ifndef MORDOR_STATE_GAME_H
#define MORDOR_STATE_GAME_H

#include "mordor/types.h"
#include "mordor/state_common.h"
#include "mordor/state_party.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"

namespace mordor {

// ---------------------------------------------------------------------------
// Rule constants (not static game data; those stay in GameData)
// ---------------------------------------------------------------------------
constexpr i32 kMaxDungeonLevel = 15;            // constant 15 (R53, R59); also mc015C/mc015E
constexpr i32 kLimitedMaxLevel = 3;             // limited build level cap (R55, R82, R86)

struct RawSetting {                 // Option.<key> / ModSettings.Values in-memory kv (R4, R9, R10)
    Str key = "";
    Str text = "";
};

// Options. Flags default False (R1, R2). Font, volume and size values from R1-R4.
struct Options {
    bool noAutoSave = false;            // autosave behaviour (R1, R2)
    bool noBlinkCursor = false;         // message/cursor behaviour (R1, R2)
    bool noDing = false;                // message-sound behaviour (R1, R2)
    bool noLoadAutomap = false;         // automap behaviour (R1, R2)
    bool noPartyFx = false;             // silences background combat sounds from party members (R1, R2)
    bool noStretch = false;             // native-size pictures; Asmodeus centred at 100x100 (R1, R2, R98)
    bool noToolbar = false;             // written NoToolBar when True, NoToolbar when False (R1, R2, R5)
    bool soundCanvas = false;           // the one flag that turns a feature on (R1, R2)
    Arr<bool, 4> unnamedFlags = {};     // Opt.Flag9..Flag12; names UNKNOWN (R1, R2, R7)
    bool noPerceptionMessages = false;  // suppress look-ahead/perception messages incl teleporter line (R39, R62)
    bool unlockAlignChoice = false;     // Quality of Life 1.12.0 toggle; false = refused (c36, c37, R3)
    i32 brightness = 0;                 // brightness option (R1, R3)
    i32 fxVolume = -65538;              // FX volume; WaveVolume.Default per source note (R1, R3)
    // CONFLICT: R1/R3 say default WaveVolume.Default; the source note gives -65538. Using -65538.
    i32 musicVolume = 0;                // music volume; default 0 when key missing (R1, R3)
    Str proFontFace = "Courier New";    // "Pro" font face; reset to default if read back empty (R1, R2, R4)
    i32 proFontSize = 9;                // "Pro" font size (R2, R4)
    Str nonProFontFace = "MS Sans Serif"; // "NonPro" font face (R1, R2, R4)
    i32 nonProFontSize = 8;             // "NonPro" font size (R2, R4)
    i32 messagesKept = kDefaultMessageLines; // FIXED: was kDefaultMessagesKept (not declared); city1.h kDefaultMessageLines. message window line count, min 1 (R42, T-N, R30)
    Vec<RawSetting> rawSettings = {};   // settings.ini first, then MORDOR.INI fallback (R4, c10)
};

// Mod manifest and settings (Table 1, F1-F6, R2-R22)
struct ModToggleSpec {
    Str key = "";
    bool defaultValue = false;          // manifest version wins over code (R17, F5)
};

struct ModChoiceSpec {
    Str key = "";
    Str defaultOption = "";             // declared default; "" never matches (F6)
    Vec<Str> options = {};              // empty = stored text returned as-is (F6)
};

struct ModTableSpec {
    Str key = "";
    Vec<Str> items = {};                // UNKNOWN: element shape of list settings (Table 1, R17)
};

struct ModManifest {
    Str id = "";                        // non-blank; may not contain [ ] = CR LF (R4, F1)
    Str name = "";                      // blank falls back to id in Title (F2)
    Str version = "";                   // "" adds no suffix in Title (F2)
    Str author = "";                    // default "" (Table 1)
    Str description = "";               // default "" (Table 1)
    Str assembly = "";                  // .dll path relative to mod folder; non-blank (F1, Table 1)
    Str entryType = "";                 // full class name implementing IMod; non-blank (F1, Table 1)
    Str gameVersion = "";               // should match gameVersion (R22, Table 1)
    Vec<ModToggleSpec> settings = {};   // declared toggles (Table 1, R17)
    Vec<ModChoiceSpec> choices = {};    // declared choices (Table 1, F6)
    Vec<ModTableSpec> tables = {};      // declared list settings (Table 1, R17)
    bool hasSettingsWindow = false;     // settingsWindow null = none; spec shape UNKNOWN (Table 1)
};

struct LoadedMod {
    Str id = "";
    bool faulted = false;               // disabled for session after a fault (R13)
    Str faultReason = "";               // logged reason (R13)
    Vec<Str> patchIds = {};             // UNKNOWN: Harmony patch records are runtime, not plain data (R10, R12)
};

struct ModSettingEntry {                // stored per mod in settings.ini (R17, F5, F6)
    Str modId = "";
    Str key = "";
    Str text = "";                      // choices: trimmed before matching (F6)
    bool toggle = false;                // toggles: default DefaultFor(key), else false (F5)
    Vec<Str> list = {};                 // tables: element type UNKNOWN (R17)
};

struct ModEnabledEntry {                // settings.ini enable flag (F4, R6)
    Str modId = "";
    bool enabled = false;
};

struct ModDisabledEntry {               // session disable after row-order mismatch or thrown hook (R37, R39, R40)
    Str modId = "";
    bool disabled = false;
};

struct GameEvent {                      // port addition, not an original rule (R24)
    Str key = "";
    bool activated = false;             // CONFLICT: R24 says both "resolved" and "activated"; kept as activated.
};

// Hall of Records (R14, R16, R18, R20, R22, Table 11, Table 12)
struct HallRecord {
    Currency value = 0;                 // 1/10000 gold; stat value * 10000, or purse+bank for Wealthiest (Table 11, R14)
    Str holder = "";                    // empty until set (Table 11)
    i64 day = 0;                        // days since 1900-01-01 + 2; 0 = blank / "Original RecordHolder" (c88, R16)
    // CONFLICT: c88 gives the 1900-01-01+2 epoch; Today uses 1899-12-30. Kept as the record epoch.
};

struct HallLegacyEntry {                // MDATA9/MDATA12 layout (c35, c88)
    i32 stat = 0;
    i64 day = 0;
    i32 unused = 0;
    Str text = "";
    // CONFLICT: MDATA9 layout conflicts with the archive 8-byte/7-byte form (c35, c88).
};

// Guild master seat (R7, R21, R24, R25, R57, R58, T2)
struct GuildMaster {
    i32 holder = -1;                    // character slot of sitting master; -1 = none (assumed) (R24)
    i32 level = 0;                      // holder's level in that guild; 0 when empty (R57)
    Str name = "";                      // "" = empty seat (R57); default from T2 seed when seeded
};

// Library record, merged from Library[key] (R49-R57) and LibRecord (c12, c57)
struct LibRecord {
    Str key = "";                       // "mr:index" creatures, "i:itemId" items (R48, R58)
    i32 id = 0;                         // item id or companion def Id; 0 = not listed (R49, R52)
    // CONFLICT: c12/c51 say creature Id 307 is shared by Morgul (278) and Shadow Ninja (389).
    i32 monId = 0;                      // source monster table index; -2 = shelf restock (R50, R51)
    i32 known = 0;                      // knowledge grade 0..4, raised only by writes (R49, R23, c12)
    i64 lastSeen = 0;                   // serial of last write; 0 = no date (R50, R53); Date i32 merged
    Str location = "";                  // "x+1,y+1,level", "the City", "?" or ""
    // CONFLICT: LibRecord.Place defaults to "?" (c12) but Library.Location defaults to "" (R50). Using "".
    i32 nDeaths = 0;                    // deaths by the creature, 0..32767 (R53, R54)
    i32 nSeen = 0;                      // times found or seen (R50, R51, R53)
    Str name = "";                      // finder or recorder name; cleared when known is 0 (R49, R51)
};

// Legacy MDATA13 library fields (Library.DeathsField etc., c88, c96)
struct LegacyLibraryFields {
    i32 deathsField = 0;                // item entry deaths field; 0xFFFF in source (c88)
    i32 itemState = 0;                  // 4 = all known; otherwise almost everything (c88)
    i32 monsterState = 0;               // 0 Unknown, 1 Seen, 2 Defeated, 3+ Fully Known (c88, c96)
    // CONFLICT: MDATA13 scale 1-4 conflicts with 0..3+ above.
    i32 seenFoundCount = 0;             // seen/found count scaled x10000 (c88)
};

// Off-map body kept past deepest level (c44, R17)
struct OffMapBody {
    i32 level = 0;
    i32 x = 0;
    i32 y = 0;
    Character body = {};
};

// Store buffer (R61, R31, R32). Content not given; empty does nothing.
struct Buffer {
    bool filled = false;
    Vec<i32> keys = {};                 // UNKNOWN: structure of stored keys
};

// Store shelf per item table index (c35, c52)
struct TownShelf {
    Arr<i32, 4> align = {};             // counts U, G, N, E; each capped at 9 on the shelf line (c35, c52)
    i32 floor = 0;                      // minimum level stocked; 0 stocks unaligned items (c35, c52)
};

// Remembered party, one per leader (R18, R19)
struct SavedParty {
    Str leader = "";
    Vec<Str> members = {};
};

// Guild log event (T29, Quirks guild log, R65)
struct GuildLogEvent {
    i64 day = 0;                        // Game.Day when written (Quirks guild log)
    Guild guild = {};                   // guild the event belongs to (T29)
    i32 kind = 0;                       // FIXED: GuildLogKind not declared; plain i32, 0 = Join
    Str text = "";                      // UTF-8 log text (R6)
};

// ---------------------------------------------------------------------------
// GameState
// ---------------------------------------------------------------------------
struct GameState {
    // ---- Party and encounter ------------------------------------------------
    Party party = {};                   // Party (state_party.h)
    Arr<Arr<Companion, kCompanionSlots>, kPartyMaxSize> memberCompanions = {};   // companion slots of party member i (0-based); not saved yet
    Encounter encounter = {};           // Encounter, valid while fighting (state_encounter.h)
    bool inCombat = false;              // a fight is in progress; blocks giving items and gold (R1, R4, R15, R22)
                                        // merged: Game.Combat.Exists, Game.InCombat
    bool underground = false;           // combat tick runs only below ground (R1)
    bool combatClockRunning = true;     // combat clock; stops when window minimised (R26, [c45])
                                        // CONFLICT: c30/c44 say the same-area block turns it off until monShowMonsters
                                        // restarts it (R92). Merged with timCombat; default kept true.
    i32 combatIntervalMs = 1500;        // FIXED: kFormCombatIntervalMs not visible in this scope; literal value 1500 from clock.h (clock.h include does not resolve here). combat tick interval (R1, P1.1)
    bool inCombatRound = false;         // overlap guard for the combat tick (R5)
    bool tooLongPeaced = false;         // blocks CombatOnly spells with base level > 7 (R14)
    bool castFlag = false;              // set on successful cast; item charges depend on it (R16, R19)
    bool refusedOrder = false;          // item cast check refused; item not charged (R10)
    i32 castOnWhoCount = 0;             // gates the "Cast on Who?" prompt when > 0 (M5B33, R64, [c34])
    i32 baseAd = 0;                     // base attack/defence value; value not given in this part (R2)
                                        // UNKNOWN: value not given in this part of the SRD.
    i32 freeAttackThreshold = 5;        // clamped free-attack threshold, 5..98 (R26)
    i32 freeAttackEvent = 0;            // FIXED: FreeAttackEvent not declared; plain i32, 0 = Moved. event that triggers a monster free attack (R33, Quirks)
    i32 moveArg = 0;                    // FIXED: MoveArg not declared; plain i32, 0 = None. movement argument (R21-R23)
    bool followLeader = false;          // player answer when leader falls with party > 1 (R11)
    i32 respawnTime = 0;                // respawn time value; definition UNKNOWN (R50)
    i32 rescueDaysUntilFound = 0;       // rescue constant; value UNKNOWN (c25, c45)
    bool rescueEasy = false;            // easy flag passed to rescue timing (R52)
    i32 rescueArea = 0;                 // set on rescue; probably equals area (M59BB, c32)
                                        // UNKNOWN: M59BB probably equals area; not stated.
    bool adShiftUnsettled = false;      // set on load if ParalysisLevel != 0 and no "ADShift" key (R1)
                                        // UNKNOWN: consequence not stated in the SRD.
    bool actionRunActive = false;       // action run active; new hook request refused and logged (R123)
    bool headless = false;              // no box shown, every question cancelled (R70)
    i32 exitCode = 0;                   // 6 when combine overflows charges; game ends without saving (c52)
    i32 confirmDialog = 0;              // FIXED: ConfirmDialog not declared; plain i32, 0 = None. exit prompt shown (R125, R127)
    bool reloadChoice = false;          // answer to the Reload prompt; default No (R18)
    i32 confineChoice = -1;             // pending Confinement alignment; -1 = waiting for press (R11, R14)
    i32 storeChoice = -1;               // pending Store Buy alignment; -1 = waiting for press (R11, R12, R13)

    // ---- Clocks (all times in milliseconds) ---------------------------------
    i32 timeOfDay = 0;                  // seconds since midnight, 0..86399; no date (c9, R49, c24)
                                        // merged: Game.Clock (i64), Game.NowSec, Game.TimeOfDay.
                                        // UNKNOWN: default at start not stated.
    i64 today = 0;                      // today as a serial, whole days since 1899-12-30; 0 = not known (R48, R62)
                                        // merged: Game.Day, Game.Today, Game.Clock (day). c57 treats clock 0 as unknown.
    i64 lastTickMs = 0;                 // last timer reading, refreshed on every tick; paused or not (c59, R27)
                                        // CONFLICT: R27 says seconds; stored in ms per port convention.
    i64 playElapsedMs = 0;              // play-time accumulator; grows only when not paused (R27, c59)
                                        // CONFLICT: census says seconds; stored in ms per port convention.
    bool paused = false;                // Settings.Paused; clock does not accumulate while true (c59, R27)
    bool minimised = false;             // window minimised; gates both timers (Quirks, R26)
    bool playClockRunning = true;       // play clock stops when window ends (P1.3, P8)
    i32 playIntervalMs = 0;             // FIXED: kPlayIntervalMs not declared and no equivalent in the index; placeholder 0, value must be verified (play clock interval, R7, P1.3)
    i32 ailmentIntervalMs = 0;          // FIXED: kAilmentIntervalMs not declared and no equivalent in the index; placeholder 0, value must be verified (ailment tick interval, R8, P1.2)
    i64 ailmentElapsedMs = 0;           // time since last ailment/item tick; merged AilmentClock and TickTimer (c45, R73)
                                        // CONFLICT: AilmentClock is f64 seconds in one line and i32 in another; ms kept.
    bool inAilment = false;             // re-entrancy guard for ailment tick (P1.2)
    i32 meleeWaitMs = 0;                // FIXED: kMeleeWaitMs not declared and no equivalent in the index; placeholder 0, value must be verified. display wait each side of a melee result (R31)
    i32 sanctuaryClockPlaceholder = 0;  // reserved; census has no field here

    // ---- Dungeon position, map, levels -------------------------------------
    i32 dungeonLevel = 0;               // 0 = town; 1..15 in dungeon; negative never occurs (R7, R52, c48)
                                        // merged: Game.DL, Game.DungeonLevel (5 lines). CONFLICT: range/default not stated.
    i32 area = 0;                       // current area; -1 marks a morgue body (c68, R5)
                                        // merged: Game.Area, Game.CurrentArea
    bool inCity = false;                // true in town; a cast from the character window is refused (c17, c58)
    bool inDungeon = false;             // dungeon or town context; changes swap and use refusals (R34, R35, R56)
    Location partyPosition = {};        // FIXED: Position not declared; uses Location (existing type). party (level, x, y) (position not given in census)
    i32 partyFacing = 0;                // party facing; encoding UNKNOWN (not given in census)
    bool lostLevel = false;             // party lost by level; cleared on arrival (R50, R53, R54)
    bool lostPosition = false;          // party lost by position; cleared on arrival (R50, R53, R54)
    bool forceSettle = false;           // area-change block runs even within one area (gv1D6A, R14, R89, R90)
                                        // merged: ForceSettle, ForcedFlag (teleporter square, Teleport, chest trap)
    bool cityGateCleared = false;       // event flag; keeps the stored shift (R6)
    bool cityRestPending = false;       // city rest waits until party walks out, per character (R1, R2)
    bool arrivalDeposit = false;        // arrival deposit and pool, mod-only (c36, c39)
    AltPoolOutcome altPMode = AltPoolOutcome::PoolOnHand; // FIXED: AltPMode not declared; uses AltPoolOutcome (city1.h). ALT-p target in town (c39, R7); merged ModPoolAndDeposit (R22)
    bool dark = false;                  // dark condition; lowers identify chance, raises recognise (Formulas Chances)
                                        // merged: Game.Dark, Game.Darkness (R72)
    bool inFog = false;                 // fog condition; lowers rename chance (Formulas Rename, R38, R72)
                                        // merged: Game.InFog, Game.M4EEE (fog flag)
    bool mapping = true;                // automap write switch; false while lost (R9, R15)
    Location lastRecorded = {};         // FIXED: Position not declared; uses Location. (level, x, y) of the last recorded square; guards repeat writes (R15)
    i32 deepestReached = 0;             // deepest level reached; shelves cap at 14 (R8, R18, R7, Terms)
                                        // merged: DeepestLevel, DeepestReached, deepestReached, MapDeepestVisited
    i32 deepestStocked = 1;             // deepest floor store and pens restock from; clamp 1..14 (R8, R28, c22)
                                        // UNKNOWN: default not stated; 1 used as the minimum.
    i32 levelCount = 0;                 // number of loaded dungeon levels (R5, R17, R19, R59, R84)
                                        // merged: FloorCount, DungeonCount, MapLevelCount. CONFLICT: wiki says 1..15, unverified.
    i32 fileLevelCount = 0;             // first word of MDATA11.MDR (R129)
    i32 levelLimit = kMaxDungeonLevel;  // highest allowed level: 15 full, 3 limited (R128, R55)
                                        // merged: MaxDungeonLevel, LevelLimit
    i32 dropLevelCap = 0;               // upper cap for item drop level (R19)
    i32 itemDropLevel = 0;              // item band top; clamped to dropLevelCap and >= 1 (R19)
    i32 locateRange = 0;                // range within which Locate answers; value UNKNOWN (R43, R57, c43)
                                        // merged: LocateRange, LocateLevelRange
    Vec<FloorState> floors = {};        // dungeon floors indexed by level (R6, R12, c4)
    Vec<Vec<bool>> sharedMapExplored = {}; // per level, explored flags indexed y*width+x; grows only on merge (R8, R9, R10)
    Vec<i32> tally = {};                // cleared rooms per level; length = level count, UNKNOWN in this part (R48, R50)
    Vec<Arr<i32, 2>> teleportTo = {};   // teleport destination (x, y); 0 elements = null (R7, R8, c4)
    Location sanctuary = {};            // FIXED: Position not declared; uses Location. set by Set Sanctuary; 1-based stored; 0 = never cast (R63, c21)
                                        // merged: Sanctuary, SanctuaryX/Y/Level, SanctuaryLoc, SanctuaryTarget, SanctuarySquare
    bool sanctuarySet = false;          // a Set Sanctuary location exists (R2)
    i32 seerDeepestLimit = kSeerLevelGateOriginal; // FIXED: kSeerDefaultDepth not declared; city2a.h value 12. Seer refuses if level above this (R64, c25, c50)
                                        // UNKNOWN: docs say 12, engine value unconfirmed (c25, c50). Merged two lines.
    Currency seerFee = 0;               // Seer fee; charged even when the search fails (c25, c50)

    // ---- Party roster, morgue, pack, misc ------------------------------------
    Vec<Character> roster = {};         // characters in creation order; deleted slot reused (c31, c34, R3)
                                        // merged: Game.Roster, Game.CharRecords (MDATA4 records from record 3)
    Vec<Str> rosterOrder = {};          // character save names in record order; kept in roster.json (R2, R3)
    Vec<i32> countedDeadSlots = {};     // characters already counted in Deaths; cleared on count (R3, R5)
    Vec<OffMapBody> bodiesOffTheMap = {}; // bodies kept past deepest level, incl after teleport (c44, R17)
    Vec<i32> pendingRaiseSlots = {};    // roster slots with a raise pending (R49)
                                        // CONFLICT-free port form: pointer Vec<Character*> replaced by slot indices.
    i32 pendingRaiseChance = 0;         // pending raise chance; zeroed by Prepare before offer (c17)
    i32 morgueListDead = 0;             // dead count copy used by Look; can be stale (R99)
    Vec<i32> morgueListHits = {};       // stale copy of other members' CurHits used by Look (R99)
    i32 morgueHireLevel = 1;            // rescuer hire level, 1 to 15 (Table 7)
                                        // CONFLICT 8: hire level default not given.
    i32 morgueRaiseChance = 10;         // counter-raise chance, percent, clamped 10..98 (c17, c19, CONFLICT 1, 2)
                                        // UNKNOWN: meaning of "lift to 1" not stated.
    Currency morgueRaiseCost = 0;       // morgue fee, round_half_even (R15)
                                        // CONFLICT 20: fee wording differs in the SRD.
    Vec<SavedParty> savedParties = {};  // remembered parties; written only at disband (R18, R19)
    Str partyPassword = "";             // join password; empty = none (R6)
    i32 viewedCharacter = 0;            // index of the on-screen character (R32)
    i32 itemSlot = 0;                   // item slot pointer; -1 = cleared by ClearsItemSlot refusal (R5, R6, [c1])
                                        // merged: M5766
    i32 secondItemSlot = 0;             // second item slot pointer; -1 = cleared (R5, R6, [c1])
                                        // merged: M56C4
    Vec<i32> combineSlots = {};         // slot numbers accumulated for Store Combine (R57)
    i32 carryIndex = -1;                // companion carry index; reset applies even if carried companion is dead; default UNKNOWN (c4)
    bool exchangeWindow = false;        // exchange window open; Chars(0) slots 0-39, Chars(1) 40-79 (R74-R78)
    Arr<Buffer, 10> buffers = {};       // ten buffers fired by keys 0..9 (0 = tenth) (R61)
                                        // merged: Game.Buffers (both lines). Naming a pack slot: cleared on withdraw (R31, R32)
    bool buffersFollowSwap = false;     // port value false; original UNKNOWN (c42)
    Vec<i32> queuedKeys = {};           // key codes pressed during a hold, replayed in order (R34)
                                        // UNKNOWN: Key type is not in this part; stored as key codes.
    i32 spellTypeIndex = 0;             // Spell Type box index; reset to first class on switch (R27)
    i32 creatureRestockPassCap = kReplenishPassCap; // FIXED: kCreatureRestockPassCap not declared; city1.h kReplenishPassCap (restock pass cap). (c9, c22)
    i32 daysAged = 0;                   // days aged by rest; drives store and pen replenish chance (c9, c22, R22)
    i32 awardLevelStep = 0;             // award-level step; recomputed after each level-up; initial UNKNOWN (R10)
    i32 chosenSubtypeCountCap = 0;      // UNKNOWN: placeholder; no census field name fits better

    // ---- Messages and captions ---------------------------------------------
    Vec<Str> messages = {};             // message window lines, max messagesKept, oldest dropped (R10, R20, R30, c37, T-10)
                                        // merged: Game.Log, Game.MessageLog, Game.Messages, Game.LogEntries
    Str lastCaption = "";               // last caption text (R10, R20)
    Str outcomeCaption = "";            // outcome caption; counts as words, capped at "A Lot" (c10)
    bool noPerceptionMessagesFlag = false; // UNKNOWN: duplicate of options.noPerceptionMessages (R62); kept false
    Vec<GuildLogEvent> guildLog = {};   // append-only guild event log (T29, R3, R6, R8, R22)
                                        // merged: Game.GuildLog (all Vec<Str> and LogEntry forms), GuildLogEvent
    i32 guildLogKeep = kGuildLogKeepOnClean; // FIXED: kGuildLogKeepDefault not declared; city2a.h kGuildLogKeepOnClean (50). entries kept on clean (R8, R32, R65)
                                        // merged: KeepOnClean, GuildLog.KeepOnClean. CONFLICT: R8 per-guild vs R65 whole-log scope.
    bool needsCleaning = false;         // guild log needs cleaning; exact comparison UNKNOWN (c49)
    bool guildLogFileOption = false;    // write discarded lines to GUILDLOG.TXT first; default UNKNOWN (R65)
    Arr<GuildMaster, kGuildCount> guildMasters = {}; // sitting master per guild, indexed by Guild order (R7, R21, R57, T2)
                                        // merged: Game.GuildMaster, Game.GuildMasters. Replaces GuildmasterId/GuildmasterLevel.
    Vec<LibRecord> library = {};        // library records; keys "mr:index" and "i:itemId" (R48, R58)
                                        // merged: Game.Library (both), Game.Library[key].*, Game.LibraryRecord, LibRecord.*
    Vec<i32> libraryGrade = {};         // knowledge grade per item kind; 4 full, 3 mostly, 0 otherwise (R7, R50, R71, c31)
                                        // UNKNOWN: N (item kind count) not stated; length left to the data load.
    Vec<Str> libraryEntries = {};       // library entries in WordSort order (R31)
    bool libraryReadOnly = false;       // file held open elsewhere; load read-only (R58)
    Str libraryFile = "library.json";   // port file name; legacy MDATA13 (R58, T-O)
    LegacyLibraryFields libraryLegacy = {}; // MDATA13 fields (c88, c96)
    Str penFile = "MDATA15";            // pen save file (R58, T-O)

    // ---- Hall of Records ----------------------------------------------------
    Arr<HallRecord, 12> hallRecords = {}; // FIXED: kHallSlots not declared; literal 12 per comment. twelve slots: 6 stats, skills, Wealthiest (Table 11, Table 12, R16, R20)
                                        // merged: Game.HallRecords, Game.Hall.Records, Game.HallOfRecords,
                                        // Game.WealthiestRecord, Game.HallWealthiest.
                                        // CONFLICT: R16/R18 say default = starting table; R20/R22 say empty. Empty used.
    Vec<HallLegacyEntry> hallEntries = {}; // list incl trailing "Nothing" entry; purpose UNKNOWN (c35, c88)
                                        // merged: Game.Hall.Entries, Game.Hall.MDATA9/MDATA12
    i64 hallMdata14Date = 0;            // Hall date as daysSince(1900-01-01)+2 (archive); engine epoch UNKNOWN (c88)
    // UNKNOWN: Game.Hall.MDATA13 layout not stated for this engine (c88); not stored.

    // ---- Store, shelves, pens ----------------------------------------------
    Vec<Arr<i32, 4>> store = {};        // shelf count per item, [Unaligned, Good, Neutral, Evil]; 0..9 per row (R7, R8, R12, R16, R28, R29)
                                        // merged: Game.Store, Game.Shelf, Game.Store[Item].Count. CONFLICT: store.json vs table row cap.
    Arr<i32, 3> storeRowCount = {};     // shelved creature rows per alignment (Good, Neutral, Evil); cap 9 per row (R16, T-P)
    i32 storeUnalignedStock = 0;        // unaligned bucket count; 99 at floor 0, else 5 (R16, Terms)
    Vec<TownShelf> shelfStock = {};     // one shelf per item table index (c35, c52)
    Arr<i32, 4> storeConfinementPens = {}; // store confinement count per alignment; cap test reads index 0 only (c63)
    Vec<Arr<i32, 3>> pen = {};          // penned count per monster ref per alignment (Good, Neutral, Evil); 0..9 (R21, R23, R27, R30, T-M)
                                        // CONFLICT: c9/c55 say pen can go to -1 on sell and exceed 9 on restock; not enforced here.
    Vec<Arr<i32, 3>> penStock = {};     // units per creature per aligned shelf; LevelFound 0 never run out (R9, R17, R19)
    i32 penGood = 0;                    // the Good pen scratch (l0288); can exceed 9 until a take-away (c9)
    i32 storeAlignChoice = 0;           // FIXED: StoreAlignment not declared; plain i32, 0 = Unaligned. alignment picked in Select Alignment; UNKNOWN default (c39, c33, R4)
    i32 storeRow = -1;                  // lit Store Buy row; cleared if sold out or hidden; -1 = none (R13, c68)
                                        // merged: Game.StoreRow, Game.StoreChoiceRow
    bool storeRowChangeWhileChoosing = false; // refuses row change while alignment is being chosen (c39, c40, R3)
    i32 storeBuyOrder = 0;              // FIXED: StoreSortKey not declared; plain i32, 0 = Original. Buy list order (c36, c39)

    // ---- Automap, file/path, version ---------------------------------------
    Str gameDir = "";                   // game directory path; precise definition UNKNOWN (R11)
    Str saveFolder = "";                // resolved save folder (R11, R65, R66)
                                        // merged: Game.saveFolder, Game.SavesDir
    Str saveOverride = "";              // value of env MORDOR_SAVES; "" when unset (R11)
    Str portTreeRoot = "";              // nearest folder above build containing Mordor.sln (R1)
    Str gameVersion = "1.1";            // FIXED: kGameVersion not declared; literal from comment (must equal "1.1" on import). version stamp (R12, F3)
                                        // merged: Game.Save.Version, Game.ModApi.GameVersion
    i32 modApiVersion = 0;              // FIXED: kModApiVersion not declared and value not in index; set 0, verify against source. ModApi version; raised when a hook changes (F3, R22)
    i32 installEdition = 0;             // FIXED: Edition not declared; plain i32, 0 = Full. edition unpacked (R11, c57)
    i32 release = 0;                    // FIXED: Release not declared; plain i32, 0 = Full. build from level count; default from detection (R129)
                                        // merged: Game.Limited, Game.LimitedBuild, Game.m0032, m009C, m00A8, m02A6, m0622
    bool limitationNoticeSeen = false;  // stored text contains capital T, incl "True" (c10)
                                        // merged: Game.LimitationNoticeSeen, Game.Welcomed
    bool limitationNoticeShown = false; // Limitation Notice flag: never set in full build (gv0180, R13, R15)
    i32 limitationNoticeLevelCount = kMaxDungeonLevel; // levels shown in Limitation Notice (gc0182, R15)
    Str splashPrefix = "";              // splash prefix "Limited "; empty in full build (m002E, R13)

    // ---- Mods ---------------------------------------------------------------
    Vec<ModManifest> mods = {};         // discovered mods in discovery order; duplicates dropped (R3, R5)
    Vec<Str> modRoots = {};             // mod root folders (R2)
    Vec<Str> liveModIds = {};           // ids of mods currently running (R15)
    Vec<Str> modSaveTag = {};           // id@version per running mod (R15)
    Vec<Str> modsWhenLoaded = {};       // mod tags in effect when the character was loaded (R1)
    Vec<LoadedMod> activeMods = {};     // mods running, in load order (R18, R19, R37)
                                        // merged: ModHost.Active, ModLive, LoadedMod.*
    Vec<Str> modHooks = {};             // live mods with a hook, in load order (R123)
    Vec<ModDisabledEntry> modDisabled = {}; // session disable flags (R37, R39, R40)
    Vec<ModEnabledEntry> modEnabled = {};   // enable flags from settings.ini (F4, R6)
    Vec<ModSettingEntry> modSettingsStored = {}; // stored toggle/choice/table values (F5, F6, R17)
                                        // merged: ModSettings.choices/tables/toggles
    Vec<RawSetting> modSettingsValues = {}; // in-memory key/value per section "Mod:"+modId (R9, R10)
    bool modSettingsWritable = false;   // mod settings file may be written to disk (R9)
    i32 modSwitchResult = 0;            // FIXED: ModSwitchResult not declared; plain i32, 0 = Success. result of a live switch request; default UNKNOWN (R11)
    Str townIncomeBankHook = "";        // hook name only; behaviour not described (UNKNOWN-11, c2)
    Currency townIncome = 0;            // sale gold received by the hook (R62); hook type UNKNOWN in source
    Vec<GameEvent> events = {};         // events loaded from JSON; port addition (R24)

    // ---- Random source ------------------------------------------------------
    u64 rngState = 0;                   // random generator state for all Rnd draws; layout UNKNOWN (R7-R96, c58)
                                        // merged: all Game.Rng / Game.Rnd lines. Draws go through IRng only.

    // ---- Quests and misc rules --------------------------------------------
    i32 questTarget = -1;               // table index of quest monster; -1 = no valid pick yet (R53)
    i32 questTier = 0;                  // quest-run monster tier pointer; default UNKNOWN (R53, c19)
    i64 chestExpAward = 0;              // chest experience award; formula UNKNOWN (UNKNOWN-3, c37, c46, CONFLICT-25)
    i32 schoolSampleAttempts = 0;       // school sampling attempts, capped at 1001; post-cap UNKNOWN (U3)
    i32 knownThreshold = 0;             // known-threshold; never set, 0 (R52)
    i32 identifyFlagP036E = 0;          // source flag used in identify difficulty; meaning not stated (R52)
    i32 deadScreenVisibility = 0;       // value used in dead-screen visibility test; UNKNOWN (c68)

    // ---- Options ------------------------------------------------------------
    Options options = {};               // option flags and display settings (R1-R5, R39, R42, R62, R98)
};

}  // namespace mordor

#endif  // MORDOR_STATE_GAME_H
