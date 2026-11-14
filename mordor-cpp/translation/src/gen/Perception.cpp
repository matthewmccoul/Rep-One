#include "core.h"

namespace mc {

cs::List<int32_t> Perception::Noticeable = cs::List<int32_t>::Of({
    Floor::FACE_NORTH, Floor::FACE_EAST, Floor::FACE_SOUTH, Floor::FACE_WEST,
    Floor::EXTINGUISHER, Floor::ROTATOR, Floor::ANTIMAGIC, Floor::STUD,
});

cs::String Perception::Certainty(double perception, bool mappedIt) {
    if (mappedIt) return "completely";
    int32_t p = cs::CastDouble<int32_t>(perception);
    if (p <= 39) return "kind of";
    if (p <= 49) return "fairly";
    if (p <= 59) return "strongly";
    if (p <= 69) return "almost very";
    if (p <= 85) return "very";
    if (p <= 95) return "almost absolutely";
    return "absolutely";
}

bool Perception::NoticesAnything(cs::Ref<cs::Random> rng, double perception, int32_t bit) {
    return bit == Floor::TELEPORTER || rng->Next(100) + 1 <= perception;
}

bool Perception::IdentifiesIt(cs::Ref<cs::Random> rng, double perception, int32_t bit) {
    return bit == Floor::TELEPORTER || rng->Next(100) + 1 <= perception / 2;
}

int32_t Perception::WrongGuess(cs::Ref<cs::Random> rng, int32_t realBit) {
    while (true) {
        int32_t bit = rng->Next(23);
        if ((bit >= 0 && bit <= 5) || bit == Floor::PIT || bit == Floor::STAIRS_UP || bit == Floor::STAIRS_DOWN ||
            bit == Floor::WATER || bit == Floor::QUICKSAND || bit == Floor::ROCK || bit == Floor::FOG ||
            bit == Floor::CHUTE || bit == Floor::TELEPORTER)
            continue;
        if (bit == realBit) continue;
        return bit;
    }
}

cs::String Perception::Say(double perception, bool inParty, cs::String who, int32_t namedBit, cs::String certainty) {
    if (perception <= 35)
        return inParty ? "\"" + cs::ToStr(who) + "\" senses something different here..."
                       : cs::String("You sense something different here...");

    cs::String name = namedBit >= 0 && namedBit < Names.Length() ? Names[namedBit] : cs::String("square");
    if (namedBit == Floor::TELEPORTER)
        return inParty ? "\"" + cs::ToStr(who) + "\" is " + cs::ToStr(certainty) + " certain the party just hit a " + cs::ToStr(name) + "."
                       : "You are " + cs::ToStr(certainty) + " certain you just hit a " + cs::ToStr(name) + ".";

    return inParty ? "\"" + cs::ToStr(who) + "\" is " + cs::ToStr(certainty) + " certain the party is standing on a " + cs::ToStr(name) + " square."
                   : "You are " + cs::ToStr(certainty) + " certain you're standing on a " + cs::ToStr(name) + " square.";
}

cs::String Perception::HitATeleporter(bool inParty) {
    return inParty ? "The party just hit a teleporter!" : "You just hit a teleporter!";
}

Perception_Outcome Perception::PerceiveSquare(cs::Ref<cs::Random> rng, int32_t bit, double best, cs::String bestName, bool inParty,
                                              bool noMessages, bool alreadyMapped, bool lost) {
    if (noMessages) return Perception_Outcome{false, ""};
    if (!NoticesAnything(rng, best, bit)) return Perception_Outcome{false, ""};
    if (alreadyMapped && bit != Floor::TELEPORTER) return Perception_Outcome{false, ""};
    bool got = IdentifiesIt(rng, best, bit);
    bool mapIt = got && !lost;
    int32_t named = got ? bit : WrongGuess(rng, bit);
    if (bit == Floor::TELEPORTER) return Perception_Outcome{mapIt, HitATeleporter(inParty)};
    return Perception_Outcome{mapIt, Say(best, inParty, bestName, named, Certainty(best, mapIt))};
}

}  // namespace mc
