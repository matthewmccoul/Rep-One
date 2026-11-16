// NOT CORE. map_check command-line entry (reads the file, then calls the core).
#pragma once
#include <iosfwd>
#include <string>
#include <vector>

namespace dd {
namespace map {

// map_check validate|neighbours|door|ways|reach ... Exit 0 ok, 1 rejected, 2 usage/I-O.
int mapCheckMain(const std::vector<std::string>& args, std::ostream& out);

}  // namespace map
}  // namespace dd
