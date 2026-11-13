// Game layer: the one public API a front end (the console demo now, Unreal later) needs.
// It owns a Session (data, RNG, event log, GameState) and drives the rules modules. No I/O, no platform calls.
// Everything a front end shows comes back as Str results or as events in session.log.
#pragma once
#include <map>
#include "mordor/city1.h"
#include "mordor/ctx.h"
#include "mordor/events.h"
#include "mordor/game_data.h"
#include "mordor/rng.h"
#include "mordor/state_character.h"
#include "mordor/state_game.h"

namespace mordor {

struct Session {
  GameData data;       // static tables (filled by sessionInit)
  Rng rng{1};          // the only randomness source
  EventLog log;        // everything the rules say; front ends read and clear it
  GameState game;      // all mutable state (party, floors, options)
  TownStock town;      // General Store shelf and Confinement pens (SRD ch.16)
  Str lastError;
  std::map<Str, Str> featureState;   // free-form state for features (keys are prefixed with the feature id); not saved yet
  Ctx ctx() { return Ctx(data, rng, log); }
};

enum class Screen : i32 { NewGame = 0, Town = 1, Dungeon = 2, Combat = 3, GameOver = 4 };

struct NewCharacter {
  Str name;
  Race race = Race::Human;
  Align alignment = Align::Neutral;
  Guild guild = Guild::Nomad;
  Arr<i32, 6> stats = {};   // final natural STR,INT,WIS,CON,CHA,DEX; all zero = use the default base for the race
};

// ---- setup --------------------------------------------------------------------------------------------
// Loads the tables from the text of combat-data.json, seeds the RNG, resets the game state (SRD ch.2, ch.4, ch.22 new game).
bool sessionInit(Session& s, const Str& dataJson, u64 seed, Str& err);
// Default starting stats for a race (SRD ch.2 R2: min(max, min+5)) and the creation points left for a given allocation (R3).
Arr<i32, 6> defaultStats(const Session& s, Race race);
i32 creationPointsLeft(const Session& s, Race race, const Arr<i32, 6>& stats);
// Creates a character per SRD ch.2 R5 (+ guild join rules of ch.4) and appends it to the party (max 4, ch.17). False + err on a rule violation.
bool createCharacter(Session& s, const NewCharacter& nc, Str& err);

// ---- what to show -------------------------------------------------------------------------------------
Screen currentScreen(const Session& s);
Str describeParty(const Session& s);                   // one line per member: name, race, guild/level, HP, SP, status, gold
Str describeCharacter(const Session& s, i32 member);   // full sheet: stats, A/D, resistances, pack, companions
Str describeSquare(const Session& s);                  // dungeon: level, x,y, facing, exits, what is here; town: the menu
Str renderAutomap(const Session& s, i32 radius);       // ASCII map around the party using the automap flags the party has seen
Str describeFight(const Session& s);                   // combat: groups, monster names and counts, who is aimed at

// ---- actions ------------------------------------------------------------------------------------------
// Each action returns false if the command is unknown or not allowed now; `reply` is the text for the player.
// Party members are numbered 1..4 in commands and in describeParty; describeCharacter takes a 0-based index.
// Town: rest | guild | levelup <member> | join <member> <guild> | shop | buy <n> <member> | sell <slot> <member> | bank deposit <gold> | bank withdraw <gold>
//       | equip <member> <slot> | unequip <member> <slot> | identify <member> <slot> | morgue | raise <name> | pool | descend
bool townAction(Session& s, const Str& command, Str& reply);
// Dungeon: forward | back | left | right (turn) | stairs | open | search | pool | cast <member> <spell> | use <member> <slot> | map
bool dungeonAction(Session& s, const Str& command, Str& reply);
// Combat: fight [group] | defend | cast <member> <spell> [group] | use <member> <slot> | open
// Sets the standing orders and advances the fight by one 1000 ms combat round (SRD ch.6/7).
bool combatAction(Session& s, const Str& command, Str& reply);
// Advances the clocks by milliseconds (combat tick 1000 ms, play clock 250 ms, ailment tick 4000 ms; ch.6). Normally called by the actions.
void advanceTime(Session& s, i64 ms);

// ---- persistence --------------------------------------------------------------------------------------
Str saveSession(const Session& s);                      // text form of the mutable state (party, floors, town, clocks)
bool loadSession(Session& s, const Str& text, Str& err);

}  // namespace mordor
