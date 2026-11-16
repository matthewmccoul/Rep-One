#include <sys/wait.h>

#include <cstdio>
#include <set>

#include "../testing.h"
#include "dd/csv.h"
#include "dd/fileio.h"
#include "dd/json.h"
#include "dd/map.h"
#include "dd/quest.h"

using namespace dd::quest;

namespace {

std::string root() { return DD_REPO_ROOT; }
std::string questPath() { return root() + "/work/T001/quest/figure_ore_lift_quest.json"; }
std::string csvPath() { return root() + "/work/T001/quest/figure_ore_lift_strings.csv"; }
std::string mapPath() { return root() + "/work/T001/map/floor1_map.json"; }
std::string fixPath(const std::string& n) { return root() + "/work/T001/code/tests/quest/fixtures/" + n; }

std::string slurp(const std::string& path) {
    std::string t;
    if (!dd::readFile(path, t)) { std::cerr << "cannot read " << path << "\n"; std::exit(2); }
    return t;
}

std::string mutate(std::string text, const std::string& from, const std::string& to) {
    std::size_t p = text.find(from);
    if (p == std::string::npos) {
        dd_test::fail(__FILE__, __LINE__, "mutation target not found: " + from);
        return text;
    }
    text.replace(p, from.size(), to);
    return text;
}

std::string dropRow(std::string csv, const std::string& key) {
    std::size_t p = csv.find("\n" + key + ",");
    if (p == std::string::npos) { dd_test::fail(__FILE__, __LINE__, "row not found: " + key); return csv; }
    std::size_t e = csv.find('\n', p + 1);
    csv.erase(p, e - p);
    return csv;
}

struct Data {
    std::string questJson, csv, mapText;
    QuestData q;
    dd::map::FloorMap m;
};

Data real() {
    Data d;
    d.questJson = slurp(questPath());
    d.csv = slurp(csvPath());
    d.mapText = slurp(mapPath());
    dd::map::Issue me;
    if (!dd::map::fromText(d.mapText, d.m, me)) { std::cerr << "map: " << me.code << "\n"; std::exit(2); }
    Issue e;
    if (!load(d.questJson, d.csv, d.q, e)) { std::cerr << "quest: " << e.code << " " << e.detail << "\n"; std::exit(2); }
    return d;
}

// First issue code for a quest/csv/map text triple ("" = accepted; "LOAD:<code>" for parse failures).
std::string firstCode(const std::string& qj, const std::string& csv, const std::string& mapText) {
    dd::map::FloorMap m;
    dd::map::Issue me;
    if (!dd::map::fromText(mapText, m, me)) return "MAP:" + me.code;
    QuestData q;
    Issue e;
    if (!load(qj, csv, q, e)) return "LOAD:" + e.code;
    std::vector<Issue> is = validate(q, m);
    return is.empty() ? "" : is[0].code;
}

class Fixed : public RewardSource {
public:
    double value = 0.999999;
    int calls = 0;
    double roll() override { ++calls; return value; }
};

const char* stateOf(const Engine& e, const std::string& ch) { return stateName(e.record(ch).state); }

std::string keyOf(const Step& s, std::size_t i = 0) { return i < s.lines.size() ? s.lines[i].key : std::string("<none>"); }

// Line text by index; "" when the line is missing, so a failed CHECK never dereferences an empty result.
std::string textOf(const Step& s, std::size_t i) { return i < s.lines.size() ? s.lines[i].text : std::string(); }

// Offers at `stage`, accepts, arrives at the lift with the chain value, leaving the character observing.
void toObserved(Engine& e, const std::string& ch, int stage, const std::string& chain) {
    Context c;
    CHECK(e.offer(ch, stage, c).ok);
    CHECK(e.accept(ch, stage, c).ok);
    CHECK(e.arrive(ch, "ore_lift", chain, c).ok);
}

}  // namespace

// ---- data and bands --------------------------------------------------------------------------
TEST(quest_real_validates) {
    Data d = real();
    std::vector<Issue> is = validate(d.q, d.m);
    for (const Issue& i : is) std::cerr << "issue " << i.code << " " << i.detail << "\n";
    CHECK(is.empty());
    CHECK_EQ(d.q.strings.rows.size(), static_cast<std::size_t>(25));
    CHECK_EQ(d.q.giverNode, std::string("sewer_grate"));
    CHECK_EQ(d.q.destNode, std::string("ore_lift"));
    CHECK_EQ(d.q.route.size(), static_cast<std::size_t>(5));
}

TEST(quest_stage_bands) {
    const int score[] = {0, 19, 20, 39, 40, 59, 60, 79, 80, 100};
    const int want[] = {-2, -2, -1, -1, 0, 0, 1, 1, 2, 2};
    for (int i = 0; i < 10; ++i) {
        int st = 99;
        CHECK(stageFromScore(score[i], st));
        CHECK_EQ(st, want[i]);
    }
    int st = 77;
    CHECK(!stageFromScore(-1, st));
    CHECK(!stageFromScore(101, st));
    CHECK_EQ(st, 77);
}

// Every row, every stage: the row is found exactly inside its CSV range, and the filled text equals
// an independent substitution of the same CSV text.
TEST(quest_dialogue_agrees_csv) {
    Data d = real();
    dd::csv::Table t;
    dd::csv::Error ce;
    CHECK(dd::csv::parse(d.csv, t, ce));
    int ck = t.column("key"), c0 = t.column("stage_min"), c1 = t.column("stage_max"), ct = t.column("text");
    CHECK_EQ(t.rows.size(), static_cast<std::size_t>(25));
    auto rawText = [&](const std::string& key) {
        for (const auto& r : t.rows) if (r[ck] == key) return r[ct];
        return std::string("<missing>");
    };
    auto sub = [](std::string s, const std::string& name, const std::string& val) {
        std::string pat = "{" + name + "}";
        for (std::size_t p; (p = s.find(pat)) != std::string::npos;) s.replace(p, pat.size(), val);
        return s;
    };
    std::map<std::string, std::string> vals;
    vals["node_name"] = "Old lift";
    vals["day"] = "7";
    vals["days_since"] = "12";
    vals["item"] = "thing";
    vals["reward_item"] = "Brass_Key";
    vals["reward_line"] = "@fig_reward_item";
    for (const auto& r : t.rows) {
        int lo = std::atoi(r[c0].c_str()), hi = std::atoi(r[c1].c_str());
        for (int stage = -2; stage <= 2; ++stage) {
            const StringRow* row = d.q.strings.forStage(r[ck], stage);
            CHECK((row != nullptr) == (stage >= lo && stage <= hi));
            if (!row) continue;
            CHECK_EQ(row->text, r[ct]);
            std::string want = r[ct];
            for (const auto& kv : vals) {
                if (kv.first == "reward_line") continue;
                want = sub(want, kv.first, kv.second);
            }
            want = sub(want, "reward_line", sub(rawText("fig_reward_item"), "reward_item", "Brass_Key"));
            std::string got;
            Issue e;
            CHECK(fillLine(d.q.strings, r[ck], stage, vals, got, e));
            CHECK_EQ(got, want);
            CHECK(got.find('{') == std::string::npos && got.find('}') == std::string::npos);
        }
    }
}

// ---- blank filling -----------------------------------------------------------------------------
TEST(quest_fill_all) {
    Data d = real();
    std::map<std::string, std::string> vals;
    vals["node_name"] = "Lift";
    vals["day"] = "3";
    vals["days_since"] = "9";
    vals["item"] = "x";
    vals["reward_item"] = "Key";
    vals["reward_line"] = "@fig_reward_item";
    int filled = 0;
    for (const StringRow& r : d.q.strings.rows) {
        for (int stage = r.stageMin; stage <= r.stageMax; ++stage) {
            std::string out;
            Issue e;
            CHECK(fillLine(d.q.strings, r.key, stage, vals, out, e));
            CHECK(out.find('{') == std::string::npos);
            // a missing value for each blank in the row is rejected, naming the blank
            for (const std::string& b : r.blanks) {
                std::map<std::string, std::string> less = vals;
                less.erase(b);
                std::string o2;
                Issue e2;
                CHECK(!fillLine(d.q.strings, r.key, stage, less, o2, e2));
                CHECK_EQ(e2.code, std::string("UNFILLED_BLANK"));
                if (b != "reward_line") CHECK_EQ(e2.detail, b);
            }
            ++filled;
        }
    }
    CHECK(filled >= 25);
}

TEST(quest_fill_rules) {
    Data d = real();
    std::string out;
    Issue e;
    std::map<std::string, std::string> v;
    v["day"] = "7";
    CHECK(fillLine(d.q.strings, "fig_repeat_done", 0, v, out, e));
    CHECK(out.find("Day 7.") != std::string::npos);
    // a supplied value is not expanded again
    v["day"] = "{day}";
    CHECK(fillLine(d.q.strings, "fig_repeat_done", 0, v, out, e));
    CHECK(out.find("Day {day}.") != std::string::npos);
    // nested reward line
    std::map<std::string, std::string> n;
    n["reward_line"] = "@fig_reward_item";
    n["reward_item"] = "Brass_Key";
    CHECK(fillLine(d.q.strings, "fig_report_present_cold", 0, n, out, e));
    CHECK(out.find("Take this. Brass_Key. Do not ask.") != std::string::npos);
    CHECK(out.find('{') == std::string::npos);
    // reward line that is not defined at the stage
    n["reward_line"] = "@fig_offer_warm";
    CHECK(!fillLine(d.q.strings, "fig_report_present_cold", 0, n, out, e));
    CHECK_EQ(e.code, std::string("NO_LINE"));
    // an unknown key, a bad stage, a cycle
    CHECK(!fillLine(d.q.strings, "fig_nope", 0, v, out, e));
    CHECK_EQ(e.code, std::string("UNKNOWN_KEY"));
    CHECK(!fillLine(d.q.strings, "fig_repeat_done", 3, v, out, e));
    CHECK_EQ(e.code, std::string("STAGE_RANGE"));
    std::map<std::string, std::string> cyc;
    cyc["reward_line"] = "@fig_report_present_cold";
    CHECK(!fillLine(d.q.strings, "fig_report_present_cold", 0, cyc, out, e));
    CHECK_EQ(e.code, std::string("FILL_DEPTH"));
    // blanksIn
    bool stray = false;
    CHECK_EQ(blanksIn("a {x} b {y_z}", &stray).size(), static_cast<std::size_t>(2));
    CHECK(!stray);
    blanksIn("a {X} b", &stray);
    CHECK(stray);
    blanksIn("a } b", &stray);
    CHECK(stray);
}

// ---- state machine ---------------------------------------------------------------------------
TEST(quest_offer_variants) {
    Data d = real();
    const char* want[] = {"fig_offer_refused", "fig_offer_cold", "fig_offer_cold", "fig_offer_warm", "fig_offer_warm"};
    for (int stage = -2; stage <= 2; ++stage) {
        Engine e(d.q, d.m);
        Step s = e.offer("A", stage, Context());
        CHECK(s.ok);
        CHECK_EQ(keyOf(s), std::string(want[stage + 2]));
        const StringRow* wantRow = d.q.strings.forStage(want[stage + 2], stage);
        CHECK(wantRow != nullptr);
        if (wantRow) CHECK_EQ(textOf(s, 0), wantRow->text);
        CHECK_EQ(s.refused, stage == -2);
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("not_offered"));
    }
    Engine e(d.q, d.m);
    CHECK(e.offer("A", 3, Context()).fatal);
    CHECK(e.offer("A", -3, Context()).fatal);
    // offering again after the quest started is not an offer
    toObserved(e, "A", 0, "present");
    Step again = e.offer("A", 0, Context());
    CHECK(!again.ok && !again.fatal);
    CHECK_EQ(again.code, std::string("NOT_OFFERABLE"));
}

TEST(quest_accept_decline) {
    Data d = real();
    Context c;
    {   // accept from not_offered
        Engine e(d.q, d.m);
        e.offer("A", 0, c);
        Step s = e.accept("A", 0, c);
        CHECK(s.ok);
        CHECK_EQ(keyOf(s, 0), std::string("fig_accept_ack"));
        CHECK_EQ(keyOf(s, 1), std::string("log_quest_accepted"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("active"));
        CHECK(textOf(s, 1).find("Old ore lift") != std::string::npos);  // {node_name} from the map
        CHECK(s.lines.size() > 1 && textOf(s, 1).find('{') == std::string::npos);
        // a second accept is not in the table
        CHECK(!e.accept("A", 0, c).ok);
        CHECK(!e.decline("A", 0, c).ok);
    }
    {   // decline, then accept later
        Engine e(d.q, d.m);
        e.offer("A", 1, c);
        Step s = e.decline("A", 1, c);
        CHECK(s.ok);
        CHECK_EQ(keyOf(s), std::string("fig_decline_reply"));
        CHECK_EQ(s.lines.size(), static_cast<std::size_t>(1));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("declined"));
        CHECK(!e.decline("A", 1, c).ok);
        CHECK(e.accept("A", 1, c).ok);
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("active"));
    }
    {   // accept without an offer is allowed in the table (QST-29), but not at -2
        Engine e(d.q, d.m);
        CHECK(e.accept("A", 0, c).ok);
        Engine h(d.q, d.m);
        Step s = h.accept("A", -2, c);
        CHECK(!s.ok && !s.fatal);
        CHECK_EQ(s.code, std::string("REFUSED"));
        CHECK(!h.decline("A", -2, c).ok);
        CHECK_EQ(std::string(stateOf(h, "A")), std::string("not_offered"));
    }
    {   // bad stage
        Engine e(d.q, d.m);
        CHECK(e.accept("A", 5, c).fatal);
        CHECK(e.decline("A", -9, c).fatal);
    }
}

TEST(quest_arrive_variants) {
    Data d = real();
    Context c;
    c.day = 4;
    {   // present
        Engine e(d.q, d.m);
        e.offer("A", 0, c);
        e.accept("A", 0, c);
        Step s = e.arrive("A", "ore_lift", "present", c);
        CHECK(s.ok);
        CHECK_EQ(keyOf(s, 0), std::string("lift_arrival_present"));
        CHECK_EQ(keyOf(s, 1), std::string("log_lift_observed_present"));
        CHECK(textOf(s, 1).find("Day 4.") != std::string::npos);
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("observed_present"));
    }
    {   // gone
        Engine e(d.q, d.m);
        e.offer("A", 0, c);
        e.accept("A", 0, c);
        Step s = e.arrive("A", "ore_lift", "gone", c);
        CHECK_EQ(keyOf(s, 0), std::string("lift_arrival_gone"));
        CHECK_EQ(keyOf(s, 1), std::string("log_lift_observed_gone"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("observed_gone"));
    }
    {   // default chain value is present; other nodes do not observe; unknown node and chain are errors
        Engine e(d.q, d.m);
        e.offer("A", 0, c);
        e.accept("A", 0, c);
        CHECK(!e.arrive("A", "collapsed_gallery", "", c).ok);
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("active"));
        CHECK(e.arrive("A", "ore_cellar", "", c).fatal);
        CHECK_EQ(e.arrive("A", "ore_cellar", "", c).code, std::string("UNKNOWN_NODE"));
        CHECK(e.arrive("A", "ore_lift", "melted", c).fatal);
        CHECK(e.arrive("A", "ore_lift", "", c).ok);
        CHECK_EQ(e.record("A").chain, std::string("present"));
        // first arrival wins
        CHECK(!e.arrive("A", "ore_lift", "gone", c).ok);
        CHECK_EQ(e.record("A").chain, std::string("present"));
    }
    {   // arriving before accepting, or after declining, records nothing
        Engine e(d.q, d.m);
        CHECK(!e.arrive("A", "ore_lift", "present", c).ok);
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("not_offered"));
        CHECK(e.accept("A", 0, c).ok);
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("active"));
        Engine f(d.q, d.m);
        f.offer("A", 0, c);
        f.decline("A", 0, c);
        CHECK(!f.arrive("A", "ore_lift", "present", c).ok);
        CHECK_EQ(std::string(stateOf(f, "A")), std::string("declined"));
    }
}

TEST(quest_report_variants) {
    Data d = real();
    Context c;
    c.day = 9;
    Fixed src;
    const char* present[] = {"fig_report_present_cold", "fig_report_present_cold", "fig_report_present_cold",
                             "fig_report_present_warm", "fig_report_present_warm"};
    const char* gone[] = {"fig_report_gone_cold", "fig_report_gone_cold", "fig_report_gone_cold",
                          "fig_report_gone_warm", "fig_report_gone_warm"};
    for (int stage = -2; stage <= 2; ++stage) {
        for (int g = 0; g < 2; ++g) {
            Engine e(d.q, d.m);
            toObserved(e, "A", stage == -2 ? 0 : stage, g ? "gone" : "present");
            Step s = e.report("A", "sewer_grate", stage, c, src);
            CHECK(s.ok);
            CHECK_EQ(keyOf(s, 0), std::string(g ? gone[stage + 2] : present[stage + 2]));
            CHECK_EQ(keyOf(s, 1), std::string("log_quest_done"));
            CHECK_EQ(keyOf(s, 2), std::string("log_reward_none"));
            CHECK(s.rewardRolled);
            CHECK_EQ(std::string(stateOf(e, "A")), std::string("completed"));
            CHECK(textOf(s, 0).find("There is still much work to be done.") != std::string::npos);
        }
    }
    // not on the quest: no offer / declined
    {
        Engine e(d.q, d.m);
        Step s = e.report("A", "sewer_grate", 0, c, src);
        CHECK(s.ok);
        CHECK_EQ(keyOf(s), std::string("fig_report_no_offer"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("not_offered"));
        e.offer("A", 0, c);
        e.decline("A", 0, c);
        CHECK_EQ(keyOf(e.report("A", "sewer_grate", 0, c, src)), std::string("fig_report_no_offer"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("declined"));
    }
    // active, not observed yet
    {
        Engine e(d.q, d.m);
        e.offer("A", 0, c);
        e.accept("A", 0, c);
        Step s = e.report("A", "sewer_grate", 0, c, src);
        CHECK_EQ(keyOf(s), std::string("fig_report_not_yet"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("active"));
        CHECK(!s.rewardRolled);
    }
    // wrong node and unknown node
    {
        Engine e(d.q, d.m);
        toObserved(e, "A", 0, "present");
        Step s = e.report("A", "barracks", 0, c, src);
        CHECK(!s.ok && !s.fatal);
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("observed_present"));
        CHECK(e.report("A", "nowhere", 0, c, src).fatal);
        CHECK(e.report("A", "sewer_grate", 4, c, src).fatal);
        Fixed fresh;
        CHECK(e.report("A", "barracks", 0, c, fresh).code == "WRONG_NODE");
        CHECK(e.report("A", "nowhere", 0, c, fresh).fatal);
        CHECK_EQ(fresh.calls, 0);  // no roll before completion
    }
    // after completion
    {
        Engine e(d.q, d.m);
        toObserved(e, "A", 0, "present");
        e.report("A", "sewer_grate", 0, c, src);
        int before = src.calls;
        Step s = e.report("A", "sewer_grate", 0, c, src);
        CHECK_EQ(keyOf(s), std::string("fig_repeat_done"));
        CHECK(textOf(s, 0).find("Day 9.") != std::string::npos);
        CHECK_EQ(s.lines.size(), static_cast<std::size_t>(1));
        CHECK_EQ(src.calls, before);
    }
}

TEST(quest_per_character) {
    Data d = real();
    Context c;
    Fixed src;
    Engine e(d.q, d.m);
    toObserved(e, "A", 0, "present");
    CHECK_EQ(keyOf(e.report("B", "sewer_grate", 0, c, src)), std::string("fig_report_no_offer"));
    CHECK_EQ(std::string(stateOf(e, "B")), std::string("not_offered"));
    CHECK_EQ(keyOf(e.report("A", "sewer_grate", 0, c, src)), std::string("fig_report_present_cold"));
    CHECK_EQ(std::string(stateOf(e, "A")), std::string("completed"));
    CHECK_EQ(std::string(stateOf(e, "B")), std::string("not_offered"));
}

// ---- hostile Figure (QST-60 .. QST-66) ------------------------------------------------------
TEST(quest_hostile_midquest) {
    Data d = real();
    Context c;
    Fixed src;
    {   // accept at 0, observe present, standing falls to -2, report completes
        Engine e(d.q, d.m);
        toObserved(e, "A", 0, "present");
        Step s = e.report("A", "sewer_grate", -2, c, src);
        CHECK(s.ok && !s.fatal);
        CHECK_EQ(keyOf(s, 0), std::string("fig_report_present_cold"));
        CHECK_EQ(keyOf(s, 1), std::string("log_quest_done"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("completed"));
    }
    {   // accepted at -1, chain gone
        Engine e(d.q, d.m);
        toObserved(e, "A", -1, "gone");
        Step s = e.report("A", "sewer_grate", -2, c, src);
        CHECK_EQ(keyOf(s, 0), std::string("fig_report_gone_cold"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("completed"));
    }
    {   // drop before observing
        Engine e(d.q, d.m);
        e.offer("A", -1, c);
        e.accept("A", -1, c);
        Step s = e.report("A", "sewer_grate", -2, c, src);
        CHECK_EQ(keyOf(s), std::string("fig_report_not_yet"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("active"));
    }
    {   // hostile and not on the quest
        Engine e(d.q, d.m);
        e.offer("A", 0, c);
        e.decline("A", 0, c);
        CHECK_EQ(keyOf(e.report("A", "sewer_grate", -2, c, src)), std::string("fig_report_no_offer"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("declined"));
        Engine f(d.q, d.m);
        CHECK_EQ(keyOf(f.report("A", "sewer_grate", -2, c, src)), std::string("fig_report_no_offer"));
        CHECK_EQ(std::string(stateOf(f, "A")), std::string("not_offered"));
    }
    {   // repeat after completion at -2
        Engine e(d.q, d.m);
        toObserved(e, "A", 0, "present");
        e.report("A", "sewer_grate", 0, c, src);
        Step s = e.report("A", "sewer_grate", -2, c, src);
        CHECK_EQ(keyOf(s), std::string("fig_repeat_done"));
        CHECK(!s.lines.empty() && textOf(s, 0).find('{') == std::string::npos);
        CHECK_EQ(s.lines.size(), static_cast<std::size_t>(1));
    }
    {   // a NEW offer at -2 is refused and accept is impossible
        Engine e(d.q, d.m);
        Step o = e.offer("A", -2, c);
        CHECK_EQ(keyOf(o), std::string("fig_offer_refused"));
        CHECK(o.refused);
        Step a = e.accept("A", -2, c);
        CHECK(!a.ok && !a.fatal);
        CHECK_EQ(a.code, std::string("REFUSED"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("not_offered"));
        // even if the standing recovers, the refused offer was never made: a fresh offer is needed...
        // ...but accepting without any offer is allowed by the table (QST-29), so the refusal is held
        CHECK(!e.accept("A", 0, c).ok);
        CHECK(e.offer("A", 0, c).ok);
        CHECK(e.accept("A", 0, c).ok);
    }
    {   // offered at 0, then standing -2: accept works; decline works; decline then accept works
        Engine e(d.q, d.m);
        e.offer("A", 0, c);
        Step a = e.accept("A", -2, c);
        CHECK(a.ok);
        CHECK_EQ(keyOf(a), std::string("fig_accept_ack"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("active"));
        Engine f(d.q, d.m);
        f.offer("A", 1, c);
        Step dcl = f.decline("A", -2, c);
        CHECK(dcl.ok);
        CHECK_EQ(keyOf(dcl), std::string("fig_decline_reply"));
        CHECK(f.accept("A", -2, c).ok);
        CHECK_EQ(std::string(stateOf(f, "A")), std::string("active"));
    }
    {   // a refused offer for a declined character
        Engine e(d.q, d.m);
        e.offer("A", 0, c);
        e.decline("A", 0, c);
        Step o = e.offer("A", -2, c);
        CHECK(o.refused);
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("declined"));
    }
}

TEST(quest_hostile_reward) {
    Data d = real();
    Context c;
    // both reward rows and the nested reward line exist at -2
    std::map<std::string, std::string> v;
    v["reward_item"] = "Brass_Key";
    std::string out;
    Issue e;
    CHECK(fillLine(d.q.strings, "fig_reward_none", -2, v, out, e));
    CHECK(fillLine(d.q.strings, "fig_reward_item", -2, v, out, e));
    CHECK(out.find("Brass_Key") != std::string::npos);
    RewardConfig cfg;
    cfg.itemChance = 0.25;
    cfg.items.push_back("Brass_Key");
    for (int pass = 0; pass < 2; ++pass) {
        Engine en(d.q, d.m);
        en.setRewardConfig(cfg);
        Fixed src;
        src.value = pass == 0 ? 0.0 : 0.999999;
        toObserved(en, "A", 0, "present");
        Step s = en.report("A", "sewer_grate", -2, c, src);
        CHECK(s.ok);
        CHECK_EQ(keyOf(s, 2), std::string(pass == 0 ? "log_reward_item" : "log_reward_none"));
        CHECK(textOf(s, 0).find(pass == 0 ? "Take this. Brass_Key. Do not ask." : "There is still much work to be done.") != std::string::npos);
        CHECK_EQ(en.record("A").rewardItem, std::string(pass == 0 ? "Brass_Key" : ""));
        en.report("A", "sewer_grate", -2, c, src);
        CHECK_EQ(src.calls, 1);
    }
}

TEST(quest_reward_once) {
    Data d = real();
    Context c;
    Fixed src;
    src.value = 0.999999;
    Engine e(d.q, d.m);
    toObserved(e, "A", 0, "present");
    CHECK_EQ(src.calls, 0);
    e.report("A", "sewer_grate", 0, c, src);
    e.report("A", "sewer_grate", 0, c, src);
    CHECK_EQ(src.calls, 1);
    CHECK(e.record("A").rewardRolled);
    // a reload does not roll again
    Engine f(d.q, d.m);
    Issue err;
    CHECK(f.restore(e.save(), err));
    f.report("A", "sewer_grate", 0, c, src);
    CHECK_EQ(src.calls, 1);
}

TEST(quest_reward_roll) {
    Data d = real();
    Context c;
    {   // no item list configured (the default): never an item, no crash
        Fixed src;
        src.value = 0.0;
        Engine e(d.q, d.m);
        toObserved(e, "A", 0, "present");
        Step s = e.report("A", "sewer_grate", 0, c, src);
        CHECK_EQ(keyOf(s, 2), std::string("log_reward_none"));
    }
    {   // chance but no items, items but chance 0
        RewardConfig a, b;
        a.itemChance = 1.0;
        b.items.push_back("X");
        RewardConfig cfgs[] = {a, b};
        for (const RewardConfig& cfg : cfgs) {
            Fixed src;
            src.value = 0.0;
            Engine e(d.q, d.m);
            e.setRewardConfig(cfg);
            toObserved(e, "A", 0, "present");
            CHECK_EQ(keyOf(e.report("A", "sewer_grate", 0, c, src), 2), std::string("log_reward_none"));
        }
    }
    {   // the roll picks the item inside the chance window; same roll, same result
        RewardConfig cfg;
        cfg.itemChance = 0.5;
        cfg.items.push_back("A1");
        cfg.items.push_back("B2");
        const double rolls[] = {0.0, 0.24, 0.25, 0.49, 0.5, 0.9};
        const char* want[] = {"A1", "A1", "B2", "B2", "", ""};
        for (int pass = 0; pass < 2; ++pass)
            for (int i = 0; i < 6; ++i) {
                Fixed src;
                src.value = rolls[i];
                Engine e(d.q, d.m);
                e.setRewardConfig(cfg);
                toObserved(e, "A", 0, "gone");
                e.report("A", "sewer_grate", 0, c, src);
                CHECK_EQ(e.record("A").rewardItem, std::string(want[i]));
            }
    }
    {   // a source out of range is an error and changes nothing
        Fixed src;
        src.value = 1.0;
        Engine e(d.q, d.m);
        toObserved(e, "A", 0, "present");
        Step s = e.report("A", "sewer_grate", 0, c, src);
        CHECK(s.fatal);
        CHECK_EQ(s.code, std::string("ROLL_RANGE"));
        CHECK_EQ(std::string(stateOf(e, "A")), std::string("observed_present"));
        CHECK(!e.record("A").rewardRolled);
        src.value = -0.1;
        CHECK(e.report("A", "sewer_grate", 0, c, src).fatal);
    }
}

// ---- save and load -----------------------------------------------------------------------------
TEST(quest_roundtrip) {
    Data d = real();
    Context c;
    Fixed src;
    RewardConfig cfg;
    cfg.itemChance = 1.0;
    cfg.items.push_back("Brass \"Key\"");
    Engine e(d.q, d.m);
    e.setRewardConfig(cfg);
    src.value = 0.0;
    toObserved(e, "A", 0, "gone");
    e.report("A", "sewer_grate", 0, c, src);                  // completed with an item
    toObserved(e, "B", 1, "present");                          // observed, not reported
    e.offer("C", 0, c);
    e.accept("C", 0, c);                                       // active
    e.offer("D", -2, c);                                       // refused
    e.offer("E", 0, c);
    e.decline("E", 0, c);                                      // declined
    std::string saved = e.save();
    Engine f(d.q, d.m);
    Issue err;
    CHECK(f.restore(saved, err));
    for (const char* ch : {"A", "B", "C", "D", "E", "Z"}) {
        Record a = e.record(ch), b = f.record(ch);
        CHECK_EQ(std::string(stateName(a.state)), std::string(stateName(b.state)));
        CHECK_EQ(a.offered, b.offered);
        CHECK_EQ(a.refused, b.refused);
        CHECK_EQ(a.chain, b.chain);
        CHECK_EQ(a.rewardRolled, b.rewardRolled);
        CHECK_EQ(a.rewardItem, b.rewardItem);
    }
    CHECK_EQ(f.record("A").chain, std::string("gone"));
    CHECK_EQ(f.record("A").rewardItem, std::string("Brass \"Key\""));
    CHECK_EQ(f.save(), saved);
    // the restored engine goes on from where the saved one was
    Fixed s2;
    CHECK_EQ(keyOf(f.report("B", "sewer_grate", 1, c, s2)), std::string("fig_report_present_warm"));
    CHECK(!f.accept("D", 0, c).ok);  // the refusal was saved
    // the hand-written good fixture loads and re-saves identically
    Engine g(d.q, d.m);
    CHECK(g.restore(slurp(fixPath("save_good.json")), err));
    CHECK_EQ(g.save(), slurp(fixPath("save_good.json")));
    // empty engine round trip
    Engine h(d.q, d.m), i2(d.q, d.m);
    CHECK(i2.restore(h.save(), err));
}

TEST(quest_load_invalid) {
    Data d = real();
    Engine e(d.q, d.m);
    Issue err;
    toObserved(e, "A", 0, "present");
    std::string before = e.save();
    // the two QA cases, from fixture files
    CHECK(!e.restore(slurp(fixPath("save_bad_state.json")), err));
    CHECK_EQ(err.code, std::string("SAVE_BAD_STATE"));
    CHECK(!e.restore(slurp(fixPath("save_completed_no_chain.json")), err));
    CHECK_EQ(err.code, std::string("SAVE_INCONSISTENT"));
    CHECK_EQ(e.save(), before);  // a rejected restore changes nothing
    std::string good = slurp(fixPath("save_good.json"));
    const char* from[] = {"\"version\":1", "quest_figure_ore_lift", "\"chain\":\"gone\"", "\"reward_rolled\":true", "\"B\":{\"state\":\"active\",\"offered\":true,\"refused\":false,\"chain\":\"\",\"reward_rolled\":false,\"reward_item\":\"\"",
                          "\"offered\":false,\"refused\":true", "\"state\":\"active\",\"offered\":true,\"refused\":false,\"chain\":\"\"", "\"B\":{"};
    const char* to[] = {"\"version\":2", "quest_other", "\"chain\":\"melted\"", "\"reward_rolled\":false", "\"B\":{\"state\":\"active\",\"offered\":true,\"refused\":false,\"chain\":\"\",\"reward_rolled\":false,\"reward_item\":\"X\"",
                        "\"offered\":true,\"refused\":true", "\"state\":\"active\",\"offered\":true,\"refused\":false,\"chain\":\"present\"", "\"B\":{\"extra\":1,"};
    const char* code[] = {"SAVE_VERSION", "SAVE_WRONG_QUEST", "SAVE_INCONSISTENT", "SAVE_INCONSISTENT", "SAVE_INCONSISTENT",
                          "SAVE_INCONSISTENT", "SAVE_INCONSISTENT", "SAVE_UNKNOWN_FIELD"};
    for (int i = 0; i < 8; ++i) {
        CHECK(!e.restore(mutate(good, from[i], to[i]), err));
        CHECK_EQ(err.code, std::string(code[i]));
    }
    // observed state with the wrong chain, bad JSON, wrong shapes
    CHECK(!e.restore("", err));
    CHECK(!e.restore("{\"version\":1,", err));
    CHECK(!e.restore("[]", err));
    CHECK_EQ(err.code, std::string("SAVE_SCHEMA"));
    CHECK(!e.restore("{\"version\":1,\"quest_id\":\"quest_figure_ore_lift\",\"characters\":{\"A\":{\"state\":\"observed_gone\",\"offered\":true,\"refused\":false,\"chain\":\"present\",\"reward_rolled\":false,\"reward_item\":\"\"}}}", err));
    CHECK(!e.restore("{\"version\":1,\"quest_id\":\"quest_figure_ore_lift\",\"characters\":{\"A\":{\"state\":\"active\"}}}", err));
    CHECK_EQ(err.code, std::string("SAVE_SCHEMA"));
    CHECK_EQ(e.save(), before);
}

// ---- ID cross-checks against the map and the data rules ----------------------------------------
TEST(quest_id_checks) {
    Data d = real();
    CHECK_EQ(firstCode(d.questJson, d.csv, d.mapText), std::string(""));
    auto qj = [&](const std::string& f, const std::string& t) { return firstCode(mutate(d.questJson, f, t), d.csv, d.mapText); };
    CHECK_EQ(qj("\"destination\": {\"node\": \"ore_lift\"", "\"destination\": {\"node\": \"ore_cellar\""), std::string("UNKNOWN_NODE"));
    CHECK_EQ(qj("\"destination\": {\"node\": \"ore_lift\"", "\"destination\": {\"node\": \"barracks\""), std::string("DEST_MISMATCH"));
    CHECK_EQ(qj("\"giver\": {\"node\": \"sewer_grate\"", "\"giver\": {\"node\": \"drain\""), std::string("UNKNOWN_NODE"));
    CHECK_EQ(qj("[\"e01\", \"e02\", \"e04\", \"e10\"]", "[\"e01\", \"e02\", \"e03\", \"e10\"]"), std::string("ROUTE_EDGE"));
    CHECK_EQ(qj("[\"e01\", \"e02\", \"e04\", \"e10\"]", "[\"e01\", \"e02\", \"e04\", \"e77\"]"), std::string("UNKNOWN_EDGE"));
    CHECK_EQ(qj("[\"e01\", \"e02\", \"e04\", \"e10\"]", "[\"e01\", \"e02\", \"e04\"]"), std::string("ROUTE_EDGE"));
    CHECK_EQ(qj("\"ore_lift\"],", "\"ore_cellar\"],"), std::string("UNKNOWN_NODE"));
    // a route edge that needs a pick or a break (edited in the map copy)
    std::string lockedMap = mutate(d.mapText, "\"to\": \"ore_lift\", \"door\": \"plain\"", "\"to\": \"ore_lift\", \"door\": \"locked\"");
    CHECK_EQ(firstCode(d.questJson, d.csv, lockedMap), std::string("ROUTE_NOT_PLAIN"));
    // the map without the quest destination
    CHECK_EQ(firstCode(d.questJson, d.csv, mutate(d.mapText, "\"figure_task_destination\": \"ore_lift\"", "\"figure_task_destination\": \"barracks\"")), std::string("DEST_MISMATCH"));
    // the fixture file
    CHECK_EQ(firstCode(slurp(fixPath("q_bad_dest.json")), d.csv, d.mapText), std::string("UNKNOWN_NODE"));
    // parse errors pass through
    CHECK_EQ(firstCode("{", d.csv, d.mapText), std::string("LOAD:JSON_TRUNCATED"));
    CHECK_EQ(firstCode("[]", d.csv, d.mapText), std::string("LOAD:SCHEMA"));
    CHECK_EQ(firstCode(mutate(d.questJson, "\"giver\"", "\"giverx\""), d.csv, d.mapText), std::string("LOAD:SCHEMA"));
    CHECK_EQ(firstCode(d.questJson, "", d.mapText), std::string("LOAD:CSV_EMPTY"));
    CHECK_EQ(firstCode(d.questJson, "key,text\nk,t\n", d.mapText), std::string("LOAD:CSV_MISSING_COLUMN"));
}

TEST(quest_csv_rules) {
    Data d = real();
    auto cs = [&](const std::string& csv) { return firstCode(d.questJson, csv, d.mapText); };
    CHECK_EQ(cs(dropRow(d.csv, "fig_reward_none")), std::string("MISSING_KEY"));
    CHECK_EQ(cs(d.csv + "fig_extra,mysterious_figure,0,0,\"Extra.\",\n"), std::string("ORPHAN_KEY"));
    CHECK_EQ(cs(d.csv + "fig_offer_cold,mysterious_figure,-1,0,\"Again.\",\n"), std::string("DUP_KEY"));
    CHECK_EQ(cs(mutate(d.csv, "Nothing to tell me?", "Nothing to tell me {gold}?")), std::string("BAD_BLANK"));
    CHECK_EQ(cs(mutate(d.csv, "Day {day}. Enough.\",\"{day}\"", "Day {day}. Enough.\",")), std::string("BLANK_MISMATCH"));
    CHECK_EQ(cs(mutate(d.csv, "fig_offer_warm,mysterious_figure,1,2", "fig_offer_warm,mysterious_figure,1,3")), std::string("STAGE_RANGE"));
    CHECK_EQ(cs(mutate(d.csv, "fig_offer_warm,mysterious_figure,1,2", "fig_offer_warm,mysterious_figure,x,2")), std::string("STAGE_RANGE"));
    CHECK_EQ(cs(mutate(d.csv, "fig_report_present_cold,mysterious_figure,-2,0", "fig_report_present_cold,mysterious_figure,-2,1")), std::string("STAGE_OVERLAP"));
    CHECK_EQ(cs(mutate(d.csv, "Come back and say.\",\nfig_offer_warm", "Come back and say. I need it because the mine is mine.\",\nfig_offer_warm")), std::string("BANNED_PHRASE"));
    // a gone line that names a cause
    CHECK_EQ(cs(mutate(d.csv, "The drum is bare. Gone. Hm.", "The drum is bare. Gone. Someone has it.")), std::string("CAUSE_WORD"));
    // a stray brace
    CHECK_EQ(cs(mutate(d.csv, "Then go.", "Then go. {")), std::string("BAD_BLANK"));
    // JSON-side: a transition table that breaks the self-test
    CHECK_EQ(firstCode(mutate(d.questJson, "{\"from\": \"observed_gone\", \"event\": \"report\", \"to\": \"completed\"}", "{\"from\": \"observed_gone\", \"event\": \"report\", \"to\": \"active\"}"), d.csv, d.mapText), std::string("STATE_TABLE"));
    CHECK_EQ(firstCode(mutate(d.questJson, "{\"from\": \"observed_gone\", \"event\": \"report\", \"to\": \"completed\"}", "{\"from\": \"observed_gone\", \"event\": \"report\", \"to\": \"finished\"}"), d.csv, d.mapText), std::string("BAD_TRANSITION"));
}

TEST(quest_coverage_rule) {
    Data d = real();
    const char* keys[] = {"fig_accept_ack",       "fig_decline_reply",  "fig_report_not_yet",      "fig_report_nothing",
                          "fig_report_no_offer",  "fig_report_present_cold", "fig_report_gone_cold", "fig_reward_none",
                          "fig_repeat_done",      "fig_reward_item"};
    for (const char* k : keys) {
        std::string csv = mutate(d.csv, std::string("\n") + k + ",mysterious_figure,-2,", std::string("\n") + k + ",mysterious_figure,-1,");
        dd::map::FloorMap m = d.m;
        QuestData q;
        Issue e;
        CHECK(load(d.questJson, csv, q, e));
        std::vector<Issue> is = validate(q, m);
        bool found = false;
        for (const Issue& i : is)
            if (i.code == "STAGE_COVERAGE" && i.detail.compare(0, std::string(k).size(), k) == 0) found = true;
        CHECK(found);
        CHECK(!is.empty() && is[0].code == "STAGE_COVERAGE");
    }
    // refused offer must cover -2 exactly; no other offer may reach -2
    CHECK_EQ(firstCode(d.questJson, mutate(d.csv, "fig_offer_refused,mysterious_figure,-2,-2", "fig_offer_refused,mysterious_figure,-1,-2"), d.mapText), std::string("STAGE_COVERAGE"));
    CHECK_EQ(firstCode(d.questJson, mutate(d.csv, "fig_offer_refused,mysterious_figure,-2,-2", "fig_offer_refused,mysterious_figure,-2,-1"), d.mapText), std::string("STAGE_COVERAGE"));
    std::string c2 = firstCode(d.questJson, mutate(d.csv, "fig_offer_cold,mysterious_figure,-1,0", "fig_offer_cold,mysterious_figure,-2,0"), d.mapText);
    CHECK(c2 == "STAGE_COVERAGE" || c2 == "STAGE_OVERLAP");
    // the fixture file (one mid-quest key at -1)
    CHECK_EQ(firstCode(d.questJson, slurp(fixPath("q_cov_report_nothing.csv")), d.mapText), std::string("STAGE_COVERAGE"));
    // lines at -2 for the ten keys; none for warm variants or normal offers
    for (const char* k : keys) CHECK(d.q.strings.forStage(k, -2) != nullptr);
    for (const char* k : {"fig_report_present_warm", "fig_report_gone_warm", "fig_offer_cold", "fig_offer_warm"})
        CHECK(d.q.strings.forStage(k, -2) == nullptr);
    CHECK(d.q.strings.forStage("fig_offer_refused", -2) != nullptr);
    CHECK(d.q.strings.forStage("fig_offer_refused", -1) == nullptr);
}

// ---- transition table (EQV-13) -----------------------------------------------------------------
// DD_QUEST_JSON (a path) points THIS test at another quest JSON, so a changed table can be tried without
// editing source. The table (`transitions`) is read from that file. Two real Engines are driven through
// every (state, event) pair using only the public operations: engine A is built from the REAL quest file,
// engine B from the file under test. Each result state must equal the table's "to" for a listed pair and
// the starting state for an unlisted pair. A is independent of the file under test, so a changed row is
// seen even where the engine does not read the table (for example Engine::report outside the observed
// states); B shows the engine following the table where it does read it.
namespace {
struct Ev { const char* name; };
const Ev kEvents[] = {{"offer"}, {"accept"}, {"decline"}, {"arrive_ore_lift_chain_present"}, {"arrive_ore_lift_chain_gone"},
                      {"arrive_elsewhere"}, {"report"}};

// Applies one event for character "A" at standing 0 (neutral).
void applyEvent(Engine& e, const QuestData& q, const std::string& ev, RewardSource& src) {
    Context c;
    if (ev == "offer") e.offer("A", 0, c);
    else if (ev == "accept") e.accept("A", 0, c);
    else if (ev == "decline") e.decline("A", 0, c);
    else if (ev == "arrive_ore_lift_chain_present") e.arrive("A", q.destNode, "present", c);
    else if (ev == "arrive_ore_lift_chain_gone") e.arrive("A", q.destNode, "gone", c);
    else if (ev == "arrive_elsewhere") e.arrive("A", q.giverNode, "", c);  // a node that is not the destination
    else if (ev == "report") e.report("A", q.giverNode, 0, c, src);
}

// Reaches `target` from a fresh character using public operations; false when it cannot.
bool reach(Engine& e, const QuestData& q, State target, RewardSource& src) {
    switch (target) {
        case State::NotOffered: break;
        case State::Declined: applyEvent(e, q, "offer", src); applyEvent(e, q, "decline", src); break;
        case State::Active: applyEvent(e, q, "offer", src); applyEvent(e, q, "accept", src); break;
        case State::ObservedPresent: reach(e, q, State::Active, src); applyEvent(e, q, "arrive_ore_lift_chain_present", src); break;
        case State::ObservedGone: reach(e, q, State::Active, src); applyEvent(e, q, "arrive_ore_lift_chain_gone", src); break;
        case State::Completed: reach(e, q, State::ObservedPresent, src); applyEvent(e, q, "report", src); break;
    }
    return e.record("A").state == target;
}

// Drives every pair on `e`; returns the number of disagreements (each reported). `pairs` counts pairs run.
int driveAll(const char* who, const dd::map::FloorMap& m, const QuestData& engineData, const QuestData& table,
             int& pairs, int& listedHit) {
    const State all[] = {State::NotOffered, State::Declined, State::Active, State::ObservedPresent, State::ObservedGone, State::Completed};
    int bad = 0;
    for (State s : all)
        for (const Ev& ev : kEvents) {
            Fixed src;
            Engine e(engineData, m);
            if (!reach(e, engineData, s, src)) {
                // Not reachable with this table: only the file under test (B) can fail here, and that is a finding.
                std::cerr << who << ": cannot reach state " << stateName(s) << "\n";
                ++bad;
                continue;
            }
            applyEvent(e, engineData, ev.name, src);
            State got = e.record("A").state;
            State want = s;
            bool listed = false;
            for (const Transition& t : table.transitions) {
                State f, to;
                if (stateFromName(t.from, f) && f == s && t.event == ev.name && stateFromName(t.to, to)) { want = to; listed = true; break; }
            }
            ++pairs;
            if (listed) ++listedHit;
            if (got != want) {
                std::cerr << who << ": DISAGREE " << stateName(s) << " --" << ev.name << "--> engine gives " << stateName(got)
                          << ", table says " << stateName(want) << (listed ? " (listed)" : " (unlisted: must be unchanged)") << "\n";
                ++bad;
            }
        }
    return bad;
}
}  // namespace

TEST(quest_transition_table) {
    Data d = real();
    const char* env = std::getenv("DD_QUEST_JSON");
    std::string tablePath = (env && *env) ? std::string(env) : questPath();
    QuestData qt;
    Issue le;
    if (!load(slurp(tablePath), d.csv, qt, le)) {
        CHECK(false && "quest JSON under test does not load");
        std::cerr << "load: " << le.code << " " << le.detail << "\n";
        return;
    }
    // every row must name known states and an event the engine can be driven with ("unknown event" rows fail)
    std::set<std::string> known;
    for (const Ev& ev : kEvents) known.insert(ev.name);
    int rows = 0;
    for (const Transition& t : qt.transitions) {
        State a, b;
        bool ok = stateFromName(t.from, a) && stateFromName(t.to, b) && known.count(t.event) && t.event != "offer" && t.event != "arrive_elsewhere";
        if (!ok) std::cerr << "table row not drivable: " << t.from << " --" << t.event << "--> " << t.to << "\n";
        CHECK(ok);
        ++rows;
    }
    CHECK(rows > 0);
    int pairs = 0, listedA = 0, listedB = 0;
    int badA = driveAll("engine(real file)", d.m, d.q, qt, pairs, listedA);
    CHECK_EQ(badA, 0);
    int pairsB = 0;
    int badB = driveAll("engine(file under test)", d.m, qt, qt, pairsB, listedB);
    CHECK_EQ(badB, 0);
    CHECK_EQ(pairs, 6 * static_cast<int>(sizeof(kEvents) / sizeof(kEvents[0])));
    CHECK_EQ(pairsB, pairs);
    CHECK_EQ(listedA, rows);  // every listed row was driven and compared exactly once (no duplicate rows)
    CHECK_EQ(listedB, rows);
}

// ---- command line (quest_check), through the real executable ---------------------------------
namespace {
struct Cli { int code = -1; std::string out; };

Cli runCli(const std::vector<std::string>& args) {
    std::string cmd = std::string("'") + QUEST_CHECK_EXE + "'";
    for (const std::string& a : args) cmd += " '" + a + "'";
    cmd += " 2>&1";
    Cli r;
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return r;
    char buf[512];
    std::size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, p)) > 0) r.out.append(buf, n);
    int st = pclose(p);
    r.code = WIFEXITED(st) ? WEXITSTATUS(st) : -1;
    return r;
}

std::vector<std::string> run3(const std::vector<std::string>& events) {
    std::vector<std::string> a = {"run", questPath(), csvPath(), mapPath()};
    for (const std::string& e : events) a.push_back(e);
    return a;
}

bool has(const Cli& c, const std::string& s) { return c.out.find(s) != std::string::npos; }
}  // namespace

TEST(quest_cli_validate) {
    Cli c = runCli({"validate", questPath(), csvPath(), mapPath()});
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "OK quest rows=25"));
    c = runCli({"validate", fixPath("q_bad_dest.json"), csvPath(), mapPath()});
    CHECK_EQ(c.code, 1);
    CHECK(c.out.compare(0, 24, "FAIL UNKNOWN_NODE ore_ce") == 0);
    c = runCli({"validate", questPath(), fixPath("q_cov_report_nothing.csv"), mapPath()});
    CHECK_EQ(c.code, 1);
    CHECK(c.out.compare(0, 36, "FAIL STAGE_COVERAGE fig_report_nothi") == 0);
    c = runCli({"validate", questPath(), csvPath(), "/nonexistent/map.json"});
    CHECK_EQ(c.code, 2);
    c = runCli({"validate", questPath()});
    CHECK_EQ(c.code, 2);
    CHECK_EQ(runCli({}).code, 2);
}

TEST(quest_cli_stage_line_fill) {
    const char* score[] = {"0", "19", "20", "39", "40", "59", "60", "79", "80", "100"};
    const char* want[] = {"-2", "-2", "-1", "-1", "0", "0", "1", "1", "2", "2"};
    for (int i = 0; i < 10; ++i) {
        Cli c = runCli({"stage", score[i]});
        CHECK_EQ(c.code, 0);
        CHECK_EQ(c.out, std::string(want[i]) + "\n");
    }
    for (const char* bad : {"101", "-1", "abc", ""}) {
        Cli c = runCli({"stage", bad});
        CHECK_EQ(c.code, 1);
        CHECK(c.out.compare(0, 16, "FAIL SCORE_RANGE") == 0);
    }
    Cli c = runCli({"line", csvPath(), "fig_offer_cold", "0"});
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "The old ore lift. Is the chain still on the drum?"));
    c = runCli({"line", csvPath(), "fig_offer_cold", "2"});
    CHECK_EQ(c.code, 1);
    CHECK(c.out.compare(0, 12, "FAIL NO_LINE") == 0);
    c = runCli({"line", csvPath(), "fig_report_not_yet", "-2"});
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "You have not looked. Go."));
    for (const char* k : {"fig_report_present_warm", "fig_report_gone_warm", "fig_offer_cold", "fig_offer_warm"}) {
        c = runCli({"line", csvPath(), k, "-2"});
        CHECK_EQ(c.code, 1);
        CHECK(has(c, "FAIL NO_LINE"));
    }
    c = runCli({"line", csvPath(), "fig_offer_cold", "3"});
    CHECK_EQ(c.code, 1);
    c = runCli({"fill", csvPath(), "fig_repeat_done", "0", "day=7"});
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "Day 7."));
    c = runCli({"fill", csvPath(), "fig_repeat_done", "0"});
    CHECK_EQ(c.code, 1);
    CHECK(c.out.compare(0, 25, "FAIL UNFILLED_BLANK day\n") == 0 || has(c, "FAIL UNFILLED_BLANK day"));
    c = runCli({"fill", csvPath(), "fig_report_nothing", "0", "days_since=12"});
    CHECK(has(c, "12 days"));
    c = runCli({"fill", csvPath(), "fig_report_present_cold", "0", "reward_line=@fig_reward_item", "reward_item=Brass_Key"});
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "Take this. Brass_Key. Do not ask."));
    CHECK(!has(c, "{"));
    c = runCli({"fill", csvPath(), "fig_repeat_done", "0", "day={day}"});
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "Day {day}."));
}

TEST(quest_cli_run) {
    Cli c = runCli(run3({"offer@-2", "offer@0", "offer@1"}));
    CHECK_EQ(c.code, 0);
    std::size_t a = c.out.find("key=fig_offer_refused"), b = c.out.find("key=fig_offer_cold"), w = c.out.find("key=fig_offer_warm");
    CHECK(a != std::string::npos && a < b && b < w);
    c = runCli(run3({"offer@-2", "accept"}));
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "accept rejected refused"));
    CHECK(has(c, "state=not_offered chain=none"));
    c = runCli(run3({"stage=-2", "offer@-2", "accept"}));
    CHECK(has(c, "accept rejected refused"));
    c = runCli(run3({"offer@0", "accept"}));
    CHECK(has(c, "log=log_quest_accepted") && has(c, "key=fig_accept_ack") && has(c, "state=active chain=none"));
    c = runCli(run3({"offer@0", "decline", "accept"}));
    CHECK(has(c, "key=fig_decline_reply") && has(c, "state=declined") && has(c, "state=active chain=none"));
    c = runCli(run3({"offer@0", "accept", "arrive@ore_lift:present"}));
    CHECK(has(c, "text=lift_arrival_present") && has(c, "log=log_lift_observed_present") && has(c, "state=observed_present chain=present"));
    c = runCli(run3({"offer@0", "accept", "arrive@ore_lift:gone"}));
    CHECK(has(c, "text=lift_arrival_gone") && has(c, "state=observed_gone chain=gone"));
    c = runCli(run3({"offer@0", "accept", "arrive@ore_lift"}));
    CHECK(has(c, "state=observed_present chain=present"));
    c = runCli(run3({"arrive@ore_lift:present", "accept"}));
    CHECK(has(c, "arrive@ore_lift:present rejected not_observing state=not_offered"));
    CHECK(has(c, "state=active chain=none"));
    c = runCli(run3({"offer@0", "accept", "arrive@ore_cellar"}));
    CHECK_EQ(c.code, 1);
    CHECK(has(c, "FAIL UNKNOWN_NODE"));
    c = runCli(run3({"offer@0", "accept", "arrive@ore_lift:present", "arrive@ore_lift:gone"}));
    CHECK(has(c, "state=observed_present chain=present"));
    c = runCli(run3({"offer@0", "accept", "arrive@ore_lift:present", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_present_cold log=log_quest_done log=log_reward_none") && has(c, "state=completed"));
    c = runCli(run3({"offer@2", "accept", "arrive@ore_lift:gone", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_gone_warm") && has(c, "state=completed"));
    c = runCli(run3({"offer@0", "accept", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_not_yet") && has(c, "state=active chain=none"));
    c = runCli(run3({"offer@0", "accept", "arrive@ore_lift:present", "report@barracks"}));
    CHECK(has(c, "report@barracks rejected wrong_node") && has(c, "state=observed_present"));
    c = runCli(run3({"report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_no_offer") && has(c, "state=not_offered"));
    c = runCli(run3({"offer@0", "decline", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_no_offer") && has(c, "state=declined"));
    c = runCli(run3({"day=7", "offer@0", "accept", "arrive@ore_lift", "report@sewer_grate", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_repeat_done"));
    CHECK(has(c, "Day 7. Enough."));
    CHECK(has(c, "reward_rolled=1 rolls=1"));
    c = runCli(run3({"char=A", "offer@0", "accept", "arrive@ore_lift", "char=B", "report@sewer_grate", "char=A", "report@sewer_grate"}));
    CHECK_EQ(c.code, 0);
    std::size_t nb = c.out.find("key=fig_report_no_offer"), pa = c.out.find("key=fig_report_present_cold");
    CHECK(nb != std::string::npos && pa != std::string::npos && nb < pa);
    CHECK(has(c, "state=completed"));
    c = runCli(run3({"char=A", "offer@0", "accept", "arrive@ore_lift:present", "char=B", "stage=2", "char=A", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_present_cold") && !has(c, "present_warm"));
}

TEST(quest_cli_hostile) {
    Cli c = runCli(run3({"offer@0", "accept", "arrive@ore_lift:present", "stage=-2", "report@sewer_grate"}));
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "key=fig_report_present_cold log=log_quest_done") && has(c, "state=completed") && !has(c, "NO_LINE"));
    c = runCli(run3({"offer@-1", "accept", "arrive@ore_lift:gone", "stage=-2", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_gone_cold") && has(c, "state=completed"));
    c = runCli(run3({"offer@-1", "accept", "stage=-2", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_not_yet") && has(c, "state=active"));
    c = runCli(run3({"offer@0", "decline", "stage=-2", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_no_offer") && has(c, "state=declined"));
    c = runCli(run3({"stage=-2", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_report_no_offer") && has(c, "state=not_offered"));
    c = runCli(run3({"offer@0", "accept", "arrive@ore_lift:present", "report@sewer_grate", "stage=-2", "report@sewer_grate"}));
    CHECK(has(c, "key=fig_repeat_done"));
    CHECK(has(c, "reward_rolled=1 rolls=1"));
    std::size_t first = c.out.find("log=log_quest_done");
    CHECK(first != std::string::npos && c.out.find("log=log_quest_done", first + 1) == std::string::npos);
    for (const char* bad : {"stage=3", "stage=-3", "stage=abc", "stage="}) {
        c = runCli(run3({"offer@0", bad}));
        CHECK_EQ(c.code, 1);
        CHECK(has(c, "FAIL STAGE_RANGE"));
    }
    for (const char* s : {"stage=-2", "stage=2"}) {
        c = runCli(run3({"offer@0", "accept", s}));
        CHECK_EQ(c.code, 0);
        CHECK(has(c, "state=active chain=none"));
        CHECK(!has(c, "stage="));
    }
    c = runCli(run3({"offer@0", "stage=-2", "accept"}));
    CHECK(has(c, "accept ok") && has(c, "state=active"));
}

int main(int argc, char** argv) { return dd_test::runMain(argc, argv); }

TEST(quest_cli_reward) {
    Cli c = runCli({"reward", "roll=0.0"});
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "key=fig_reward_item") && has(c, "log=log_reward_item"));
    Cli again = runCli({"reward", "roll=0.0"});
    CHECK_EQ(c.out, again.out);
    c = runCli({"reward", "roll=0.999999"});
    CHECK(has(c, "key=fig_reward_none") && has(c, "log=log_reward_none"));
    c = runCli({"reward", "roll=0.0", "--items", ""});
    CHECK_EQ(c.code, 0);
    CHECK(has(c, "key=fig_reward_none"));
    for (const char* bad : {"roll=1.5", "roll=-0.1", "roll=1.0", "roll=abc", "roll="}) {
        c = runCli({"reward", bad});
        CHECK_EQ(c.code, 1);
        CHECK(has(c, "FAIL ROLL_RANGE"));
    }
    CHECK_EQ(runCli({"reward", "7"}).out, runCli({"reward", "7"}).out);
    CHECK_EQ(runCli({"reward", "x"}).code, 1);
    CHECK_EQ(runCli({"reward"}).code, 2);
    // bad events and bad files
    c = runCli(run3({"dance"}));
    CHECK_EQ(c.code, 2);
    c = runCli(run3({"offer@9"}));
    CHECK_EQ(c.code, 1);
    c = runCli(run3({"roll=2"}));
    CHECK_EQ(c.code, 1);
    // injected rolls in run: roll below the chance gives the item
    c = runCli(run3({"offer@0", "accept", "arrive@ore_lift", "roll=0.0", "report@sewer_grate"}));
    CHECK(has(c, "log=log_reward_item") && has(c, "Test_Item"));
}
