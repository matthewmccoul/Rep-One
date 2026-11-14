#include "core.h"

namespace mc {

namespace {
// C# implicit operator VbCurrency(long whole): checked whole * Scale
VbCurrency WholeCurrency(int64_t whole) {
    if (whole > INT64_MAX / VbCurrency::Scale || whole < INT64_MIN / VbCurrency::Scale)
        throw cs::OverflowException("Arithmetic operation resulted in an overflow.");
    return VbCurrency::FromRaw(whole * VbCurrency::Scale);
}
}  // namespace

Party::Party(cs::List<cs::Ref<Character>> members) {
    for (auto& m : members) Add(m);
}

cs::Ref<Character> Party::Selected() { return Members[cs::Math::Clamp<int32_t>(SelectedIndex, 0, Members.Count() - 1)]; }
cs::Ref<Character> Party::Leader() { return Members[0]; }
int32_t Party::Size() { return Members.Count(); }

cs::List<cs::Ref<Character>> Party::Able() {
    return Members.Where([&](auto& c) { return !c->IsDead && !c->Stoned && c->CurHits > 0; });
}

bool Party::AnyAble() { return Able().Any(); }

cs::List<Tup_owner_companion_66faa> Party::Targets() {
    cs::List<Tup_owner_companion_66faa> _result = cs::List<Tup_owner_companion_66faa>::New();
    for (auto& c : Able()) {
        _result.Add(Tup_owner_companion_66faa{c, nullptr});
        for (auto& comp : c->Companions->All()) _result.Add(Tup_owner_companion_66faa{c, comp});
    }
    return _result;
}

cs::String Party::WhyCannotJoin(cs::Ref<Character> c) {
    if (Members.Count() == 0) return "";      // the first member is the leader, and always joins

    // "Your party is too full!" is fn1521's (MAINMOD2.BAS:3447); frmInteract (FRMINTER.BAS:192-245) words the rest
    if (Members.Count() >= MaxSize) return "Your party is too full!";
    if (Members.Any([&](auto& m) { return m->Name == c->Name; })) return Interact::AlreadyInParty;

    if (c->IsDead) return Interact::IsDead;

    if (!SamePlace(Leader(), c)) return Interact::NotHere;

    for (auto& m : Members)
        if (Opposed(m->Alignment, c->Alignment))
            return "\"" + cs::Str::TrimEnd(c->Name) + "\" is oppositely aligned to one or more members of the party!";
    return "";
}

cs::List<cs::Ref<Character>> Party::HereToJoin(cs::List<cs::Ref<Character>> everyone, int32_t level, int32_t area, int32_t floors,
                                               cs::Func<int32_t(cs::Ref<Character>)> savedArea) {
    return everyone.Where([&](auto& c) {
                       return Members.All([&](auto& m) { return m->Name != c->Name; }) && c->DungeonLevel == level
                              && c->DungeonLevel >= 1 && c->DungeonLevel <= floors
                              && savedArea(c) == area;
                   })
        .ToList();
}

bool Party::SamePlace(cs::Ref<Character> a, cs::Ref<Character> b) {
    return (a->DungeonLevel == 0 && b->DungeonLevel == 0)
           || (a->DungeonLevel == b->DungeonLevel && a->X == b->X && a->Y == b->Y);
}

void Party::ShareLostFlags(cs::Ref<Character> first, cs::Ref<Character> newcomer) {
    newcomer->LostLevel = first->LostLevel;
    newcomer->LostPosition = first->LostPosition;
    newcomer->LostFacing = first->LostFacing;
}

bool Party::Opposed(Alignment a, Alignment b) {
    return (a == Alignment::Good && b == Alignment::Evil) || (a == Alignment::Evil && b == Alignment::Good);
}

cs::String Party::PoolCompleted(cs::Ref<Character> to) {
    return "Pool of party's on-hand gold to \"" + cs::Str::TrimEnd(to->Name) + "\" completed!";
}

VbCurrency Party::PoolGold() {
    auto to = Selected();
    VbCurrency moved = VbCurrency::Zero;
    try {
        for (auto& m : Members) {
            if (m == to) continue;
            auto took = m->GoldOnHand;
            to->GoldOnHand = to->GoldOnHand + took;      // MAINMOD2.BAS:3644
            m->GoldOnHand = VbCurrency::Zero;            // 3645
            moved = VbCurrency::AddSaturating(moved, took);
        }
    } catch (const cs::OverflowException&) {
    }   // fn1521's trap (4169-4171)
    return moved;
}

bool Party::AllOrNothing(cs::List<cs::Ref<Character>> who, cs::Func<void()> move) {
    auto before = who.Distinct().Select([&](auto& c) { return std::make_tuple(c, c->GoldOnHand, c->GoldInBank); }).ToList();
    try {
        move();
        return true;
    } catch (const cs::OverflowException&) {
        for (auto& t : before) {
            auto& c = std::get<0>(t);
            c->GoldOnHand = std::get<1>(t);
            c->GoldInBank = std::get<2>(t);
        }
        return false;
    }
}

bool Party::PoolToBank(cs::List<cs::Ref<Character>> members, cs::Ref<Character> to) {
    auto all = members.ToList();
    return AllOrNothing(all.Append(to), [&]() {
        for (auto& m : all) {
            to->GoldInBank = to->GoldInBank + m->GoldOnHand;
            m->GoldOnHand = VbCurrency::Zero;
        }
    });
}

VbCurrency Party::PoolOnHandToBank(cs::List<cs::Ref<Character>> members, cs::Ref<Character> to) {
    auto all = members.ToList();
    VbCurrency moved = VbCurrency::Zero;
    for (auto& m : all) moved = VbCurrency::AddSaturating(moved, m->GoldOnHand);   // the port's own total, for the mod's line
    return PoolToBank(all, to) ? moved : VbCurrency::Zero;
}

bool Party::PoolAndDeposit(cs::List<cs::Ref<Character>> members, cs::Ref<Character> to) {
    auto all = members.ToList();
    return AllOrNothing(all.Append(to), [&]() {
        for (auto& m : all) {
            if (m == to) continue;
            // two statements there (54-55), so the two balances are never summed first
            to->GoldInBank = to->GoldInBank + m->GoldInBank;
            to->GoldInBank = to->GoldInBank + m->GoldOnHand;
            m->GoldInBank = VbCurrency::Zero;
            m->GoldOnHand = VbCurrency::Zero;
        }
        to->GoldInBank = to->GoldInBank + to->GoldOnHand;
        to->GoldOnHand = VbCurrency::Zero;
    });
}

VbCurrency Party::PartyDeposit(cs::List<cs::Ref<Character>> members) {
    auto all = members.ToList();
    VbCurrency moved = VbCurrency::Zero;
    bool done = AllOrNothing(all, [&]() {
        for (auto& m : all) {
            moved = VbCurrency::AddSaturating(moved, m->GoldOnHand);   // the port's own total, for the mod's line
            m->GoldInBank = m->GoldInBank + m->GoldOnHand;
            m->GoldOnHand = VbCurrency::Zero;
        }
    });
    return done ? moved : VbCurrency::Zero;
}

Party_BankAnswer Party::BankTyped(cs::Ref<Character> ch, cs::String text, bool deposit) {
    VbCurrency amount{};
    if (!VbCurrency::TryParse(text, amount) || amount == VbCurrency::Zero) return Party_BankAnswer::Ignored;
    if (deposit) {
        if (amount > ch->GoldOnHand || amount <= VbCurrency::Zero) return Party_BankAnswer::Beep;
        VbCurrency bank{};
        if (VbCurrency::TryAdd(ch->GoldInBank, amount, bank)) ch->GoldInBank = bank;          // 315
        VbCurrency hand{};
        if (VbCurrency::TrySubtract(ch->GoldOnHand, amount, hand)) ch->GoldOnHand = hand;     // 316
    } else {
        if (amount > ch->GoldInBank || ch->GoldInBank < VbCurrency::Zero) return Party_BankAnswer::Beep;
        VbCurrency hand{};
        if (VbCurrency::TryAdd(ch->GoldOnHand, amount, hand)) ch->GoldOnHand = hand;          // 323
        VbCurrency bank{};
        if (VbCurrency::TrySubtract(ch->GoldInBank, amount, bank)) ch->GoldInBank = bank;     // 324
    }
    return Party_BankAnswer::Moved;
}

cs::String Party::BankAllText(VbCurrency balance) { return cs::Str::TrimStart(balance.Str(), ' '); }

cs::List<cs::Ref<Party_ShortfallGift>> Party::PlanShortfall(cs::List<cs::Ref<Character>> members, cs::Ref<Character> payer, int64_t cost,
                                                            cs::List<cs::Ref<Character>> from) {
    VbCurrency need = VbCurrency::SubtractSaturating(WholeCurrency(cost), payer->Purse());
    if (need <= VbCurrency::Zero) return cs::List<cs::Ref<Party_ShortfallGift>>::New();
    if (payer->DungeonLevel != 0 || !members.Contains(payer)) return nullptr;
    auto givers = (from ? from : members).Distinct()
                      .Where([&](auto& m) { return m != payer && members.Contains(m) && m->DungeonLevel == 0 && !m->IsDead; })
                      .ToList();
    if (givers.Count() == 0) return nullptr;
    auto gifts = cs::List<cs::Ref<Party_ShortfallGift>>::New();
    for (auto& g : givers) {
        if (need <= VbCurrency::Zero) break;
        VbCurrency part = VbCurrency::Min(VbCurrency::Max(VbCurrency::Zero, g->GoldInBank), need);
        if (part <= VbCurrency::Zero) continue;
        gifts.Add([&] { auto _o = cs::New<Party_ShortfallGift>(); _o->Giver = g; _o->Amount = part; return _o; }());
        need = need - part;
    }
    return need > VbCurrency::Zero ? nullptr : gifts;
}

cs::List<cs::Ref<Party_ShortfallGift>> Party::CoverShortfall(cs::List<cs::Ref<Character>> members, cs::Ref<Character> payer, int64_t cost,
                                                             cs::List<cs::Ref<Character>> from) {
    auto gifts = PlanShortfall(members, payer, cost, from);
    if (gifts == nullptr) return nullptr;
    bool done = AllOrNothing(gifts.Select([&](auto& g) { return g->Giver; }).Append(payer), [&]() {
        for (auto& g : gifts) {
            g->Giver->GoldInBank = g->Giver->GoldInBank - g->Amount;
            payer->GoldOnHand = payer->GoldOnHand + g->Amount;
        }
    });
    return done ? gifts : nullptr;
}

VbCurrency Party::GiveGold(cs::Ref<Character> from, cs::Ref<Character> to, VbCurrency amount) {
    VbCurrency moved = VbCurrency::Min(VbCurrency::Max(amount, VbCurrency::Zero), from->GoldOnHand);
    VbCurrency got{};
    VbCurrency left{};
    if (!VbCurrency::TryAdd(to->GoldOnHand, moved, got) || !VbCurrency::TrySubtract(from->GoldOnHand, moved, left))
        return VbCurrency::Zero;
    from->GoldOnHand = left;
    to->GoldOnHand = got;
    return moved;
}

cs::String Party::GaveGold(int64_t amount, cs::Ref<Character> to) {
    return "Give of " + cs::Format(amount, "N0") + " gold to \"" + cs::Str::TrimEnd(to->Name) + "\" Completed!";
}

cs::String Party::TransferGold(cs::Ref<Character> from, cs::Ref<Character> to, int64_t amount) {
    if (amount == 0) return "";
    VbCurrency purse{};
    if (!VbCurrency::TryAdd(from->GoldOnHand, from->GoldInBank, purse)) return "";   // 115, the trap
    VbCurrency available = from->DungeonLevel != 0 ? from->GoldOnHand : purse;
    if (WholeCurrency(amount) > available) return "Too Much!";

    try {
        to->GoldOnHand = to->GoldOnHand + WholeCurrency(amount);            // 119
        from->GoldOnHand = from->GoldOnHand - WholeCurrency(amount);        // 120
        if (from->GoldOnHand < VbCurrency::Zero) {                          // the shortfall comes out of the bank
            from->GoldInBank = from->GoldInBank + from->GoldOnHand;
            from->GoldOnHand = VbCurrency::Zero;
        }
    } catch (const cs::OverflowException&) {
    }        // the trap: Resume to the end, silently
    return "";
}

cs::String Party::SwapItems(cs::Ref<Character> from, int32_t slotFrom, cs::Ref<Character> to, int32_t slotTo, cs::Func<cs::Ref<Item>(int16_t)> item) {
    // refuses when either end is slot 0, the character's Hands (FRMEXCHA.BAS:177)
    if (slotFrom <= 0 || slotFrom >= Character::PackSize) return "";   // refused in silence
    if (slotTo <= 0 || slotTo >= Character::PackSize) return "";
    if (from == to && slotFrom == slotTo) return "";

    auto source = from->Pack()[slotFrom];
    auto destination = to->Pack()[slotTo];
    cs::Ref<Item> itSource = source == nullptr ? nullptr : item(source->ItemId);
    cs::Ref<Item> itDestination = destination == nullptr ? nullptr : item(destination->ItemId);

    if (itDestination != nullptr && !ChestTraps::Destructible(itDestination)) return "Destination Item is a Guild-Issued Item!";
    if (itSource != nullptr && !ChestTraps::Destructible(itSource)) return "Source Item is a Guild-Issued Item!";
    if (from != to) {
        if (destination && destination->Equipped == true) return "Destination Item is either Equipped or in the Bank!";
        if (source && source->Equipped == true) return "Source Item is either Equipped or in the Bank!";
    }

    from->PutInSlot(slotFrom, destination);
    to->PutInSlot(slotTo, source);

    if (destination == nullptr) SpellBuffers::PackSlotEmptied(from, slotFrom);
    if (source == nullptr) SpellBuffers::PackSlotEmptied(to, slotTo);

    if (source != nullptr && itSource != nullptr) Loot::OnReceived(to, source, itSource, item);
    return "";
}

cs::String Party::NoFreeSlots(cs::Ref<Character> to) { return "\"" + cs::Str::TrimEnd(to->Name) + "\" has no free slots!"; }

cs::String Party::GiveItem(cs::Ref<Character> from, cs::Ref<Character> to, cs::Ref<InventorySlot> slot, cs::Ref<Item> it,
                           cs::Func<cs::Ref<Item>(int16_t)> lookup, cs::Func<void(cs::String)> identified,
                           cs::Func<void(int32_t, IdLevel)> filed) {
    if (it != nullptr && !ChestTraps::Destructible(it)) return "Cannot give Guild-Issued Items!";
    if (slot->Equipped) return "Cannot give equipped items!";
    if (to->FirstFreeSlot() < 0) return NoFreeSlots(to);

    from->RemoveFromPack(slot);
    to->PutInPack(slot);
    if (it != nullptr) ReadGiven(to, slot, it, lookup, identified, filed);
    return "";
}

cs::String Party::GaveItem(cs::Ref<Character> to) { return "Give of Item to \"" + cs::Str::TrimEnd(to->Name) + "\" Completed!"; }

cs::String Party::NoFreeSlotsFromVault(cs::Ref<Character> to) { return to->Name + " has no free slots!"; }

cs::String Party::GiveFromVault(cs::Ref<Character> from, int32_t bankSlot, cs::Ref<Character> to,
                                cs::Func<cs::Ref<Item>(int16_t)> lookup, cs::Func<void(cs::String)> identified,
                                cs::Func<void(int32_t, IdLevel)> filed) {
    if (bankSlot < 0 || bankSlot >= from->Bank.Count() || from->Bank[bankSlot]->ItemId <= 0) return "";
    auto slot = from->Bank[bankSlot];
    cs::Ref<Item> it = lookup ? lookup(slot->ItemId) : nullptr;
    // isDestructible(M5860(p049C).Ref) = False (MAINMOD2.BAS:2974)
    if (it != nullptr && !ChestTraps::Destructible(it)) return "Cannot give Guild-Issued Items!";
    // For l04A4 = 1 To mc0190 - 1 (MAINMOD2.BAS:2978)
    int32_t free = to->FirstFreeSlot();
    if (free < 0) return NoFreeSlotsFromVault(to);
    to->PutInSlot(free, slot);                    // Pack(l04A0) = M5860(p049C) (MAINMOD2.BAS:2988)
    if (it != nullptr) ReadGiven(to, slot, it, lookup, identified, filed);
    from->Bank[bankSlot] = cs::New<InventorySlot>();   // M5860(p049C).Ref = False (MAINMOD2.BAS:2997)
    return "";
}

void Party::ReadGiven(cs::Ref<Character> to, cs::Ref<InventorySlot> slot, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> lookup,
                      cs::Func<void(cs::String)> identified, cs::Func<void(int32_t, IdLevel)> filed) {
    if (slot->Known() == IdLevel::All) return;
    auto can = Loot::ReadableLevel(to, it, lookup);
    if (can <= slot->Known()) return;
    slot->set_Known(can);
    if (identified)
        identified("\"" + cs::Str::TrimEnd(to->Name) + "\" identified the given item as: "
                   + Loot::NameFor(it, slot->Known(), slot->Alignment, slot->Charges));
    if (filed) filed(it->Id, slot->Known());
}

bool Party::Add(cs::Ref<Character> c) {
    if (!WhyCannotJoin(c).empty()) return false;
    Members.Add(c);
    return true;
}

bool Party::Remove(cs::Ref<Character> c) {
    int32_t i = Members.IndexOf(c);
    if (i < 0 || Members.Count() == 1) return false;
    Members.RemoveAt(i);
    SelectedIndex = cs::Math::Clamp<int32_t>(SelectedIndex, 0, Members.Count() - 1);
    return true;
}

cs::Ref<Character> Party::LeaderAfterRemoving(cs::Ref<Character> c) {
    return Members.Count() > 1 && Members.IndexOf(c) == 0 ? Members[1] : nullptr;
}

cs::String Party::NowLeader(cs::Ref<Character> c) { return "\"" + c->Name + "\" is now party leader!"; }

cs::List<cs::String> Party::RemoveTheDead(cs::Func<void(cs::Ref<Character>)> save, cs::Func<void(cs::Ref<Character>)> setDead) {
    auto said = cs::List<cs::String>::New();
    if (Members.Count() <= 1) return said;

    int32_t leaderLevel = Members[0]->DungeonLevel;
    for (int32_t i = 0; i < Members.Count() && Members.Count() > 1;) {
        auto c = Members[i];
        if (c->CurHits > 0 && !c->IsDead && !c->Stoned) { i++; continue; }

        c->CurHits = 0;
        if (c->DungeonLevel == leaderLevel) said.Add("\"" + cs::Str::TrimEnd(c->Name) + "\" has died!");
        // "If DeathState = False Then mdrcharSetDead" (FRMGROUP.BAS:333-335)
        if (!c->IsDead) { if (setDead) setDead(c); }
        if (save) save(c);
        Members.RemoveAt(i);
        if (i < SelectedIndex) SelectedIndex--;
        // sub0956 (FRMGROUP.BAS:342) puts everyone left on the leader's floor and square after each removal
        for (auto& m : Members) { m->DungeonLevel = Members[0]->DungeonLevel; m->X = Members[0]->X; m->Y = Members[0]->Y; }
        // the loop index does not advance, as the original's l0118 = l0118 - 1
    }
    SelectedIndex = cs::Math::Clamp<int32_t>(SelectedIndex, 0, Members.Count() - 1);
    return said;
}

bool Party::Select(int32_t index) {
    if (index < 0 || index >= Members.Count()) return false;
    SelectedIndex = index;
    return true;
}

bool Party::SelectNextAble() {
    for (int32_t step = 1; step <= Members.Count(); step++) {
        int32_t i = (SelectedIndex + step) % Members.Count();
        auto c = Members[i];
        if (!c->IsDead && !c->Stoned && c->CurHits > 0) { SelectedIndex = i; return true; }
    }
    return false;
}

cs::String Party::Swap(int32_t a, int32_t b) {
    if (a < 0 || a >= Members.Count() || b < 0 || b >= Members.Count() || a == b) return "";
    auto ta = Members[b];
    auto tb = Members[a];
    Members[a] = ta;
    Members[b] = tb;
    if (SelectedIndex == a) SelectedIndex = b;
    else if (SelectedIndex == b) SelectedIndex = a;
    // FRMGROUP.BAS:238 - announced only when one end of the swap was the leader's place
    return a == 0 || b == 0 ? NowLeader(Members[0]) : cs::String();
}

void Party::MoveTo(int32_t from, int32_t to) { Swap(from, to); }

cs::Ref<CombatOrder> Party::OrderFor(cs::Ref<Character> c) {
    cs::Ref<CombatOrder> o{};
    if (!Orders.TryGetValue(c, o)) { o = cs::New<CombatOrder>(); Orders[c] = o; }
    return o;
}

void Party::ClearOrders() { for (auto& o : Orders.Values()) o->Clear(); }

cs::List<cs::Ref<Character>> Party::ApplyDefaultOrders(cs::List<cs::Ref<Spell>> spells) {
    auto took = cs::List<cs::Ref<Character>>::New();
    for (auto& c : Members) {
        auto o = OrderFor(c);
        if (o->Kind != OrderKind::None) continue;
        int32_t aimed = o->Target;
        o->ApplyDefault(c, spells);
        o->Target = aimed;
        took.Add(c);
    }
    return took;
}

void Party::RestInCity() { for (auto& c : Members) c->RestInCity(); }

void Party::PlaceAt(int32_t level, int32_t x, int32_t y) {
    for (auto& c : Members) { c->DungeonLevel = level; c->X = x; c->Y = y; }
}

void Party::SaveAll(cs::String savesDir) {
    cs::Directory::CreateDirectory(savesDir);
    for (auto& c : Members)
        try { c->Save(cs::Path::Combine(savesDir, Character::SafeFileName(c->Name) + ".json")); } catch (...) {}
}

cs::String Party::RosterPath(cs::String savesDir, cs::String leader) {
    return cs::Path::Combine(savesDir, "party-" + Character::SafeFileName(leader) + ".json");
}

cs::String Party::ReloadQuestion() {
    return "Do you wish to Reload this Party when \"" + cs::Str::TrimEnd(Leader()->Name) + "\" is loaded again?";
}

bool Party::SaveParty(cs::String savesDir, bool remember, cs::Func<void(cs::String)> errorLog) {
    try {
        cs::String p = RosterPath(savesDir, Leader()->Name);
        if (!remember) {
            if (cs::File::Exists(p)) cs::File::Delete(p);
            return true;
        }
        cs::Directory::CreateDirectory(savesDir);
        auto r = [&] { auto _o = cs::New<Party_Roster>(); _o->Leader = Leader()->Name; _o->Members = Members.Select([&](auto& m) { return m->Name; }).ToList(); return _o; }();
        cs::File::WriteAllText(p, cs::Json::Serialize(r, cs::Json::Indented()));
        return true;
    } catch (const std::exception& ex) {
        if (errorLog) errorLog("Error '" + cs::String(ex.what()) + "' in MULTIMOD:mdrmulSaveParty");
        return false;
    }
}

cs::Ref<Party> Party::LoadFor(cs::Ref<Character> leader, cs::String savesDir, cs::Func<void(cs::String)> say,
                              cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> monsters, cs::Func<int32_t(cs::Ref<Character>)> areaOf) {
    auto party = cs::New<Party>(cs::List<cs::Ref<Character>>::Of({leader}));
    auto SameArea = [&](cs::Ref<Character> a, cs::Ref<Character> b) -> bool {
        // every record saved in town holds M59BB = -1 (FRMCTYM.BAS:112, 132), so level 0 reads as -1
        if (a->DungeonLevel <= 0 && b->DungeonLevel <= 0) return true;
        if (areaOf) return areaOf(a) == areaOf(b);
        if (a->Area.has_value() && b->Area.has_value()) return a->Area.value() == b->Area.value();
        return a->X == b->X && a->Y == b->Y;   // a save from before M59BB was kept, and no maze to hand
    };
    auto Said = [&](cs::String line) { if (say) say(AutoloadPrefix + line); };
    // a dead leader brings nobody back (MULTIMOD.BAS:219)
    if (leader->IsDead) return party;
    try {
        cs::String p = RosterPath(savesDir, leader->Name);
        if (!cs::File::Exists(p)) return party;
        auto r = cs::Json::Deserialize<cs::Ref<Party_Roster>>(cs::File::ReadAllText(p));
        if (r == nullptr) return party;
        for (auto& name : r->Members) {
            if (name == leader->Name) continue;
            cs::String f = cs::Path::Combine(savesDir, Character::SafeFileName(name) + ".json");
            cs::Ref<Character> them;
            try { them = Character::Load(f); }
            catch (...) {
                if (say) say("An error occurred auto-loading \"" + cs::Str::TrimEnd(name) + "\".  Has the character been deleted?");
                continue;
            }

            cs::String q = "\"" + cs::Str::TrimEnd(them->Name) + "\"";
            if (!SameArea(them, leader) || them->DungeonLevel != leader->DungeonLevel)
            { Said(q + " is not here anymore!"); continue; }
            if (them->IsDead)
            { Said(q + " is dead!  Can't join!"); continue; }
            if (party->Members.Any([&](auto& m) { return cs::Math::Abs(static_cast<int32_t>(m->Alignment) - static_cast<int32_t>(them->Alignment)) > 1; }))
            { Said(q + " is oppositely aligned to one or more members of the party!"); continue; }

            if (party->Add(them)) {
                Said(q + " is now a member of your party.");
                // then sub154A (MULTIMOD.BAS:266), whose mdrcharGetCarry (MULTIMOD.BAS:380) settles fallen companions
                if (monsters)
                    for (cs::String line : CompanionCarry::Run(them, false, monsters)) { if (say) say(line); }
            }
        }
    } catch (...) {
    }
    return party;
}

}  // namespace mc
