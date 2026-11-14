#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct UserFolders {
    inline static const cs::String Name = "OpenDejenol";

    inline static const cs::String OldName = "Mordor Port";

    static cs::String Roaming();

    static cs::String Local();

    static cs::String RoamingParent();

    static cs::String LocalParent();

    static cs::String In(cs::String parent);

    static cs::String Migrate(cs::String parent);

    static void MigrateAll();
};

}  // namespace mc
