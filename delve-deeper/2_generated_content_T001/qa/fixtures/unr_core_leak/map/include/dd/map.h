// Floor map model, validator and queries. Standard library only (no Unreal includes).
#pragma once
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "dd/json.h"

namespace dd {
namespace map {

struct Issue {
    std::string code;
    std::string detail;
};

struct Encounter {
    std::string slot, kind, monsterType, faction;
};
struct Trap {
    std::string slot, kind;
};

struct Node {
    std::string id, type, name, area, insideRoom;
    int x = 0, y = 0;
    bool hidden = false;
    std::vector<Encounter> encounters;
    std::vector<Trap> traps;
    std::vector<std::string> lootSlots;
    std::vector<std::string> controls;
};

struct Edge {
    std::string id, from, to, door, base;
    std::vector<std::string> solutions;
    bool hasBase = false;
    bool hasSolutions = false;
};

struct PuzzleRef {
    std::string id, clue, controls, opensEdge;
};

struct QuestDestination {
    std::string key, node;
};

// Fixed GDD counts for the demo (demo counts, not universal limits): kept in one struct so a
// later floor can pass different values. The four legal door types are a fixed GDD rule.
struct Rules {
    std::size_t traps = 3;
    std::size_t monsterTypes = 3;
};

struct ReachOptions {
    std::set<std::string> blockedEdges;
    std::set<std::string> blockedNodes;
    bool plainOnly = false;          // only edges whose door is "plain"
    bool throughSites = true;        // a node with inside_room is linked to its room both ways
};

struct FloorMap {
    std::string version, status;
    std::vector<std::string> legalDoorTypes;
    std::vector<std::string> areaIds;
    std::vector<std::string> monsterTypes;
    std::vector<Node> nodes;
    std::vector<Edge> edges;
    std::vector<std::string> reserved;
    std::vector<QuestDestination> questDestinations;
    PuzzleRef puzzle;

    const Node* node(const std::string& id) const;
    const Edge* edge(const std::string& id) const;
    // Nodes joined to `id` by an edge (any door type), sorted, unique. Sites are not neighbours.
    std::vector<std::string> neighbours(const std::string& id) const;
    // Undirected: doors pass both ways. Includes `start`.
    std::set<std::string> reachable(const std::string& start, const ReachOptions& opt = ReachOptions()) const;
    bool canReach(const std::string& from, const std::string& to, const ReachOptions& opt = ReachOptions()) const;
};

// Builds a FloorMap from parsed JSON. Only shape problems fail here (code SCHEMA).
bool fromJson(const json::Value& root, FloorMap& out, Issue& err);
// Strict parse + fromJson from text. Issue code is a JSON_* code or SCHEMA.
bool fromText(const std::string& text, FloorMap& out, Issue& err);

// All rule violations in a fixed order (first entry is the primary one).
std::vector<Issue> validate(const FloorMap& m, const Rules& rules = Rules());

}  // namespace map
}  // namespace dd
