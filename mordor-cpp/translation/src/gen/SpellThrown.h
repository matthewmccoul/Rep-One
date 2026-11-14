#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct SpellThrown_Target {
    using Party = SpellThrown_Target_Party;
    using Nobody = SpellThrown_Target_Nobody;
    using Person = SpellThrown_Target_Person;
    using Companion = SpellThrown_Target_Companion;
    virtual ~SpellThrown_Target() = default;
};

struct SpellThrown_Target_Party : SpellThrown_Target {
};

struct SpellThrown_Target_Nobody : SpellThrown_Target {
};

struct SpellThrown_Target_Person : SpellThrown_Target {
    cs::Ref<Character> Who;
    SpellThrown_Target_Person() = default;
    explicit SpellThrown_Target_Person(cs::Ref<Character> Who) : Who(Who) {}
};

struct SpellThrown_Target_Companion : SpellThrown_Target {
    int32_t Slot = 0;
    SpellThrown_Target_Companion() = default;
    explicit SpellThrown_Target_Companion(int32_t Slot) : Slot(Slot) {}
};

struct SpellThrown {
    using Target = SpellThrown_Target;
    using Ask = SpellThrown_Ask;

    static cs::String Line(cs::Ref<Party> party, cs::Ref<Spell> s, cs::Ref<SpellThrown_Target> t, cs::Ref<Character> commander = {});
    static bool Handles(cs::Ref<Spell> s);
    static cs::String Apply(cs::Ref<Party> party, cs::Ref<Character> caster, cs::Ref<Spell> s, cs::Ref<SpellThrown_Target> t, int32_t healLevel,
                            cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> item, cs::Ref<CombatEngine> fight, cs::Ref<Character> commander = {});
    static SpellThrown_Ask WhoToAsk(cs::Ref<Spell> s);
};

}  // namespace mc
