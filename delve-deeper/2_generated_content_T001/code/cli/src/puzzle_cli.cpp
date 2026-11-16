#include "dd/puzzle_cli.h"

#include <iostream>

#include "dd/fileio.h"
#include "dd/map.h"
#include "dd/puzzle.h"

namespace dd {
namespace puzzle {

namespace {

const char* kUsage =
    "usage: puzzle_check validate <puzzle.json> <map.json> | seq <puzzle.json> <control>... |\n"
    "       bypass <puzzle.json> <route> [<control>...] | roundtrip <puzzle.json> <control>... |\n"
    "       load <puzzle.json> <state.json>\n";

int failLine(std::ostream& out, const std::string& code, const std::string& detail) {
    out << "FAIL " << code << ": " << detail << "\n";
    return 1;
}

int cannotRead(std::ostream& out, const std::string& path) {
    out << "ERROR cannot read file: " << path << "\n";
    return 2;
}

// Presses each control, printing "<control> <result> <string key>". False (after printing FAIL) on an
// unknown control.
bool press(Puzzle& p, const std::vector<std::string>& controls, std::size_t from, std::ostream& out) {
    for (std::size_t i = from; i < controls.size(); ++i) {
        UseOutcome o = p.use(controls[i]);
        if (o.result == UseResult::UnknownControl) {
            failLine(out, "UNKNOWN_CONTROL", controls[i]);
            return false;
        }
        out << controls[i] << " " << o.resultKey << " " << o.stringKey << "\n";
    }
    return true;
}

}  // namespace

int puzzleCheckMain(const std::vector<std::string>& args, std::ostream& out) {
    if (args.size() < 2) { out << kUsage; return 2; }
    const std::string& cmd = args[0];
    if (cmd == "validate" || cmd == "load") {
        if (args.size() != 3) { out << kUsage; return 2; }
    } else if (cmd == "bypass") {
        if (args.size() < 3) { out << kUsage; return 2; }
    } else if (cmd != "seq" && cmd != "roundtrip") {
        out << kUsage;
        return 2;
    }

    std::string text;
    if (!readFile(args[1], text)) return cannotRead(out, args[1]);
    Puzzle p;
    Issue err;
    if (!Puzzle::fromText(text, p, err)) return failLine(out, err.code, err.detail);

    if (cmd == "validate") {
        std::string mapText;
        if (!readFile(args[2], mapText)) return cannotRead(out, args[2]);
        map::FloorMap m;
        map::Issue merr;
        if (!map::fromText(mapText, m, merr)) return failLine(out, merr.code, merr.detail);
        std::vector<Issue> is = validateAgainstMap(p.data(), m);
        if (!is.empty()) {
            failLine(out, is[0].code, is[0].detail);
            for (std::size_t i = 1; i < is.size(); ++i) out << "  also " << is[i].code << ": " << is[i].detail << "\n";
            return 1;
        }
        out << "OK puzzle controls=" << p.data().controls.size() << " solution=" << p.data().solution.size() << "\n";
        return 0;
    }

    if (cmd == "load") {
        std::string stateText;
        if (!readFile(args[2], stateText)) return cannotRead(out, args[2]);
        if (!p.loadState(stateText, err)) return failLine(out, err.code, err.detail);
        out << "OK load\n" << p.summary() << "\n";
        return 0;
    }

    if (cmd == "seq") {
        if (!press(p, args, 2, out)) return 1;
        out << p.summary() << "\n";
        return 0;
    }

    if (cmd == "bypass") {
        // bypass <puzzle> <route> [controls]: controls first, then the route, then one more press of the
        // first control (shows the controls are inert afterwards). Contract detail from check_spec PZL-19.
        const std::string& route = args[2];
        if (!p.data().hasRoute(route)) return failLine(out, "UNKNOWN_ROUTE", route);
        if (!press(p, args, 3, out)) return 1;
        BypassResult r = p.openOther(route);
        out << "route " << route << " " << (r == BypassResult::Applied ? "applied" : "not_applied") << "\n";
        if (!press(p, std::vector<std::string>{p.data().controls[0]}, 0, out)) return 1;
        out << p.summary() << "\n";
        return 0;
    }

    // roundtrip: press the controls, save, load into a fresh puzzle, compare.
    if (!press(p, args, 2, out)) return 1;
    std::string saved = p.toJson();
    Puzzle q(p.data());
    if (!q.loadState(saved, err)) return failLine(out, "ROUNDTRIP", "own save rejected: " + err.code + ": " + err.detail);
    if (q.toJson() != saved) return failLine(out, "ROUNDTRIP", "state after load differs from state before save");
    out << "saved " << saved << "\n";
    out << q.summary() << "\n";
    return 0;
}

}  // namespace puzzle
}  // namespace dd
