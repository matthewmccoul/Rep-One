#include <iostream>

#include "dd/fileio.h"
#include "dd/map.h"
#include "dd/map_cli.h"

namespace dd {
namespace map {

namespace {
const char* kUsage =
    "usage: map_check validate <map.json> | neighbours <map.json> <node> | door <map.json> <edge> |\n"
    "       ways <map.json> <edge> | reach <map.json> <from> <to> [blocked_edge ...]\n";

int failLine(std::ostream& out, const std::string& code, const std::string& detail) {
    out << "FAIL " << code << ": " << detail << "\n";
    return 1;
}
}  // namespace

int mapCheckMain(const std::vector<std::string>& args, std::ostream& out) {
    if (args.size() < 2) { out << kUsage; return 2; }
    const std::string& cmd = args[0];
    std::size_t need;  // exact argument count (including command), 0 = variable
    if (cmd == "validate") need = 2;
    else if (cmd == "neighbours" || cmd == "door" || cmd == "ways") need = 3;
    else if (cmd == "reach") need = 0;
    else { out << kUsage; return 2; }
    if (need && args.size() != need) { out << kUsage; return 2; }
    if (cmd == "reach" && args.size() < 4) { out << kUsage; return 2; }

    std::string text;
    if (!readFile(args[1], text)) {
        out << "ERROR cannot read file: " << args[1] << "\n";
        return 2;
    }
    FloorMap m;
    Issue err;
    if (!fromText(text, m, err)) return failLine(out, err.code, err.detail);

    if (cmd == "validate") {
        std::vector<Issue> is = validate(m);
        if (!is.empty()) {
            failLine(out, is[0].code, is[0].detail);
            for (std::size_t i = 1; i < is.size(); ++i) out << "  also " << is[i].code << ": " << is[i].detail << "\n";
            return 1;
        }
        out << "OK map nodes=" << m.nodes.size() << " edges=" << m.edges.size() << "\n";
        return 0;
    }
    if (cmd == "neighbours") {
        if (!m.node(args[2])) return failLine(out, "UNKNOWN_NODE", args[2]);
        for (const std::string& n : m.neighbours(args[2])) out << n << "\n";
        return 0;
    }
    if (cmd == "door" || cmd == "ways") {
        const Edge* e = m.edge(args[2]);
        if (!e) return failLine(out, "UNKNOWN_EDGE", args[2]);
        if (cmd == "door") { out << e->door << "\n"; return 0; }
        out << e->id << " =";
        if (e->solutions.empty()) out << " (none)";
        for (std::size_t i = 0; i < e->solutions.size(); ++i) out << (i ? ", " : " ") << e->solutions[i];
        out << "\n";
        return 0;
    }
    // reach
    if (!m.node(args[2])) return failLine(out, "UNKNOWN_NODE", args[2]);
    if (!m.node(args[3])) return failLine(out, "UNKNOWN_NODE", args[3]);
    ReachOptions opt;
    for (std::size_t i = 4; i < args.size(); ++i) {
        if (!m.edge(args[i])) return failLine(out, "UNKNOWN_EDGE", args[i]);
        opt.blockedEdges.insert(args[i]);
    }
    out << (m.canReach(args[2], args[3], opt) ? "reachable" : "unreachable") << "\n";
    return 0;
}

}  // namespace map
}  // namespace dd
