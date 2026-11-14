#include "core.h"

namespace mc {

int32_t CompanionMishap::BodiesOnYourSide(cs::Ref<Party> party) {
    int32_t n = 0;
    for (auto& c : party->Members) {
        n++;
        for (int32_t slot = 0; slot < 4; slot++) {
            auto comp = c->Companions->Index(slot);
            if (comp != nullptr && comp->Hits > 0) n++;
        }
    }
    return n;
}

double CompanionMishap::Chance(CompanionMishap_Kind kind, int32_t companionGuildLevel, int32_t bodies) {
    double level = companionGuildLevel <= 0 ? 1 : companionGuildLevel;
    double factor, low, high;
    if (kind == CompanionMishap_Kind::Breath) {
        factor = 2.0; low = 2.0; high = 40.0;
    } else if (kind == CompanionMishap_Kind::Spell) {
        factor = 1.5; low = 1.0; high = 40.0;
    } else {
        factor = 1.0; low = 0.5; high = 20.0;
    }
    double c = (999.0 / level) * (bodies / 20.0) * factor;
    return cs::Math::Clamp(c, low, high);
}

cs::Nullable<int32_t> CompanionMishap::Victim(cs::Ref<cs::Random> rng, CompanionMishap_Kind kind, int32_t companionGuildLevel, cs::Ref<Party> party, int32_t owner) {
    if (party->Members.Count() == 0) return cs::null_value;
    if (!(rng->NextDouble() * 100 < Chance(kind, companionGuildLevel, BodiesOnYourSide(party))))
        return cs::null_value;

    int32_t last = party->Members.Count() - 1;
    int32_t drawn = cs::CastDouble<int32_t>(cs::Math::Round(rng->NextDouble() * last, cs::MidpointRounding::ToEven)) + 1;
    if (drawn == owner + 1) return cs::null_value;
    return drawn - 1;
}

}  // namespace mc
