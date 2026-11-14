#include "core.h"

namespace mc {

int32_t SpecialHits::Floor(double v) { return cs::CastDouble<int32_t>(cs::Math::Floor(v)); }

int32_t SpecialHits::VbInt(double v) { return cs::CastDouble<int32_t>(cs::Math::Round(v, cs::MidpointRounding::ToEven)); }

int32_t SpecialHits::Range(int32_t attackerGuildLevel, int32_t targetLevel) {
    double a = (double)attackerGuildLevel + 1;
    return SpecialHits::Floor((cs::Math::Log(a) + a / 15.0) / cs::Math::Log(1.08) - targetLevel / 10.0 + 1);
}

int32_t SpecialHits::Raw(cs::Ref<cs::Random> rng, SpecialHits_Kind kind, int32_t attackerGuildLevel, int32_t targetLevel) {
    int32_t range = Range(attackerGuildLevel, targetLevel);
    double share = kind == SpecialHits_Kind::Breath ? range / 4.0 : range / 2.0;
    return VbInt((SpecialHits::Floor((rng->NextDouble() * range + 1) / 2) + share) * 0.7);
}

int32_t SpecialHits::AfterResistance(int32_t damage, int32_t resistancePercent) {
    return resistancePercent <= 0 ? damage : VbInt(damage - damage * (resistancePercent / 2.0 * 0.01));
}

int32_t SpecialHits::ResistedBy(SpecialHits_Kind kind, bool attackerBreathesFire) {
    if (kind == SpecialHits_Kind::Breath) return attackerBreathesFire ? Fire : Cold;
    if (kind == SpecialHits_Kind::AcidSpit) return Acid;
    return Electrical;
}

}  // namespace mc
