#include "test_support.h"
#include "mordor/modding.h"
#include <cstring>

using namespace mordor;

static AncestorFolder makeAncestor(const char* path, bool hasSln) {
    AncestorFolder f;
    f.path = Str(path);
    f.hasMordorSln = hasSln;
    return f;
}

static AutomapMenuItem makeItem(const char* label, bool separator) {
    AutomapMenuItem item;
    item.label = Str(label);
    item.separator = separator;
    return item;
}

// R2 / R3: documented names of the environment variable, separator and manifest file.
TEST(modding_documented_constant_names) {
    CHECK(std::strcmp(kModsEnvVar, "MORDOR_MODS") == 0);
    CHECK(kModsEnvSeparator == ';');
    CHECK(std::strcmp(kManifestFileName, "mod.json") == 0);
}

