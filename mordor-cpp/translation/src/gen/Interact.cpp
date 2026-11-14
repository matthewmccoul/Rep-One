#include "core.h"

namespace mc {

cs::String Interact::NobodyFor(cs::String tag) {
    if (tag == "PICKUP" || tag == "JOIN") return "No other characters in the area!";
    else if (tag == "SPELLHEAL" || tag == "GIVE") return "No characters in the party!";
    else if (tag == "SPELLRAISE") return "No characters to resurrect!";
    else return "No characters available for this selection!";
}

cs::String Interact::FellFromTeleport(bool inAParty) {
    return inAParty ? cs::String("The party has teleported hundreds of feet above the city and fallen to their deaths!")
                    : cs::String("You have teleported hundreds of feet above the city and fallen to your death!");
}

cs::String Interact::TeleportedIntoRock(cs::Ref<Character> who, bool inAParty) {
    return inAParty ? "\"" + cs::Str::TrimEnd(who->Name) + "\" teleported into solid rock!"
                    : cs::String("You teleported into solid rock!");
}

cs::String Interact::Hidden(cs::String name) {
    return "\"" + cs::ToStr(cs::Str::TrimEnd(name)) + "\" is Hidden!";
}

cs::String Interact::NowCarrying(cs::Ref<Character> who, cs::String whom) {
    return "\"" + cs::ToStr(cs::Str::TrimEnd(who->Name)) + "\" is now carrying \"" + cs::ToStr(cs::Str::TrimEnd(whom)) + "\".";
}

cs::String Interact::NoLongerCarrying(cs::Ref<Character> who, cs::String whom) {
    return "\"" + cs::ToStr(cs::Str::TrimEnd(who->Name)) + "\" is no longer carrying \"" + cs::ToStr(cs::Str::TrimEnd(whom)) + "\".";
}

cs::String Interact::NoLongerCarryingCompanion(cs::Ref<Character> who, cs::String companion) {
    return "\"" + cs::ToStr(cs::Str::TrimEnd(who->Name)) + "\" is no longer carrying " + cs::ToStr(cs::Str::TrimEnd(companion)) + ".";
}

}  // namespace mc
