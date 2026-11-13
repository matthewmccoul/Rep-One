// state_party.h: Party state (SRD Party block). Plain data only.
#ifndef MORDOR_STATE_PARTY_H
#define MORDOR_STATE_PARTY_H

#include "mordor/types.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"


namespace mordor {

// CONFLICT: companion slot count. Party.CompanionSlot uses 0..4 (c14, c27), R68 names
// MaxCompanionSlots (not in chunk), and R36/R41/R43 say 0..3. Kept 4 per R36/R41/R43.
// CONFLICT: party size minimum. R1 says 1..4; R4/R5/R6 say 0..4. Kept 0..4 (0 = no loaded party).

// Terms, R33: leader's stored facing; port index 0-3.
// CONFLICT: Facing values. Some lines say the values are not given (R51, R1/R2, R55); the
// specific line (R33, Terms) gives {N, E, S, W}. Kept {N, E, S, W}.
enum class Facing { N, E, S, W };

// Terms: one body or one companion carried at a time.
enum class CarryingSlot { None, DeadCharacter, Companion };

// R11, R13, R15: standing party order.
// UNKNOWN: name of the non-Fight order value is not in the chunk (spell order set by BecomesOrder).
enum class OrderKind { Fight, SpellOrder };

struct ItemOrder { i32 slot = -1; i32 item = -1; bool cast = false; };  // per-member item order

struct Party {
    // ---- Members and size ----
    Arr<Character, kPartyMaxSize> members{};  // R5, R6, R18, R22: loaded members; members[0] is leader
    i32 size = 0;                             // R4-R6, R44, R58: member count 0..4 (merges Count, Size, PartySize, Bodies, BodyCount)
    i32 aliveCompanions = 0;                  // R34, Terms: living companions; 0 means the character is always hit
    bool anyAble = false;                     // [c34]: true if any member is able
    Vec<Vec<Character>> groups{};             // R15, R22: alive groups, front first. UNKNOWN: derived, may be recomputed

    // ---- Carrying ----
    CarryingSlot carryingSlot = CarryingSlot::None;  // Terms: one body or one companion at a time
    i32 carryingCompanion = -1;               // R41: companion slot carried; -1 = none, range -1..3

    // ---- Companions ----

    // ---- Gold (all Currency: i64 in 1/10000 gold) ----
    Currency fightGold = 0;                   // R4, R17, R46: gold found before split; paid at victory or on chest open
    i32 goldLeaderIndex = 0;                  // R22: index of first living city member in party order

    // ---- Position and level ----
    i32 dungeonLevel = 0;                     // R45, R124, R128: 0 = town, 1..15 in dungeon; 0 or below = fall from sky (merges Floor, Level, LostLevel value)
                                              // UNKNOWN: floor range for Teleport (R2) not stated.
    i32 x = 0;                                // R45, R53, R55: square column. CONFLICT: API counts from 0, internal index is 1-based (R45, c39, c47)
    i32 y = 0;                                // R45, R53, R55: square row. CONFLICT: same as x. Place uses Y+1 (R53)
    Facing facing = Facing::N;                // R33, R51, Terms: stored facing. UNKNOWN: default not stated
    bool inDungeon = false;                   // R124: the climb continues only while true
    bool onStairs = false;                    // R124: the climb continues only while true
    bool loaded = false;                      // R125: unloaded on exit to Main Menu

    // ---- Lost state ----
    bool lost = false;                        // R64, c31: cleared by Depth Perception and Find Direction
    bool lostFacing = false;                  // Terms: party does not know its facing ("?" in Look)
    bool lostLevel = false;                   // Terms: party does not know its floor
    bool lostPosition = false;                // R25, Terms: party does not know its square

    // ---- Engagement, combat and rescue ----
    bool engaged = false;                     // R2, R5, R16, R25: monsters engaged; one free round on move or level change
    bool wiped = false;                       // R5: party wiped in combat
    bool awaitingRescue = false;              // R5: set when wiped and waiting; cleared when wait is entered
    bool helpCryNeeded = false;               // [c45]: port behaviour; set when size > 1 and HP is low
    OrderKind order = OrderKind::Fight;       // R11, R13, R15. UNKNOWN: default not stated; becomes Fight when caster lacks SP
    i32 orderTarget = -1;                     // R15: target kept by the standing spell order. UNKNOWN: default not stated
    i32 killer = -1;                          // R54: index of killing member; only the killer counts
    i32 opener = 0;                           // R5, [c4]: index into members of the character who triggered trap 7
    i32 selectedIndex = 0;                    // R26, R27: commanded member index, 0..size-1
    Arr<CombatOrder, kPartyMaxSize> combatOrders{};  // [c34]: per-character order; combatOrders(c).M547A is target group index
    Arr<ItemOrder, kPartyMaxSize> itemOrders{};      // R10, R19: per-member item order (slot, item, cast flag). N assumed = party max
    Vec<Creature> foes{};                     // R23: encounter foes; the fight record, emptied on chest open
    Vec<Creature> leadGroup{};                // Terms: group on the square the party faces or fights. UNKNOWN: relation to foes not stated

    // ---- Spells ----
    bool arrivedBySpell = false;              // R55: set when caster arrives via ArriveBySpell after a move spell
    bool inParty = false;                     // R1, [c1]: read only. UNKNOWN: default n/a in SRD; false used

    // ---- Inventory and storage ----
    Vec<Character> joinList{};                // R62: members delivered by a chute for a later join
    Vec<Character> morgueList{};              // quirk [c12]: copies of hits and death for Look; may be stale

    // UNKNOWN: Party.Pool (R19) has no stated type and is not declared.

    // Tiny helpers.
    inline bool isSolo() const { return size == 1; }                    // R47: size 1 gates rescue and pooling
    inline const Character& leader() const { return members[0]; }       // R5, R22: index 0 is the leader
  Arr<Inventory, kPartyMaxSize> inventories{};  // ADDED: per-member carried items; index matches members (Inventory and storage section)
};

}  // namespace mordor

#endif  // MORDOR_STATE_PARTY_H
