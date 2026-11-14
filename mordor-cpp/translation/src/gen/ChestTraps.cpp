#include "core.h"

namespace mc {

bool ChestTraps::Destructible(cs::Ref<Item> it) {
    return it->SpecialType < 500 || it->SpecialType > 500 + GuildData::All.Count() - 1;
}

cs::Ref<TrapResult> ChestTraps::Spring(
    int32_t trap, cs::List<cs::Ref<Character>> party, cs::Ref<Character> opener, int32_t dungeonLevel,
    cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> item, cs::Func<int32_t(int32_t, int32_t)> walls) {
    auto r = cs::New<TrapResult>();
    switch (trap) {
        case 1:     // Poison Gas
        case 2:     // Disease Spores
        {
            bool poison = trap == 1;
            int32_t kind = poison ? ResistPoison : ResistDisease;
            for (auto& c : party) {
                // As written, monOpenChest afflicts you when the roll falls *under* half your
                // resistance, which reads backwards - see docs/FIDELITY.md. Reproduced.
                if (!(rng->NextDouble() * 100 < Effects::ResistancePercent(c, kind, item) / 2.0)) continue;
                if (poison) { c->Poisoned = true; c->PoisonLevel = dungeonLevel; }
                else { c->Diseased = true; c->DiseaseLevel = dungeonLevel; }
            }
            break;
        }

        case 3:     // Exploding Trap - no resistance applies
        case 4:     // Fire Trap
        {
            int32_t mult = trap == 3 ? 4 : 6;
            for (auto& c : party) {
                double dmg = cs::CastDouble<int32_t>(rng->NextDouble() * (dungeonLevel * mult * cs::Math::Log(dungeonLevel + 2))) + 1;
                if (trap == 4)
                    dmg -= dmg * (Effects::ResistancePercent(c, ResistFire, item) / 2.0 * 0.01);
                // Hits - (a Single) goes back into an Integer, rounded half to even (MONSMOD.BAS:3530-3536)
                c->CurHits = cs::CastDouble<int32_t>(cs::Math::Round(c->CurHits - dmg, cs::MidpointRounding::ToEven));
            }
            break;
        }

        case 5:     // Slime - destroys one item
        {
            if (rng->Next(100) + 1 >= 50 + dungeonLevel * 2) break;
            // The original rolls a *slot* in the 40-slot pack, not one of the items you have.
            int32_t slot = rng->Next(PackSlots - 1) + 1;
            auto held = opener->Pack()[slot];
            if (held == nullptr || held->ItemId <= 0) break;
            auto it = item(held->ItemId);
            if (it == nullptr || !Destructible(it)) break;
            opener->RemoveFromPack(held);
            r->Lines.Add("SLIME: " + cs::ToStr(it->Name) + " Destroyed!");
            break;
        }

        case 6:     // Fate - sub0F29: both pools are rerolled
        {
            opener->CurHits = cs::CastDouble<int32_t>(rng->NextDouble() * opener->MaxHits) + 1;
            opener->CurSpellPoints = cs::CastDouble<int32_t>(rng->NextDouble() * opener->MaxSpellPoints) + 1;
            break;
        }

        case 7:     // Teleporter - anywhere on the level that is not rock and not a teleporter
        {
            const int32_t Rock = 19, Teleporter = 14;
            for (int32_t guard = 0; guard < 10000; guard++) {
                int32_t x = rng->Next(30) + 1, y = rng->Next(30) + 1;
                int32_t w = walls(x, y);
                if ((w & (1 << Rock)) != 0 || (w & (1 << Teleporter)) != 0) continue;
                r->TeleportTo = Teleported{x, y};
                break;
            }
            break;
        }

        case 8:     // Blackout - every spell you have running goes out
        {
            Effects::ClearSpellEffects(opener, item);
            break;
        }

        case 9:     // Fear - only a companion bound for life stands its ground
        {
            // the fallen are not scared off (MONSMOD.BAS:3618) and keep their slots
            int32_t fled = 0;
            for (auto& t : opener->Companions->Occupied().ToList()) {
                auto slot = t.Slot;
                auto c = t.Companion;
                if (c->Hits > 0 && c->BindingLevel != Binding::ForLife) { opener->Companions->Release(slot); fled++; }
            }
            // "If M5943 < mc01A0 Then M5943 = -1" (MONSMOD.BAS:3625)
            if (opener->CarryingCompanion < CompanionSlots::Slots) opener->CarryingCompanion = -1;
            if (fled > 0) r->Lines.Add("Your companions were scared away!");
            break;
        }

        case 10:    // Withering
        {
            // mdrcharAddAge True, pv04F2, mc0582, False (MONSMOD.BAS:3633; MAINMOD2.BAS:6400-6405)
            cs::String older = opener->AgeBy(WitheringDays, party.Count() > 1);
            if (older != "") r->Lines.Add(older);
            break;
        }
    }

    for (auto& c : party) if (c->CurHits < 1) c->CurHits = 0;
    return r;
}

}  // namespace mc
