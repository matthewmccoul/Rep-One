#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CombatDamage {
    cs::Ref<::mc::Character> Character;
    int32_t Amount = 0;
    DamageKind Kind{};
    int32_t Kills = 0;
    bool Shown = false;

    CombatDamage() = default;
    CombatDamage(cs::Ref<::mc::Character> character, int32_t amount, DamageKind kind, int32_t kills, bool shown)
        : Character(character), Amount(amount), Kind(kind), Kills(kills), Shown(shown) {}

    bool operator==(const CombatDamage& o) const {
        return Character == o.Character && Amount == o.Amount && Kind == o.Kind && Kills == o.Kills && Shown == o.Shown;
    }
    bool operator!=(const CombatDamage& o) const { return !(*this == o); }
};

struct CombatExperience {
    cs::Ref<::mc::Character> Character;
    int64_t Amount = 0;
    ExperienceSource Source{};

    CombatExperience() = default;
    CombatExperience(cs::Ref<::mc::Character> character, int64_t amount, ExperienceSource source)
        : Character(character), Amount(amount), Source(source) {}

    bool operator==(const CombatExperience& o) const {
        return Character == o.Character && Amount == o.Amount && Source == o.Source;
    }
    bool operator!=(const CombatExperience& o) const { return !(*this == o); }
};

}  // namespace mc
