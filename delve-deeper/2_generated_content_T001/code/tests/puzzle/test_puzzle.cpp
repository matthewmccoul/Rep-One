// Tests for the storeroom puzzle core (dd_puzzle) and puzzle_check. Ports every case of the Python
// reference tests (work/T001/puzzle/test_puzzle.py) and adds the cases QA's spec names (PZL-xx).
#include <algorithm>
#include <sstream>

#include "../testing.h"
#include "dd/fileio.h"
#include "dd/map.h"
#include "dd/puzzle.h"
#include "dd/puzzle_cli.h"

using namespace dd::puzzle;

namespace {

const std::vector<std::string> kSol = {"ctl_pick", "ctl_cart", "ctl_furnace"};

std::string slurp(const std::string& path) {
    std::string t;
    if (!dd::readFile(path, t)) { std::cerr << "cannot read " << path << "\n"; std::exit(2); }
    return t;
}
std::string puzText() { return slurp(std::string(DD_REPO_ROOT) + "/work/T001/puzzle/storeroom_puzzle.json"); }
std::string mapText() { return slurp(std::string(DD_REPO_ROOT) + "/work/T001/map/floor1_map.json"); }
std::string fixture(const std::string& name) { return slurp(std::string(DD_TEST_SRC) + "/tests/puzzle/fixtures/" + name); }

std::string mutate(std::string text, const std::string& from, const std::string& to) {
    std::size_t p = text.find(from);
    if (p == std::string::npos) {
        dd_test::fail(__FILE__, __LINE__, "mutation target not found: " + from);
        return text;
    }
    text.replace(p, from.size(), to);
    return text;
}

Puzzle fresh() {
    Puzzle p;
    Issue err;
    if (!Puzzle::fromText(puzText(), p, err)) { std::cerr << err.code << ": " << err.detail << "\n"; std::exit(2); }
    return p;
}

dd::map::FloorMap realMap() {
    dd::map::FloorMap m;
    dd::map::Issue err;
    if (!dd::map::fromText(mapText(), m, err)) { std::cerr << err.code << ": " << err.detail << "\n"; std::exit(2); }
    return m;
}

std::string dataCode(const std::string& text) {
    Data d;
    Issue err;
    return dataFromText(text, d, err) ? std::string() : err.code;
}

std::string mapIssue(const std::string& puzzleJson, const std::string& mapJson) {
    Data d;
    Issue err;
    if (!dataFromText(puzzleJson, d, err)) return "DATA:" + err.code;
    dd::map::FloorMap m;
    dd::map::Issue merr;
    if (!dd::map::fromText(mapJson, m, merr)) return "MAP:" + merr.code;
    std::vector<Issue> is = validateAgainstMap(d, m);
    return is.empty() ? std::string() : is[0].code;
}

void pressAll(Puzzle& p, const std::vector<std::string>& cs) {
    for (const std::string& c : cs) p.use(c);
}

// All control sequences of length 0..maxLen over the three controls.
std::vector<std::vector<std::string>> allSequences(std::size_t maxLen) {
    std::vector<std::vector<std::string>> all = {{}};
    std::size_t begin = 0;
    for (std::size_t len = 1; len <= maxLen; ++len) {
        std::size_t end = all.size();
        for (std::size_t i = begin; i < end; ++i)
            for (const std::string& c : kSol) {
                std::vector<std::string> s = all[i];
                s.push_back(c);
                all.push_back(s);
            }
        begin = end;
    }
    return all;
}

std::string saveOf(const Puzzle& p) { return p.toJson(); }

Puzzle loaded(const Puzzle& p) {
    Puzzle q(p.data());
    Issue err;
    if (!q.loadState(p.toJson(), err)) dd_test::fail(__FILE__, __LINE__, "own save rejected: " + err.code + " " + err.detail);
    return q;
}

std::string stateText(const std::string& progress, const char* solved, const char* door, const std::string& via,
                      const std::string& wrong, const char* xp) {
    return "{\"progress\":[" + progress + "],\"solved\":" + solved + ",\"door_open\":" + door + ",\"opened_via\":" + via +
           ",\"wrong_count\":" + wrong + ",\"xp_awarded\":" + xp + "}";
}

std::string loadCode(const std::string& stateJson) {
    Puzzle p = fresh();
    Issue err;
    return p.loadState(stateJson, err) ? std::string() : err.code;
}

const char* const kRoutes[] = {"magic_key", "pick_lock", "break"};

}  // namespace

TEST(pzl_data_real) {
    Data d;
    Issue err;
    CHECK(dataFromText(puzText(), d, err));
    CHECK_EQ(d.controls.size(), (std::size_t)3);
    CHECK(d.solution == kSol);
    CHECK_EQ(d.opensEdge, std::string("e08"));
    CHECK((d.otherRoutes == std::vector<std::string>{"magic_key", "pick_lock", "break"}));
    CHECK(d.hasRoute("break"));
    CHECK(!d.hasRoute("puzzle"));
    CHECK(!d.hasRoute("kick"));
    CHECK_EQ(d.xpClass, std::string("puzzle_solve"));
    Puzzle p = fresh();
    CHECK_EQ(std::string(Puzzle::stateName(p.state())), std::string("idle"));
    CHECK(!p.doorOpen());
}

TEST(pzl_data_errors) {
    const std::string real = puzText();
    const std::string sol = "\"solution\": [\"ctl_pick\", \"ctl_cart\", \"ctl_furnace\"]";
    const std::string ctl = "\"controls\": [\"ctl_pick\", \"ctl_cart\", \"ctl_furnace\"]";
    CHECK_EQ(dataCode(mutate(real, sol, "\"solution\": [\"ctl_pick\", \"ctl_cart\", \"ctl_lever\"]")), std::string("BAD_SOLUTION"));
    CHECK_EQ(dataCode(mutate(real, sol, "\"solution\": []")), std::string("BAD_SOLUTION"));
    CHECK_EQ(dataCode(mutate(real, ctl, "\"controls\": []")), std::string("NO_CONTROLS"));
    CHECK_EQ(dataCode(mutate(real, ctl, "\"controls\": [\"ctl_pick\", \"ctl_pick\", \"ctl_furnace\"]")), std::string("DUP_CONTROL"));
    CHECK_EQ(dataCode(mutate(real, "\"str.use.wrong\":", "\"str.use.wrongx\":")), std::string("MISSING_STRING"));
    CHECK_EQ(dataCode(mutate(real, "\"str.use.step_2\":", "\"str.use.step_9\":")), std::string("MISSING_STRING"));
    CHECK_EQ(dataCode(mutate(real, "\"opens_edge\"", "\"opens_edgx\"")), std::string("SCHEMA"));
    CHECK_EQ(dataCode(mutate(real, "\"controls\": [", "\"controls\": [1, ")), std::string("SCHEMA"));
    CHECK_EQ(dataCode("[1]"), std::string("SCHEMA"));
    CHECK_EQ(dataCode(""), std::string("JSON_EMPTY"));
    CHECK(dataCode(real.substr(0, real.size() / 2)).rfind("JSON_", 0) == 0);
    // A longer solution needs a step string per step (data-driven, not fixed at three).
    CHECK_EQ(dataCode(mutate(real, sol, "\"solution\": [\"ctl_pick\", \"ctl_cart\", \"ctl_furnace\", \"ctl_pick\"]")), std::string("MISSING_STRING"));
}

TEST(pzl_map_cross_real) {
    Data d;
    Issue err;
    CHECK(dataFromText(puzText(), d, err));
    CHECK(validateAgainstMap(d, realMap()).empty());
}

TEST(pzl_map_cross_broken) {
    const std::string real = puzText();
    const std::string rm = mapText();
    CHECK_EQ(mapIssue(real, rm), std::string());
    CHECK_EQ(mapIssue(mutate(real, "\"opens_edge\": \"e08\"", "\"opens_edge\": \"e03\""), rm), std::string("PUZZLE_EDGE"));
    CHECK_EQ(mapIssue(mutate(real, "\"opens_edge\": \"e08\"", "\"opens_edge\": \"e99\""), rm), std::string("PUZZLE_EDGE"));
    CHECK_EQ(mapIssue(mutate(real, "\"controls_site\": \"puzzle_controls_site\"", "\"controls_site\": \"smelter_room\""), rm), std::string("UNKNOWN_NODE"));
    CHECK_EQ(mapIssue(mutate(real, "\"clue_site\": \"puzzle_clue_site\"", "\"clue_site\": \"nowhere\""), rm), std::string("UNKNOWN_NODE"));
    CHECK_EQ(mapIssue(mutate(real, "\"other_routes\": [\"magic_key\", \"pick_lock\", \"break\"]", "\"other_routes\": [\"magic_key\", \"pick_lock\"]"), rm), std::string("ROUTES_MISMATCH"));
    CHECK_EQ(mapIssue(mutate(real, "\"other_routes\": [\"magic_key\", \"pick_lock\", \"break\"]", "\"other_routes\": [\"magic_key\", \"pick_lock\", \"break\", \"kick\"]"), rm), std::string("ROUTES_MISMATCH"));
    // Map side: e08 not locked, or without the puzzle among its solutions.
    CHECK_EQ(mapIssue(real, mutate(rm, "\"to\": \"storeroom\", \"door\": \"locked\"", "\"to\": \"storeroom\", \"door\": \"plain\"")), std::string("PUZZLE_EDGE"));
    CHECK_EQ(mapIssue(real, mutate(rm, "\"solutions\": [\"puzzle\", \"magic_key\", \"pick_lock\", \"break\"]", "\"solutions\": [\"magic_key\", \"pick_lock\", \"break\"]")), std::string("PUZZLE_EDGE"));
    CHECK_EQ(mapIssue(real, mutate(rm, "\"solutions\": [\"puzzle\", \"magic_key\", \"pick_lock\", \"break\"]", "\"solutions\": [\"puzzle\", \"magic_key\", \"pick_lock\"]")), std::string("ROUTES_MISMATCH"));
    CHECK_EQ(mapIssue(real, mutate(rm, "\"controls\": [\"ctl_pick\", \"ctl_cart\", \"ctl_furnace\"]", "\"controls\": [\"ctl_pick\", \"ctl_cart\"]")), std::string("CONTROLS_MISMATCH"));
    CHECK_EQ(mapIssue(real, "{"), std::string("MAP:JSON_TRUNCATED"));
}

TEST(pzl_all_orderings) {
    std::vector<std::string> perm = kSol;
    std::sort(perm.begin(), perm.end());
    int n = 0;
    do {
        Puzzle p = fresh();
        pressAll(p, perm);
        ++n;
        if (perm == kSol) CHECK(p.doorOpen() && p.solved());
        else CHECK(!p.doorOpen() && !p.solved());
        // No softlock: after any 3 presses the right order still opens the door within two tries.
        for (int i = 0; i < 2; ++i) pressAll(p, kSol);
        CHECK(p.doorOpen());
        CHECK_EQ(p.openedVia(), std::string("puzzle"));
    } while (std::next_permutation(perm.begin(), perm.end()));
    CHECK_EQ(n, 6);
}

TEST(pzl_oracle_sequences) {
    // Every sequence of length 0..4 (121): C++ against a three-line oracle of the reference rule.
    std::size_t count = 0;
    for (const auto& seq : allSequences(4)) {
        Puzzle p = fresh();
        std::size_t prog = 0;
        long long wrong = 0;
        bool solved = false;
        for (const std::string& c : seq) {
            UseOutcome o = p.use(c);
            if (solved) {
                CHECK_EQ(o.resultKey, std::string("inert"));
            } else if (c == kSol[prog]) {
                ++prog;
                if (prog == kSol.size()) { solved = true; CHECK_EQ(o.resultKey, std::string("solved")); }
                else {
                    CHECK_EQ(o.resultKey, std::string("step_ok"));
                    CHECK_EQ(o.stringKey, "str.use.step_" + std::to_string(prog));
                }
            } else {
                prog = 0;
                ++wrong;
                CHECK_EQ(o.resultKey, std::string("wrong_reset"));
                CHECK_EQ(o.stringKey, std::string("str.use.wrong"));
            }
        }
        CHECK_EQ(p.progress().size(), solved ? kSol.size() : prog);
        CHECK_EQ(p.wrongCount(), wrong);
        CHECK_EQ(p.solved(), solved);
        CHECK_EQ(p.doorOpen(), solved);
        ++count;
    }
    CHECK_EQ(count, (std::size_t)121);
}

TEST(pzl_repeat_and_reset) {
    Puzzle p = fresh();
    CHECK(p.use("ctl_pick").result == UseResult::StepOk);
    CHECK(p.use("ctl_pick").result == UseResult::WrongReset);   // repeating is wrong
    CHECK_EQ(p.summary(), std::string("state=idle door_open=0 opened_via=none wrong=1 progress="));
    p.use("ctl_pick");
    CHECK(p.use("ctl_furnace").result == UseResult::WrongReset);
    CHECK(p.progress().empty());
    CHECK_EQ(p.wrongCount(), (long long)2);
    UseOutcome r{};
    for (const std::string& c : kSol) r = p.use(c);
    CHECK(r.result == UseResult::Solved);
    // reset(): clears partial progress only; never re-locks.
    Puzzle q = fresh();
    q.use("ctl_pick");
    q.reset();
    CHECK_EQ(std::string(Puzzle::stateName(q.state())), std::string("idle"));
    pressAll(q, kSol);
    q.reset();
    CHECK(q.doorOpen());
    CHECK(q.solved());
}

TEST(pzl_solve) {
    Puzzle p = fresh();
    UseOutcome a = p.use("ctl_pick"), b = p.use("ctl_cart"), c = p.use("ctl_furnace");
    CHECK_EQ(a.resultKey + " " + a.stringKey, std::string("step_ok str.use.step_1"));
    CHECK_EQ(b.resultKey + " " + b.stringKey, std::string("step_ok str.use.step_2"));
    CHECK_EQ(c.resultKey + " " + c.stringKey, std::string("solved str.use.solved"));
    CHECK_EQ(p.summary(), std::string("state=solved door_open=1 opened_via=puzzle wrong=0 progress=ctl_pick,ctl_cart,ctl_furnace"));
    for (const std::string& k : kSol) {
        UseOutcome o = p.use(k);
        CHECK(o.result == UseResult::Inert);
        CHECK_EQ(o.stringKey, std::string("str.use.inert"));
    }
    CHECK_EQ(p.summary(), std::string("state=solved door_open=1 opened_via=puzzle wrong=0 progress=ctl_pick,ctl_cart,ctl_furnace"));
    for (auto it = kSol.rbegin(); it != kSol.rend(); ++it) CHECK(p.use(*it).result == UseResult::Inert);
}

TEST(pzl_no_lockout) {
    Puzzle p = fresh();
    for (int i = 0; i < 20; ++i) CHECK(p.use("ctl_cart").result == UseResult::WrongReset);
    CHECK_EQ(p.wrongCount(), (long long)20);
    CHECK(!p.doorOpen());
    pressAll(p, kSol);
    CHECK(p.solved());
    CHECK(p.doorOpen());
    CHECK_EQ(p.wrongCount(), (long long)20);   // wrong presses cost nothing but the count
    // also after 20 mixed wrong presses
    Puzzle q = fresh();
    for (int i = 0; i < 10; ++i) { q.use("ctl_furnace"); q.use("ctl_pick"); q.use("ctl_pick"); }
    pressAll(q, kSol);
    pressAll(q, kSol);
    CHECK(q.solved());
}

TEST(pzl_unknown_control) {
    Puzzle p = fresh();
    p.use("ctl_pick");
    const std::string before = p.toJson();
    for (const char* c : {"ctl_door", "obj_carving", "ctl_lever", ""}) {
        UseOutcome o = p.use(c);
        CHECK(o.result == UseResult::UnknownControl);
        CHECK(o.stringKey.empty());
        CHECK_EQ(p.toJson(), before);
    }
    Puzzle s = fresh();
    pressAll(s, kSol);
    CHECK(s.use("ctl_door").result == UseResult::UnknownControl);   // unknown beats inert, as in the reference
    Puzzle b = fresh();
    b.openOther("break");
    CHECK(b.use("ctl_door").result == UseResult::UnknownControl);
}

TEST(pzl_bypass_routes) {
    for (const char* route : kRoutes) {
        Puzzle p = fresh();
        CHECK(p.openOther(route) == BypassResult::Applied);
        CHECK(p.doorOpen());
        CHECK_EQ(p.openedVia(), std::string(route));
        CHECK(!p.solved());
        CHECK_EQ(std::string(Puzzle::stateName(p.state())), std::string("bypassed"));
        CHECK_EQ(p.summary(), std::string("state=bypassed door_open=1 opened_via=") + route + " wrong=0 progress=");
        for (const std::string& c : kSol) {
            UseOutcome o = p.use(c);
            CHECK_EQ(o.resultKey + " " + o.stringKey, std::string("inert str.use.inert"));
        }
        CHECK_EQ(std::string(Puzzle::stateName(p.state())), std::string("bypassed"));
        CHECK(p.claimXp().empty());
    }
    Puzzle p = fresh();
    CHECK(p.openOther("puzzle") == BypassResult::UnknownRoute);   // "puzzle" is not a bypass route
    CHECK(p.openOther("kick") == BypassResult::UnknownRoute);
    CHECK(p.openOther("") == BypassResult::UnknownRoute);
    CHECK(!p.doorOpen());
}

TEST(pzl_bypass_after_progress) {
    for (const char* route : kRoutes) {
        Puzzle p = fresh();
        p.use("ctl_pick");
        p.use("ctl_cart");
        CHECK(p.openOther(route) == BypassResult::Applied);
        CHECK(p.progress().empty());
        CHECK(p.use("ctl_furnace").result == UseResult::Inert);
        CHECK_EQ(std::string(Puzzle::stateName(p.state())), std::string("bypassed"));
        CHECK_EQ(p.openedVia(), std::string(route));
    }
    // after wrong presses too: wrong count is kept
    Puzzle q = fresh();
    q.use("ctl_cart");
    q.openOther("break");
    CHECK_EQ(q.wrongCount(), (long long)1);
}

TEST(pzl_second_open) {
    for (const char* route : kRoutes) {
        Puzzle p = fresh();
        pressAll(p, kSol);
        CHECK(p.openOther(route) == BypassResult::NotApplied);
        CHECK_EQ(p.openedVia(), std::string("puzzle"));
        CHECK_EQ(std::string(Puzzle::stateName(p.state())), std::string("solved"));
        CHECK_EQ(p.summary(), std::string("state=solved door_open=1 opened_via=puzzle wrong=0 progress=ctl_pick,ctl_cart,ctl_furnace"));
    }
    // a second bypass after a bypass is also not applied, and keeps the first route
    Puzzle q = fresh();
    q.openOther("magic_key");
    CHECK(q.openOther("break") == BypassResult::NotApplied);
    CHECK_EQ(q.openedVia(), std::string("magic_key"));
}

TEST(pzl_solve_after_bypass) {
    for (const char* route : kRoutes) {
        Puzzle p = fresh();
        p.openOther(route);
        pressAll(p, kSol);
        pressAll(p, kSol);
        CHECK(!p.solved());
        CHECK_EQ(p.openedVia(), std::string(route));
        CHECK(p.progress().empty());
        CHECK(p.claimXp().empty());
    }
}

TEST(pzl_roundtrip_prefixes) {
    // Save and load at every sequence of length 0..4 (every prefix of the correct order and every wrong
    // order); the loaded puzzle then behaves exactly like the original for the rest of the play.
    for (const auto& seq : allSequences(4)) {
        Puzzle p = fresh();
        pressAll(p, seq);
        Puzzle q = loaded(p);
        CHECK_EQ(q.toJson(), p.toJson());
        CHECK_EQ(q.summary(), p.summary());
        for (const std::string& c : kSol) {
            UseOutcome a = p.use(c), b = q.use(c);
            CHECK_EQ(a.resultKey + a.stringKey, b.resultKey + b.stringKey);
        }
        CHECK_EQ(q.toJson(), p.toJson());
    }
    Puzzle p = fresh();
    p.use("ctl_pick");
    p.use("ctl_cart");
    Puzzle q = loaded(p);
    CHECK_EQ(q.summary(), std::string("state=partial door_open=0 opened_via=none wrong=0 progress=ctl_pick,ctl_cart"));
    CHECK(q.use("ctl_furnace").result == UseResult::Solved);
    Puzzle r = loaded(q);
    CHECK_EQ(std::string(Puzzle::stateName(r.state())), std::string("solved"));
    CHECK_EQ(saveOf(p).substr(0, 10), std::string("{\"progress"));
}

TEST(pzl_roundtrip_bypass) {
    for (const char* route : kRoutes) {
        Puzzle p = fresh();
        p.use("ctl_pick");
        p.openOther(route);
        Puzzle q = loaded(p);
        CHECK_EQ(q.toJson(), p.toJson());
        CHECK(q.doorOpen());
        CHECK_EQ(q.openedVia(), std::string(route));
        CHECK_EQ(std::string(Puzzle::stateName(q.state())), std::string("bypassed"));
        CHECK(q.use("ctl_pick").result == UseResult::Inert);
        CHECK(q.openOther("magic_key") == BypassResult::NotApplied);
        CHECK(q.claimXp().empty());
    }
    Puzzle p = fresh();
    pressAll(p, kSol);
    Puzzle q = loaded(p);
    CHECK_EQ(q.openedVia(), std::string("puzzle"));
    CHECK(q.solved());
}

TEST(pzl_persist_open) {
    // The puzzle has no input for the calendar: a day passing cannot reach it. What can happen between
    // sessions is save and load, repeated, plus the debug reset. None of it re-locks the door.
    for (int variant = 0; variant < 4; ++variant) {
        Puzzle p = fresh();
        if (variant == 0) pressAll(p, kSol);
        else p.openOther(kRoutes[variant - 1]);
        const std::string open = p.toJson();
        Puzzle q = p;
        for (int i = 0; i < 5; ++i) {
            q = loaded(q);
            q.reset();
            CHECK(q.doorOpen());
            CHECK_EQ(q.toJson(), open);
        }
    }
}

TEST(pzl_xp_claim) {
    Puzzle p = fresh();
    CHECK(p.claimXp().empty());               // nothing before a solve
    p.use("ctl_pick");
    CHECK(p.claimXp().empty());
    pressAll(p, {"ctl_cart", "ctl_furnace"});
    CHECK_EQ(p.claimXp(), std::string("puzzle_solve"));
    CHECK(p.claimXp().empty());               // once only
    pressAll(p, kSol);
    CHECK(p.claimXp().empty());
    Puzzle q = loaded(p);                     // the claim survives save and load
    CHECK(q.xpAwarded());
    CHECK(q.claimXp().empty());
    // saved after the solve, before the claim: still claimable once after load
    Puzzle r = fresh();
    pressAll(r, kSol);
    Puzzle s = loaded(r);
    CHECK_EQ(s.claimXp(), std::string("puzzle_solve"));
    CHECK(s.claimXp().empty());
    // never after a bypass, even if the solve sequence is pressed afterwards
    for (const char* route : kRoutes) {
        Puzzle b = fresh();
        b.openOther(route);
        pressAll(b, kSol);
        CHECK(b.claimXp().empty());
    }
}

TEST(pzl_bad_state_text) {
    const std::string F = "SAVE_FIELD", V = "SAVE_VALUE", S = "SAVE_STATE";
    const std::string ok = stateText("\"ctl_pick\"", "false", "false", "null", "2", "false");
    CHECK_EQ(loadCode(ok), std::string());
    // text that is not JSON
    CHECK_EQ(loadCode(""), std::string("JSON_EMPTY"));
    CHECK(loadCode("{\"progress\":[").rfind("JSON_", 0) == 0);
    CHECK(loadCode("not json").rfind("JSON_", 0) == 0);
    CHECK(loadCode(ok + " x").rfind("JSON_", 0) == 0);
    // shape
    CHECK_EQ(loadCode("[]"), F);
    CHECK_EQ(loadCode("{}"), F);
    for (const char* f : {"progress", "solved", "door_open", "opened_via", "wrong_count", "xp_awarded"}) {
        std::string t = ok;
        std::size_t at = t.find(std::string("\"") + f + "\"");
        CHECK(at != std::string::npos);
        t.replace(at, std::string(f).size() + 2, std::string("\"x_") + f + "\"");   // renamed: one missing, one unknown
        CHECK_EQ(loadCode(t), F);
    }
    CHECK_EQ(loadCode(mutate(ok, "}", ",\"extra\":1}")), F);
    CHECK_EQ(loadCode(stateText("1", "false", "false", "null", "0", "false")), F);
    CHECK_EQ(loadCode(stateText("", "0", "false", "null", "0", "false")), F);
    CHECK_EQ(loadCode(stateText("", "false", "\"no\"", "null", "0", "false")), F);
    CHECK_EQ(loadCode(stateText("", "false", "false", "5", "0", "false")), F);
    CHECK_EQ(loadCode(stateText("", "false", "false", "null", "\"0\"", "false")), F);
    CHECK_EQ(loadCode(stateText("", "false", "false", "null", "1.5", "false")), F);
    CHECK_EQ(loadCode(stateText("", "false", "false", "null", "0", "null")), F);
    // values
    CHECK_EQ(loadCode(stateText("", "false", "true", "\"kick\"", "0", "false")), V);
    CHECK_EQ(loadCode(stateText("", "false", "true", "\"\"", "0", "false")), V);
    CHECK_EQ(loadCode(stateText("\"ctl_pick\",\"ctl_cart\",\"ctl_furnace\",\"ctl_pick\"", "false", "false", "null", "0", "false")), V);
    CHECK_EQ(loadCode(stateText("\"ctl_lever\"", "false", "false", "null", "0", "false")), V);
    CHECK_EQ(loadCode(stateText("", "false", "false", "null", "-1", "false")), V);
    // state: contradictions with the rules
    CHECK_EQ(loadCode(stateText("\"ctl_pick\",\"ctl_cart\",\"ctl_furnace\"", "true", "false", "\"puzzle\"", "0", "false")), S);
    CHECK_EQ(loadCode(stateText("\"ctl_pick\",\"ctl_cart\",\"ctl_furnace\"", "true", "true", "null", "0", "false")), S);
    CHECK_EQ(loadCode(stateText("\"ctl_pick\",\"ctl_cart\",\"ctl_furnace\"", "false", "true", "\"puzzle\"", "0", "false")), S);  // opened by puzzle but not solved
    CHECK_EQ(loadCode(stateText("\"ctl_pick\"", "true", "true", "\"puzzle\"", "0", "false")), S);                              // solved, progress short
    CHECK_EQ(loadCode(stateText("", "false", "true", "null", "0", "false")), S);                                                // door open, no route
    CHECK_EQ(loadCode(stateText("", "false", "false", "\"break\"", "0", "false")), S);                                         // route, door closed
    CHECK_EQ(loadCode(stateText("\"ctl_pick\"", "false", "true", "\"break\"", "0", "false")), S);                               // bypassed with progress
    CHECK_EQ(loadCode(stateText("", "true", "true", "\"break\"", "0", "false")), S);                                            // solved by a bypass
    CHECK_EQ(loadCode(stateText("\"ctl_cart\"", "false", "false", "null", "0", "false")), S);                                   // not a prefix
    CHECK_EQ(loadCode(stateText("\"ctl_pick\",\"ctl_cart\",\"ctl_furnace\"", "false", "false", "null", "0", "false")), S);       // full but unsolved
    CHECK_EQ(loadCode(stateText("", "false", "false", "null", "0", "true")), S);                                                // xp without a solve
    CHECK_EQ(loadCode(stateText("", "false", "true", "\"break\"", "0", "true")), S);                                            // xp after a bypass
    // and the consistent ones are accepted
    CHECK_EQ(loadCode(stateText("", "false", "true", "\"break\"", "3", "false")), std::string());
    CHECK_EQ(loadCode(stateText("\"ctl_pick\",\"ctl_cart\",\"ctl_furnace\"", "true", "true", "\"puzzle\"", "0", "true")), std::string());
}

TEST(pzl_state_fixtures) {
    CHECK_EQ(loadCode(fixture("save_partial.json")), std::string());
    CHECK_EQ(loadCode(fixture("save_missing.json")), std::string("SAVE_FIELD"));
    CHECK_EQ(loadCode(fixture("save_inconsistent.json")), std::string("SAVE_STATE"));
    CHECK_EQ(loadCode(fixture("save_bad_via.json")), std::string("SAVE_VALUE"));
    CHECK_EQ(loadCode(fixture("save_long_progress.json")), std::string("SAVE_VALUE"));
    CHECK_EQ(loadCode(fixture("save_unknown_control.json")), std::string("SAVE_VALUE"));
    Data d;
    Issue err;
    CHECK(dataFromText(fixture("bad_edge.json"), d, err));
    CHECK(!validateAgainstMap(d, realMap()).empty());
    CHECK_EQ(validateAgainstMap(d, realMap())[0].code, std::string("PUZZLE_EDGE"));
}

TEST(pzl_load_keeps_state_on_reject) {
    Puzzle p = fresh();
    p.use("ctl_pick");
    p.use("ctl_furnace");
    p.use("ctl_pick");
    const std::string before = p.toJson();
    Issue err;
    CHECK(!p.loadState(fixture("save_inconsistent.json"), err));
    CHECK_EQ(p.toJson(), before);
    CHECK(!p.loadState("garbage", err));
    CHECK_EQ(p.toJson(), before);
    CHECK(!p.loadState(fixture("save_bad_via.json"), err));
    CHECK_EQ(err.code, std::string("SAVE_VALUE"));
    CHECK_EQ(p.toJson(), before);
    CHECK(p.loadState(fixture("save_partial.json"), err));   // a good one replaces it
    CHECK_EQ(p.summary(), std::string("state=partial door_open=0 opened_via=none wrong=1 progress=ctl_pick,ctl_cart"));
}

TEST(pzl_ref_python_cases) {
    // test_partial
    Puzzle p = fresh();
    CHECK(p.use("ctl_pick").result == UseResult::StepOk);
    CHECK_EQ(std::string(Puzzle::stateName(p.state())), std::string("partial"));
    p.use("ctl_cart");
    CHECK(!p.doorOpen());
    // test_wrong_then_right
    Puzzle w = fresh();
    w.use("ctl_pick");
    CHECK(w.use("ctl_furnace").result == UseResult::WrongReset);
    CHECK(w.progress().empty());
    CHECK_EQ(w.wrongCount(), (long long)1);
    UseOutcome r{};
    for (const std::string& c : kSol) r = w.use(c);
    CHECK(r.result == UseResult::Solved);
    // test_break_after_partial_progress / test_break_save_load
    Puzzle b = fresh();
    b.use("ctl_pick");
    CHECK(b.openOther("break") == BypassResult::Applied);
    CHECK(b.progress().empty());
    CHECK(b.use("ctl_cart").result == UseResult::Inert);
    Puzzle bq = loaded(b);
    CHECK_EQ(std::string(Puzzle::stateName(bq.state())), std::string("bypassed"));
    CHECK_EQ(bq.openedVia(), std::string("break"));
    CHECK(bq.use("ctl_pick").result == UseResult::Inert);
    // test_save_load: pick, furnace, pick; save; load; cart, furnace; save; load
    Puzzle s = fresh();
    s.use("ctl_pick");
    s.use("ctl_furnace");
    s.use("ctl_pick");
    Puzzle q = loaded(s);
    CHECK_EQ(q.toJson(), s.toJson());
    q.use("ctl_cart");
    q.use("ctl_furnace");
    CHECK(q.doorOpen());
    CHECK_EQ(std::string(Puzzle::stateName(loaded(q).state())), std::string("solved"));
    // test_other_routes_strings_match_json: route list order and content equal the map's e08 minus puzzle
    CHECK((fresh().data().otherRoutes == std::vector<std::string>{"magic_key", "pick_lock", "break"}));
    // test_json_ids_in_map and e08's solutions = puzzle + other_routes
    dd::map::FloorMap m = realMap();
    const dd::map::Edge* e = m.edge("e08");
    CHECK(e != nullptr);
    if (e) {
        CHECK_EQ(e->door, std::string("locked"));
        std::vector<std::string> expect = {"puzzle"};
        const Puzzle owner = fresh();
        for (const std::string& rt : owner.data().otherRoutes) expect.push_back(rt);
        CHECK(e->solutions == expect);
    }
}

TEST(pzl_strings_exist) {
    Puzzle p = fresh();
    for (const std::string& c : std::vector<std::string>{"ctl_pick", "ctl_cart", "ctl_furnace", "ctl_pick", "ctl_pick"}) {
        UseOutcome o = p.use(c);
        const std::string* t = p.data().text(o.stringKey);
        CHECK(t != nullptr);
        if (t) CHECK(!t->empty());
    }
    CHECK(p.data().text("str.log.door_other") != nullptr);
    CHECK(p.data().text("str.nope") == nullptr);
}

namespace {
int runCli(const std::vector<std::string>& a, std::string& output) {
    std::ostringstream os;
    int rc = puzzleCheckMain(a, os);
    output = os.str();
    return rc;
}
std::string lastLine(const std::string& s) {
    std::size_t end = s.size();
    if (end && s[end - 1] == '\n') --end;
    std::size_t start = s.rfind('\n', end ? end - 1 : 0);
    start = (start == std::string::npos || end == 0) ? 0 : start + 1;
    return s.substr(start, end - start);
}
}  // namespace

TEST(pzl_cli_functions) {
    const std::string P = std::string(DD_REPO_ROOT) + "/work/T001/puzzle/storeroom_puzzle.json";
    const std::string M = std::string(DD_REPO_ROOT) + "/work/T001/map/floor1_map.json";
    const std::string FX = std::string(DD_TEST_SRC) + "/tests/puzzle/fixtures/";
    std::string out;
    struct Row { std::vector<std::string> seq; std::string last; };
    const Row rows[] = {
        {{"ctl_pick", "ctl_cart", "ctl_furnace"}, "state=solved door_open=1 opened_via=puzzle wrong=0 progress=ctl_pick,ctl_cart,ctl_furnace"},
        {{"ctl_pick", "ctl_furnace", "ctl_cart"}, "state=idle door_open=0 opened_via=none wrong=2 progress="},
        {{"ctl_cart", "ctl_pick", "ctl_furnace"}, "state=idle door_open=0 opened_via=none wrong=2 progress="},
        {{"ctl_cart", "ctl_furnace", "ctl_pick"}, "state=partial door_open=0 opened_via=none wrong=2 progress=ctl_pick"},
        {{"ctl_furnace", "ctl_pick", "ctl_cart"}, "state=partial door_open=0 opened_via=none wrong=1 progress=ctl_pick,ctl_cart"},
        {{"ctl_furnace", "ctl_cart", "ctl_pick"}, "state=partial door_open=0 opened_via=none wrong=2 progress=ctl_pick"},
        {{"ctl_pick", "ctl_pick"}, "state=idle door_open=0 opened_via=none wrong=1 progress="},
        {{"ctl_cart", "ctl_pick", "ctl_cart", "ctl_furnace"}, "state=solved door_open=1 opened_via=puzzle wrong=1 progress=ctl_pick,ctl_cart,ctl_furnace"},
        {{"ctl_pick", "ctl_cart", "ctl_furnace", "ctl_pick"}, "state=solved door_open=1 opened_via=puzzle wrong=0 progress=ctl_pick,ctl_cart,ctl_furnace"},
    };
    for (const Row& r : rows) {
        std::vector<std::string> a = {"seq", P};
        a.insert(a.end(), r.seq.begin(), r.seq.end());
        CHECK_EQ(runCli(a, out), 0);
        CHECK_EQ(lastLine(out), r.last);
    }
    // step keys printed (PZL-06) and inert (PZL-15)
    runCli({"seq", P, "ctl_pick", "ctl_cart", "ctl_furnace", "ctl_pick"}, out);
    CHECK(out.find("ctl_pick step_ok str.use.step_1\nctl_cart step_ok str.use.step_2\nctl_furnace solved str.use.solved\nctl_pick inert str.use.inert\n") == 0);
    runCli({"seq", P, "ctl_pick", "ctl_furnace"}, out);
    CHECK(out.find("ctl_furnace wrong_reset str.use.wrong\n") != std::string::npos);
    // unknown control
    CHECK_EQ(runCli({"seq", P, "ctl_door"}, out), 1);
    CHECK(out.rfind("FAIL UNKNOWN_CONTROL", 0) == 0);
    // bypass
    for (const char* route : kRoutes) {
        CHECK_EQ(runCli({"bypass", P, route}, out), 0);
        CHECK_EQ(lastLine(out), std::string("state=bypassed door_open=1 opened_via=") + route + " wrong=0 progress=");
    }
    CHECK_EQ(runCli({"bypass", P, "magic_key", "ctl_pick"}, out), 0);   // PZL-19
    CHECK(out.find("ctl_pick inert str.use.inert\nstate=bypassed") != std::string::npos);
    CHECK_EQ(lastLine(out), std::string("state=bypassed door_open=1 opened_via=magic_key wrong=0 progress="));
    CHECK_EQ(runCli({"bypass", P, "break", "ctl_pick", "ctl_cart", "ctl_furnace"}, out), 0);   // after solve: not applied
    CHECK(out.find("route break not_applied") != std::string::npos);
    CHECK_EQ(lastLine(out), std::string("state=solved door_open=1 opened_via=puzzle wrong=0 progress=ctl_pick,ctl_cart,ctl_furnace"));
    CHECK_EQ(runCli({"bypass", P, "kick"}, out), 1);
    CHECK(out.rfind("FAIL UNKNOWN_ROUTE", 0) == 0);
    CHECK_EQ(runCli({"bypass", P, "puzzle"}, out), 1);
    // roundtrip
    CHECK_EQ(runCli({"roundtrip", P, "ctl_pick", "ctl_cart"}, out), 0);
    CHECK_EQ(lastLine(out), std::string("state=partial door_open=0 opened_via=none wrong=0 progress=ctl_pick,ctl_cart"));
    CHECK_EQ(runCli({"roundtrip", P}, out), 0);
    CHECK_EQ(lastLine(out), std::string("state=idle door_open=0 opened_via=none wrong=0 progress="));
    CHECK_EQ(runCli({"roundtrip", P, "ctl_door"}, out), 1);
    // load
    CHECK_EQ(runCli({"load", P, FX + "save_partial.json"}, out), 0);
    CHECK_EQ(lastLine(out), std::string("state=partial door_open=0 opened_via=none wrong=1 progress=ctl_pick,ctl_cart"));
    const std::pair<const char*, const char*> bad[] = {{"save_missing.json", "FAIL SAVE_FIELD"}, {"save_inconsistent.json", "FAIL SAVE_STATE"},
        {"save_bad_via.json", "FAIL SAVE_VALUE"}, {"save_long_progress.json", "FAIL SAVE_VALUE"}, {"save_unknown_control.json", "FAIL SAVE_VALUE"}};
    for (const auto& b : bad) {
        CHECK_EQ(runCli({"load", P, FX + b.first}, out), 1);
        CHECK(out.rfind(b.second, 0) == 0);
    }
    // validate
    CHECK_EQ(runCli({"validate", P, M}, out), 0);
    CHECK_EQ(lastLine(out), std::string("OK puzzle controls=3 solution=3"));
    CHECK_EQ(runCli({"validate", FX + "bad_edge.json", M}, out), 1);
    CHECK(out.rfind("FAIL PUZZLE_EDGE", 0) == 0);
    CHECK_EQ(runCli({"validate", P, FX + "save_partial.json"}, out), 1);   // not a map
    CHECK(out.rfind("FAIL", 0) == 0);
    CHECK_EQ(runCli({"validate", FX + "save_partial.json", M}, out), 1);   // not a puzzle
    CHECK(out.rfind("FAIL", 0) == 0);
    // usage and I/O: exit 2
    CHECK_EQ(runCli({}, out), 2);
    CHECK_EQ(runCli({"seq"}, out), 2);
    CHECK_EQ(runCli({"nope", P}, out), 2);
    CHECK_EQ(runCli({"validate", P}, out), 2);
    CHECK_EQ(runCli({"load", P}, out), 2);
    CHECK_EQ(runCli({"bypass", P}, out), 2);
    CHECK_EQ(runCli({"seq", "/no/such/file.json", "ctl_pick"}, out), 2);
    CHECK_EQ(runCli({"load", P, "/no/such/state.json"}, out), 2);
}

int main(int argc, char** argv) { return dd_test::runMain(argc, argv); }
