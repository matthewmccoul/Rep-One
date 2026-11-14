#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct PendingRaise {
    inline static const cs::String OfferTitle = "RAISE";
    inline static const cs::String AlreadyCast = "Raise already cast on Character!";
    static bool IsPending(cs::Ref<Character> c);
    static void Initiate(cs::Ref<Character> body, cs::Ref<Character> raiser, int32_t chance, int32_t kind);
    static void Prepare(cs::Ref<Character> c);
    static cs::String Offer(cs::Ref<Character> c, bool alone);
    static cs::Ref<RaiseOutcome> Accept(cs::Ref<cs::Random> rng, cs::Ref<Character> c, cs::Ref<Race> race, cs::Func<cs::Ref<Item>(int16_t)> lookup, bool alone, cs::String& raiser);
    static void Forget(cs::Ref<Character> c);
    static cs::String BoxTitleFor(cs::String line);
    static cs::String VbStr(int64_t n);
};

struct Morgue {
    inline static const cs::String Welcome = "Welcome to the Morgue!";
    inline static const cs::String Rocked = "We are unable to help a rocked character.  You'll need to find somebody who can cast 'resurrect'.  Sorry!";
    inline static const cs::String NotEnoughGold = "Not enough gold!";
    inline static const cs::String TakenCareOf = "Character taken care of!";
    inline static const cs::String RaiseFailed = "Raise Failed!";
    inline static const cs::String RaiseWorked = "Raise Was Successful!";
    static int64_t RaiseCost(int32_t maxHits);
    static int32_t RaiseChance(int32_t con, int32_t causeOfDeath);
    static cs::String CostOffer(cs::String name, int64_t cost);
    static int64_t CompanionCost(int32_t maxHits, int32_t floor);
    static cs::String CompanionCaption(cs::String companionName, int64_t cost);
    static int32_t CompanionChance(int32_t con);
    static cs::String FormatNumber(int64_t v);
};

}  // namespace mc
