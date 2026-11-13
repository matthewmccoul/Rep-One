#pragma once
// state_dungeon.h: mutable per-level and per-square state for the dungeon.
// UNKNOWN: header names for Creature, Character and Item are assumed (creature.h, character.h, item.h).
// Static map data (walls, doors, features, rock terrain, chutes, teleporters, lock/box/item
// generation thresholds, level width/height, area count, deepest level) lives in GameData FloorDef
// and is omitted here. Square index = y*width+x (0-based); SRD R19 uses 1-based X+30*(Y-1).

#include "mordor/types.h"
#include "mordor/state_common.h"
#include "mordor/state_creature.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"

namespace mordor {

// Currency: i64 gold in units of 1/10000 gold. Alias is harmless if types.h already defines it.
using Currency = i64;

// Encounter stance (SRD names as given in the census).
// UNKNOWN: default stance is not stated; Peaced chosen as the safe default.
enum class EncounterKind : i32 {
    Hostile,  // R17 R19
    Peaced    // R17 R19
};

// Chest/box/container class and container type.
// CONFLICT: Square.chestClass uses i32 codes (0 none, 1 never generated, 2 box, 3 chest) and
// Square.ChestClass uses {CONTAINER, BOX, CHEST}; Square.containerKind uses
// {NONE, CHEST, BOX, CONTAINER, ASMODEUS_CHEST}. Merged here with the numeric codes kept.
// Lock is rolled only when class > 1 (c12), so CONTAINER = 1 is unlocked.
// CONFLICT: R1 says code 1 is never generated, but CONTAINER is listed as a class. Kept CONTAINER = 1.
// UNKNOWN: ASMODEUS_CHEST numeric value is not stated; 4 assumed.
enum class ChestClass : i32 {
    NONE = 0,             // default (containerKind default NONE)
    CONTAINER = 1,        // c12 c105
    BOX = 2,              // R5
    CHEST = 3,            // R5
    ASMODEUS_CHEST = 4    // c105
};

// Chest lock state.
// CONFLICT: Square.chestLock (0 unlocked, 1 magically locked, -1 charm-unlocked; CONFLICT-17) and
// Square.lockState (-1 charm-unlocked) and Square.ChestLocked (bool) merged into one enum.
// UNKNOWN: enum value names are descriptive; the SRD names are not given in the census.
enum class ChestLock : i32 {
    UNLOCKED = 0,         // R3 R10 R116
    MAGIC_LOCKED = 1,     // R3 R10
    CHARM_OPENED = -1     // CONFLICT-17: unlocked by Charm of Opening
};

// Chest trap kind: Not Trapped plus ten trap kinds (trap id 0..10).
// UNKNOWN: trap kind names are not in the census; only ids 1, 2, 6 and 8 are defined in R38-R41.
// UNKNOWN: selection method (R1, R2).

// Naming level for a monster group.
// UNKNOWN: enum value names are descriptive; the SRD names are not given in the census.
enum class IdentifyLevel : i32 {
    OWN_NAME = 0,    // R19 0 own name
    CLASS_NAME = 1,  // R19 1 class name
    GROUP_NAME = 2   // R19 2 group name
};

// One monster group in a square. Four slots per square (MaxGroups 4), front-first.
struct FoeGroup {
    i32 monsterIndex = 0;          // R20 real monster table index of group
    i32 knownAsIndex = 0;          // R20 table index the party sees; may differ from monsterIndex
    i32 count = 0;                 // R17 R42 R48 members in group; 0 after clear.
                                   // asmodeus removal drops count by one, not below one (c29 c32 c46)
    Vec<Creature> members{};       // R12 R15 c10 c65 creatures, front monster at index 0
    bool engaged = false;          // R18 R24 1 hostile, 0 peaceful. R18 default 1 applies to group 0 only
                                   // UNKNOWN: default for groups 1..3; false chosen
    bool fogged = false;           // R19 group carries bit 20 (fog); identify level 1 when true
    IdentifyLevel identifyLevel = IdentifyLevel::OWN_NAME;  // R19
    i32 joinOffer = 0;             // R25 R42 R48 >0 = group offers to join; cleared to 0 on clear and
                                   // at top of group turn.
                                   // CONFLICT: Square.JoinOffer (bool, read for group 0 only, c25 c31).
                                   // Kept the per-group i32 (more specific).
    i32 ref = 0;                   // R48 room reference marker 0..3; -1 = room emptied.
                                   // UNKNOWN: default (census says UNKNOWN); 0 chosen
    i32 stamp = 0;                 // R48 R49 seconds since midnight when cleared; 0 = never set
};

// Per-area (room) record. Floor holds 201 of these.
struct Area {
    i64 stamp = 0;                 // R9 R66 seconds since midnight when room was cleared, 0..86399;
                                   // 0 = never cleared.
                                   // CONFLICT: Square.ClearedAt (time, R46) duplicates this; dropped.
    i32 emptiedAt = 0;             // c9 c24 seconds since midnight when room was emptied; 0 = never set
                                   // (refills on next visit). A clear does not restamp an empty room.
    i32 respawnT = 0;              // R9 R66 seconds before an emptied room refills; minimum 120.
                                   // UNKNOWN: default not stated; 0 chosen
    i32 lairId = 0;                // c7 c13 monster Id fixed to this area; 0 = none (inferred).
                                   // c20 says the lair identity is matched by the Seer; same field kept.
    i32 spawnMask = 0;             // c13 c20 bit n = MonsterKind n (bits 0..17); 0 = default
    i32 fight = 0;                 // c35 Fight record for this area. UNKNOWN: content not stated
                                   // CONFLICT: Square.FightRecord (bool, c9) is per square; kept this per-area one.
    Arr<Creature, 4> foes{};       // c35 four Foe slots per area. UNKNOWN: exact meaning vs FoeGroup
    Vec<Character> occupants{};    // c17 c30 roster counted for dead bodies and other living characters
    Vec<Creature> standingGroup{}; // R48 kept while stamp is recent
    Vec<Creature> waiting{};       // R97 waiting[0].Def leads when there is no combat
};

// Per-square mutable state. One per map square, indexed y*width+x.
struct SquareState {
    // Automap and seen flags
    bool explored = false;         // R10 R15 Automap explored bit; set only while Mapping.
                                   // CONFLICT: Square.Explored (c13) says set whether or not fogged; kept R10/R15.
    bool identified = false;       // R62 square identified by perception; written to automap once
    bool rockRecorded = false;     // R54 rock recorded on the automap; false by default.
                                   // Merges Floor.AutomapRock (R17, rock squares written by teleport).
    bool marked = false;           // R56 set by Ethereal Portal into rock
    i32 automap = 0;               // R19 c63 per-square automap bits: bits 0-22 features, 23-26 facing arrows.
                                   // CONFLICT: Floor.Automap as AutomapCell[900] (R48, owner not stated) and
                                   // Square.BeenHere (automap bit 23, c35). Kept the i32 bitfield;
                                   // BeenHere dropped because bit 23 is a facing arrow here.
    u32 bits = 0;                  // R56 Detect Rock sets the rock-ahead bit; bit 23 = not rock when party lost.
    i32 areaId = 0;                // R8 R49 positive area id; 0 or less = no area, no spawn. Index into Floor.areas.

    // Teleporter / chute presence (static record lives in FloorDef)
    bool teleporterPresent = false;// R13 teleporter or chute on this square; prunes only if party size dropped
    bool teleportRerun = false;    // c44 c63 teleporter re-run flag. UNKNOWN: RerunSquare has no loop guard.

    // Chest
    bool hasChest = false;         // R47.2 R52 chest present; gold waits in it while true.
                                   // CONFLICT: Square.Chest (Item optional, R60) is covered by this flag.
    ChestClass chestClass = ChestClass::NONE;  // R1 R5 c5 c12 treasure class / container kind
    ChestLock chestLock = ChestLock::UNLOCKED; // R3 R10 Q4 c12 c19 c24 lock state (see CONFLICT-17 above)
    ChestTrap chestTrap = ChestTrap::NotTrapped;  // R1 R2 R38-R41 chest trap; selection method UNKNOWN
    bool chestTeleporterTrap = false;  // R13 R118 chest trap teleports the opener; prunes after removal.
                                       // Merges Square.ChestTeleporter (R118) and ChestTeleporterTrap (R13).
    i32 trapFlags = 0;             // c18 c19 set trap bits 0-11 (engine order); bit 11 unnamed; 0 = no trap.
                                   // CONFLICT-29: bitfield here vs single index in chestTrap; both kept.
                                   // UNKNOWN-2: relation between the two
    i32 chestDisarm = 0;           // R119 disarm value, clamped 5..98. Merges Square.DisarmChance.
                                   // UNKNOWN: default not stated; 0 chosen
    i32 chestCount = 2;            // c65 opening countdown; starts at 2, decrements each turn, opens on
                                   // the third turn (reaches -1). CONFLICT-22: 2,1,0,-1 is four values.
    bool slotCursed = false;       // c63 chest-slot cursed flag; lossy copy of Item.Cursed. CONFLICT-32.
    i32 chestMonsterGL = 0;        // c18 c27 guild level of the monster whose chest is opened
    Currency chestGold = 0;        // R5 R11 R47.2 gold waiting in chest; default 0
    Currency fightGold = 0;        // R46 fight gold; emptied on split and written back
    Vec<Item> chestItems{};        // R7 R18 c63 items the chest holds, rolled at creation.
                                   // Merges Square.Chest (R48) and Square.Items (c63). Capacity UNKNOWN.
    Vec<Character> bodies{};       // R47 R50 c63 dead bodies left on the square (pointer copies dropped)

    // Encounter and monster groups
    EncounterKind encounterKind = EncounterKind::Peaced;  // R17 R19 stance drives the engagement roll
    bool encounterWaiting = false; // R17 hostile encounter waiting to attack on the transition tick
    bool hasEncounter = false;     // R48 fight has an Encounter source
    bool joinOfferPending = false; // R18 pending join offer; peaced encounter stays put.
                                   // UNKNOWN: definition not stated
    Arr<FoeGroup, 4> foes{};       // R27 c10 c47 c65 four monster group slots (MaxGroups 4).
                                   // Merges Square.Foes, Square.Groups and Square.GroupCount.
                                   // Square.Creatures and Square.Monsters (pointers) are derived and omitted.
};

// Mutable state of one dungeon level.
struct FloorState {
    i32 dungeonLevel = 0;          // R5 R50 R53 R83 R112 dungeon level number; 0 = town/surface.
                                   // Merges Floor.Level, Floor.Number, Floor.DungeonLevel and Floor.dungeonLevel
                                   // (poison/disease level, identification band).
                                   // CONFLICT: range 1..15 (R53) vs 0 = town (R83). Level 0 paralysis
                                   // stored-shift meaning UNKNOWN (R6).
    Vec<SquareState> squares{};    // one per map square, index y*width+x (0-based). Dimensions come from FloorDef.
    Arr<Area, 201> areas{};        // c13 c35 area records per level (201)
    i32 tally = 0;                 // c9 c24 c66 cleared rooms on this level, 0..areaCount.
                                   // CONFLICT: Floor.RespawnTally (MDATA10, c35) may be the same count; merged here.
    i32 roomTally = 0;             // c66 "tally" in respawn formula. UNKNOWN: definition
    i32 roomTotal = 0;             // c66 "total" in respawn formula. UNKNOWN: definition
    i64 roomStamp = -1;            // R48 time of the room's last standing group or empty state; -1 = unset (UNKNOWN sentinel)
};

}  // namespace mordor
