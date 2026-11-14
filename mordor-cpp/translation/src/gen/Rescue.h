#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Rescue_Outcome {
    cs::String Message{};
    bool BroughtBack = false;
    Rescue_Outcome() = default;
    Rescue_Outcome(cs::String Message, bool BroughtBack) : Message(Message), BroughtBack(BroughtBack) {}
    bool operator==(const Rescue_Outcome& o) const { return Message == o.Message && BroughtBack == o.BroughtBack; }
    bool operator!=(const Rescue_Outcome& o) const { return !(*this == o); }
};

struct Rescue {
    using Outcome = Rescue_Outcome;

    static int32_t DaysUntilFound(cs::Ref<cs::Random> rng, int32_t dungeonLevel, int32_t areaId, bool easyToFind);
    static cs::String Found(int32_t days);
    static bool EasyToFind(cs::Ref<Floor> floor, int32_t x, int32_t y);
    static void CarryHome(cs::Ref<Character> body, int32_t days);
    static void TurnOverTown(cs::String savesDir, cs::List<cs::Ref<Item>> items, cs::List<cs::Ref<Monster>> monsters,
                             cs::Ref<cs::Random> rng, int32_t deepestReached);
    static int64_t SearchCost(int32_t level);

    inline static const cs::String BadCoordinates = "We require proper coordinates (X,Y,Level) before we can dispatch rescuers!";
    inline static const cs::String InvalidCoordinates = "Invalid Coordinates!";
    inline static const cs::String NotEnoughGold = "Not enough gold!";
    inline static const cs::String NeverReturned = "Rescuers never returned...  Don't you feel guilty now?";

    static cs::String Offer(int64_t cost, cs::String name);
    static bool RescuersReturn(cs::Ref<cs::Random> rng, int32_t level);
    static int32_t Val(cs::String s);
    static cs::List<int32_t> SplitCoords(cs::String s);
    static bool CloseEnough(int32_t guessX, int32_t guessY, int32_t guessLevel, int32_t x, int32_t y, int32_t level);
    static Rescue_Outcome Report(cs::Ref<Character> who, bool foundAtAll);

    inline static const cs::String DeadInTheDungeon =
        "You are Dead.  You may be resurrected by the morgue or a character, wait for rescue, or run another character.";
    inline static const cs::String DeadInRock =
        "You are Dead.  You must be summoned out of the Rock before you can be resurrected.";
};

}  // namespace mc
