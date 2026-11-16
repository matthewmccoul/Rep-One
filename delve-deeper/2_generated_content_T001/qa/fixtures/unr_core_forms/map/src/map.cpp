#include "dd/map.h"

#include <algorithm>
#include <deque>
#include <map>

namespace dd {
namespace map {

// ---------------------------------------------------------------- queries

const Node* FloorMap::node(const std::string& id) const {
    for (const Node& n : nodes) if (n.id == id) return &n;
    return nullptr;
}

const Edge* FloorMap::edge(const std::string& id) const {
    for (const Edge& e : edges) if (e.id == id) return &e;
    return nullptr;
}

std::vector<std::string> FloorMap::neighbours(const std::string& id) const {
    std::set<std::string> s;
    for (const Edge& e : edges) {
        if (e.from == id) s.insert(e.to);
        else if (e.to == id) s.insert(e.from);
    }
    return std::vector<std::string>(s.begin(), s.end());
}

std::set<std::string> FloorMap::reachable(const std::string& start, const ReachOptions& opt) const {
    std::map<std::string, std::vector<std::string>> adj;
    if (opt.throughSites) {
        for (const Node& n : nodes) {
            if (n.insideRoom.empty() || !node(n.insideRoom)) continue;
            adj[n.id].push_back(n.insideRoom);
            adj[n.insideRoom].push_back(n.id);
        }
    }
    for (const Edge& e : edges) {
        if (opt.blockedEdges.count(e.id)) continue;
        if (opt.plainOnly && e.door != "plain") continue;
        if (!node(e.from) || !node(e.to)) continue;
        adj[e.from].push_back(e.to);
        adj[e.to].push_back(e.from);
    }
    std::set<std::string> seen;
    if (opt.blockedNodes.count(start)) return seen;
    seen.insert(start);
    std::deque<std::string> q;
    q.push_back(start);
    while (!q.empty()) {
        std::string u = q.front();
        q.pop_front();
        auto it = adj.find(u);
        if (it == adj.end()) continue;
        for (const std::string& v : it->second) {
            if (seen.count(v) || opt.blockedNodes.count(v)) continue;
            seen.insert(v);
            q.push_back(v);
        }
    }
    return seen;
}

bool FloorMap::canReach(const std::string& from, const std::string& to, const ReachOptions& opt) const {
    return reachable(from, opt).count(to) > 0;
}

// ---------------------------------------------------------------- loading

namespace {

const std::string kRoot = "root";

// No exceptions: bad() records the first problem; helpers then return empty defaults so the
// builder can run on to its end harmlessly, and fromJson reports `failed`/`detail`.
struct Loader {
bool failed = false;
std::string detail;
json::Value nullValue;

void bad(const std::string& d) {
    if (failed) return;
    failed = true;
    detail = d;
}

const json::Value& req(const json::Value& obj, const char* key, const std::string& where) {
    const json::Value* v = obj.find(key);
    if (!v) {
        bad(where + ": missing field \"" + key + "\"");
        return nullValue;
    }
    return *v;
}

std::string reqString(const json::Value& obj, const char* key, const std::string& where) {
    const json::Value& v = req(obj, key, where);
    if (!v.isString()) bad(where + ": field \"" + key + "\" must be a string");
    return v.string;
}

std::string optString(const json::Value& obj, const char* key, const std::string& where) {
    const json::Value* v = obj.find(key);
    if (!v) return "";
    if (!v->isString()) bad(where + ": field \"" + key + "\" must be a string");
    return v->string;
}

int reqInt(const json::Value& obj, const char* key, const std::string& where) {
    const json::Value& v = req(obj, key, where);
    if (!v.isNumber() || !v.isInteger) bad(where + ": field \"" + key + "\" must be an integer");
    return static_cast<int>(v.integer);
}

std::vector<std::string> stringList(const json::Value& v, const std::string& where) {
    if (!v.isArray()) bad(where + " must be an array of strings");
    std::vector<std::string> r;
    for (const json::Value& i : v.items) {
        if (!i.isString()) bad(where + " must be an array of strings");
        r.push_back(i.string);
    }
    return r;
}

void build(const json::Value& root, FloorMap& m) {
    if (!root.isObject()) bad("top level must be an object");
    m.version = optString(root, "version", "root");
    m.status = optString(root, "status", "root");
    m.legalDoorTypes = stringList(req(root, "legal_door_types", "root"), "legal_door_types");
    const json::Value& areas = req(root, "areas", kRoot);
    if (!areas.isObject()) bad("areas must be an object");
    m.areaIds = areas.keys;
    m.monsterTypes = stringList(req(root, "monster_types", "root"), "monster_types");

    const json::Value& nodes = req(root, "nodes", kRoot);
    if (!nodes.isArray()) bad("nodes must be an array");
    for (std::size_t i = 0; i < nodes.items.size(); ++i) {
        const json::Value& jn = nodes.items[i];
        std::string w = "nodes[" + std::to_string(i) + "]";
        if (!jn.isObject()) bad(w + " must be an object");
        Node n;
        n.id = reqString(jn, "id", w);
        w = "node " + n.id;
        n.type = reqString(jn, "type", w);
        n.area = reqString(jn, "area", w);
        n.x = reqInt(jn, "x", w);
        n.y = reqInt(jn, "y", w);
        n.name = optString(jn, "name", w);
        n.insideRoom = optString(jn, "inside_room", w);
        if (const json::Value* h = jn.find("hidden")) {
            if (!h->isBool()) bad(w + ": hidden must be true/false");
            n.hidden = h->boolean;
        }
        if (const json::Value* enc = jn.find("encounters")) {
            if (!enc->isArray()) bad(w + ": encounters must be an array");
            for (const json::Value& je : enc->items) {
                if (!je.isObject()) bad(w + ": encounter must be an object");
                Encounter e;
                e.slot = reqString(je, "slot", w + " encounter");
                e.kind = reqString(je, "kind", w + " encounter " + e.slot);
                e.monsterType = optString(je, "monster_type", w + " encounter " + e.slot);
                e.faction = optString(je, "faction", w + " encounter " + e.slot);
                n.encounters.push_back(e);
            }
        }
        if (const json::Value* tr = jn.find("traps")) {
            if (!tr->isArray()) bad(w + ": traps must be an array");
            for (const json::Value& jt : tr->items) {
                if (!jt.isObject()) bad(w + ": trap must be an object");
                Trap t;
                t.slot = reqString(jt, "slot", w + " trap");
                t.kind = optString(jt, "trap_kind", w + " trap " + t.slot);
                n.traps.push_back(t);
            }
        }
        if (const json::Value* ls = jn.find("loot_slots")) n.lootSlots = stringList(*ls, w + ": loot_slots");
        if (const json::Value* cs = jn.find("controls")) n.controls = stringList(*cs, w + ": controls");
        m.nodes.push_back(n);
    }

    const json::Value& edges = req(root, "edges", kRoot);
    if (!edges.isArray()) bad("edges must be an array");
    for (std::size_t i = 0; i < edges.items.size(); ++i) {
        const json::Value& je = edges.items[i];
        std::string w = "edges[" + std::to_string(i) + "]";
        if (!je.isObject()) bad(w + " must be an object");
        Edge e;
        e.id = reqString(je, "id", w);
        w = "edge " + e.id;
        e.from = reqString(je, "from", w);
        e.to = reqString(je, "to", w);
        e.door = reqString(je, "door", w);
        if (const json::Value* b = je.find("base")) {
            if (!b->isString()) bad(w + ": base must be a string");
            e.base = b->string;
            e.hasBase = true;
        }
        if (const json::Value* s = je.find("solutions")) {
            e.solutions = stringList(*s, w + ": solutions");
            e.hasSolutions = true;
        }
        m.edges.push_back(e);
    }

    m.reserved = stringList(req(root, "reserved", "root"), "reserved");
    const json::Value& qd = req(root, "quest_destinations", kRoot);
    if (!qd.isObject()) bad("quest_destinations must be an object");
    for (std::size_t i = 0; i < qd.keys.size(); ++i) {
        if (!qd.items[i].isString()) bad("quest_destinations." + qd.keys[i] + " must be a string");
        m.questDestinations.push_back({qd.keys[i], qd.items[i].string});
    }
    const json::Value& pz = req(root, "puzzle", kRoot);
    if (!pz.isObject()) bad("puzzle must be an object");
    m.puzzle.id = reqString(pz, "id", "puzzle");
    m.puzzle.clue = reqString(pz, "clue", "puzzle");
    m.puzzle.controls = reqString(pz, "controls", "puzzle");
    m.puzzle.opensEdge = reqString(pz, "opens_edge", "puzzle");
}

};

}  // namespace

bool fromJson(const json::Value& root, FloorMap& out, Issue& err) {
    out = FloorMap();
    Loader loader;
    loader.build(root, out);
    if (loader.failed) {
        err.code = "SCHEMA";
        err.detail = loader.detail;
        return false;
    }
    return true;
}

bool fromText(const std::string& text, FloorMap& out, Issue& err) {
    json::Value v;
    json::Error je;
    if (!json::parse(text, v, je)) {
        err.code = je.code;
        err.detail = je.describe();
        return false;
    }
    return fromJson(v, out, err);
}

// ---------------------------------------------------------------- validation

namespace {

// The four door types are a fixed GDD rule.
const char* const kDoorTypes[] = {"plain", "secret", "locked", "stuck"};

bool legalDoor(const std::string& d) {
    for (const char* t : kDoorTypes) if (d == t) return true;
    return false;
}

bool has(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}

std::vector<const Node*> nodesOfType(const FloorMap& m, const std::string& type) {
    std::vector<const Node*> r;
    for (const Node& n : m.nodes) if (n.type == type) r.push_back(&n);
    return r;
}

}  // namespace

std::vector<Issue> validate(const FloorMap& m, const Rules& rules) {
    std::vector<Issue> is;
    auto add = [&](const std::string& code, const std::string& detail) { is.push_back({code, detail}); };

    // A. unique IDs
    {
        std::set<std::string> seen;
        for (const Node& n : m.nodes)
            if (!seen.insert(n.id).second) add("DUP_ID", "duplicate node id " + n.id);
        seen.clear();
        for (const Edge& e : m.edges)
            if (!seen.insert(e.id).second) add("DUP_ID", "duplicate edge id " + e.id);
        seen.clear();
        for (const Node& n : m.nodes) {
            for (const Encounter& e : n.encounters)
                if (!seen.insert(e.slot).second) add("DUP_ID", "duplicate slot id " + e.slot);
            for (const Trap& t : n.traps)
                if (!seen.insert(t.slot).second) add("DUP_ID", "duplicate slot id " + t.slot);
            for (const std::string& l : n.lootSlots)
                if (!seen.insert(l).second) add("DUP_ID", "duplicate slot id " + l);
        }
    }

    // B. areas
    for (const Node& n : m.nodes)
        if (!has(m.areaIds, n.area)) add("UNKNOWN_AREA", "node " + n.id + " has unknown area " + n.area);

    // C. references and door types
    {
        std::vector<std::string> fixed(std::begin(kDoorTypes), std::end(kDoorTypes));
        std::vector<std::string> a = m.legalDoorTypes, b = fixed;
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        if (a != b) add("LEGAL_DOOR_LIST", "legal_door_types must be exactly plain, secret, locked, stuck");
    }
    for (const Edge& e : m.edges) {
        if (!m.node(e.from)) add("UNKNOWN_NODE", "edge " + e.id + " endpoint missing: " + e.from);
        if (!m.node(e.to)) add("UNKNOWN_NODE", "edge " + e.id + " endpoint missing: " + e.to);
        if (!legalDoor(e.door)) add("BAD_DOOR_TYPE", "edge " + e.id + " illegal door type " + e.door);
    }
    for (const Node& n : m.nodes)
        if (!n.insideRoom.empty() && !m.node(n.insideRoom))
            add("UNKNOWN_NODE", "node " + n.id + " inside_room missing: " + n.insideRoom);
    for (const std::string& r : m.reserved)
        if (!m.node(r)) add("UNKNOWN_NODE", "reserved node missing: " + r);
    for (const QuestDestination& q : m.questDestinations)
        if (!m.node(q.node)) add("UNKNOWN_NODE", "quest destination " + q.key + " unknown node: " + q.node);
    if (!m.node(m.puzzle.clue)) add("UNKNOWN_NODE", "puzzle clue site missing: " + m.puzzle.clue);
    if (!m.node(m.puzzle.controls)) add("UNKNOWN_NODE", "puzzle controls site missing: " + m.puzzle.controls);

    // D. door rules
    for (const Edge& e : m.edges) {
        if (e.door == "locked") {
            if (!has(e.solutions, "break"))
                add("LOCKED_NEEDS_BREAK", "locked edge " + e.id + " must list break");
            if (!has(e.solutions, "magic_key") || !has(e.solutions, "pick_lock"))
                add("LOCKED_ROUTES", "locked edge " + e.id + " must list magic_key and pick_lock");
        } else if (e.door == "stuck") {
            if (!(e.solutions.size() == 1 && e.solutions[0] == "break"))
                add("STUCK_BREAK_ONLY", "stuck edge " + e.id + " solutions must be exactly [break]");
        } else if (e.door == "secret") {
            bool baseOk = e.hasBase && (e.base == "plain" || e.base == "locked" || e.base == "stuck");
            bool routeOk = has(e.solutions, "search") || has(e.solutions, "magic");
            if (!baseOk) add("SECRET_BASE", "secret edge " + e.id + " needs base plain, locked or stuck");
            if (!routeOk) add("SECRET_BASE", "secret edge " + e.id + " needs a search or magic solution");
        }
    }

    // E. puzzle edge
    {
        const Edge* pe = m.edge(m.puzzle.opensEdge);
        if (!pe) add("PUZZLE_EDGE", "puzzle opens_edge unknown: " + m.puzzle.opensEdge);
        else if (pe->door != "locked") add("PUZZLE_EDGE", "puzzle opens_edge " + pe->id + " is not a locked door");
        else if (!has(pe->solutions, "puzzle")) add("PUZZLE_EDGE", "puzzle opens_edge " + pe->id + " does not list puzzle");
    }

    // F. counts and node roles
    if (m.monsterTypes.size() != rules.monsterTypes)
        add("MONSTER_TYPES", "declared monster types " + std::to_string(m.monsterTypes.size()) +
                                 ", expected " + std::to_string(rules.monsterTypes));
    {
        std::set<std::string> used;
        bool bandits = false, boss = false;
        for (const Node& n : m.nodes)
            for (const Encounter& e : n.encounters) {
                if (e.kind == "monster") {
                    if (!has(m.monsterTypes, e.monsterType))
                        add("UNKNOWN_MONSTER_TYPE", "encounter " + e.slot + " monster type unknown: " + e.monsterType);
                    else used.insert(e.monsterType);
                } else if (e.kind == "bandits") bandits = true;
                else if (e.kind == "boss") boss = true;
            }
        for (const std::string& t : m.monsterTypes)
            if (!used.count(t)) add("MONSTER_UNUSED", "monster type " + t + " has no encounter slot");
        if (!bandits) add("NO_BANDIT_ENCOUNTER", "no encounter of kind bandits");
        if (!boss) add("NO_BOSS_ENCOUNTER", "no encounter of kind boss");
    }
    {
        std::size_t traps = 0;
        for (const Node& n : m.nodes) traps += n.traps.size();
        if (traps != rules.traps)
            add("TRAP_COUNT", "trap count " + std::to_string(traps) + ", expected " + std::to_string(rules.traps));
    }
    const char* roles[] = {"entrance", "boss_room", "descent", "puzzle_clue"};
    bool rolesOk = true;
    for (const char* r : roles) {
        std::size_t c = nodesOfType(m, r).size();
        if (c != 1) {
            rolesOk = false;
            add("NODE_TYPE_COUNT", std::string("expected exactly one node of type ") + r + ", found " + std::to_string(c));
        }
    }

    // G. graph rules
    if (rolesOk) {
        const std::string entrance = nodesOfType(m, "entrance")[0]->id;
        const std::string boss = nodesOfType(m, "boss_room")[0]->id;
        const std::string descent = nodesOfType(m, "descent")[0]->id;

        std::set<std::string> full = m.reachable(entrance);
        for (const Node& n : m.nodes)
            if (!full.count(n.id)) add("UNREACHABLE_NODE", "node " + n.id + " not reachable from " + entrance);

        ReachOptions noBoss;
        noBoss.blockedNodes.insert(boss);
        if (m.canReach(entrance, descent, noBoss))
            add("DESCENT_NOT_VIA_BOSS", "descent is reachable without passing " + boss);

        // Puzzle clue and controls must be reachable while the puzzle door is closed.
        if (m.edge(m.puzzle.opensEdge)) {
            ReachOptions closed;
            closed.blockedEdges.insert(m.puzzle.opensEdge);
            std::set<std::string> c = m.reachable(entrance, closed);
            if (m.node(m.puzzle.clue) && !c.count(m.puzzle.clue))
                add("PUZZLE_CLUE_BEHIND_DOOR", "puzzle clue site unreachable while " + m.puzzle.opensEdge + " is closed");
            if (m.node(m.puzzle.controls) && !c.count(m.puzzle.controls))
                add("PUZZLE_CONTROLS_BEHIND_DOOR", "puzzle controls site unreachable while " + m.puzzle.opensEdge + " is closed");
        }

        // A route with no lock, secret door or stuck door to the places the player must get to.
        ReachOptions plain;
        plain.plainOnly = true;
        std::set<std::string> open = m.reachable(entrance, plain);
        std::set<std::string> must = {m.puzzle.clue, m.puzzle.controls};
        for (const QuestDestination& q : m.questDestinations) must.insert(q.node);
        for (const std::string& t : must)
            if (m.node(t) && !open.count(t)) add("NO_OPEN_ROUTE", "no plain-door route from " + entrance + " to " + t);
        if (!open.count(boss)) add("NO_OPEN_ROUTE_TO_DESCENT", "no plain-door route from " + entrance + " to " + boss);
        if (!open.count(descent)) add("NO_OPEN_ROUTE_TO_DESCENT", "no plain-door route from " + entrance + " to " + descent);
    }

    // H. door types in use, grid cells
    {
        std::set<std::string> used;
        for (const Edge& e : m.edges) used.insert(e.door);
        for (const char* t : kDoorTypes)
            if (!used.count(t)) add("DOOR_TYPES_USED", std::string("no edge uses door type ") + t);
    }
    for (std::size_t i = 0; i < m.nodes.size(); ++i)
        for (std::size_t j = i + 1; j < m.nodes.size(); ++j) {
            const Node& a = m.nodes[i];
            const Node& b = m.nodes[j];
            if (a.x != b.x || a.y != b.y) continue;
            if (a.insideRoom == b.id || b.insideRoom == a.id) continue;
            add("CELL_CLASH", "nodes " + a.id + " and " + b.id + " share cell (" + std::to_string(a.x) + "," +
                                  std::to_string(a.y) + ") but neither is inside_room of the other");
        }
    return is;
}

}  // namespace map
}  // namespace dd
