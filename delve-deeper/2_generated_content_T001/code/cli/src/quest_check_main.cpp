// quest_check: command-line check tool for the Figure quest (NOT core: reads files, uses iostream).
//   quest_check validate <quest.json> <strings.csv> <map.json>
//   quest_check stage <score>
//   quest_check line <strings.csv> <key> <stage>
//   quest_check fill <strings.csv> <key> <stage> <name=value>...
//   quest_check run <quest.json> <strings.csv> <map.json> <event>...
//   quest_check reward <seed|roll=<0..1>> [--chance <c>] [--items a,b]
// Exit 0 accepted, 1 rejected (stdout line "FAIL <CODE> <detail>"), 2 usage or I/O error.
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "dd/fileio.h"
#include "dd/map.h"
#include "dd/quest.h"

namespace {

using dd::quest::Issue;

int usage() {
    std::cout << "usage: quest_check validate <quest.json> <strings.csv> <map.json>\n"
                 "       quest_check stage <score>\n"
                 "       quest_check line <strings.csv> <key> <stage>\n"
                 "       quest_check fill <strings.csv> <key> <stage> <name=value>...\n"
                 "       quest_check run <quest.json> <strings.csv> <map.json> <event>...\n"
                 "       quest_check reward <seed|roll=<0..1>> [--chance <c>] [--items a,b]\n";
    return 2;
}

int fail(const std::string& code, const std::string& detail) {
    std::cout << "FAIL " << code;
    if (!detail.empty()) std::cout << " " << detail;
    std::cout << "\n";
    return 1;
}

int fail(const Issue& i) { return fail(i.code, i.detail); }

bool readOrReport(const std::string& path, std::string& out) {
    if (dd::readFile(path, out)) return true;
    std::cout << "ERROR cannot read file: " << path << "\n";
    return false;
}

bool parseInt(const std::string& s, int& out) {
    if (s.empty() || s.size() > 9) return false;
    std::size_t i = (s[0] == '-') ? 1 : 0;
    if (i >= s.size()) return false;
    long v = 0;
    for (std::size_t k = i; k < s.size(); ++k) {
        if (s[k] < '0' || s[k] > '9') return false;
        v = v * 10 + (s[k] - '0');
    }
    out = static_cast<int>(i ? -v : v);
    return true;
}

bool parseStage(const std::string& s, int& out) { return parseInt(s, out) && out >= -2 && out <= 2; }

bool parseDouble(const std::string& s, double& out) {
    if (s.empty() || s[0] == ' ') return false;
    char* end = nullptr;
    double v = std::strtod(s.c_str(), &end);
    if (end == s.c_str() || *end != '\0') return false;
    out = v;
    return true;
}

std::vector<std::string> splitCommas(const std::string& s) {
    std::vector<std::string> r;
    std::string cur;
    for (char c : s) {
        if (c == ',') { if (!cur.empty()) r.push_back(cur); cur.clear(); }
        else cur += c;
    }
    if (!cur.empty()) r.push_back(cur);
    return r;
}

struct Loaded {
    dd::quest::QuestData q;
    dd::map::FloorMap m;
};

// Loads quest, strings and map and validates. Returns 0 when usable, else the exit code (message printed).
int loadAll(const std::string& qp, const std::string& cp, const std::string& mp, Loaded& l) {
    std::string qt, ct, mt;
    if (!readOrReport(qp, qt) || !readOrReport(cp, ct) || !readOrReport(mp, mt)) return 2;
    dd::map::Issue me;
    if (!dd::map::fromText(mt, l.m, me)) return fail(me.code, me.detail);
    Issue e;
    if (!dd::quest::load(qt, ct, l.q, e)) return fail(e);
    std::vector<Issue> issues = dd::quest::validate(l.q, l.m);
    if (!issues.empty()) {
        fail(issues[0]);
        for (std::size_t i = 1; i < issues.size(); ++i) std::cout << "  also " << issues[i].code << " " << issues[i].detail << "\n";
        return 1;
    }
    return 0;
}

int cmdValidate(const std::vector<std::string>& a) {
    if (a.size() != 4) return usage();
    Loaded l;
    int rc = loadAll(a[1], a[2], a[3], l);
    if (rc) return rc;
    std::cout << "OK quest rows=" << l.q.strings.rows.size() << " route=" << l.q.route.size() << "\n";
    return 0;
}

int cmdStage(const std::vector<std::string>& a) {
    if (a.size() != 2) return usage();
    int score = 0, stage = 0;
    if (!parseInt(a[1], score) || !dd::quest::stageFromScore(score, stage)) return fail("SCORE_RANGE", a[1]);
    std::cout << stage << "\n";
    return 0;
}

int cmdLine(const std::vector<std::string>& a) {
    if (a.size() != 4) return usage();
    std::string ct;
    if (!readOrReport(a[1], ct)) return 2;
    dd::quest::Strings s;
    Issue e;
    if (!dd::quest::loadStrings(ct, s, e)) return fail(e);
    int stage = 0;
    if (!parseStage(a[3], stage)) return fail("STAGE_RANGE", a[3]);
    if (!s.find(a[2])) return fail("UNKNOWN_KEY", a[2]);
    const dd::quest::StringRow* r = s.forStage(a[2], stage);
    if (!r) return fail("NO_LINE", a[2] + " at stage " + a[3]);
    std::cout << r->text << "\n";
    return 0;
}

int cmdFill(const std::vector<std::string>& a) {
    if (a.size() < 4) return usage();
    std::string ct;
    if (!readOrReport(a[1], ct)) return 2;
    dd::quest::Strings s;
    Issue e;
    if (!dd::quest::loadStrings(ct, s, e)) return fail(e);
    int stage = 0;
    if (!parseStage(a[3], stage)) return fail("STAGE_RANGE", a[3]);
    std::map<std::string, std::string> values;
    for (std::size_t i = 4; i < a.size(); ++i) {
        std::size_t eq = a[i].find('=');
        if (eq == std::string::npos || eq == 0) return usage();
        values[a[i].substr(0, eq)] = a[i].substr(eq + 1);
    }
    std::string out;
    if (!dd::quest::fillLine(s, a[2], stage, values, out, e)) return fail(e);
    std::cout << out << "\n";
    return 0;
}

// Reward source for tests and the command line.
class FixedSource : public dd::quest::RewardSource {
public:
    double value = 0.999999;
    int calls = 0;
    double roll() override { ++calls; return value; }
};

double fromSeed(std::uint64_t z) {
    z += 0x9E3779B97F4A7C15ULL;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    z ^= z >> 31;
    return static_cast<double>(z >> 11) / 9007199254740992.0;
}

// Command-line test default only: the real chance and item list are TBD in the content (Systems).
dd::quest::RewardConfig defaultReward() {
    dd::quest::RewardConfig c;
    c.itemChance = 0.5;
    c.items.push_back("Test_Item");
    return c;
}

std::string lowerCode(std::string s) {
    for (char& c : s)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    return s;
}

int cmdReward(const std::vector<std::string>& a) {
    if (a.size() < 2) return usage();
    dd::quest::RewardConfig cfg = defaultReward();
    for (std::size_t i = 2; i < a.size(); i += 2) {
        if (i + 1 >= a.size()) return usage();
        if (a[i] == "--chance") {
            double c = 0;
            if (!parseDouble(a[i + 1], c) || c < 0 || c > 1) return fail("CHANCE_RANGE", a[i + 1]);
            cfg.itemChance = c;
        } else if (a[i] == "--items") {
            cfg.items = splitCommas(a[i + 1]);
        } else {
            return usage();
        }
    }
    double x = 0;
    if (a[1].compare(0, 5, "roll=") == 0) {
        if (!parseDouble(a[1].substr(5), x) || !(x >= 0.0 && x < 1.0)) return fail("ROLL_RANGE", a[1].substr(5));
    } else {
        int seed = 0;
        if (!parseInt(a[1], seed) || seed < 0) return fail("BAD_SEED", a[1]);
        x = fromSeed(static_cast<std::uint64_t>(seed));
    }
    std::string item;
    if (cfg.itemChance > 0.0 && !cfg.items.empty() && x < cfg.itemChance) {
        std::size_t idx = static_cast<std::size_t>(x / cfg.itemChance * static_cast<double>(cfg.items.size()));
        if (idx >= cfg.items.size()) idx = cfg.items.size() - 1;
        item = cfg.items[idx];
    }
    std::cout << "reward roll=" << x << " key=" << (item.empty() ? "fig_reward_none" : "fig_reward_item")
              << " log=" << (item.empty() ? "log_reward_none" : "log_reward_item");
    if (!item.empty()) std::cout << " item=" << item;
    std::cout << "\n";
    return 0;
}

void printStep(const std::string& event, const dd::quest::Step& s) {
    if (!s.ok) {
        std::cout << event << " rejected " << lowerCode(s.code) << " state=" << dd::quest::stateName(s.after) << "\n";
        return;
    }
    std::cout << event << " ok";
    for (const dd::quest::Line& l : s.lines) {
        const char* tag = l.kind == "log" ? "log" : l.kind == "narration" ? "text" : "key";
        std::cout << " " << tag << "=" << l.key;
    }
    if (s.refused) std::cout << " refused=1";
    std::cout << " state=" << dd::quest::stateName(s.after) << "\n";
    for (const dd::quest::Line& l : s.lines) std::cout << "  " << l.key << ": " << l.text << "\n";
}

int cmdRun(const std::vector<std::string>& a) {
    if (a.size() < 4) return usage();
    Loaded l;
    int rc = loadAll(a[1], a[2], a[3], l);
    if (rc) return rc;
    dd::quest::Engine eng(l.q, l.m);
    eng.setRewardConfig(defaultReward());
    FixedSource src;
    dd::quest::Context ctx;
    std::map<std::string, int> standing;
    std::string ch = "default";
    for (std::size_t i = 4; i < a.size(); ++i) {
        const std::string& ev = a[i];
        dd::quest::Step st;
        int& cur = standing[ch];
        if (ev.compare(0, 6, "offer@") == 0) {
            int s = 0;
            if (!parseStage(ev.substr(6), s)) return fail("STAGE_RANGE", ev);
            cur = s;
            st = eng.offer(ch, cur, ctx);
        } else if (ev == "accept") {
            st = eng.accept(ch, cur, ctx);
        } else if (ev == "decline") {
            st = eng.decline(ch, cur, ctx);
        } else if (ev.compare(0, 7, "arrive@") == 0) {
            std::string rest = ev.substr(7), node = rest, chain;
            std::size_t colon = rest.find(':');
            if (colon != std::string::npos) { node = rest.substr(0, colon); chain = rest.substr(colon + 1); }
            st = eng.arrive(ch, node, chain, ctx);
        } else if (ev.compare(0, 7, "report@") == 0) {
            st = eng.report(ch, ev.substr(7), cur, ctx, src);
        } else if (ev.compare(0, 5, "char=") == 0) {
            if (ev.size() == 5) return fail("BAD_CHAR", ev);
            ch = ev.substr(5);
            std::cout << ev << "\n";
            continue;
        } else if (ev.compare(0, 6, "stage=") == 0) {
            int s = 0;
            if (!parseStage(ev.substr(6), s)) return fail("STAGE_RANGE", ev);
            cur = s;
            continue;  // sets the standing only: no state change, no output
        } else if (ev.compare(0, 4, "day=") == 0) {
            int d = 0;
            if (!parseInt(ev.substr(4), d) || d < 0) return fail("BAD_DAY", ev);
            ctx.day = d;
            continue;
        } else if (ev.compare(0, 5, "roll=") == 0) {
            double x = 0;
            if (!parseDouble(ev.substr(5), x) || !(x >= 0.0 && x < 1.0)) return fail("ROLL_RANGE", ev);
            src.value = x;
            continue;
        } else {
            std::cout << "unknown event: " << ev << "\n";
            return 2;
        }
        if (st.fatal) return fail(st.code, st.detail);
        printStep(ev, st);
    }
    dd::quest::Record r = eng.record(ch);
    std::cout << "state=" << dd::quest::stateName(r.state) << " chain=" << (r.chain.empty() ? "none" : r.chain)
              << " reward_rolled=" << (r.rewardRolled ? 1 : 0) << " rolls=" << src.calls << "\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    std::vector<std::string> a(argv + 1, argv + argc);
    if (a.empty()) return usage();
    if (a[0] == "validate") return cmdValidate(a);
    if (a[0] == "stage") return cmdStage(a);
    if (a[0] == "line") return cmdLine(a);
    if (a[0] == "fill") return cmdFill(a);
    if (a[0] == "run") return cmdRun(a);
    if (a[0] == "reward") return cmdReward(a);
    return usage();
}
