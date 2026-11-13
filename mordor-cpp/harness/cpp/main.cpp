// C++ side of the harness: loads GameData, runs every registered copy on every case with the same scripted draws.
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include "case_api.h"
using namespace mordor;
static Str readAll(const char* p) { std::ifstream f(p); std::stringstream s; s << f.rdbuf(); return s.str(); }
int main(int argc, char** argv) {
    if (argc < 3) { std::fprintf(stderr, "usage: harness <combat-data.json> <cases.json>\n"); return 2; }
    GameData data; Str err;
    if (!loadGameDataFromJson(readAll(argv[1]), data, err)) { std::fprintf(stderr, "data: %s\n", err.c_str()); return 3; }
    JsonValue cases; if (!parseJson(readAll(argv[2]), cases, err)) { std::fprintf(stderr, "cases: %s\n", err.c_str()); return 4; }
    for (std::size_t c = 0; c < kNumCopies; ++c) {
        for (std::size_t k = 0; k < cases.arr.size(); ++k) {
            const JsonValue& cs = cases.arr[k];
            Vec<double> draws; if (const JsonValue* r = cs.find("rng")) for (const auto& x : r->arr) draws.push_back(x.asDouble());
            ScriptedRng rng(draws); EventLog log; Ctx ctx(data, rng, log);
            const JsonValue* a = cs.find("args"); static const JsonValue empty;
            Str res = kCopies[c].fn(a ? *a : empty, ctx);
            std::printf("%s\t%zu\t%s\n", kCopies[c].name, k, res.c_str());
        }
    }
    return 0;
}
