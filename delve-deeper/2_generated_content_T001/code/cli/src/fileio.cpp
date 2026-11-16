#include "dd/fileio.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include "dd/csv.h"
#include "dd/json.h"

namespace dd {

bool readFile(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    if (f.bad()) return false;
    out = ss.str();
    return true;
}

namespace {
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
}  // namespace

int comCheckMain(const std::vector<std::string>& args, std::ostream& out) {
    if (args.size() < 2 || (args[0] != "json" && args[0] != "csv")) {
        out << "usage: com_check json <file> | com_check csv <file> [--require col,col]\n";
        return 2;
    }
    const std::string& mode = args[0];
    const std::string& path = args[1];
    std::vector<std::string> required = {"text"};
    if (mode == "json") {
        if (args.size() != 2) { out << "usage: com_check json <file>\n"; return 2; }
    } else if (args.size() == 4 && args[2] == "--require") {
        required = splitCommas(args[3]);
    } else if (args.size() != 2) {
        out << "usage: com_check csv <file> [--require col,col]\n";
        return 2;
    }
    std::string text;
    if (!readFile(path, text)) {
        out << "ERROR cannot read file: " << path << "\n";
        return 2;
    }
    if (mode == "json") {
        json::Value v;
        json::Error e;
        if (!json::parse(text, v, e)) {
            out << "FAIL " << e.code << ": " << e.describe() << "\n";
            return 1;
        }
        out << "OK json\n";
        return 0;
    }
    csv::Table t;
    csv::Error e;
    if (!csv::parse(text, t, e, required)) {
        out << "FAIL " << e.code << ": " << e.describe() << "\n";
        return 1;
    }
    out << "OK csv columns=" << t.header.size() << " rows=" << t.rows.size() << "\n";
    return 0;
}

}  // namespace dd
