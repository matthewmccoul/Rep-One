// Storeroom puzzle (dig, haul, smelt): data model, state machine, save/load.
// Standard library only (no Unreal includes), no exceptions, no RTTI. Takes text, never file paths.
// Behaviour equals work/T001/puzzle/puzzle_state_machine.py (the reference).
#pragma once
#include <string>
#include <utility>
#include <vector>

#include "dd/json.h"
#include "dd/map.h"

namespace dd {
namespace puzzle {

struct Issue {
    std::string code;
    std::string detail;
};

// Static puzzle content (from storeroom_puzzle.json). Data-driven: control count and solution length
// are not fixed (the demo has three).
struct Data {
    std::string puzzleId, clueSite, clueRoom, controlsSite, controlsRoom, opensEdge, opensRoom, xpClass;
    std::vector<std::string> controls;
    std::vector<std::string> solution;
    std::vector<std::string> otherRoutes;                         // bypass routes: magic_key, pick_lock, break
    std::vector<std::pair<std::string, std::string>> strings;     // str.* key -> text

    bool hasControl(const std::string& id) const;
    bool hasRoute(const std::string& route) const;
    const std::string* text(const std::string& key) const;        // nullptr when absent
};

// Parses and checks the puzzle JSON. Codes: JSON_*, SCHEMA, NO_CONTROLS, DUP_CONTROL, BAD_SOLUTION,
// MISSING_STRING (a string key the machine can return is absent).
bool dataFromText(const std::string& text, Data& out, Issue& err);

// Cross-check against the map; all problems, primary first. Codes: UNKNOWN_NODE, PUZZLE_EDGE,
// ROUTES_MISMATCH, PUZZLE_REF (map.puzzle disagrees), CONTROLS_MISMATCH (map controls site disagrees).
std::vector<Issue> validateAgainstMap(const Data& d, const map::FloorMap& m);

enum class State { Idle, Partial, Solved, Bypassed };

enum class UseResult { StepOk, WrongReset, Solved, Inert, UnknownControl };

struct UseOutcome {
    UseResult result = UseResult::UnknownControl;
    std::string resultKey;   // step_ok, wrong_reset, solved, inert, "" for UnknownControl
    std::string stringKey;   // str.use.step_<n>, str.use.wrong, str.use.solved, str.use.inert, "" if unknown
};

enum class BypassResult { Applied, NotApplied, UnknownRoute };

class Puzzle {
public:
    Puzzle() = default;
    explicit Puzzle(const Data& d) : data_(d) {}
    // Create from the puzzle JSON text. The puzzle starts idle (door closed).
    static bool fromText(const std::string& text, Puzzle& out, Issue& err);

    // The only player action: Use on a control (approved by the user for the three controls only).
    UseOutcome use(const std::string& control);
    // Door e08 opened by magic_key, pick_lock or break. NotApplied when the door is already open.
    BypassResult openOther(const std::string& route);
    // "puzzle_solve" once after a puzzle solve, never after a bypass; "" otherwise. PROPOSAL (XP TBD).
    std::string claimXp();
    // Clears partial progress only; an open door never re-locks.
    void reset();

    State state() const;
    const std::vector<std::string>& progress() const { return progress_; }
    bool solved() const { return solved_; }
    bool doorOpen() const { return doorOpen_; }
    const std::string& openedVia() const { return openedVia_; }   // "" = not opened
    long long wrongCount() const { return wrongCount_; }
    bool xpAwarded() const { return xpAwarded_; }
    const Data& data() const { return data_; }

    static const char* stateName(State s);                         // idle, partial, solved, bypassed
    // "state=<..> door_open=<0|1> opened_via=<none|..> wrong=<n> progress=<a,b>"
    std::string summary() const;

    // Save state as compact JSON text (fields: progress, solved, door_open, opened_via, wrong_count, xp_awarded).
    std::string toJson() const;
    // Replaces the state from JSON text only if it is valid; on failure *this is unchanged.
    // Codes: JSON_*, SAVE_FIELD (missing, extra or wrong-type field), SAVE_VALUE (unknown opened_via,
    // unknown control, progress longer than the solution, negative wrong_count), SAVE_STATE (fields
    // contradict each other or the rules).
    bool loadState(const std::string& text, Issue& err);

private:
    Data data_;
    std::vector<std::string> progress_;
    bool solved_ = false;
    bool doorOpen_ = false;
    std::string openedVia_;
    long long wrongCount_ = 0;
    bool xpAwarded_ = false;
};

}  // namespace puzzle
}  // namespace dd
