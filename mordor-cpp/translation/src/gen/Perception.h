#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Perception_Outcome {
    bool MapIt = false;
    cs::String Line{};
    bool operator==(const Perception_Outcome& o) const { return MapIt == o.MapIt && Line == o.Line; }
    bool operator!=(const Perception_Outcome& o) const { return !(*this == o); }
};

struct Perception {
    using Outcome = Perception_Outcome;

    // defined in the .cpp: the initializer needs the complete Floor type
    static cs::List<int32_t> Noticeable;

    inline static cs::List<cs::String> Names = cs::List<cs::String>::Of({
        "Right Wall", "Upper Wall", "Right Door", "Upper Door", "Right SDoor", "Upper SDoor",
        "Face North", "Face East", "Face South", "Face West", "Extinguish", "Pit",
        "Stairs Up", "Stairs Down", "Teleporter", "Water", "Quicksand", "Rotator",
        "Anti-Magic", "Rock", "Fog", "Chute", "Stud",
    });

    static cs::String Certainty(double perception, bool mappedIt);
    static bool NoticesAnything(cs::Ref<cs::Random> rng, double perception, int32_t bit);
    static bool IdentifiesIt(cs::Ref<cs::Random> rng, double perception, int32_t bit);
    static int32_t WrongGuess(cs::Ref<cs::Random> rng, int32_t realBit);
    static cs::String Say(double perception, bool inParty, cs::String who, int32_t namedBit, cs::String certainty);
    static cs::String HitATeleporter(bool inParty);
    static Perception_Outcome PerceiveSquare(cs::Ref<cs::Random> rng, int32_t bit, double best, cs::String bestName, bool inParty,
                                             bool noMessages, bool alreadyMapped, bool lost);
};

}  // namespace mc
