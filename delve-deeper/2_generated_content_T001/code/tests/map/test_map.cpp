#include <filesystem>
#include <fstream>
#include <sstream>

#include "../testing.h"
#include "dd/fileio.h"
#include "dd/map.h"
#include "dd/map_cli.h"

using namespace dd::map;

namespace {

std::string mapPath() { return std::string(DD_REPO_ROOT) + "/work/T001/map/floor1_map.json"; }

std::string realText() {
    std::string t;
    if (!dd::readFile(mapPath(), t)) { std::cerr << "cannot read " << mapPath() << "\n"; std::exit(2); }
    return t;
}

// Replaces exactly one occurrence of `from`; fails the test when it is not found.
std::string mutate(std::string text, const std::string& from, const std::string& to) {
    std::size_t p = text.find(from);
    if (p == std::string::npos) {
        dd_test::fail(__FILE__, __LINE__, "mutation target not found: " + from);
        return text;
    }
    text.replace(p, from.size(), to);
    return text;
}

// Code of the first problem, or "" when the map is accepted.
std::string firstCode(const std::string& text) {
    FloorMap m;
    Issue err;
    if (!fromText(text, m, err)) return err.code;
    std::vector<Issue> is = validate(m);
    return is.empty() ? "" : is[0].code;
}

FloorMap realMap() {
    FloorMap m;
    Issue err;
    if (!fromText(realText(), m, err)) { std::cerr << err.code << ": " << err.detail << "\n"; std::exit(2); }
    return m;
}

std::string writeTmp(const std::string& name, const std::string& content) {
    std::filesystem::create_directories(DD_TMP_DIR);
    std::string p = std::string(DD_TMP_DIR) + "/" + name;
    std::ofstream f(p, std::ios::binary);
    f << content;
    return p;
}

int cli(const std::vector<std::string>& a, std::string& output) {
    std::ostringstream os;
    int rc = mapCheckMain(a, os);
    output = os.str();
    return rc;
}

bool reach(const FloorMap& m, const std::string& from, const std::string& to, std::set<std::string> blocked = {}) {
    ReachOptions o;
    o.blockedEdges = blocked;
    return m.canReach(from, to, o);
}

}  // namespace

TEST(map_real_validates) {
    FloorMap m = realMap();
    std::vector<Issue> is = validate(m);
    for (const Issue& i : is) std::cerr << "  issue " << i.code << ": " << i.detail << "\n";
    CHECK(is.empty());
    CHECK_EQ(m.nodes.size(), static_cast<std::size_t>(17));
    CHECK_EQ(m.edges.size(), static_cast<std::size_t>(15));
    std::string out;
    CHECK_EQ(cli({"validate", mapPath()}, out), 0);
    CHECK(out.find("nodes=17") != std::string::npos);
    CHECK(out.find("edges=15") != std::string::npos);
}

TEST(map_dup_ids) {
    std::string t = realText();
    CHECK_EQ(firstCode(mutate(t, "{\"id\": \"entry_hall\", \"type\": \"room\"", "{\"id\": \"mess_hall\", \"type\": \"room\"")),
             std::string("DUP_ID"));
    CHECK_EQ(firstCode(mutate(t, "{\"id\": \"e15\"", "{\"id\": \"e14\"")), std::string("DUP_ID"));
}

TEST(map_bad_door_type) {
    CHECK_EQ(firstCode(mutate(realText(), "{\"id\": \"e03\", \"from\": \"entry_hall\", \"to\": \"mess_hall\", \"door\": \"plain\"}",
                              "{\"id\": \"e03\", \"from\": \"entry_hall\", \"to\": \"mess_hall\", \"door\": \"iron\"}")),
             std::string("BAD_DOOR_TYPE"));
}

TEST(map_unknown_refs) {
    std::string t = realText();
    CHECK_EQ(firstCode(mutate(t, "\"from\": \"ore_lift\", \"to\": \"barracks\"", "\"from\": \"ore_lift\", \"to\": \"barrack\"")),
             std::string("UNKNOWN_NODE"));
    CHECK_EQ(firstCode(mutate(t, "\"figure_task_destination\": \"ore_lift\"", "\"figure_task_destination\": \"ore_cellar\"")),
             std::string("UNKNOWN_NODE"));
    CHECK_EQ(firstCode(mutate(t, "\"inside_room\": \"foreman_office\"", "\"inside_room\": \"nowhere\"")),
             std::string("UNKNOWN_NODE"));
    CHECK_EQ(firstCode(mutate(t, "\"clue\": \"puzzle_clue_site\"", "\"clue\": \"no_such_site\"")),
             std::string("UNKNOWN_NODE"));
}

TEST(map_locked_door_rules) {
    std::string t = realText();
    const std::string sol = "\"solutions\": [\"puzzle\", \"magic_key\", \"pick_lock\", \"break\"]";
    CHECK_EQ(firstCode(mutate(t, sol, "\"solutions\": [\"puzzle\", \"magic_key\", \"pick_lock\"]")),
             std::string("LOCKED_NEEDS_BREAK"));
    CHECK_EQ(firstCode(mutate(t, sol, "\"solutions\": [\"puzzle\", \"break\"]")), std::string("LOCKED_ROUTES"));
    CHECK_EQ(firstCode(mutate(t, sol, "\"solutions\": [\"puzzle\", \"pick_lock\", \"break\"]")), std::string("LOCKED_ROUTES"));
    CHECK_EQ(firstCode(mutate(t, sol, "\"solutions\": [\"puzzle\", \"magic_key\", \"break\"]")), std::string("LOCKED_ROUTES"));
}

TEST(map_stuck_door_rules) {
    std::string t = realText();
    CHECK_EQ(firstCode(mutate(t, "\"solutions\": [\"break\"]", "\"solutions\": [\"break\", \"pick_lock\"]")),
             std::string("STUCK_BREAK_ONLY"));
    CHECK_EQ(firstCode(mutate(t, "\"solutions\": [\"break\"]", "\"solutions\": [\"pick_lock\"]")),
             std::string("STUCK_BREAK_ONLY"));
}

TEST(map_secret_door_rules) {
    std::string t = realText();
    CHECK_EQ(firstCode(mutate(t, "\"base\": \"plain\", ", "")), std::string("SECRET_BASE"));
    CHECK_EQ(firstCode(mutate(t, "\"base\": \"plain\"", "\"base\": \"secret\"")), std::string("SECRET_BASE"));
    CHECK_EQ(firstCode(mutate(t, "\"solutions\": [\"search\", \"magic\"]", "\"solutions\": [\"break\"]")),
             std::string("SECRET_BASE"));
}

TEST(map_descent_rules) {
    std::string t = realText();
    const std::string e15 = "{\"id\": \"e15\", \"from\": \"boss_room\", \"to\": \"descent\", \"door\": \"plain\"}";
    CHECK_EQ(firstCode(mutate(t, e15, e15 + ",\n    {\"id\": \"e16\", \"from\": \"toll_hall\", \"to\": \"descent\", \"door\": \"plain\"}")),
             std::string("DESCENT_NOT_VIA_BOSS"));
    CHECK_EQ(firstCode(mutate(t, "{\"id\": \"e14\", \"from\": \"toll_hall\", \"to\": \"boss_room\", \"door\": \"plain\"}",
                              "{\"id\": \"e14\", \"from\": \"toll_hall\", \"to\": \"boss_room\", \"door\": \"locked\", "
                              "\"solutions\": [\"magic_key\", \"pick_lock\", \"break\"]}")),
             std::string("NO_OPEN_ROUTE_TO_DESCENT"));
}

TEST(map_orphan_rooms) {
    std::string t = realText();
    t = mutate(t, "    {\"id\": \"e04\", \"from\": \"entry_hall\", \"to\": \"collapsed_gallery\", \"door\": \"plain\"},\n", "");
    t = mutate(t, "    {\"id\": \"e10\", \"from\": \"collapsed_gallery\", \"to\": \"ore_lift\", \"door\": \"plain\"},\n", "");
    t = mutate(t, "    {\"id\": \"e13\", \"from\": \"hidden_cache\", \"to\": \"toll_hall\", \"door\": \"stuck\", \"solutions\": [\"break\"], "
                  "\"note\": \"optional shortcut; stuck doors can only be broken\"},\n", "");
    CHECK_EQ(firstCode(t), std::string("UNREACHABLE_NODE"));
}

TEST(map_puzzle_rules) {
    std::string t = realText();
    CHECK_EQ(firstCode(mutate(t, "\"inside_room\": \"foreman_office\"", "\"inside_room\": \"storeroom\"")),
             std::string("PUZZLE_CLUE_BEHIND_DOOR"));
    CHECK_EQ(firstCode(mutate(t, "\"x\": 1, \"y\": 4, \"inside_room\": \"smelter_hall\"",
                              "\"x\": 0, \"y\": 4, \"inside_room\": \"storeroom\"")),
             std::string("PUZZLE_CONTROLS_BEHIND_DOOR"));
    CHECK_EQ(firstCode(mutate(t, "\"opens_edge\": \"e08\"", "\"opens_edge\": \"e03\"")), std::string("PUZZLE_EDGE"));
    CHECK_EQ(firstCode(mutate(t, "\"opens_edge\": \"e08\"", "\"opens_edge\": \"e99\"")), std::string("PUZZLE_EDGE"));
}

TEST(map_count_rules) {
    std::string t = realText();
    CHECK_EQ(firstCode(mutate(t, "{\"id\": \"entry_hall\", \"type\": \"room\", \"name\": \"Entry hall\", \"area\": \"area_upper\", \"x\": 2, \"y\": 1}",
                              "{\"id\": \"entry_hall\", \"type\": \"room\", \"name\": \"Entry hall\", \"area\": \"area_upper\", \"x\": 2, \"y\": 1, "
                              "\"traps\": [{\"slot\": \"trap_extra\", \"trap_kind\": \"x\"}]}")),
             std::string("TRAP_COUNT"));
    CHECK_EQ(firstCode(mutate(t, "[\"mon_type_1\", \"mon_type_2\", \"mon_type_3\"]", "[\"mon_type_1\", \"mon_type_2\"]")),
             std::string("MONSTER_TYPES"));
    CHECK_EQ(firstCode(mutate(t, "\"monster_type\": \"mon_type_2\"", "\"monster_type\": \"mon_type_9\"")),
             std::string("UNKNOWN_MONSTER_TYPE"));
    CHECK_EQ(firstCode(mutate(t, "\"area\": \"area_bandit\", \"x\": 3, \"y\": 6", "\"area\": \"area_x\", \"x\": 3, \"y\": 6")),
             std::string("UNKNOWN_AREA"));
}

TEST(map_cell_clash) {
    CHECK_EQ(firstCode(mutate(realText(), "\"name\": \"Bandit barracks\", \"area\": \"area_bandit\", \"x\": 3, \"y\": 4",
                              "\"name\": \"Bandit barracks\", \"area\": \"area_bandit\", \"x\": 3, \"y\": 5")),
             std::string("CELL_CLASH"));
}

TEST(map_cells) {
    FloorMap m = realMap();
    std::set<std::string> pairs;
    for (std::size_t i = 0; i < m.nodes.size(); ++i)
        for (std::size_t j = i + 1; j < m.nodes.size(); ++j)
            if (m.nodes[i].x == m.nodes[j].x && m.nodes[i].y == m.nodes[j].y) {
                const Node& a = m.nodes[i];
                const Node& b = m.nodes[j];
                CHECK(a.insideRoom == b.id || b.insideRoom == a.id);
                pairs.insert(a.id + "+" + b.id);
            }
    std::set<std::string> want = {"foreman_office+puzzle_clue_site", "smelter_hall+puzzle_controls_site"};
    CHECK(pairs == want);
}

TEST(map_neighbours) {
    FloorMap m = realMap();
    std::vector<std::string> a = m.neighbours("entry_hall");
    CHECK(a == (std::vector<std::string>{"collapsed_gallery", "mess_hall", "town_entrance"}));
    CHECK(m.neighbours("descent") == (std::vector<std::string>{"boss_room"}));
    std::string out;
    CHECK_EQ(cli({"neighbours", mapPath(), "descent"}, out), 0);
    CHECK_EQ(out, std::string("boss_room\n"));
    CHECK_EQ(cli({"neighbours", mapPath(), "nowhere"}, out), 1);
    CHECK(out.rfind("FAIL UNKNOWN_NODE", 0) == 0);
}

TEST(map_door_ways) {
    FloorMap m = realMap();
    CHECK_EQ(m.edge("e08")->door, std::string("locked"));
    CHECK_EQ(m.edge("e09")->door, std::string("secret"));
    CHECK_EQ(m.edge("e13")->door, std::string("stuck"));
    CHECK_EQ(m.edge("e03")->door, std::string("plain"));
    std::string out;
    CHECK_EQ(cli({"door", mapPath(), "e08"}, out), 0);
    CHECK_EQ(out, std::string("locked\n"));
    CHECK_EQ(cli({"ways", mapPath(), "e08"}, out), 0);
    CHECK_EQ(out, std::string("e08 = puzzle, magic_key, pick_lock, break\n"));
    CHECK_EQ(cli({"ways", mapPath(), "e09"}, out), 0);
    CHECK_EQ(out, std::string("e09 = search, magic\n"));
    CHECK_EQ(cli({"ways", mapPath(), "e13"}, out), 0);
    CHECK_EQ(out, std::string("e13 = break\n"));
    CHECK_EQ(cli({"ways", mapPath(), "e99"}, out), 1);
    CHECK(out.rfind("FAIL UNKNOWN_EDGE", 0) == 0);
    CHECK_EQ(cli({"door", mapPath(), "e99"}, out), 1);
}

TEST(map_reach) {
    FloorMap m = realMap();
    CHECK(reach(m, "town_entrance", "descent"));
    CHECK(!reach(m, "town_entrance", "descent", {"e14"}));
    CHECK(!reach(m, "town_entrance", "storeroom", {"e08"}));
    for (const char* t : {"puzzle_clue_site", "puzzle_controls_site", "foreman_office", "smelter_hall"})
        CHECK(reach(m, "town_entrance", t, {"e08"}));
    CHECK(reach(m, "town_entrance", "ore_lift", {"e04"}));          // long way round
    CHECK(!reach(m, "town_entrance", "ore_lift", {"e04", "e09"}));
    CHECK(reach(m, "sewer_grate", "ore_lift", {"e08", "e09", "e13"}));
    // the descent is only past the boss room
    ReachOptions o;
    o.blockedNodes.insert("boss_room");
    CHECK(!m.canReach("town_entrance", "descent", o));
    // the real map's route to descent needs only plain doors
    o = ReachOptions();
    o.plainOnly = true;
    CHECK(m.canReach("town_entrance", "descent", o));
    std::string out;
    CHECK_EQ(cli({"reach", mapPath(), "town_entrance", "descent"}, out), 0);
    CHECK_EQ(out, std::string("reachable\n"));
    CHECK_EQ(cli({"reach", mapPath(), "town_entrance", "descent", "e14"}, out), 0);
    CHECK_EQ(out, std::string("unreachable\n"));
    CHECK_EQ(cli({"reach", mapPath(), "town_entrance", "descent", "e99"}, out), 1);
    CHECK(out.rfind("FAIL UNKNOWN_EDGE", 0) == 0);
    CHECK_EQ(cli({"reach", mapPath(), "town_entrance", "nowhere"}, out), 1);
    CHECK(out.rfind("FAIL UNKNOWN_NODE", 0) == 0);
}

TEST(map_cli_failures) {
    std::string out;
    std::string bad = writeTmp("map_e08_no_break.json",
                               mutate(realText(), "\"solutions\": [\"puzzle\", \"magic_key\", \"pick_lock\", \"break\"]",
                                      "\"solutions\": [\"puzzle\", \"magic_key\", \"pick_lock\"]"));
    CHECK_EQ(cli({"validate", bad}, out), 1);
    CHECK(out.rfind("FAIL LOCKED_NEEDS_BREAK", 0) == 0);
    CHECK_EQ(cli({"validate", writeTmp("map_trunc.json", "{\"nodes\": [")}, out), 1);
    CHECK(out.rfind("FAIL JSON_TRUNCATED", 0) == 0);
    CHECK_EQ(cli({"validate", writeTmp("map_schema.json", "{\"nodes\": []}")}, out), 1);
    CHECK(out.rfind("FAIL SCHEMA", 0) == 0);
    CHECK_EQ(cli({"validate", std::string(DD_TMP_DIR) + "/no_such_file.json"}, out), 2);
    CHECK_EQ(cli({"validate"}, out), 2);
    CHECK_EQ(cli({"frobnicate", mapPath()}, out), 2);
    CHECK_EQ(cli({"reach", mapPath(), "town_entrance"}, out), 2);
}

// Every shape-error path of the exception-free loader returns SCHEMA with the first problem.
TEST(map_schema_errors) {
    struct Case { std::string from, to, expect; };
    const std::string t = realText();
    const std::vector<Case> cases = {
        {"\"legal_door_types\"", "\"legal_door_typez\"", "missing field \"legal_door_types\""},
        {"\"nodes\"", "\"nodez\"", "missing field \"nodes\""},
        {"\"edges\"", "\"edgez\"", "missing field \"edges\""},
        {"\"puzzle\": {", "\"puzzle\": [], \"zzz\": {", "puzzle must be an object"},
    };
    for (const Case& c : cases) {
        FloorMap m;
        Issue err;
        CHECK(!fromText(mutate(t, c.from, c.to), m, err));
        CHECK_EQ(err.code, std::string("SCHEMA"));
        CHECK(err.detail.find(c.expect) != std::string::npos);
    }
    FloorMap m;
    Issue err;
    CHECK(!fromText("[]", m, err));
    CHECK_EQ(err.code, std::string("SCHEMA"));
    CHECK(err.detail.find("top level must be an object") != std::string::npos);
    CHECK(!fromText("{\"nodes\": []}", m, err));
    CHECK_EQ(err.code, std::string("SCHEMA"));
    // Wrong types: the first problem wins and is reported once.
    CHECK(!fromText(mutate(t, "\"x\": 2, \"y\": 1}", "\"x\": \"two\", \"y\": 1}"), m, err));
    CHECK(err.detail.find("must be an integer") != std::string::npos);
    CHECK(!fromText(mutate(t, "\"legal_door_types\": [", "\"legal_door_types\": [1, "), m, err));
    CHECK(err.detail.find("must be an array of strings") != std::string::npos);
    // Parse errors keep their JSON_* code through fromText.
    CHECK(!fromText("{\"nodes\": [1,]}", m, err));
    CHECK_EQ(err.code, std::string("JSON_TRAILING_COMMA"));
}

int main(int argc, char** argv) { return dd_test::runMain(argc, argv); }
