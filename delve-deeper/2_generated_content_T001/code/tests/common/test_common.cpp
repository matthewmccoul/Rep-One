#include <filesystem>
#include <fstream>
#include <sstream>

#include "../testing.h"
#include "dd/csv.h"
#include "dd/fileio.h"
#include "dd/json.h"

namespace {

std::string root() { return DD_REPO_ROOT; }

std::string writeTmp(const std::string& name, const std::string& content) {
    std::filesystem::create_directories(DD_TMP_DIR);
    std::string p = std::string(DD_TMP_DIR) + "/" + name;
    std::ofstream f(p, std::ios::binary);
    f << content;
    return p;
}

bool parseJson(const std::string& t, std::string* code = nullptr) {
    dd::json::Value v;
    dd::json::Error e;
    bool ok = dd::json::parse(t, v, e);
    if (code) *code = e.code;
    return ok;
}

// Runs com_check in-process: returns exit code, fills output.
int cli(const std::vector<std::string>& a, std::string& output) {
    std::ostringstream os;
    int rc = dd::comCheckMain(a, os);
    output = os.str();
    return rc;
}

}  // namespace

TEST(com_json_valid_real_files) {
    const char* files[] = {"/work/T001/map/floor1_map.json", "/work/T001/puzzle/storeroom_puzzle.json",
                           "/work/T001/quest/figure_ore_lift_quest.json"};
    for (const char* f : files) {
        std::string out;
        CHECK_EQ(cli({"json", root() + f}, out), 0);
    }
}

TEST(com_json_escapes) {
    dd::json::Value v;
    dd::json::Error e;
    // \u00e9 -> UTF-8 e-acute, \" and \n decoded, raw UTF-8 passes through, surrogate pair -> U+1F600.
    CHECK(dd::json::parse("{\"a\":\"\\u00e9 \\\"q\\\" \\n x \xC3\xA9 \\ud83d\\ude00 \\/\"}", v, e));
    const dd::json::Value* a = v.find("a");
    CHECK(a != nullptr);
    if (a) CHECK_EQ(a->string, std::string("\xC3\xA9 \"q\" \n x \xC3\xA9 \xF0\x9F\x98\x80 /"));
    CHECK(!parseJson("\"\\ud800\""));        // lone high surrogate
    CHECK(!parseJson("\"\\udc00\""));        // lone low surrogate
    CHECK(!parseJson("\"\\x\""));            // bad escape
    CHECK(!parseJson("\"a\nb\""));           // raw newline in string
    CHECK(!parseJson("\"\xC3\""));           // truncated UTF-8
}

TEST(com_json_numbers_literals) {
    CHECK(parseJson("[0, -1, 1.5, 2e3, -0.5E-2, true, false, null]"));
    CHECK(!parseJson("[01]"));
    CHECK(!parseJson("[1.]"));
    CHECK(!parseJson("[.5]"));
    CHECK(!parseJson("[+1]"));
    CHECK(!parseJson("[NaN]"));
    CHECK(!parseJson("[Infinity]"));
    CHECK(!parseJson("[tru]"));
    CHECK(!parseJson("[nulll]"));
    CHECK(!parseJson("[1e999]"));
    dd::json::Value v;
    dd::json::Error e;
    CHECK(dd::json::parse("[7, 7.0]", v, e));
    CHECK(v.items[0].isInteger);
    CHECK(!v.items[1].isInteger);
}

TEST(com_json_trailing_comma) {
    std::string code;
    CHECK(!parseJson("{\"a\":[1,2,]}", &code));
    CHECK_EQ(code, std::string("JSON_TRAILING_COMMA"));
    CHECK(!parseJson("{\"a\":1,}", &code));
    CHECK_EQ(code, std::string("JSON_TRAILING_COMMA"));
    CHECK(!parseJson("[,1]"));
    CHECK(!parseJson("[1,,2]"));
}

TEST(com_json_truncated) {
    std::string code;
    CHECK(!parseJson("{\"a\": [1, 2", &code));
    CHECK_EQ(code, std::string("JSON_TRUNCATED"));
    CHECK(!parseJson("{\"a\": \"abc", &code));
    CHECK_EQ(code, std::string("JSON_TRUNCATED"));
    CHECK(!parseJson("{\"a\"", &code));
    CHECK_EQ(code, std::string("JSON_TRUNCATED"));
    CHECK(!parseJson("{\"a\":", &code));
    CHECK_EQ(code, std::string("JSON_TRUNCATED"));
    CHECK(!parseJson("[1, tr", &code));
    // real file cut at line 30
    std::string full;
    CHECK(dd::readFile(root() + "/work/T001/map/floor1_map.json", full));
    std::size_t pos = 0;
    for (int i = 0; i < 30 && pos != std::string::npos; ++i) pos = full.find('\n', pos + 1);
    CHECK(pos != std::string::npos);
    CHECK(!parseJson(full.substr(0, pos), &code));
    CHECK_EQ(code, std::string("JSON_TRUNCATED"));
}

TEST(com_json_dup_key) {
    std::string code;
    CHECK(!parseJson("{\"a\":1,\"a\":2}", &code));
    CHECK_EQ(code, std::string("JSON_DUP_KEY"));
    CHECK(!parseJson("{\"x\":{\"a\":1,\"b\":2,\"a\":3}}", &code));
    CHECK_EQ(code, std::string("JSON_DUP_KEY"));
    // same key in different objects is fine
    CHECK(parseJson("{\"x\":{\"a\":1},\"y\":{\"a\":2},\"z\":[{\"a\":1},{\"a\":2}]}"));
}

TEST(com_json_nonstrict) {
    CHECK(!parseJson("{'a': 1}"));
    CHECK(!parseJson("{\"a\": NaN}"));
    CHECK(!parseJson("{'a': NaN}"));
    CHECK(!parseJson("// c\n{\"a\":1}"));
    CHECK(!parseJson("{\"a\":1 /* c */}"));
    CHECK(!parseJson("{\"a\":1} x"));
    CHECK(!parseJson("{\"a\":1}{\"b\":2}"));
    CHECK(!parseJson("{a:1}"));
    CHECK(!parseJson("\xEF\xBB\xBF{}"));  // BOM rejected
    CHECK(!parseJson("{\"a\":\"\xFF\"}"));  // invalid UTF-8
}

TEST(com_json_empty) {
    std::string code;
    CHECK(!parseJson("", &code));
    CHECK_EQ(code, std::string("JSON_EMPTY"));
    CHECK(!parseJson("  \n\t ", &code));
    CHECK_EQ(code, std::string("JSON_EMPTY"));
}

TEST(com_json_depth) {
    CHECK(!parseJson(std::string(1000, '[') + std::string(1000, ']')));
    CHECK(parseJson(std::string(100, '[') + std::string(100, ']')));
}

// Every error path of the exception-free parser: the exact code, and the line (where it matters).
// A failed parse must leave the caller's Value untouched.
TEST(com_json_error_paths) {
    struct Case { const char* text; const char* code; std::size_t line; };
    const std::string deep(300, '[');
    const Case cases[] = {
        {"", "JSON_EMPTY", 1},
        {"{\"a\":1} x", "JSON_TRAILING_CONTENT", 1},
        {"[[[[[[[[[[", "JSON_TRUNCATED", 1},
        {deep.c_str(), "JSON_TOO_DEEP", 1},
        {"{\"a\":\n  @}", "JSON_SYNTAX", 2},
        {"{\"a\":\n tru}", "JSON_SYNTAX", 2},
        {"[truex]", "JSON_SYNTAX", 1},
        {"[01]", "JSON_SYNTAX", 1},
        {"[1.]", "JSON_SYNTAX", 1},
        {"[1e]", "JSON_SYNTAX", 1},
        {"[-]", "JSON_SYNTAX", 1},
        {"[-x]", "JSON_SYNTAX", 1},
        {"[1 2]", "JSON_SYNTAX", 1},
        {"{\"a\" 1}", "JSON_SYNTAX", 1},
        {"{1:2}", "JSON_SYNTAX", 1},
        {"[1e999]", "JSON_NUMBER_RANGE", 1},
        {"[\"a\nb\"]", "JSON_CONTROL_CHAR", 1},
        {"[\"\\x\"]", "JSON_BAD_ESCAPE", 1},
        {"[\"\\u12G4\"]", "JSON_BAD_ESCAPE", 1},
        {"[\"\\uD800\"]", "JSON_BAD_ESCAPE", 1},
        {"[\"\\uD800\\u0041\"]", "JSON_BAD_ESCAPE", 1},
        {"[\"\\uDC00\"]", "JSON_BAD_ESCAPE", 1},
        {"[1,]", "JSON_TRAILING_COMMA", 1},
        {"{\"a\":1,\n}", "JSON_TRAILING_COMMA", 2},
        {"{\"a\":1,\n\"a\":2}", "JSON_DUP_KEY", 2},
        {"[\"abc", "JSON_TRUNCATED", 1},
        {"{\"a\"", "JSON_TRUNCATED", 1},
        {"{\"a\":", "JSON_TRUNCATED", 1},
        {"[\"\\u12", "JSON_TRUNCATED", 1},
        {"tr", "JSON_TRUNCATED", 1},
    };
    for (const Case& c : cases) {
        dd::json::Value v;
        v.string = "untouched";
        dd::json::Error e;
        bool ok = dd::json::parse(c.text, v, e);
        CHECK(!ok);
        CHECK_EQ(e.code, std::string(c.code));
        if (c.line) CHECK_EQ(e.line, c.line);
        CHECK_EQ(v.string, std::string("untouched"));
    }
    dd::json::Value v;
    dd::json::Error e;
    CHECK(!dd::json::parse("\xFF", v, e));
    CHECK_EQ(e.code, std::string("JSON_BAD_UTF8"));
}

TEST(com_cli_json_exit_codes) {
    std::string out;
    CHECK_EQ(cli({"json", writeTmp("t.json", "{\"a\":[1,2,]}")}, out), 1);
    CHECK(out.rfind("FAIL ", 0) == 0);
    CHECK_EQ(cli({"json", writeTmp("dup.json", "{\"a\":1,\"a\":2}")}, out), 1);
    CHECK(out.rfind("FAIL ", 0) == 0);
    CHECK_EQ(cli({"json", writeTmp("empty.json", "")}, out), 1);   // empty file: rejected
    CHECK(out.rfind("FAIL ", 0) == 0);
    CHECK_EQ(cli({"json", root() + "/work/T001/does_not_exist.json"}, out), 2);  // missing: I/O error
    CHECK(out.rfind("FAIL", 0) != 0);
    CHECK_EQ(cli({"json"}, out), 2);
    CHECK_EQ(cli({"xml", "a"}, out), 2);
    CHECK_EQ(cli({"json", writeTmp("ok.json", "{\"a\":1}")}, out), 0);
}

TEST(com_csv_valid_real) {
    std::string out;
    CHECK_EQ(cli({"csv", root() + "/work/T001/quest/figure_ore_lift_strings.csv"}, out), 0);
    CHECK(out.find("rows=25") != std::string::npos);
    CHECK(out.find("columns=6") != std::string::npos);
}

TEST(com_csv_quoted_fields) {
    dd::csv::Table t;
    dd::csv::Error e;
    const std::string text =
        "key,text,note\r\n"
        "a,\"one, two\",x\r\n"
        "b,\"say \"\"hi\"\"\",\r\n"
        "c,\"line1\nline2\",z\n"
        "d,,\n";
    CHECK(dd::csv::parse(text, t, e));
    CHECK_EQ(t.rows.size(), static_cast<std::size_t>(4));
    if (t.rows.size() == 4) {
        CHECK_EQ(t.rows[0][1], std::string("one, two"));
        CHECK_EQ(t.rows[1][1], std::string("say \"hi\""));
        CHECK_EQ(t.rows[1][2], std::string(""));
        CHECK_EQ(t.rows[2][1], std::string("line1\nline2"));
        CHECK_EQ(t.rows[3][1], std::string(""));
        CHECK_EQ(t.lines[3], static_cast<std::size_t>(6));
    }
    // no final newline is fine
    CHECK(dd::csv::parse("a,b\n1,2", t, e));
    CHECK_EQ(t.rows.size(), static_cast<std::size_t>(1));
    // trailing comma at EOF is an empty last field
    CHECK(dd::csv::parse("a,b\n1,", t, e));
}

TEST(com_csv_ragged) {
    dd::csv::Table t;
    dd::csv::Error e;
    CHECK(!dd::csv::parse("a,b,c\n1,2,3\n4,5\n6,7,8\n", t, e));
    CHECK_EQ(e.code, std::string("CSV_RAGGED"));
    CHECK_EQ(e.line, static_cast<std::size_t>(3));
    CHECK(!dd::csv::parse("a,b\n1,2,3\n", t, e));
    CHECK_EQ(e.line, static_cast<std::size_t>(2));
    CHECK(!dd::csv::parse("a,b\n\n1,2\n", t, e));  // blank line = ragged row
    // ragged row in the real file, with the line number reported
    std::string full;
    CHECK(dd::readFile(root() + "/work/T001/quest/figure_ore_lift_strings.csv", full));
    std::string needle = "\"I'll look.\",";
    std::size_t p = full.find("fig_choice_accept,player,-1,2,\"I'll look.\",");
    CHECK(p != std::string::npos);
    if (p != std::string::npos) {
        std::string broken = full;
        broken.replace(p, std::string("fig_choice_accept,player,-1,2,\"I'll look.\",").size(),
                       "fig_choice_accept,player,-1,\"I'll look.\",");
        CHECK(!dd::csv::parse(broken, t, e));
        CHECK_EQ(e.code, std::string("CSV_RAGGED"));
        CHECK_EQ(e.line, static_cast<std::size_t>(5));
        std::string out;
        CHECK_EQ(cli({"csv", writeTmp("ragged.csv", broken)}, out), 1);
        CHECK(out.rfind("FAIL CSV_RAGGED", 0) == 0);
        CHECK(out.find("line 5") != std::string::npos);
    }
}

TEST(com_csv_open_quote) {
    dd::csv::Table t;
    dd::csv::Error e;
    CHECK(!dd::csv::parse("a,b\n1,\"two\n3,4\n", t, e));
    CHECK_EQ(e.code, std::string("CSV_OPEN_QUOTE"));
    CHECK(!dd::csv::parse("a,b\n1,\"two\"x\n", t, e));
    CHECK_EQ(e.code, std::string("CSV_BAD_QUOTE"));
    CHECK(!dd::csv::parse("a,b\n1,tw\"o\n", t, e));
    CHECK_EQ(e.code, std::string("CSV_BAD_QUOTE"));
    CHECK(!dd::csv::parse("a,b\r1,2\r", t, e));
    CHECK_EQ(e.code, std::string("CSV_BAD_LINE_END"));
    std::string out;
    CHECK_EQ(cli({"csv", writeTmp("open.csv", "key,text\nk,\"never closed\n")}, out), 1);
    CHECK(out.rfind("FAIL ", 0) == 0);
}

TEST(com_csv_header_rules) {
    dd::csv::Table t;
    dd::csv::Error e;
    CHECK(!dd::csv::parse("", t, e));
    CHECK_EQ(e.code, std::string("CSV_EMPTY"));
    CHECK(!dd::csv::parse("a,a\n1,2\n", t, e));
    CHECK_EQ(e.code, std::string("CSV_HEADER"));
    CHECK(!dd::csv::parse("a,\n1,2\n", t, e));
    CHECK_EQ(e.code, std::string("CSV_HEADER"));
    CHECK(!dd::csv::parse("key,speaker\nk,s\n", t, e, {"text"}));
    CHECK_EQ(e.code, std::string("CSV_MISSING_COLUMN"));
    CHECK(dd::csv::parse("key,text\nk,s\n", t, e, {"text"}));
    CHECK_EQ(t.column("text"), 1);
    CHECK_EQ(t.column("zzz"), -1);
    // CLI: default requires "text"; --require overrides; --require "" disables
    std::string out;
    std::string f = writeTmp("no_text.csv", "key,speaker\nk,s\n");
    CHECK_EQ(cli({"csv", f}, out), 1);
    CHECK(out.rfind("FAIL CSV_MISSING_COLUMN", 0) == 0);
    CHECK_EQ(cli({"csv", f, "--require", "key,speaker"}, out), 0);
    CHECK_EQ(cli({"csv", f, "--require", ""}, out), 0);
    CHECK_EQ(cli({"csv", root() + "/work/T001/quest/missing.csv"}, out), 2);
    // real CSV with `text` removed from the header
    std::string full;
    CHECK(dd::readFile(root() + "/work/T001/quest/figure_ore_lift_strings.csv", full));
    std::size_t p = full.find("stage_max,text,blanks");
    CHECK(p != std::string::npos);
    if (p != std::string::npos) {
        full.replace(p, std::string("stage_max,text,blanks").size(), "stage_max,txt,blanks");
        CHECK_EQ(cli({"csv", writeTmp("hdr.csv", full)}, out), 1);
        CHECK(out.rfind("FAIL ", 0) == 0);
    }
}

int main(int argc, char** argv) { return dd_test::runMain(argc, argv); }
