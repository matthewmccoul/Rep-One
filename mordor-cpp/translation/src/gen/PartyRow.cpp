#include "core.h"

namespace mc {

cs::String PartyRow::Status(cs::Ref<Character> c) {
    cs::String s = "";
    if (c->Poisoned) s += "Po";
    if (c->Diseased) s += "Di";
    if (c->Paralysed()) s += "Pa";   // Ailments(mc014A) <> False (FRMGROUP.BAS:443)
    return s == "" ? cs::String("OK") : s;
}

bool PartyRow::StatusIsWell(cs::Ref<Character> c) {
    return Status(c) == "OK";
}

}  // namespace mc
