// NOT CORE. File helper and the com_check command-line entry, used by the command-line tools and
// tests only. The core (common/, map/) takes text, never paths; in Unreal the engine's own file
// APIs read a file and hand its text to the core.
#pragma once
#include <iosfwd>
#include <string>
#include <vector>

namespace dd {

// Reads a whole file in binary mode. Returns false when it cannot be opened/read.
bool readFile(const std::string& path, std::string& out);

// com_check json <file> | com_check csv <file> [--require a,b | --require ""]
// Exit 0 ok, 1 rejected (prints "FAIL <CODE>: <detail>"), 2 usage or I/O error.
// csv default required header columns: "text" (see README_code.md, contract change C1).
int comCheckMain(const std::vector<std::string>& args, std::ostream& out);

}  // namespace dd
