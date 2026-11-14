#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Seer {
    using Found = Seer_Found;

    static constexpr int32_t DeepestHeCanReach = 12;
    inline static const cs::String SeveralMonsters = "The name you provided is shared by more than one creature.";
    inline static const cs::String SeveralItems = "The name you provided is shared by more than one item.";
    inline static const cs::String MonsterTooDeep = "Unfortunately, this creature dwells beyond the reach of my powers.";
    inline static const cs::String ItemTooDeep = "Unfortunately, this item is found on a level beyond the reach of my powers.";
    inline static const cs::String NotEnoughGold = "Not enough gold!";
    inline static const cs::String NoPlaceForMonster = "I cannot locate a general place for this creature.  Another try may work, though.";
    inline static const cs::String NoCarrier = "I cannot locate a creature who may have this item.  Another try may work, though.";
    inline static const cs::String CannotDetect = "I am unable to detect the creature in the depths below. ";
    inline static const cs::String NoSpecificCarrier = "I was unable to sense the presense of a creature who specifically carries such an item.";
    inline static const cs::String InTheCity = "Do you mock my abilities?!\nThey are here in the city!";
    inline static const cs::String AlwaysSeen = "almost always seen";

    static int64_t Fee(cs::Ref<cs::Random> rng, int32_t level);
    static Tup_found_index_95301 Match(cs::String typed, cs::List<cs::String> names, cs::List<cs::String> alsoNames = {});
    static cs::String NoSuchMonster(cs::String typed);
    static cs::String NoSuchItem(cs::String typed);
    static cs::String OfferMonster(cs::String name, cs::String fee);
    static cs::String OfferItem(cs::String name, cs::String fee);
    static cs::String MonsterLevelOnly(int32_t level);
    static cs::String ItemLevelOnly(int32_t level);
    static bool SearchFailsEarly(cs::Ref<cs::Random> rng, int32_t level);
    static bool FailureNamesTheLevel(cs::Ref<cs::Random> rng, int32_t level);
    static int64_t LocateCharacterFee(int32_t level);
    static cs::String OfferCharacter(cs::String name, cs::String fee);
    static cs::String LocateCharacter(cs::Ref<cs::Random> rng, cs::String name, int32_t level, int32_t x, int32_t y, bool dead);
    static cs::String Commonality(int32_t classesInArea);
    static cs::String Frequency(int32_t chance);
    static cs::String Sighting(cs::String name, int32_t x, int32_t y, int32_t level);
    static cs::String TypeIsCommon(cs::String howCommon);
    static cs::String SeenAmongItsType(cs::String howOften);
    static Tup_x_y_82717 Blur(cs::Ref<cs::Random> rng, int32_t x, int32_t y, int32_t spread, int32_t width, int32_t height);
    static cs::Nullable<Tup_x_y_classesInArea_lair_d7641> Locate(cs::Ref<Floor> floor, cs::Ref<Monster> def, cs::Ref<cs::Random> rng);
    static cs::List<Tup_def_certain_5800d> Carriers(cs::List<cs::Ref<Monster>> monsters, cs::Ref<Item> it, cs::Ref<cs::Random> rng);
    static cs::String Carriers(cs::List<Tup_def_certain_5800d> found, cs::Ref<cs::Random> rng);
};

}  // namespace mc
