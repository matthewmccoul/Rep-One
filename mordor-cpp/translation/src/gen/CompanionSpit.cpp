#include "core.h"

namespace mc {

int32_t CompanionSpit::Slot(cs::Ref<cs::Random> rng, cs::Ref<Character> of) {
    int32_t count = 0;
    for (int32_t i = 0; i < SlotsWalked; i++) {
        auto c = of->Companions->Index(i);
        if (c != nullptr && c->Hits > 0) count++;
    }
    if (count == 0) return -1;
    if (!(rng->NextDouble() * 100 < count / (double)(count + 1) * 100)) return -1;

    // Int(Rnd * 4) until it lands on one that is alive; the cap keeps the port from hanging on data the original never saw.
    for (int32_t guard = 0; guard < 1000; guard++) {
        int32_t slot = cs::CastDouble<int32_t>(cs::Math::Floor(rng->NextDouble() * SlotsWalked));
        auto c = of->Companions->Index(slot);
        if (c != nullptr && c->Hits > 0) return slot;
    }
    return -1;
}

}  // namespace mc
