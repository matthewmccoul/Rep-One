#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct SoundFiles {
    inline static cs::List<cs::String> Names = cs::List<cs::String>::Of({
        "FIGHT", "DIE", "HIT", "KILL", "MAGIC", "OPEN", "FALL", "BLACKOUT",              //  0-7
        "SCURRY", "MOAN", "DSTSLAM", "RAISE", "STEAL", "BUG", "SLIME", "GOBLIN",          //  8-15
        "DOG", "DEVIL", "SNAKE", "UNDEAD", "WATERD", "GIANT", "MYTHIC", "LYCAN",          // 16-23
        "MAGE", "WARRIOR", "WATER", "WATER1", "FIRE", "TELEPORT", "DI-DEMON", "DI-HUMAN", // 24-31
        "DI-ANIM", "SHOCK", "BYE", "OUCH", "SWING", "PARAL", "POISON", "DISEASE",         // 32-39
        "BETTER", "FOUND", "COLD", "EXPLOS", "MAGIC2", "WADE", "SLICE", "DRAIN",          // 40-47
        "FEAR", "SLIMETR", "SPIT", "STONE", "WITHER", "MAKELEV", "BUGDIE", "RIP",         // 48-55
        "HELP", "DEMON", "DRAGON", "BIRD", "BIRDDIE", "EXTING", "HELLO", "YEECH",         // 56-63
        "DI-UNDEA", "FOG", "MESSAGE", "DROWN", "ASMO", "PLANTATT", "BSTAB", "CHIT",       // 64-71
    });

    static cs::String Name(int32_t n);
};

struct SpellSounds {
    static constexpr int32_t Fire = 28;
    static constexpr int32_t Shock = 33;
    static constexpr int32_t Cold = 42;
    static constexpr int32_t Magic2 = 44;
    static constexpr int32_t Magic = 4;
    static constexpr int32_t Explos = 43;

    static cs::List<int32_t> Numbers(int32_t kind, int32_t level);
    static cs::List<cs::String> For(cs::Ref<Spell> s);
};

}  // namespace mc
