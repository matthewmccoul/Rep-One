// NOT CORE. puzzle_check command-line entry (reads files, then calls the core).
#pragma once
#include <iosfwd>
#include <string>
#include <vector>

namespace dd {
namespace puzzle {

// puzzle_check validate <puzzle> <map> | seq <puzzle> <control>... | bypass <puzzle> <route> [<control>...]
//            | roundtrip <puzzle> <control>... | load <puzzle> <state.json>
// Exit 0 ok, 1 rejected (a line "FAIL <CODE>: <detail>"), 2 usage or I/O error.
int puzzleCheckMain(const std::vector<std::string>& args, std::ostream& out);

}  // namespace puzzle
}  // namespace dd
