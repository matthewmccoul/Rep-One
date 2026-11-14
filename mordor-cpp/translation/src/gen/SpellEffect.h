#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

// Return shapes of TeleportTo / RelativeDirection (the catalog lists no tuple struct for them)
struct Tup_X_Y_Level_SpellEffect { int32_t X{}; int32_t Y{}; int32_t Level{}; };
struct Tup_Right_Forward_Down_SpellEffect { int32_t Right{}; int32_t Forward{}; int32_t Down{}; };

struct SpellEffect {
    static constexpr int32_t PartyOffset = 200;

    static constexpr int32_t Damage = 0;            // and healing: the branch keys off Damage2
    static constexpr int32_t CurePoison = 1;
    static constexpr int32_t CureDisease = 2;
    static constexpr int32_t Levitate = 3;
    static constexpr int32_t Invisible = 4;
    static constexpr int32_t SeeInvisible = 5;
    static constexpr int32_t Protect = 6;
    static constexpr int32_t Teleport = 7;
    static constexpr int32_t Displacement = 8;
    static constexpr int32_t Passwall = 9;
    static constexpr int32_t CureParalysis = 10;
    static constexpr int32_t DetectRock = 11;
    static constexpr int32_t DepthPerception = 12;
    static constexpr int32_t FindDirection = 13;
    static constexpr int32_t SoulSearch = 14;
    static constexpr int32_t RetrieveSoul = 15;
    static constexpr int32_t SetSanctuary = 16;
    static constexpr int32_t Sanctuary = 17;
    static constexpr int32_t Cure = 18;
    static constexpr int32_t Fate = 19;
    static constexpr int32_t RaiseFirst = 20;
    static constexpr int32_t RaiseFull = 22;
    static constexpr int32_t RaiseLast = 25;
    static constexpr int32_t OpenFirst = 30;
    static constexpr int32_t OpenLast = 32;
    static constexpr int32_t BindFirst = 40;
    static constexpr int32_t BindLast = 42;
    static constexpr int32_t ResistFirst = 50;
    static constexpr int32_t CharmFirst = 80;
    static constexpr int32_t CharmLast = 99;
    static constexpr int32_t Kill = 100;
    static constexpr int32_t BindForLife = 150;

    static constexpr int32_t ResistGranted = 95;

    static constexpr int32_t SanctuaryDays = 365;

    static constexpr int32_t LocateLevelRange = 3;

    static constexpr int32_t MaxDungeonLevel = 15;

    static int32_t Of(cs::Ref<Spell> s);
    static bool IsParty(cs::Ref<Spell> s);
    static bool IsState(int32_t effect);
    static int32_t ResistKind(int32_t effect);
    static cs::String ApplyToCharacter(int32_t effect, cs::Ref<Character> c, cs::Ref<Spell> s, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> item);
    static Tup_X_Y_Level_SpellEffect TeleportTo(int32_t x, int32_t y, Facing facing, int32_t level, int32_t right, int32_t forwards, int32_t down = 0, int32_t width = 30, int32_t height = 30);
    static Tup_Right_Forward_Down_SpellEffect RelativeDirection(int32_t myX, int32_t myY, int32_t myLevel, Facing facing, int32_t theirX, int32_t theirY, int32_t theirLevel);
    static int32_t Wrap(int32_t v, int32_t size);
    static int32_t BindLevelFrom(int32_t spellLevel, int32_t casterSpellLevel);
    static cs::String BindCompanion(cs::Ref<Companion> comp, int32_t slot, cs::Ref<Spell> s, int32_t casterSpellLevel);
    static int32_t OpensLockUpTo(int32_t effect);
    static bool IsRaise(int32_t effect);
    static int32_t CompanionRaiseChance(int32_t monsterCon);
    static bool RaisesToFull(int32_t effect);
};

}  // namespace mc
