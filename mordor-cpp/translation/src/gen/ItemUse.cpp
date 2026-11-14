#include "core.h"

namespace mc {

bool ItemUse::IsGuildIssued(cs::Ref<Item> it) {
    return it->SpecialType >= GuildIssuedBase && it->SpecialType < GuildIssuedBase + GuildData::All.Count();
}

cs::String ItemUse::RechargeGuildItems(cs::Ref<Character> c, bool party, cs::Func<cs::Ref<Item>(int16_t)> item) {
    bool recharged = false;
    for (int32_t i = 1; i < Character::PackSize; i++) {
        // If Chars(pv0652).Pack(l0654).Ref > 0
        auto sl = c->Pack()[i];
        if (!sl || sl->ItemId <= 0) continue;
        auto it = item(sl->ItemId);
        if (!it) continue;
        // Ailments in 500 .. 500 + (NumGuilds - 1), and Pack charges < Items charges
        if (IsGuildIssued(it) && sl->Charges < it->Charges) {
            recharged = true;
            sl->Charges = (int16_t)it->Charges;
        }
    }
    if (!recharged) return "";
    return party ? "\"" + cs::Str::TrimEnd(c->Name) + "\" had their Guild Item(s) recharged when they entered the city."
                 : "Your Guild Item(s) were recharged when you entered the city.";
}

bool ItemUse::MustBeEquippedToUse(cs::Ref<Item> it) {
    return (it->Type >= 1 && it->Type <= 22) || it->Type == 29 || it->Type == 32 || it->Type == 33 || it->Type == 34;
}

bool ItemUse::ConsumedWhenEmpty(cs::Ref<Item> it) {
    return (it->Type >= 23 && it->Type <= 28) || it->Type == 30 || it->Type == 31;
}

cs::String ItemUse::Problem(cs::Ref<Character> c, cs::Ref<InventorySlot> slot, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> lookup) {
    // a negative SpecialType means the item is not a usable thing at all, and says nothing
    if (it->SpecialType < 0) return "";

    if (it->Charges <= 0)
        return "\"" + c->Name + "\", the Item doesn't appear to do much of anything.";
    if (slot->Charges <= 0)
        return "\"" + c->Name + "\", the Item is out of charges!";

    // fn11B6 with its fourth argument set, so the hands and worn-count limits are skipped
    auto why = c->EquipProblem(it);
    if (!why.empty()) return why;

    if (it->SpellIndex > 0) {
        // a spell out of an aligned item only works for its own: not half, nothing
        if (slot->Alignment != ItemAlignment::Unaligned
            && ItemAlignments::AsCharacter(slot->Alignment) != c->Alignment) return "Nothing Happens.";
        if (MustBeEquippedToUse(it) && !slot->Equipped) return "Item must be Equipped to use!";
    }
    return "";
}

cs::Ref<Spell> ItemUse::SpellOf(cs::Ref<Item> it, cs::List<cs::Ref<Spell>> spells) {
    return it->SpellIndex > 0 && it->SpellIndex - 1 < spells.Count() ? spells[it->SpellIndex - 1] : nullptr;
}

int32_t ItemUse::CastLevel(cs::Ref<Item> it, cs::Ref<Character> c) {
    return it->SpellLvl > 0 ? (int32_t)it->SpellLvl : Rules::SpellLevel(c->Guild()->Level);
}

cs::List<cs::String> ItemUse::ApplySpecial(cs::Ref<Character> c, cs::Ref<InventorySlot> slot, cs::Ref<Item> it,
                                           cs::Ref<HallRecords> hall, cs::String savesDir,
                                           bool inParty, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    auto said = cs::List<cs::String>::New();
    bool reversed = false;
    if (slot->Alignment != ItemAlignment::Unaligned) {
        switch (ItemAlignments::FitFor(slot->Alignment, c->Alignment)) {
            case ItemFit::Adjacent: said.Add("Nothing Happens."); return said;
            case ItemFit::Opposed: reversed = true; break;
            default: break;
        }
    }

    if (it->SpecialType <= 0) return said;

    // MAINMOD2.BAS:6157-6162 - everything but a guild crest makes a noise about it
    if (!IsGuildIssued(it)) said.Add(Sound("MAGIC"));

    bool applyStats = false;
    switch (it->SpecialType) {
        case 1:
            applyStats = true;
            break;
        case 11:
            if (!reversed) c->CurSpellPoints += 50;    // MAINMOD2.BAS:6164-6168, and nothing said
            break;
        case 12:
            if (!reversed) c->CurSpellPoints += 200;   // 6169-6173, likewise silent
            break;
        case 13:
            if (!reversed) said.Add(Younger(c, 1, "one year"));
            applyStats = true;
            break;
        case 14:
            if (!reversed) said.Add(Younger(c, 10, "ten years"));
            applyStats = true;
            break;
    }

    if (applyStats) {
        said.AddRange(ApplyStatMods(c, it, reversed, hall, savesDir, inParty));
        // mdrcharCheckItems(pv0850, True) (MAINMOD2.BAS:6227)
        if (gear != nullptr) said.AddRange(GearCheck::Run(c, gear, inParty, true));
        // Kind = 25: mdrcharAddAge (MAINMOD2.BAS:6228), whether or not the item was reversed
        if (it->Type == 25) said.Add(c->AgeBy(ItemKind25AgeDays, inParty));
    }
    said.RemoveAll([&](const cs::String& s) { return cs::Str::IsNullOrEmpty(s); });
    return said;
}

bool ItemUse::ChangesStats(cs::Ref<Character> c, cs::Ref<InventorySlot> slot, cs::Ref<Item> it) {
    return !(slot->Alignment != ItemAlignment::Unaligned && ItemAlignments::FitFor(slot->Alignment, c->Alignment) == ItemFit::Adjacent)
        && (it->SpecialType == 1 || it->SpecialType == 13 || it->SpecialType == 14);
}

cs::String ItemUse::Sound(cs::String name) {
    return SoundPrefix + name;
}

cs::String ItemUse::SoundIn(cs::String line) {
    return cs::Str::StartsWith(line, SoundPrefix) ? cs::Str::Substring(line, (int32_t)SoundPrefix.size()) : cs::String();
}

cs::String ItemUse::Younger(cs::Ref<Character> c, int32_t years, cs::String howMany) {
    c->Age -= years * 365;
    if (c->Age < 16 * 365) { c->Age = 16 * 365; return "You cannot get any younger."; }
    return "You are now " + howMany + " younger.";
}

cs::List<cs::String> ItemUse::ApplyStatMods(cs::Ref<Character> c, cs::Ref<Item> it, bool reversed,
                                            cs::Ref<HallRecords> hall, cs::String savesDir, bool inParty) {
    auto said = cs::List<cs::String>::New();
    auto race = c->RaceDef();
    for (int32_t i = 0; i < 6 && i < it->StatsMod.Length(); i++) {
        int32_t mod = it->StatsMod[i];
        if (mod != 0) {
            int32_t ceiling = race->Max[i] + 5;
            if (reversed && mod > 0) {
                c->Stats[i] -= mod;
                if (c->Stats[i] > ceiling) c->Stats[i] = ceiling;
                else said.Add(StatLine(c, i, -mod, inParty));
            } else {
                c->Stats[i] += mod;
                if (c->Stats[i] > ceiling) c->Stats[i] = ceiling;
                else said.Add(StatLine(c, i, mod, inParty));
                // mdrhalCheckRecord (6219) is after the If, not in its Else
                if (hall != nullptr && !savesDir.empty()) {
                    cs::String took = hall->CheckAndLog(StatRecord(i), c, VbCurrency((int64_t)c->Stats[i]), savesDir);
                    if (!took.empty()) said.Add(took);
                }
            }
        }
        if (c->Stats[i] < race->Min[i]) c->Stats[i] = race->Min[i];
    }
    c->RecalcSpellPoints();
    c->RecalcSwings();
    return said;
}

HallRecords_Kind ItemUse::StatRecord(int32_t i) {
    switch (i) {
        case 0: return HallRecords_Kind::Strength;
        case 1: return HallRecords_Kind::Intelligence;
        case 2: return HallRecords_Kind::Wisdom;
        case 3: return HallRecords_Kind::Constitution;
        case 4: return HallRecords_Kind::Charisma;
        default: return HallRecords_Kind::Dexterity;
    }
}

cs::String ItemUse::StatLine(cs::Ref<Character> c, int32_t stat, int32_t delta, bool inParty) {
    cs::String name = RaceData::StatNamesLong[stat];
    if (inParty) {
        cs::String their = c->Sex == Sex::Male ? "his" : "her";
        return delta > 0
            ? "\"" + c->Name + "\" had " + their + " " + name + " increased by " + cs::ToStr(delta) + "."
            : "\"" + c->Name + "\" had " + their + " " + name + " decreased by " + cs::ToStr(cs::Math::Abs(delta)) + ".";
    }
    return delta > 0
        ? "Your " + name + " has increased by " + cs::ToStr(delta) + "."
        : "Your " + name + " has decreased by " + cs::ToStr(cs::Math::Abs(delta)) + ".";
}

cs::String ItemUse::DropProblem(cs::Ref<InventorySlot> slot, cs::Ref<Item> it, int32_t packSlot) {
    if (packSlot <= 0) return "";   // your hands, and the original says nothing about it
    if (IsGuildIssued(it)) return "Cannot drop Guild-Issued Items!";
    return "";
}

cs::String ItemUse::CursedRefusal(cs::Ref<InventorySlot> slot, cs::Ref<Item> it, int32_t packIndex) {
    return slot->SlotCursed && slot->Equipped
        ? "You can't drop item " + cs::ToStr(packIndex + 1) + ". " + Loot::NameFor(it, slot->Known(), slot->Alignment, slot->Charges) + "!  It's cursed!"
        : cs::String();
}

}  // namespace mc
