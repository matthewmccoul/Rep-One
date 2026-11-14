#include "core.h"

namespace mc {

cs::String Loot::TypeName(cs::Ref<Item> it) {
    return Mdata1::Loaded ? Mdata1::ItemTypeName(it->Type)
        : (it->Type >= 0 && it->Type < TypeNames.Length() ? TypeNames[it->Type] : cs::String("Item"));
}

cs::String Loot::IdLevelName(IdLevel known) {
    switch (known) {
        case IdLevel::All: return "Completely Identified";
        case IdLevel::Mostly: return "Mostly identified";
        case IdLevel::Partial: return "Partially identified";
        default: return "Barely identified";
    }
}

cs::String Loot::NameFor(cs::Ref<Item> it, IdLevel known, ItemAlignment alignment, int32_t charges) {
    switch (known) {
        case IdLevel::All: {
            cs::String s = it->Name;
            if (alignment > ItemAlignment::Unaligned) s += " " + ItemInfoPage::AlignmentTags[cs::Math::Clamp<int32_t>(static_cast<int32_t>(alignment), 0, 3)];
            if (charges > 0 || it->Charges > 0) s += " [" + cs::ToStr(charges) + "]";
            if (it->Cursed != 0) s += " (cursed)";
            return s;
        }
        case IdLevel::Mostly: return it->Name;
        case IdLevel::Partial: return TypeName(it);
        default:
            return Mdata1::Loaded ? Mdata1::ItemDescriptionName(it->Type) : cs::String("Unidentified Item");
    }
}

cs::String Loot::UnknownName(cs::Ref<Item> it) { return NameFor(it, IdLevel::Barely); }

double Loot::IdentifyDifficulty(cs::Ref<Item> it) {
    return 25 + (cs::Math::Log(it->DamageMod + 1) + 0.3) * cs::Math::Log(it->LevelScale + 1) * it->Floor;
}

IdLevel Loot::ReadableLevel(cs::Ref<Character> c, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    // mdrcharGetIDSkill reads Stats + StatMods plainly (MAINMOD2.BAS:2511-2512); both skill and difficulty round (fn12F0)
    int32_t intel = gear ? c->StatWithMods(static_cast<int32_t>(Stat::Int), gear) : c->Stats[static_cast<int32_t>(Stat::Int)];
    int32_t wis = gear ? c->StatWithMods(static_cast<int32_t>(Stat::Wis), gear) : c->Stats[static_cast<int32_t>(Stat::Wis)];
    int32_t skill = cs::CastDouble<int32_t>(cs::Math::Round(Rules::IdentifySkill(c->TopGuildLevel(), intel, wis), cs::MidpointRounding::ToEven));
    int32_t difficulty = cs::CastDouble<int32_t>(cs::Math::Round(IdentifyDifficulty(it), cs::MidpointRounding::ToEven));

    if (difficulty <= skill) return it->Floor <= 5 ? IdLevel::Mostly : IdLevel::Partial;
    return difficulty - skill >= 20 ? IdLevel::Barely : IdLevel::Partial;
}

void Loot::OnReceived(cs::Ref<Character> receiver, cs::Ref<InventorySlot> slot, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    if (slot->Known() == IdLevel::All) return;
    // the test, which asks about the item you are actually holding
    if (ReadableLevel(receiver, it, gear) <= slot->Known()) return;
    // and the assignment, which asks about whatever item LevelScale names
    cs::Ref<Item> stray = gear ? gear((int16_t)it->LevelScale) : nullptr;
    slot->set_Known(ReadableLevel(receiver, stray ? stray : it, gear));
}

ItemAlignment Loot::RollAlignment(cs::Ref<Item> it, cs::Ref<cs::Random> rng) {
    auto options = ItemAlignments::Possible(it);
    return options[rng->Next(options.Count())];
}

bool Loot::AttachesItself(cs::Ref<Item> it) { return it->Cursed == 2; }

cs::Ref<InventorySlot> Loot::NewSlot(cs::Ref<Item> it, cs::Ref<cs::Random> rng, bool identified) {
    auto _o = cs::New<InventorySlot>();
    _o->ItemId = it->Id;
    _o->set_Identified(identified);
    _o->Charges = it->Charges > 0 ? (int16_t)it->Charges : (int16_t)0;
    _o->Alignment = RollAlignment(it, rng);
    return _o;
}

bool Loot::DropsItem(cs::Ref<Monster> lead, int32_t chestClass, cs::Ref<cs::Random> rng, int32_t lockState, int32_t dungeonLevel) {
    int32_t adj = chestClass == 1 ? 15 : (chestClass == 3 ? -25 : 0);
    double roll = rng->Next(100) + 1 + adj;
    if (lockState == -1) roll += cs::Math::Log(dungeonLevel + 10) * 5;
    if (roll > 90 && lead->ItemChance != 100) roll = 90;
    return roll <= lead->ItemChance;
}

bool Loot::DropsAnother(cs::Ref<Monster> lead, cs::Ref<cs::Random> rng) {
    return rng->Next(100) + 1 <= (int32_t)(lead->ItemChance / 10);
}

cs::Ref<Item> Loot::RollDrop(cs::List<cs::Ref<Item>> all, cs::Ref<Monster> lead, int32_t chestClass, cs::Ref<cs::Random> rng) {
    for (int32_t restart = 0; restart < 60; restart++) {
        // MONSMOD.BAS:3677 lands this float division in an Integer, so it rounds half to even
        int32_t top = lead->ItemDropLevel;
        int32_t low = cs::CastDouble<int32_t>(cs::Math::Round(lead->ItemDropLevel / 2.0, cs::MidpointRounding::ToEven));
        if (rng->Next(100) < lead->BoxChance[3]) low = top - 1;
        if (chestClass == 1) { top -= 1; low -= 1; }
        else if (chestClass == 3) low += 1;
        if (top > lead->ItemDropLevel) top = lead->ItemDropLevel;
        if (low < 1) low = 1;
        if (top < 1) top = 1;
        if (low > top) low = top;
        // last, and unclamped, exactly where MODULE4.BAS:3697 puts it
        if (Asmodeus::Is(lead)) { auto band = Asmodeus::DropBand(lead); low = band.low; top = band.top; }

        bool restartBand = false;
        for (int32_t pick = 0; pick < 200 && !restartBand; pick++) {
            int32_t entry = lead->Items.Length() > 0 ? (int32_t)lead->Items[rng->Next(cs::Math::Min(10, lead->Items.Length()))] : -1;
            if (entry == -1) entry = rng->Next(cs::Math::Max(1, Mdata1::ItemTypes.Count()));

            if (entry < -1) {
                // a named item rather than a type
                int16_t want = (int16_t)(cs::Math::Abs(entry) - 2);
                auto exact = all.FirstOrDefault([&](auto& i) { return i->Id == want; });
                if (exact != nullptr) return exact;
                continue;
            }

            // record 0 is the null item, and the original treats index 0 as "not found"
            int32_t first = 0;
            for (int32_t i = 0; i < all.Count(); i++)
                if (all[i]->Type == entry && all[i]->Floor >= low) { first = i; break; }

            // sitting exactly on the top of the band: widen downwards and draw again
            if (first > 0 && all[first]->Floor == top && low > 1) { low -= 1; continue; }
            // nothing of this type in reach at all: the whole generation starts over
            if (first == 0 || all[first]->Floor > top) { restartBand = true; break; }

            int32_t last = first;
            for (int32_t i = first; i < all.Count(); i++)
                if (all[i]->Floor > top || all[i]->Type != entry) { last = i - 1; break; }
            if (last < first) last = first;

            double total = 0;
            for (int32_t i = first; i <= last; i++) total += all[i]->Rarity;
            if (total <= 0) continue;

            double draw = rng->NextDouble() * 100, acc = 0;
            for (int32_t i = first; i <= last; i++) {
                acc += all[i]->Rarity / total * 100;
                if (draw <= acc) return all[i];
            }
        }
    }
    return nullptr;
}

int16_t Loot::RollCharges(cs::Ref<Item> it, cs::Ref<cs::Random> rng) {
    if (it->Charges <= 0) return 0;
    int32_t n = it->Charges;
    if (it->Charges > 1) {
        // sequenced: C# draws the sign before the magnitude
        int32_t sign = rng->Next(2) == 0 ? 1 : -1;
        n += sign * cs::CastDouble<int32_t>(rng->NextDouble() * it->Charges / 2);
    }
    return (int16_t)cs::Math::Max(0, n);
}

}  // namespace mc
