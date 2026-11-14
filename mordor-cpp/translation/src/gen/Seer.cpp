#include "core.h"

namespace mc {

int64_t Seer::Fee(cs::Ref<cs::Random> rng, int32_t level) {
    double fee = 132567.52 * cs::Math::Log(level + 1) * level;
    return cs::CastDouble<int64_t>((fee + rng->NextDouble() * fee / 4) / 2);
}

Tup_found_index_95301 Seer::Match(cs::String typed, cs::List<cs::String> names, cs::List<cs::String> alsoNames) {
    if ((int32_t)typed.size() == 0) return Tup_found_index_95301{Found::None, -1};
    int32_t count = 0, last = -1;
    for (int32_t i = 0; i < names.Count(); i++) {
        // UCase(typed) = Left(UCase(name), Len(typed)): a name shorter than the typing never matches
        cs::String name = names[i];
        if ((int32_t)name.size() < (int32_t)typed.size() || !cs::Str::StartsWithIgnoreCase(name, typed)) {
            if (alsoNames == nullptr || i >= alsoNames.Count()) continue;
            name = alsoNames[i];
            if ((int32_t)name.size() < (int32_t)typed.size() || !cs::Str::StartsWithIgnoreCase(name, typed)) continue;
        }
        last = i;
        count++;
        if (cs::Str::EqualsIgnoreCase(name, typed)) return Tup_found_index_95301{Found::One, i};
    }
    if (count == 0) return Tup_found_index_95301{Found::None, -1};
    if (count == 1) return Tup_found_index_95301{Found::One, last};
    return Tup_found_index_95301{Found::Several, -1};
}

cs::String Seer::NoSuchMonster(cs::String typed) {
    return "I am unable to locate a creature called '" + cs::ToStr(typed) + "'.";
}

cs::String Seer::NoSuchItem(cs::String typed) {
    return "I am unable to locate an item by the name of '" + cs::ToStr(typed) + "'.";
}

cs::String Seer::OfferMonster(cs::String name, cs::String fee) {
    return "I can sense the general presense of a creature known as '" + cs::ToStr(name) + "' somewhere in the depths below.  "
        + ("A small fee of " + cs::ToStr(fee) + " will be required to do the search.\nDo you accept?");
}

cs::String Seer::OfferItem(cs::String name, cs::String fee) {
    return "I can sense the general presense of a creature who may carry the item known as '" + cs::ToStr(name) + "' somewhere in the depths below.  "
        + ("A small fee of " + cs::ToStr(fee) + " will be required to do the search.\nDo you accept?");
}

cs::String Seer::MonsterLevelOnly(int32_t level) {
    return "I can detect the presense of this creature starting somewhere on level " + cs::ToStr(level) + ", but that is all.\n"
        + cs::String("Another try may reveal more information.");
}

cs::String Seer::ItemLevelOnly(int32_t level) {
    return "I can detect this item starting to be found somewhere on level " + cs::ToStr(level) + ", but that is all.\n"
        + cs::String("Another try may reveal more information.");
}

bool Seer::SearchFailsEarly(cs::Ref<cs::Random> rng, int32_t level) {
    return rng->Next(100) + 1 < level * 5;
}

bool Seer::FailureNamesTheLevel(cs::Ref<cs::Random> rng, int32_t level) {
    return rng->NextDouble() * 100 >= level * 5;
}

int64_t Seer::LocateCharacterFee(int32_t level) {
    return cs::CastDouble<int64_t>(132567.52 * cs::Math::Log(level + 1) * level);
}

cs::String Seer::OfferCharacter(cs::String name, cs::String fee) {
    return "I can feel their presense.  I will need " + cs::ToStr(fee) + " to attempt to locate " + cs::ToStr(name) + ".\nIs this acceptable?";
}

cs::String Seer::LocateCharacter(cs::Ref<cs::Random> rng, cs::String name, int32_t level, int32_t x, int32_t y, bool dead) {
    int32_t need = level * 3;
    int32_t roll = rng->Next(100) + 1;
    if (roll < need)
        return roll + 10 >= need
            ? "I can detect them somewhere on level " + cs::ToStr(level) + ", but I've lost connection."
            : "I cannot locate \"" + cs::ToStr(name) + "\".  Another try may work, though.";

    cs::String said = "I have detected \"" + cs::ToStr(name) + "\" at: " + cs::ToStr(x) + "," + cs::ToStr(y) + "," + cs::ToStr(level) + ".";
    if (dead) said += "\nI also feel that they are among the dead.";
    return said;
}

cs::String Seer::Commonality(int32_t classesInArea) {
    if (classesInArea <= 2) return "very common";
    if (classesInArea <= 4) return "fairly common";
    if (classesInArea <= 7) return "uncommon";
    if (classesInArea <= 11) return "rare";
    return "almost unheard of";
}

cs::String Seer::Frequency(int32_t chance) {
    if (chance <= 2) return "almost never";
    if (chance <= 8) return "very rarely";
    if (chance <= 15) return "rarely";
    if (chance <= 35) return "uncommonly";
    if (chance <= 50) return "commonly";
    return "very commonly";
}

cs::String Seer::Sighting(cs::String name, int32_t x, int32_t y, int32_t level) {
    return "I detect the possible presense of the creature known as '" + cs::ToStr(name) + "' in the area of " + cs::ToStr(x) + "," + cs::ToStr(y) + "," + cs::ToStr(level) + ".";
}

cs::String Seer::TypeIsCommon(cs::String howCommon) {
    return "I also sense that creatures of this general type are " + cs::ToStr(howCommon) + " in this area.";
}

cs::String Seer::SeenAmongItsType(cs::String howOften) {
    return "I can also detect that this creature is " + cs::ToStr(howOften) + " seen among its type.";
}

Tup_x_y_82717 Seer::Blur(cs::Ref<cs::Random> rng, int32_t x, int32_t y, int32_t spread, int32_t width, int32_t height) {
    x = rng->Next(2) == 1 ? x - cs::CastDouble<int32_t>(rng->NextDouble() * spread) : x + cs::CastDouble<int32_t>(rng->NextDouble() * spread);
    y = rng->Next(2) == 1 ? y - cs::CastDouble<int32_t>(rng->NextDouble() * spread) : y + cs::CastDouble<int32_t>(rng->NextDouble() * spread);
    return Tup_x_y_82717{cs::Math::Clamp<int32_t>(x, 1, width), cs::Math::Clamp<int32_t>(y, 1, height)};
}

cs::Nullable<Tup_x_y_classesInArea_lair_d7641> Seer::Locate(cs::Ref<Floor> floor, cs::Ref<Monster> def, cs::Ref<cs::Random> rng) {
    struct Cand { int32_t x{}; int32_t y{}; int32_t classes{}; };
    auto seen = cs::HashSet<int32_t>::New();
    auto candidates = cs::List<Cand>::New();
    for (int32_t x = 1; x <= floor->Width; x++) {
        for (int32_t y = 1; y <= floor->Height; y++) {
            int32_t id = floor->AreaIdAt(x - 1, y - 1);
            if (!seen.Add(id)) continue;
            auto areaOpt = floor->AreaAt(x - 1, y - 1);
            if (!areaOpt.has_value()) continue;
            Floor_Area area = areaOpt.value();
            if (area.LairId > 0 && area.LairId == def->Id && rng->NextDouble() * 100 < 75)
                return Tup_x_y_classesInArea_lair_d7641{x, y, 0, true};
            if (((uint32_t)area.SpawnMask & (1u << (def->Type & 31))) == 0) continue;
            int32_t classes = 0;
            for (int32_t c = 0; c < Mdata1::MonsterClasses.Count(); c++)
                if (((uint32_t)area.SpawnMask & (1u << (c & 31))) != 0) classes++;
            candidates.Add(Cand{x, y, classes});
        }
    }
    if (candidates.Count() == 0) return std::nullopt;
    auto pick = candidates[rng->Next(candidates.Count())];
    return Tup_x_y_classesInArea_lair_d7641{pick.x, pick.y, pick.classes, false};
}

cs::List<Tup_def_certain_5800d> Seer::Carriers(cs::List<cs::Ref<Monster>> monsters, cs::Ref<Item> it, cs::Ref<cs::Random> rng) {
    int32_t band = rng->Next(3);
    auto found = cs::List<Tup_def_certain_5800d>::New();
    for (auto& m : monsters) {
        if (m->Id == 0) continue;
        for (int16_t entry : m->Items) {
            bool hit = false, certain = false;
            if (entry == it->Type && m->ItemDropLevel >= it->Floor && m->ItemDropLevel <= it->Floor + band) hit = true;
            if (entry < -1 && cs::Math::Abs(entry) - 2 == it->Id) { hit = true; certain = true; }
            if (hit) found.Add(Tup_def_certain_5800d{m, certain});
        }
    }
    return found;
}

cs::String Seer::Carriers(cs::List<Tup_def_certain_5800d> found, cs::Ref<cs::Random> rng) {
    if (found.Count() == 0) return NoSpecificCarrier;
    // l0070 is an Integer (FRMSEEK.BAS:44), so the Single it is given rounds, half to even
    int32_t take = cs::CastDouble<int32_t>(cs::Math::Round(found.Count() - (double)(float)rng->NextDouble() * found.Count(), cs::MidpointRounding::ToEven));
    if (take > 3) take = 3;
    if (take > found.Count()) take = found.Count();
    if (take < 1) take = 1;

    // FRMSEEK.BAS:304-317: repeats add nothing; the last five characters are always cut, kept as written
    auto used = cs::HashSet<int32_t>::New();
    cs::String text = "The creature(s) known as ";
    for (int32_t i = 0; i < take; i++) {
        int32_t pick = rng->Next(found.Count());
        if (!used.Add(pick)) continue;
        text += found[pick].def->Name + (found[pick].certain ? " (very strong) and " : " and ");
    }
    text = cs::Str::Substring(text, 0, (int32_t)text.size() - 5);
    return text + " come to me as possible carriers for the item you seek.";
}

}  // namespace mc
