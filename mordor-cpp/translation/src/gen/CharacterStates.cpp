#include "core.h"

namespace mc {

cs::List<cs::String> CharacterStates::Showing(cs::Ref<Character> c) {
    auto on = cs::List<cs::String>::New();
    if (c->Poisoned) on.Add(Names[0]);
    if (c->Diseased) on.Add(Names[1]);
    if (c->Paralysed()) on.Add(Names[2]);
    if (c->Levitating) on.Add(Names[3]);
    if (c->InvisibleNow) on.Add(Names[4]);
    if (c->SeeingInvisible) on.Add(Names[5]);
    // the label is just "Protected" - the strength of it is not painted here
    if (c->Protection > 0) on.Add(Names[6]);
    return on;
}

}  // namespace mc
