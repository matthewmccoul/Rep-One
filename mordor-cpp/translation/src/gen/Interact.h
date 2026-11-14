#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Interact {
    // ---- frmInteract's refusals, in the order it makes them ----
    static cs::String NobodyFor(cs::String tag);

    inline static const cs::String NoBoundCompanions = "No bound companions to select!";

    static cs::String FellFromTeleport(bool inAParty);

    inline static const cs::String FellFromTeleportTitle = "Teleport";

    static cs::String TeleportedIntoRock(cs::Ref<Character> who, bool inAParty);

    inline static const cs::String NotFound = "Not Found!";

    inline static const cs::String Ambiguous = "ambiguous!";

    inline static const cs::String Yourself = "Yourself?  HA!";

    inline static const cs::String NotHere = "Character is not here!";

    inline static const cs::String IsDead = "Character is Dead!";

    inline static const cs::String AlreadyInParty = "Character is already in Party!";

    inline static const cs::String NotDead = "Character is not Dead!";

    // ---- picking up and putting down ----
    static cs::String Hidden(cs::String name);
    static cs::String NowCarrying(cs::Ref<Character> who, cs::String whom);
    static cs::String NoLongerCarrying(cs::Ref<Character> who, cs::String whom);
    static cs::String NoLongerCarryingCompanion(cs::Ref<Character> who, cs::String companion);
};

}  // namespace mc
