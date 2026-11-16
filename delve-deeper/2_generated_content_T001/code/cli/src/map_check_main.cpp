#include <iostream>
#include <string>
#include <vector>

#include "dd/map_cli.h"

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    return dd::map::mapCheckMain(args, std::cout);
}
