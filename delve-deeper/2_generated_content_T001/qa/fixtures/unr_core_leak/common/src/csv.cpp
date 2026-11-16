#include "dd/csv.h"

#include <set>

namespace dd {
namespace csv {

int Table::column(const std::string& name) const {
    for (std::size_t i = 0; i < header.size(); ++i)
        if (header[i] == name) return static_cast<int>(i);
    return -1;
}

std::string Error::describe() const {
    return "line " + std::to_string(line) + ": " + message;
}

namespace {

struct Reader {
    const std::string& s;
    std::size_t pos = 0;
    std::size_t line = 1;

    explicit Reader(const std::string& t) : s(t) {}

    bool fail(Error& err, const char* code, const std::string& msg, std::size_t atLine) {
        err.code = code;
        err.message = msg;
        err.line = atLine;
        return false;
    }

    // Reads one record into fields. Returns false on error.
    bool readRecord(std::vector<std::string>& fields, Error& err, std::size_t& startLine) {
        fields.clear();
        startLine = line;
        for (;;) {
            std::string field;
            if (pos < s.size() && s[pos] == '"') {
                std::size_t openLine = line;
                ++pos;
                bool closed = false;
                while (pos < s.size()) {
                    char c = s[pos];
                    if (c == '"') {
                        if (pos + 1 < s.size() && s[pos + 1] == '"') { field += '"'; pos += 2; continue; }
                        ++pos;
                        closed = true;
                        break;
                    }
                    if (c == '\n') ++line;
                    field += c;
                    ++pos;
                }
                if (!closed) return fail(err, "CSV_OPEN_QUOTE", "unterminated quoted field", openLine);
                if (pos < s.size() && s[pos] != ',' && s[pos] != '\n' && s[pos] != '\r')
                    return fail(err, "CSV_BAD_QUOTE", "text after closing quote", line);
            } else {
                while (pos < s.size() && s[pos] != ',' && s[pos] != '\n' && s[pos] != '\r') {
                    if (s[pos] == '"') return fail(err, "CSV_BAD_QUOTE", "quote inside an unquoted field", line);
                    field += s[pos];
                    ++pos;
                }
            }
            fields.push_back(field);
            if (pos >= s.size()) return true;
            char c = s[pos];
            if (c == ',') { ++pos; continue; }
            if (c == '\n') { ++pos; ++line; return true; }
            // c == '\r'
            if (pos + 1 < s.size() && s[pos + 1] == '\n') { pos += 2; ++line; return true; }
            return fail(err, "CSV_BAD_LINE_END", "bare carriage return", line);
        }
    }
};

}  // namespace

bool parse(const std::string& text, Table& out, Error& err, const std::vector<std::string>& required) {
    out = Table();
    if (text.empty()) {
        err.code = "CSV_EMPTY";
        err.message = "empty file (no header row)";
        err.line = 0;
        return false;
    }
    Reader r(text);
    std::vector<std::string> fields;
    std::size_t startLine = 1;
    bool first = true;
    while (r.pos < text.size()) {
        if (!r.readRecord(fields, err, startLine)) return false;
        if (first) {
            first = false;
            std::set<std::string> seen;
            for (const std::string& h : fields) {
                if (h.empty()) {
                    err.code = "CSV_HEADER"; err.message = "empty column name in header"; err.line = startLine;
                    return false;
                }
                if (!seen.insert(h).second) {
                    err.code = "CSV_HEADER"; err.message = "duplicate column name '" + h + "'"; err.line = startLine;
                    return false;
                }
            }
            out.header = fields;
            for (const std::string& req : required) {
                if (out.column(req) < 0) {
                    err.code = "CSV_MISSING_COLUMN";
                    err.message = "required header column '" + req + "' is missing";
                    err.line = startLine;
                    return false;
                }
            }
            continue;
        }
        if (fields.size() != out.header.size()) {
            err.code = "CSV_RAGGED";
            err.message = "row has " + std::to_string(fields.size()) + " fields, header has " +
                          std::to_string(out.header.size());
            err.line = startLine;
            return false;
        }
        out.rows.push_back(fields);
        out.lines.push_back(startLine);
    }
    return true;
}

}  // namespace csv
}  // namespace dd
