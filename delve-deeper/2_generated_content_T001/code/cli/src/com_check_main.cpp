#include <iostream>
#include <string>
#include <vector>

#include "dd/fileio.h"

int main(int argc, char** argv) {
    std::vector<std::string> args(argv + 1, argv + argc);
    return dd::comCheckMain(args, std::cout);
}
