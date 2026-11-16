// NOT CORE. eqv_check <golden.json> [--puzzle <puzzle.json>]
// Replays every golden vector (made by tools/gen_golden.py from the Python reference) through the C++
// puzzle core and compares each per-op result and the final state.
// Exit 0 all match ("N vectors, 0 mismatches"); 1 mismatch or bad golden file (first line names the
// vector id, or "FAIL <CODE>: ..."); 2 usage or I/O error.
// The puzzle data is the REAL storeroom_puzzle.json (not taken from the golden file), so a golden file made
// from a reference with a different solution order must fail.
#include <iostream>
#include <string>
#include <vector>

#include "dd/fileio.h"
#include "dd/json.h"
#include "dd/puzzle.h"

#ifndef EQV_DEFAULT_PUZZLE
#define EQV_DEFAULT_PUZZLE "work/T001/puzzle/storeroom_puzzle.json"
#endif

namespace {

using dd::json::Type;
using dd::json::Value;

int fail(const std::string& code, const std::string& detail) {
    std::cout << "FAIL " << code << ": " << detail << "\n";
    return 1;
}

const Value* field(const Value& o, const char* key, Type t) {
    const Value* v = o.find(key);
    return (v && v->type == t) ? v : nullptr;
}

std::string joinProgress(const std::vector<std::string>& p) {
    std::string s;
    for (std::size_t i = 0; i < p.size(); ++i) s += (i ? "," : "") + p[i];
    return s;
}

// Compares the final state with the expected to_dict object. Returns "" when equal.
std::string compareState(const dd::puzzle::Puzzle& p, const Value& e) {
    const Value* progress = field(e, "progress", Type::Array);
    const Value* solved = field(e, "solved", Type::Bool);
    const Value* door = field(e, "door_open", Type::Bool);
    const Value* via = e.find("opened_via");
    const Value* wrong = field(e, "wrong_count", Type::Number);
    const Value* xp = field(e, "xp_awarded", Type::Bool);
    if (!progress || !solved || !door || !via || !wrong || !xp || !wrong->isInteger ||
        !(via->isNull() || via->isString()))
        return "expected state has a missing or wrong-type field";
    std::vector<std::string> ep;
    for (const Value& it : progress->items) {
        if (!it.isString()) return "expected progress holds a non-string";
        ep.push_back(it.string);
    }
    std::string d;
    if (ep != p.progress()) d += " progress(expected=" + joinProgress(ep) + " got=" + joinProgress(p.progress()) + ")";
    if (solved->boolean != p.solved()) d += std::string(" solved(expected=") + (solved->boolean ? "true" : "false") + ")";
    if (door->boolean != p.doorOpen()) d += std::string(" door_open(expected=") + (door->boolean ? "true" : "false") + ")";
    std::string gotVia = p.openedVia();
    std::string expVia = via->isNull() ? "" : via->string;
    if (gotVia != expVia) d += " opened_via(expected=" + (via->isNull() ? std::string("none") : expVia) + " got=" + (gotVia.empty() ? "none" : gotVia) + ")";
    if (wrong->integer != p.wrongCount()) d += " wrong_count(expected=" + std::to_string(wrong->integer) + " got=" + std::to_string(p.wrongCount()) + ")";
    if (xp->boolean != p.xpAwarded()) d += std::string(" xp_awarded(expected=") + (xp->boolean ? "true" : "false") + ")";
    return d;
}

// Replays one vector. Returns "" on match, otherwise a description of the first problem.
std::string replay(const std::string& puzzleText, const Value& vec) {
    const Value* ops = field(vec, "ops", Type::Array);
    const Value* results = field(vec, "results", Type::Array);
    const Value* expected = field(vec, "expected", Type::Object);
    if (!ops || !results || !expected || ops->items.size() != results->items.size())
        return "vector has no ops/results/expected, or their lengths differ";
    dd::puzzle::Puzzle p;
    dd::puzzle::Issue err;
    if (!dd::puzzle::Puzzle::fromText(puzzleText, p, err)) return "puzzle data rejected: " + err.code;
    for (std::size_t i = 0; i < ops->items.size(); ++i) {
        const Value& op = ops->items[i];
        const Value& res = results->items[i];
        const Value* kind = field(op, "op", Type::String);
        if (!kind) return "op " + std::to_string(i) + " has no name";
        const Value* want = res.find("result");
        std::string where = "op " + std::to_string(i) + " (" + kind->string + ")";
        if (kind->string == "use") {
            const Value* c = field(op, "control", Type::String);
            if (!c) return where + " has no control";
            dd::puzzle::UseOutcome o = p.use(c->string);
            if (o.result == dd::puzzle::UseResult::UnknownControl) {
                if (!want || !want->isString() || want->string != "error") return where + " " + c->string + ": C++ rejects an unknown control, reference did not";
                continue;
            }
            const Value* sk = res.find("string_key");
            if (!want || !want->isString() || want->string != o.resultKey)
                return where + " " + c->string + ": result expected=" + (want && want->isString() ? want->string : "?") + " got=" + o.resultKey;
            if (!sk || !sk->isString() || sk->string != o.stringKey)
                return where + " " + c->string + ": string_key expected=" + (sk && sk->isString() ? sk->string : "?") + " got=" + o.stringKey;
        } else if (kind->string == "open") {
            const Value* r = field(op, "route", Type::String);
            if (!r) return where + " has no route";
            dd::puzzle::BypassResult b = p.openOther(r->string);
            std::string got = b == dd::puzzle::BypassResult::Applied ? "applied"
                              : b == dd::puzzle::BypassResult::NotApplied ? "not_applied" : "error";
            if (!want || !want->isString() || want->string != got)
                return where + " " + r->string + ": result expected=" + (want && want->isString() ? want->string : "?") + " got=" + got;
        } else if (kind->string == "claim_xp") {
            std::string got = p.claimXp();
            bool ok = got.empty() ? (want && want->isNull()) : (want && want->isString() && want->string == got);
            if (!ok) return where + ": claim expected differs, got '" + got + "'";
        } else if (kind->string == "reset") {
            p.reset();
        } else if (kind->string == "save_load") {
            // C++ writes its own save, loads it into a fresh puzzle; the saved state must equal the reference's.
            std::string saved = p.toJson();
            const Value* sv = res.find("saved");
            if (!sv || !sv->isObject()) return where + ": golden has no saved state";
            std::string diff = compareState(p, *sv);
            if (!diff.empty()) return where + ": state before save differs:" + diff;
            dd::puzzle::Puzzle q(p.data());
            dd::puzzle::Issue le;
            if (!q.loadState(saved, le)) return where + ": C++ cannot load its own save: " + le.code;
            p = q;
        } else {
            return where + ": unknown op";
        }
    }
    std::string diff = compareState(p, *expected);
    if (!diff.empty()) return "final state differs:" + diff;
    return "";
}

}  // namespace

int main(int argc, char** argv) {
    std::string golden, puzzlePath = EQV_DEFAULT_PUZZLE;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--puzzle") {
            if (i + 1 >= argc) { std::cout << "usage: eqv_check <golden.json> [--puzzle <puzzle.json>]\n"; return 2; }
            puzzlePath = argv[++i];
        } else if (golden.empty()) {
            golden = a;
        } else {
            std::cout << "usage: eqv_check <golden.json> [--puzzle <puzzle.json>]\n";
            return 2;
        }
    }
    if (golden.empty()) { std::cout << "usage: eqv_check <golden.json> [--puzzle <puzzle.json>]\n"; return 2; }

    std::string gtext, ptext;
    if (!dd::readFile(golden, gtext)) { std::cout << "ERROR cannot read file: " << golden << "\n"; return 2; }
    if (!dd::readFile(puzzlePath, ptext)) { std::cout << "ERROR cannot read file: " << puzzlePath << "\n"; return 2; }

    Value root;
    dd::json::Error jerr;
    if (!dd::json::parse(gtext, root, jerr)) return fail(jerr.code, jerr.describe());
    if (!root.isObject()) return fail("GOLDEN_SHAPE", "top level is not an object");
    const Value* vecs = field(root, "vectors", Type::Array);
    if (!vecs) return fail("GOLDEN_SHAPE", "no \"vectors\" array");
    if (vecs->items.empty()) return fail("NO_VECTORS", "the golden file holds zero vectors");

    std::size_t mismatches = 0;
    for (const Value& v : vecs->items) {
        const Value* id = field(v, "id", Type::String);
        std::string name = id ? id->string : "<no id>";
        std::string d = id ? replay(ptext, v) : "vector has no id";
        if (!d.empty()) {
            ++mismatches;
            if (mismatches <= 25) std::cout << "MISMATCH " << name << ": " << d << "\n";
        }
    }
    if (mismatches > 25) std::cout << "... " << (mismatches - 25) << " more mismatches not shown\n";
    std::cout << vecs->items.size() << " vectors, " << mismatches << " mismatches\n";
    return mismatches ? 1 : 0;
}
