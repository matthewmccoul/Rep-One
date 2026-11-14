#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Saves {
    inline static const cs::String EnvDir = "MORDOR_SAVES";

    inline static cs::Func<cs::List<cs::String>()> ModTag{};

    static cs::String PerUserDir();

    static cs::String LegacyDirFor(cs::String gameDir);

    static cs::String DirFor(cs::String gameDir);

    static cs::String Resolve(cs::String gameDir, cs::String overridden, cs::String perUser);

    static bool HasAnything(cs::String dir);
};

}  // namespace mc
