#include "core.h"

namespace mc {

cs::String RoomRoster::DeadBodies(int32_t count) {
    if (count <= 0) return cs::String{};
    if (count == 1) return "There is a dead body here!";
    return "There are dead bodies here!";
}

cs::String RoomRoster::Strangers(int32_t count) {
    if (count <= 0) return cs::String{};
    if (count == 1) return "Somebody else is in the room!";
    return "Other characters are in the room!";
}

cs::List<cs::String> RoomRoster::Lines(int32_t living, int32_t dead, int32_t carrying, int32_t partySize) {
    auto said = cs::List<cs::String>::New();
    // null string maps to empty
    auto bodies = DeadBodies(dead - carrying);
    if (!bodies.empty()) said.Add(bodies);
    auto strangers = Strangers(living - partySize);
    if (!strangers.empty()) said.Add(strangers);
    return said;
}

}  // namespace mc
