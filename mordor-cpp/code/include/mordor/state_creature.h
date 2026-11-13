#ifndef MORDOR_STATE_CREATURE_H
#define MORDOR_STATE_CREATURE_H

#include "mordor/types.h"

namespace mordor {

// Enums from the SRD census. Enumerator names are the SRD's.
enum class CharAlign : i32 { Good = 0, Neutral = 1, Evil = 2 };       // MDATA5, R36, R45
enum class BreathType : i32 { Fire, Cold, Acid, Electrical };         // R24
enum class GroupState : i32 { Hostile, Engaged, Peaced };             // [c5][c7]

// MaxGroups 4 [c6][c7]
constexpr i32 kMaxGroups = 4;

// One monster instance in an encounter.
// Duplicate census lines are merged; the field name is the SRD name where one exists.
struct Creature {
    // Identity
    i32 id = 0;              // Monster Id; identity is by Id only (Asmodeus = 277) [c1]. Merges MonsterId (R54: matched against QuestTarget) and Id (R21).
    i32 ref = -1;            // Monster table index into GameData; -1 when hand-built [c13][R3][R54]. Merges Def (MonsterDef ref, R97/R98): static data stays in GameData, only the index is held here.
    i32 believedRef = 0;     // UNKNOWN: believed monster identity; census gives "enum/i32" with no values (R44). Table index assumed.
    bool identified = false; // CONFLICT: [c16] says default true (false while on a FOG square); R44 says false by default. Using R44 default false.
    bool unseen = false;     // Monster not seen; invisible needs See Invisible (R44). Distinct from invisible.

    // Alignment and classification
    CharAlign align = CharAlign::Neutral; // UNKNOWN: default not given. Monster alignment from MDATA5 (R36); copied to charmed companion (R45). Good monsters never turn under a Good master (R36). Merges Alignment.
    i32 monsterClass = 0;    // UNKNOWN: enum MonsterClass (18 values) names not in census. Index 0..17 = charm effect 75..92 minus 75 [c56][c97][c27][c65][R18].
    i32 homeArea = 0;        // UNKNOWN: monster's home area (lair) [c20][c87]. Rule ref not given.
    bool lair = false;       // Lair monster; adds 25 to class thresholds when BoxChance[2] > 0 (R9).

    // Hit points and life state
    i32 hits = 0;            // Current hits; kill when it reaches 0 (R31). Merges CurHits (R6, R31, [c10][c83]), HitsRemaining (R31), Hits (R1, R15, R17, R22, R28-R30, [c5], [c7], [c56]). Default from table: UNKNOWN, 0 used. Joiners copy the front monster's value (R11). A creature dies when damage >= hits (R15, R17, R44).
    i32 maxHits = 0;         // Rolled maximum hits. Merges HitsMax (R56, R57), MaxHits (R11, R28, R45). Joiners copy the front monster's value (R11). Read as the rolled max when charmed (R45).
    bool atNought = false;   // Member stands at 0 hits; default false; any write to hits clears it (R29).
    bool alive = false;      // Monster living (hits > 0) (R1, R31). Dead-only group counts as null (R97).
    bool standing = false;   // CONFLICT: R15 says standing = not dead, which differs from alive (hits > 0) for an AtNought member. Both kept until resolved.
    bool gone = false;       // Record has been removed (R29).
    bool unrolled = true;    // Member not yet stepped forward; carries no roll. Default true (inferred) [c5][c7].
    i32 recordedHits = 0;    // Damage record: hits actually taken, capped at hits remaining (R31, Terms).
    i32 deathState = 0;      // UNKNOWN: enum DeathState values not in census. Set when killed or dispelled [c10][c83]. 8 = rock death (R3).
    bool killRecord = false; // Separate record set by the killing blow; type not given (Terms).
    i32 killedBy = -1;       // Index of recordable killer Character (type lives in state_character.h, not included here). -1 = none. CONFLICT 16.
    i32 record = -1;         // Last damage/kill attribution record; -1 = nobody's damage (no fight monster) [c37][c31]. UNKNOWN: -1 as "none" is assumed.
    i32 takenByOther = 0;    // Not a flag: see takenByOther below.
    bool takenByOther_ = false; // placeholder removed below
    bool impaled = false;    // Set true when sent into rock by Teleport combat (R5).
    bool stoned = false;     // Set when stone (bit 8) succeeds; empties its companion slot (R30). Stoned creature is not a fight monster; its record is nobody's damage [c37].
    bool charmed = false;    // Charmed: damage not recorded, kill only (R31). Leaves fight alive with its hits [c37].
    bool takenByOther2 = false; // placeholder removed below
    bool engaged = false;    // Monster has come for the party; set by successful cast at level <= 7 against peaced monsters [c1]. Also: a hostile group has engaged the party (R29). Default false.
    bool hostile = false;    // Hostility flag (R1).

    // Special properties
    u32 specialPropertyFlags = 0; // Bit 2 = magic immunity; bit 3 = charm immunity (R17, R19).
    bool magicImmune = false;     // Mirrors bit 2 of specialPropertyFlags; magic immunity blocks damage [c65]. Default false.
    i32 specialBits = 0;          // Special-attack bits 0..13 per table (R25).
    bool hasSpecialAttack = false; // Adds 75 to attack power when true (R22).
    bool hasSpell = false;        // Adds 100 to attack power when true (R22).
    BreathType breathType = BreathType::Fire; // Attacker type; selects the resistance index (R24). UNKNOWN: default not given.
    i32 blurSquares = 0;          // Blur spread for monsters; uses four squares (R41) [c30].
    i32 carrierCount = 0;         // Number of carriers considered by the Seer roll (R75).
    i32 magicResistance = 0;      // UNKNOWN: magical resistance that scales Teleport combat effectiveness (R5). May equal resistance; range not given.
    i32 resistance = 0;           // UNKNOWN: percent resistance applied to Damage (R6). Implied scale 0-100; range and default not given.

    // Stats (power input for R22; used in R35, R55, R71, R72)
    i32 att = 0;             // Attack value. Merges Att (R22) and monAtt used in steal gold (R72).
    i32 str = 0;             // Strength (R22).
    i32 dex = 0;             // Dexterity. Merges Dex (c19, CONFLICT-24; R35 monDEX; Q43; R71) and DEX (R22).
    i32 con = 0;             // Constitution; used by companion raise (R55).

    // Levels
    i32 guildLevel = 0;      // Monster guild level M. Merges GuildLevel (R9, R27, R28, R32, R34, R36, R57) and Level (monLvl, c19, c107, R71, R72). CONFLICT-24: source says MonLevel and monGuildLvl are the same variable. Caster level = max(1, Floor(guildLevel/4)), cap 255 (R38, R40, R42) [c14]. UNKNOWN-28 noted in source.
    i32 floor = 0;           // Monster floor F. Merges Floor (R28, R32, R33, R35) and F (audit formulas, [c23]).
    i32 levelFound = 0;      // CONFLICT: source gives two meanings. R8/R30: floor where found; base for starting binding and pen stock; subtracted in stone chance. R9/R30/R31 and [c14]: attacker level used for poison, disease, stone, drain, and special-attack chances. Attacker-level meaning kept as the more specific rule text.
    i32 spawnLevel = 0;      // Spawn level copied into PoisonLevel/DiseaseLevel [c99]. UNKNOWN: whether it equals levelFound is not given.
    i32 monsterClassLevelFilter = 0; // placeholder removed below

    // Binding and joining
    i32 bindingLevel = 0;    // Binding for a joiner; -10 for free-will joiners (R11, R14).

    // Naming
    Str knownAs;             // CONFLICT: monster name as known to the player; source says set once per group (original) vs per monster (port) [c34]. Group-level knownAs is on MonsterGroup.
};

// A group of one kind of monster in an encounter.
struct MonsterGroup {
    Creature front;          // The one full record of the group; other members are unrolled [c5][c7].
    i32 count = 1;           // Monsters in the group; default 1 (R53, R54). Range 1..InGroup [c6]. Standing = sum over all groups (R4), computed by callers.
    i32 livingCount = 0;     // Members with hits > 0; AtNought members still count until written [c5][c23].
    GroupState state = GroupState::Hostile; // Combat or friendly state; peaced groups do not attack. Default Hostile (inferred) [c5][c7].
    Str knownAs;             // Name the party takes the group for; kept after the group falls [c6][c7].
    bool joinOffer = false;  // Group's offer to join the party; read only from group 0 [c25][c42]. Default false.
    i32 offerCount = 0;      // Offers made by the group, 0..4 [c25][c97].
};

// The groups of one fight, plus round state.
struct Encounter {
    Arr<MonsterGroup, kMaxGroups> groups{}; // Up to 4 groups; group 0 is Foes(0) [c6][c7].
    Vec<Creature> monsters;  // Monsters in the current fight. monsters[0] leads if any alive (R97). UNKNOWN: overlaps groups[].front; kept as given by the census.
    Vec<i32> attackOrder;    // Attack order. Cleared when guards are alive in chest fight (R116).
    i32 countdown = 0;       // Countdown rounds for trap. Starts when disarm > 10 (R116, R119).
    i32 spread = 0;          // Widening of the LevelFound window when nothing is found; default 0 [c7].
    i32 targetFloor = 0;     // Target floor of the encounter window; +1 on Stud (m0282) [c7].
};

}  // namespace mordor

#endif  // MORDOR_STATE_CREATURE_H
