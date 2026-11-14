#include "core.h"

namespace mc {

// ---------------------------------------------------------------- InventorySlot

IdLevel InventorySlot::Known() { return _known; }
void InventorySlot::set_Known(IdLevel value) { _known = value; _knownRead = true; }

bool InventorySlot::Identified() { return Known() == IdLevel::All; }
void InventorySlot::set_Identified(bool value) { set_Known(value ? IdLevel::All : IdLevel::Barely); }

cs::Nullable<bool> InventorySlot::LegacyIdentified() { return {}; }
void InventorySlot::set_LegacyIdentified(cs::Nullable<bool> value) {
    if (value.has_value() && !_knownRead) _known = value.value() ? IdLevel::All : IdLevel::Barely;
}

// ---------------------------------------------------------------- Companion

bool Companion::IsDead() { return Hits <= 0; }

// ---------------------------------------------------------------- Binding

int32_t Binding::NextLevel(int32_t current) {
    return cs::Math::Abs(current) + Step > Max ? Max : cs::Math::Abs(current) + Step;
}

cs::Decimal Binding::Cost(int32_t monsterGuildLevel, int32_t current) {
    int32_t a = cs::Math::Abs(current);
    int32_t f = a + Step > Max ? Max - a : a + Step;
    return cs::Decimal(monsterGuildLevel) * cs::Dec("1237") * cs::Decimal(f) * cs::Decimal(current) / cs::Dec("4");
}

int64_t Binding::Charge(cs::Decimal cost) { return cs::Decimal::Round(cost, 0, cs::MidpointRounding::ToEven).ToInt64(); }

int32_t Binding::Initial(int32_t monsterFloor) { return cs::Math::Min(5 + monsterFloor, 10); }

cs::String Binding::Describe(int32_t level, double chanceToLeave) {
    if (level == ForLife) return "Bound for Life";
    cs::String strength;
    if (chanceToLeave == 0) strength = "Completely";
    else if (chanceToLeave <= 3) strength = "Excellently";
    else if (chanceToLeave <= 10) strength = "Fairly";
    else if (chanceToLeave <= 15) strength = "Marginally";
    else if (chanceToLeave <= 20) strength = "Weakly";
    else strength = "Very Weakly";
    return level < 0 ? strength + " content" : strength + " bound";
}

double Binding::ChanceToLeave(int32_t bindingLevel, int32_t monsterGuildLevel, int32_t topGuildLevel,
                              int32_t charisma, int32_t hits, int32_t maxHits) {
    if (bindingLevel == ForLife) return 0;
    double bound = cs::Math::Round(cs::Math::Abs(bindingLevel) * 4.5, cs::MidpointRounding::ToEven);   // l0376 is an Integer (MONSMOD.BAS:2058)
    double mon = cs::Math::Log(monsterGuildLevel + 1) * 6;
    double x = mon * 4 - (bound * 2 + cs::Math::Log(topGuildLevel / 10.0 + 1) * 4);
    x -= charisma / 2.0;
    x = x / 200 * 100;
    x += (maxHits - hits) / (double)maxHits * 15;
    x /= 2;
    return cs::Math::Clamp(x, 0.0, 90.0);
}

// ---------------------------------------------------------------- Character

Character::Character() {
    SpellResist = cs::List<int32_t>::New(Effects::ResistanceKinds);
    Buffers = cs::Enumerable::Repeat(SpellBuffers::Empty, SpellBuffers::Count).ToArray();
    Options = cs::New<CharacterOptions>();
    Windows = WindowLayout::Fresh();
    Companions = cs::New<CompanionSlots>();
}

bool Character::Paralysed() { return ParalysisLevel != 0; }

int32_t Character::StatsADNow(cs::Func<cs::Ref<Item>(int16_t)> item) {
    return Rules::StatsAD(cs::Enumerable::Range(0, 6).Select([&](auto& i) { return EffectiveStat(i, item); }).ToArray());
}

int32_t Character::StoredADShift(cs::Func<cs::Ref<Item>(int16_t)> item) {
    if (ADShiftUnsettled) { ADShiftUnsettled = false; ADShift = -StatsADNow(item); }
    return ADShift;
}

void Character::Paralyse(int32_t level, cs::Func<cs::Ref<Item>(int16_t)> item) {
    if (ParalysisLevel != 0) return;   // "If p05B0 = 0 And Ailments(mc0122) <> False Then Exit Sub"
    ADShift = StoredADShift(item) - StatsADNow(item);
    ParalysisLevel = level;
}

void Character::Unparalyse(cs::Func<cs::Ref<Item>(int16_t)> item) {
    if (ParalysisLevel == 0) return;   // "If p05B0 = 1 And Ailments(mc0122) = False Then Exit Sub"
    ADShift = StoredADShift(item) + StatsADNow(item);
    ParalysisLevel = 0;
}

void Character::CurePoison() { Poisoned = false; PoisonLevel = 0; }

void Character::CureDisease() { Diseased = false; DiseaseLevel = 0; }

void Character::EndAilmentsAndSpellResists() {
    CurePoison();
    CureDisease();
    for (int32_t i = 0; i < SpellResist.Length(); i++) SpellResist[i] = 0;
}

void Character::EndEveryAilment(cs::Func<cs::Ref<Item>(int16_t)> item) {
    Unparalyse(item);
    EndAilmentsAndSpellResists();
    Effects::Recompute(shared_from_this(), item);
}

void Character::Sub0930Head(cs::Func<cs::Ref<Item>(int16_t)> item) {
    ReleaseFollowers();
    EndEveryAilment(item);
}

cs::String Character::AilmentText() {
    return cs::Str::Join(" ", cs::List<cs::String>::Of({Poisoned ? cs::String("POISONED") : cs::String(""), Diseased ? cs::String("DISEASED") : cs::String(""), Stoned ? cs::String("STONE") : cs::String("")})
                                  .Where([&](auto& s) { return s != ""; }));
}

void Character::ReleaseFollowers() {
    Companions->ReleaseAll();
    CarryingCompanion = -1;
    CarryingCharacter = "";
}

bool Character::IsCarrying() { return CarryingCompanion >= 0 || CarryingCharacter != ""; }

cs::String Character::CompanionName(int32_t index) {
    // fn128B reads the record's name without asking whether the slot is occupied (MAINMOD2.BAS:4745)
    return index < 0 || index >= CompanionSlots::Slots ? cs::String("")
        : cs::Str::Trim(Companions->Left(index)->Name) == "" ? "Companion #" + cs::ToStr(index + 1)
                                                            : "\"" + cs::Str::Trim(Companions->Left(index)->Name) + "\"";
}

cs::String Character::CarryText() {
    return CarryingCharacter != "" ? "Carrying \"" + CarryingCharacter + "\"."
        : CarryingCompanion >= 0 && CarryingCompanion < CompanionSlots::Slots
            ? "Carrying " + CompanionName(CarryingCompanion) + "."
            : cs::String("");
}

bool Character::IsLost() { return LostPosition || LostLevel; }

void Character::BecomeLost() { LostLevel = LostPosition = LostFacing = true; }

void Character::FoundYourself() { LostLevel = LostPosition = LostFacing = false; }

void Character::RecalcSpellPoints(bool refill) {
    MaxSpellPoints = Spellbook::MaxSpellPoints(shared_from_this());
    if (refill) CurSpellPoints = MaxSpellPoints;
    else CurSpellPoints = cs::Math::Min(CurSpellPoints, MaxSpellPoints);
}

int64_t Character::GoldOnHandWhole() { return GoldOnHand.Whole(); }
void Character::set_GoldOnHandWhole(int64_t value) { GoldOnHand = VbCurrency::Saturate(value); }
cs::Nullable<cs::Decimal> Character::GoldOnHandExact() { return GoldOnHand.ExactForSave(); }
void Character::set_GoldOnHandExact(cs::Nullable<cs::Decimal> value) { _handExact = value; }
int64_t Character::GoldInBankWhole() { return GoldInBank.Whole(); }
void Character::set_GoldInBankWhole(int64_t value) { GoldInBank = VbCurrency::Saturate(value); }
cs::Nullable<cs::Decimal> Character::GoldInBankExact() { return GoldInBank.ExactForSave(); }
void Character::set_GoldInBankExact(cs::Nullable<cs::Decimal> value) { _bankExact = value; }

void Character::OnDeserialized() {
    // with no exact figure the whole part (already held to the range) stands as read
    if (_handExact.has_value()) GoldOnHand = VbCurrency::FromSave(GoldOnHand.Whole(), _handExact);
    if (_bankExact.has_value()) GoldInBank = VbCurrency::FromSave(GoldInBank.Whole(), _bankExact);
    _handExact = _bankExact = cs::Nullable<cs::Decimal>{};
}

VbCurrency Character::Purse() { return VbCurrency::AddSaturating(GoldOnHand, GoldInBank); }

bool Character::Pay(int64_t cost) {
    auto c = VbCurrency::Saturate(cost);
    if (c > GoldOnHand) {
        if (c > Purse()) return false;
        VbCurrency owed = VbCurrency::Zero;
        if (!VbCurrency::TrySubtract(c, GoldOnHand, owed)) return false;
        int64_t rest = owed.RoundToWhole();   // 699, into the Long
        VbCurrency bank = VbCurrency::Zero;
        if (!VbCurrency::TrySubtract(GoldInBank, VbCurrency::Saturate(rest), bank)) return false;
        GoldOnHand = VbCurrency::Zero;
        GoldInBank = bank;
        return true;
    }
    VbCurrency hand = VbCurrency::Zero;
    if (!VbCurrency::TrySubtract(GoldOnHand, c, hand)) return false;
    GoldOnHand = hand;
    return true;
}

bool Character::Receive(VbCurrency amount, bool toBank) {
    VbCurrency sum = VbCurrency::Zero;
    if (!VbCurrency::TryAdd(toBank ? GoldInBank : GoldOnHand, amount, sum)) return false;
    if (toBank) GoldInBank = sum; else GoldOnHand = sum;
    return true;
}

bool Character::CanReceive(VbCurrency amount, bool toBank) {
    VbCurrency _discard = VbCurrency::Zero;
    return VbCurrency::TryAdd(toBank ? GoldInBank : GoldOnHand, amount, _discard);
}

cs::List<cs::Ref<InventorySlot>> Character::Pack() { return _pack; }
void Character::set_Pack(cs::List<cs::Ref<InventorySlot>> value) { _pack = Resize(value); }

cs::List<cs::Ref<InventorySlot>> Character::FreshPack() {
    auto p = cs::List<cs::Ref<InventorySlot>>::New(PackSize);
    p[0] = [&] { auto _o = cs::New<InventorySlot>(); _o->ItemId = HandsItemId; _o->set_Known(IdLevel::All); return _o; }();
    return p;
}

cs::List<cs::Ref<InventorySlot>> Character::Resize(cs::List<cs::Ref<InventorySlot>> given) {
    auto p = cs::List<cs::Ref<InventorySlot>>::New(PackSize);
    if (given != nullptr)
        for (int32_t i = 0; i < PackSize && i < given.Length(); i++) p[i] = given[i];
    if (!p[0]) p[0] = [&] { auto _o = cs::New<InventorySlot>(); _o->ItemId = HandsItemId; _o->set_Known(IdLevel::All); return _o; }();
    return p;
}

bool Character::SlotEmpty(int32_t i) { return i > 0 && i < PackSize && (_pack[i] == nullptr || _pack[i]->ItemId == 0); }

int32_t Character::FirstFreeSlot() {
    for (int32_t i = 1; i < PackSize; i++) if (SlotEmpty(i)) return i;
    return -1;
}

cs::List<cs::Ref<InventorySlot>> Character::Inventory() {
    auto live = cs::List<cs::Ref<InventorySlot>>::New();
    for (int32_t i = 1; i < PackSize; i++) if (!SlotEmpty(i)) live.Add(_pack[i]);
    return live;
}

int32_t Character::SlotOf(cs::Ref<InventorySlot> slot) {
    for (int32_t i = 0; i < PackSize; i++) if (_pack[i] == slot) return i;
    return -1;
}

bool Character::PutInPack(cs::Ref<InventorySlot> slot) {
    int32_t i = FirstFreeSlot();
    if (i < 0) return false;
    _pack[i] = slot;
    return true;
}

void Character::PutInSlot(int32_t i, cs::Ref<InventorySlot> slot) {
    if (i > 0 && i < PackSize) _pack[i] = slot;
}

void Character::ClearPack() {
    for (int32_t i = 1; i < PackSize; i++) _pack[i] = nullptr;
}

void Character::MigrateLegacyPack() {
    if (LegacyInventory == nullptr) return;
    if (LegacyInventory.Count() > 0 && Inventory().Count() == 0) {
        int32_t i = 1;
        for (auto& slot : LegacyInventory) {
            if (i >= PackSize) break;
            _pack[i++] = slot;
        }
    }
    LegacyInventory = nullptr;
}

bool Character::DepositToVault(int32_t packSlot) {
    if (packSlot <= 0 || packSlot >= PackSize) return false;
    int32_t hole = Bank.FindIndex([&](auto& b) { return b->ItemId <= 0; });
    if (hole < 0 && Bank.Count() >= BankSlots) return false;
    auto sl = Pack()[packSlot];
    if (sl == nullptr || sl->ItemId == 0) return true;
    if (hole >= 0) Bank[hole] = sl; else Bank.Add(sl);
    PutInSlot(packSlot, nullptr);
    return true;
}

cs::Ref<WindowRect> Character::WindowFor(GameWindow which) { return NormaliseWindows()[static_cast<int32_t>(which)]; }

cs::List<cs::Ref<WindowRect>> Character::NormaliseWindows() {
    auto w = Windows ? Windows : cs::List<cs::Ref<WindowRect>>::New();
    if (w.Length() < WindowLayout::Count) {
        auto grown = WindowLayout::Fresh();
        for (int32_t j = 0; j < w.Length(); j++) grown[j] = w[j] ? w[j] : cs::New<WindowRect>();
        w = grown;
    }
    for (int32_t j = 0; j < w.Length(); j++) if (!w[j]) w[j] = cs::New<WindowRect>();
    Windows = w;
    return Windows;
}

bool Character::HasPassword() { return (int32_t)cs::Str::TrimEnd(Password).size() > 0; }

bool Character::PasswordMatches(cs::String entered) { return entered == cs::Str::TrimEnd(Password); }

int32_t Character::MaxCompanions() { return MaxCompanionSlots; }

bool Character::HasSanctuary() { return SanctuaryX != 0 && SanctuaryY != 0; }

int32_t Character::AgeYears() { return Age / 365; }   // the original divides by 365 to display it

cs::String Character::GuildTitle() { return GuildTitles::Of(Guild()->Guild, Guild()->Level); }

cs::String Character::Title() { return GuildTitle() + " (" + cs::ToStr(Guild()->Level) + ")"; }

int32_t Character::RestInCity() {
    // FRMCTYM.BAS:111 - spells holding up end at the city gate, for everyone
    SpellEffects = 0;
    Area = -1;   // "Chars(gv1A84).M59BB = -1" (FRMCTYM.BAS:112, again at 132)
    // FRMCTYM.BAS:115 zeroes Ailments(2); ADShift keeps what sub0D1D took (MONSMOD.BAS:3897-3898)
    ParalysisLevel = 0;
    // FRMCTYM.BAS:113-114 and sub048C (MAINMOD2.BAS:4362-4371)
    EndAilmentsAndSpellResists();
    // mdrcharAddAge (MAINMOD2.BAS): l0896 is an Integer, so the Single rounds half to even
    int32_t days = MaxHits > 0 && CurHits < MaxHits
        ? cs::CastDouble<int32_t>(cs::Math::Round((100 - ((double)CurHits / MaxHits * 100)) / 3.25, cs::MidpointRounding::ToEven))
        : 0;
    Age += days;
    CurHits = MaxHits;
    RecalcSpellPoints(true);
    AilmentSteps = 0;
    // FRMCTYM.BAS:113 heals only companions still on their feet - a dead one stays dead
    for (auto& c : Companions->All()) if (c->Hits > 0) c->Hits = c->MaxHits;
    return days;
}

cs::String Character::DescribeDays(int32_t days) {
    int32_t weeks = days > 7 ? days / 7 : 0;
    int32_t rest = days - weeks * 7;
    if (weeks > 0 && rest > 0) return cs::ToStr(weeks) + " week(s) and " + cs::ToStr(rest) + " day(s)";
    if (weeks > 0) return cs::ToStr(weeks) + " week(s)";
    return cs::ToStr(rest) + " day(s)";
}

cs::String Character::AgeBy(int32_t days, bool inAParty) {
    Age += days;
    if (days <= 0) return "";   // l0898 = 0 And l0896 = 0 (a negative l0896 is folded to 0 at 6373)
    return inAParty ? "\"" + cs::Str::TrimEnd(Name) + "\" is now " + DescribeDays(days) + " older."
                    : "You are now " + DescribeDays(days) + " older.";
}

void Character::NoteMonster(int32_t id, bool killed) {
    int32_t n = 0;
    MonstersSeen.TryGetValue(id, n);
    MonstersSeen[id] = n + (killed ? 1 : 0);
}

int32_t Character::TotalLevel() { return Guilds.Sum([&](auto& g) { return g->Level; }); }

int32_t Character::TopGuildLevel() { return Guilds.Count() == 0 ? 0 : Guilds.Max([&](auto& g) { return g->Level; }); }
cs::String Character::PrimaryGuild() { return Guilds.Count() > 0 ? Guilds[0]->Guild : cs::String("Nomad"); }
cs::Ref<GuildMembership> Character::Guild() { return Guilds[0]; }

int32_t Character::GetStat(Stat s) { return Stats[static_cast<int32_t>(s)]; }

int32_t Character::EffectiveStat(int32_t i, cs::Func<cs::Ref<Item>(int16_t)> item) {
    int32_t v = Stats[i];
    for (auto& s : Inventory().Where([&](auto& x) { return x->Equipped; })) {
        auto it = item(s->ItemId);
        if (it != nullptr && i < it->StatsMod.Length())
            v += ItemAlignments::StatModWorth(it->StatsMod[i], ItemAlignments::FitFor(s->Alignment, Alignment));
    }
    return cs::Math::Max(1, v);
}

bool Character::RemoveFromPack(cs::Ref<InventorySlot> slot) {
    int32_t i = SlotOf(slot);
    if (i <= 0) return false;
    _pack[i] = nullptr;
    SpellBuffers::PackSlotEmptied(shared_from_this(), i);
    return true;
}

bool Character::RemoveFromPackAt(int32_t index) {
    auto live = Inventory();
    if (index < 0 || index >= live.Count()) return false;
    return RemoveFromPack(live[index]);
}

bool Character::SwapSlots(int32_t oneBasedA, int32_t oneBasedB, bool buffersFollow) {
    if (oneBasedA <= 1 || oneBasedA > PackSize) return false;
    if (oneBasedB <= 1 || oneBasedB > PackSize) return false;
    int32_t a = oneBasedA - 1, b = oneBasedB - 1;
    std::swap(_pack[a], _pack[b]);
    if (buffersFollow) SpellBuffers::PackSlotsSwapped(shared_from_this(), a, b);
    return true;
}

int32_t Character::WearClassOf(cs::Ref<Item> it) {
    return it->Type >= 0 && it->Type < Mdata1::ItemTypes.Count() ? Mdata1::ItemTypes[it->Type].Index : -1;
}

int32_t Character::MaxWornOfClass(int32_t wearClass) {
    return wearClass >= 0 && wearClass < Mdata1::ItemDescriptions.Count() ? Mdata1::ItemDescriptions[wearClass].Index : 0;
}

int32_t Character::HandsUsed(cs::Func<cs::Ref<Item>(int16_t)> item) {
    return Inventory().Where([&](auto& s) { return s->Equipped; }).Select([&](auto& s) { return item(s->ItemId); })
                      .Where([&](auto& i) { return i != nullptr; }).Sum([&](auto& i) { return (int32_t)i->NHands; });
}

int32_t Character::WornOfClass(int32_t wearClass, cs::Func<cs::Ref<Item>(int16_t)> item) {
    return Inventory().Where([&](auto& s) { return s->Equipped; }).Select([&](auto& s) { return item(s->ItemId); })
                      .Count([&](auto& i) { return i != nullptr && WearClassOf(i) == wearClass; });
}

cs::String Character::EquipProblem(cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> item) {
    if (item) {
        // Two hands, no more. A two-handed sword and a shield will not go on together.
        if (HandsUsed(item) + it->NHands > 2) return "No Free Hands!";

        int32_t cls = WearClassOf(it);
        int32_t max = MaxWornOfClass(cls);
        if (max > 0 && WornOfClass(cls, item) >= max) return "Using too Many!";
    }

    if (!GuildData::CanUse(it, shared_from_this()))
        return it->ClassRestricted != 0
            ? "Your current guild cannot use this item!"
            : "Your currently joined guilds cannot use this item!";

    if (!MeetsUseLevel(it))
        return it->ClassRestricted != 0
            ? "Your guild level is too low to use item!"
            : "All guild levels too low to use item!";

    // MODULE6.BAS:645 does not say which stat
    for (int32_t i = 0; i < 6 && i < it->StatsRequired.Length(); i++)
        if (Stats[i] < it->StatsRequired[i]) return "One or more stats are too low!";
    return "";
}

bool Character::IsWearable(cs::Ref<Item> it) { return MaxWornOfClass(WearClassOf(it)) != 0; }

int32_t Character::StatWithMods(int32_t i, cs::Func<cs::Ref<Item>(int16_t)> item) {
    int32_t v = Stats[i];
    for (auto& s : Inventory().Where([&](auto& x) { return x->Equipped; })) {
        auto it = item(s->ItemId);
        if (it != nullptr && i < it->StatsMod.Length())
            v += ItemAlignments::StatModWorth(it->StatsMod[i], ItemAlignments::FitFor(s->Alignment, Alignment));
    }
    return v;
}

cs::String Character::WearRefusal(cs::Ref<InventorySlot> slot, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> item) {
    auto why = EquipProblem(it, item);
    if (!why.empty()) return why;

    auto fit = ItemAlignments::FitFor(slot->Alignment, Alignment);
    auto min = RaceDef()->Min;
    for (int32_t i = 0; i < 6 && i < it->StatsMod.Length() && i < min.Length(); i++)
        if (StatWithMods(i, item) + ItemAlignments::StatModWorth(it->StatsMod[i], fit) < min[i])
            // MODULE6.BAS:1809-1813 - a cursed item gets its own wording
            return it->Cursed == 2
                ? "Your stats are too low for the item to properly attach!"
                : "You are too weak to equip this item!";
    return "";
}

bool Character::MeetsUseLevel(cs::Ref<Item> it) {
    if (it->ClassRestricted != 0) {
        auto cur = Guild();
        return cur->Level >= UseLevel(it, GuildData::ByName(cur->Guild));
    }
    for (auto& m : Guilds) {
        if (m->Level <= 0) continue;
        auto g = GuildData::ByName(m->Guild);
        int32_t idx = GuildData::IndexOf(m->Guild);
        // C# masks the shift count to 5 bits
        if (idx >= 0 && (it->Guilds & cs::Wrap<int32_t>(1LL << (idx & 31))) == 0) continue;
        if (m->Level >= UseLevel(it, g)) return true;
    }
    return false;
}

int32_t Character::UseLevel(cs::Ref<Item> it, cs::Ref<GuildDef> g) {
    double f = g->ItemUseFactor;
    // fn030F assigns a Single expression to an Integer: VB rounds (banker's), it does not truncate
    return cs::Math::Max(1, cs::CastDouble<int32_t>(cs::Math::Round((f + f * 2.97) * it->LevelScale, cs::MidpointRounding::ToEven)));
}

bool Character::ItemHelps(cs::Ref<InventorySlot> slot) { return ItemAlignments::FitFor(slot->Alignment, Alignment) == ItemFit::Fine; }

bool Character::CanUnequip(cs::Ref<InventorySlot> slot, cs::Ref<Item> it, mc::Alignment who) { return !slot->SlotCursed; }

bool Character::CanUnequip(cs::Ref<InventorySlot> slot, cs::Ref<Item> it) { return CanUnequip(slot, it, Alignment); }

void Character::ApplyEquipCurse(cs::Ref<InventorySlot> slot, cs::Ref<Item> it) {
    if (it != nullptr && it->Cursed != 0) slot->SlotCursed = true;
    if (ItemAlignments::FitFor(slot->Alignment, Alignment) == ItemFit::Opposed) slot->SlotCursed = true;
}

void Character::SyncSlotCurses(cs::Func<cs::Ref<Item>(int16_t)> lookup) {
    for (auto& s : Inventory().Where([&](auto& x) { return x->Equipped; })) ApplyEquipCurse(s, lookup(s->ItemId));
}

cs::Ref<mc::Race> Character::RaceDef() { return RaceData::ByName(Race); }

cs::Ref<GuildDef> Character::ActiveGuildDef() { return GuildData::ByName(Guild()->Guild); }

int64_t Character::ExpForNextLevel() { return Rules::Fn12A1(Guild()->Level, ActiveGuildDef()->ExpFactor, RaceDef()->ExpFactor); }

int64_t Character::ExpForThisLevel() { return Rules::Fn12A1(Guild()->Level - 1, ActiveGuildDef()->ExpFactor, RaceDef()->ExpFactor); }

int64_t Character::GoldForNextLevel() { return Rules::Fn12B2(Guild()->Level, ActiveGuildDef()->GoldFactor); }

bool Character::CanLevelUp() { return Guild()->Level < ActiveGuildDef()->MaxLevel && Guild()->Experience >= ExpForNextLevel(); }

int32_t Character::LevelUp(cs::Ref<cs::Random> rng) {
    auto _levels = Guilds.Where([&](auto& g) { return g != Guild(); }).Select([&](auto& g) { return g->Level; });
    int32_t highestOther = _levels.Any() ? _levels.Max() : 0;
    Guild()->Level++;
    auto def = ActiveGuildDef();
    bool aheadOfEveryOtherGuild = Guild()->Level > highestOther;
    int32_t before = MaxHits;

    if (Guild()->Level > def->HitsCapLevel || !aheadOfEveryOtherGuild) {
        // past ML: exactly MH, and nothing at all if another guild has already been here
        if (aheadOfEveryOtherGuild) MaxHits += def->LateHits;
    } else {
        if (rng->Next(3) + 1 <= 2) MaxHits += cs::CastDouble<int32_t>(rng->NextDouble() * def->AvgHits / 5) + 1;
        MaxHits += def->AvgHits;
        if (Stats[static_cast<int32_t>(Stat::Con)] > 16 && Guild()->Level < def->HitsCapLevel)
            MaxHits += cs::CastDouble<int32_t>(rng->NextDouble() * (Stats[static_cast<int32_t>(Stat::Con)] - 16)) + 1;
    }
    if (MaxHits > 999) MaxHits = 999;
    CurHits = MaxHits;
    LastLevelGains.Clear();
    RecalcSpellPoints(true);
    RecalcSwings();
    return MaxHits - before;
}

void Character::RecalcSwings() {
    for (auto& m : Guilds) {
        if (m->Level <= 0) continue;
        double mod = GuildData::ByName(m->Guild)->Abilities.ElementAtOrDefault(3);
        if (mod <= 0) continue;

        // FRMCTYG.BAS:184 declares the threshold `As Integer`, so each assignment rounds half to even
        double step = cs::Math::Log(10 - mod + 1);
        double threshold = cs::Math::Round(step * 50, cs::MidpointRounding::ToEven);
        int32_t extra = 0;
        for (int32_t level = 1; level <= m->Level; level++)
            if (level > threshold) {
                extra++;
                threshold = cs::Math::Round(step * (threshold * 2), cs::MidpointRounding::ToEven);
            }

        if (extra > Swings) Swings = extra;
    }
}

cs::List<cs::String> Character::RollQuestForLevel(cs::List<cs::Ref<Monster>> monsters, cs::List<cs::Ref<Item>> items, cs::Ref<cs::Random> rng) {
    Guild()->QuestType = 0; Guild()->QuestTarget = 0; Guild()->QuestDone = false;
    auto _q = Rules::RollQuest(ActiveGuildDef(), Guild()->Level, rng);
    auto type = _q.Type;
    auto tier = _q.Tier;
    if (type == 0) return cs::List<cs::String>::New();
    // the Messages lines, FRMCTYG.BAS:470-480 and the quest itself
    auto said = cs::List<cs::String>::Of({"Questmaster is going through his books..."});
    if (type == -1) { said.Add("Questmaster lets you off the hook."); return said; }

    if (type == 1) {
        auto m = Rules::QuestMonster(monsters, tier, rng);
        if (m != nullptr) {
            Guild()->QuestType = 1; Guild()->QuestTarget = m->Id;
            said.Add("You must defeat one " + m->Name + ".");
            return said;
        }
    }
    if (type == 2) {
        auto it = Rules::QuestItem(items, tier, rng);
        if (it != nullptr) {
            Guild()->QuestType = 2; Guild()->QuestTarget = it->Id;
            said.Add("You must return one " + it->Name + ".");   // FRMCTYG.BAS:563
            return said;
        }
    }
    return said;
}

bool Character::QuestSatisfied(bool consume) {
    if (Guild()->QuestType == 0) return true;
    if (Guild()->QuestType == 1) return Guild()->QuestDone;

    // FRMCTYG.BAS:254-259 scans every pack slot by item id, worn or not
    auto slot = Inventory().FirstOrDefault([&](auto& s) { return s->ItemId == Guild()->QuestTarget; });
    if (slot == nullptr) return false;
    if (consume) { RemoveFromPack(slot); ItemQuestsDone++; }   // M5906(4), FRMCTYG.BAS:265
    return true;
}

int64_t Character::ExperienceCap() { return Rules::Fn13ED(Guild()->Level, ActiveGuildDef()->ExpFactor, RaceDef()->ExpFactor); }

bool Character::IsPinned() { return Guild()->Experience >= ExperienceCap(); }

int32_t Character::ForfeitCost() { return cs::CastDouble<int32_t>(cs::Math::Floor(cs::Math::Log(Guild()->Level + 1))); }

cs::String Character::ForfeitRefusal(cs::Func<cs::Ref<Item>(int16_t)> lookup) {
    for (auto& s : Inventory().Where([&](auto& x) { return x->Equipped; }))
        if (s->SlotCursed)
            return "Your Aura is Tainted!\nCnanot forfeit quest until cursed items are removed.";
    return "";
}

cs::List<cs::Ref<InventorySlot>> Character::ForfeitQuest(cs::Func<cs::Ref<Item>(int16_t)> lookup, cs::Ref<cs::Random> rng) {
    int32_t cost = ForfeitCost();
    int32_t from = Guild()->Level;
    auto def = ActiveGuildDef();

    // FRMCTYG.BAS:862-885 - each lost level takes back the hits it gave. Never below 15.
    for (int32_t lvl = from; lvl >= from - cost + 1; lvl--) {
        bool wasHighest = !Guilds.Any([&](auto& g) { return g != Guild() && g->Level >= lvl; });
        if (lvl > def->HitsCapLevel || !wasHighest) {
            if (wasHighest) MaxHits -= def->LateHits;
        } else {
            int32_t _r = rng->Next(3) + 1;
            if (_r == 1 || _r == 2) MaxHits -= cs::CastDouble<int32_t>(rng->NextDouble() * def->AvgHits / 5) - 1;
            MaxHits -= def->AvgHits;
        }
        if (MaxHits < 15) MaxHits = 15;
    }
    CurHits = MaxHits;

    Guild()->Level = cs::Math::Max(1, from - cost);
    Guild()->QuestType = 0; Guild()->QuestTarget = 0; Guild()->QuestDone = false;

    auto removed = cs::List<cs::Ref<InventorySlot>>::New();
    for (auto& slot : Inventory().Where([&](auto& x) { return x->Equipped; }).ToList()) {
        auto it = lookup(slot->ItemId);
        if (it == nullptr) continue;
        if (!EquipProblem(it).empty()) { slot->Equipped = false; removed.Add(slot); }
    }
    RecalcSpellPoints(false);
    // FRMCTYG.BAS:919 overwrites M56AF with this guild's figure alone - a demotion can lower it
    Swings = SwingsFor(Guild());
    // FRMCTYG.BAS:942-944 - the experience is assigned, not adjusted
    Guild()->Experience = Rules::Fn12A1(Guild()->Level - 1, ActiveGuildDef()->ExpFactor, RaceDef()->ExpFactor);   // fn12A1 a level down (FRMCTYG.BAS:946-948)
    return removed;
}

int32_t Character::SwingsFor(cs::Ref<GuildMembership> m) {
    double mod = GuildData::ByName(m->Guild)->Abilities.ElementAtOrDefault(3);
    if (mod <= 0 || m->Level <= 0) return 0;
    double step = cs::Math::Log(10 - mod + 1);
    double threshold = cs::Math::Round(step * 50, cs::MidpointRounding::ToEven);
    int32_t extra = 0;
    for (int32_t level = 1; level <= m->Level; level++)
        if (level > threshold) { extra++; threshold = cs::Math::Round(step * (threshold * 2), cs::MidpointRounding::ToEven); }

    return extra;
}

bool Character::JoinGuild(cs::Ref<GuildDef> g) {
    if (Guilds.Any([&](auto& m) { return m->Guild == g->Name; })) return SetActiveGuild(g->Name);
    if (g->Unmet(shared_from_this()).Count() > 0) return false;
    Guilds.Insert(0, [&] { auto _o = cs::New<GuildMembership>(); _o->Guild = g->Name; _o->Level = 1; _o->Experience = 0; return _o; }());
    RecalcSpellPoints(true);
    return true;
}

bool Character::SetActiveGuild(cs::String name) {
    int32_t i = Guilds.FindIndex([&](auto& m) { return m->Guild == name; });
    if (i <= 0) return i == 0;
    auto m = Guilds[i]; Guilds.RemoveAt(i); Guilds.Insert(0, m);
    return true;
}

cs::Ref<Character> Character::Create(cs::String name, cs::Ref<mc::Race> race, mc::Alignment alignment, mc::Sex sex, cs::List<int32_t> stats) {
    auto c = [&] {
        auto _o = cs::New<Character>();
        _o->Name = name;
        _o->Race = race->Name;
        _o->Alignment = alignment;
        _o->Sex = sex;
        _o->Stats = stats.Clone();
        _o->StatMax = race->Max.Clone();
        // CHARCREA.BAS:248, CLng(Date): DateTime.Today.ToOADate(), whole days since 1899-12-30
        _o->CreatedOn = [] {
            int64_t t = cs::DateTime::Now().ticks;
            t -= t % 864000000000LL;
            return (double)((t - 599264352000000000LL) / 864000000000LL);
        }();
        return _o;
    }();
    c->Guilds.Add([&] { auto _o = cs::New<GuildMembership>(); _o->Guild = GuildData::StartingGuild; _o->Level = 1; _o->Experience = 0; return _o; }());
    // 15 hits flat, whatever the constitution
    c->MaxHits = 15;
    c->CurHits = c->MaxHits;
    c->RecalcSpellPoints(true);
    c->GoldOnHand = VbCurrency::FromRaw(1500LL * VbCurrency::Scale);   // the real game starts you with 1,500 gold
    // CHARCREA.BAS:239 sets the Combat Setting to 1, which is Fight
    c->CombatSetting = 1;
    c->Area = -1;   // "Chars(m00C2).M59BB = -1" (CHARCREA.BAS:242)
    return c;
}

int32_t Character::AreaOr(cs::Func<int32_t(cs::Ref<Character>)> fromSquare) {
    return Area.has_value() ? Area.value()
        : (IsDead && CauseOfDeath == Rules::DeathRock ? (DungeonLevel > 0 ? 0 : -1) : fromSquare(shared_from_this()));
}

void Character::AreaForRockDeath() {
    if (DungeonLevel <= 0) { DungeonLevel = 0; Area = -1; }
    else Area = 0;
}

void Character::Save(cs::String path) {
    throw cs::NotSupportedException("not translated: JSON serialization of Character");
}

void Character::CheckNames(cs::String path) {
    if (RaceData::IndexOf(Race) < 0)
        throw cs::InvalidDataException(path + ": race \"" + Race + "\" is not in MDATA1");
    for (auto& m : Guilds)
        if (GuildData::IndexOf(m->Guild) < 0)
            throw cs::InvalidDataException(path + ": guild \"" + m->Guild + "\" is not in MDATA1");
}

cs::Ref<Character> Character::ReadRaw(cs::String path) {
    throw cs::NotSupportedException("not translated: JSON deserialization of Character");
}

cs::Ref<Character> Character::Load(cs::String path) {
    throw cs::NotSupportedException("not translated: JSON deserialization of Character");
}

cs::List<cs::Ref<Character>> Character::LoadAllFrom(cs::String savesDir) {
    return Roster::Saved(savesDir).Select([&](auto& p) { return p.Ch; }).ToList();
}

cs::String Character::SafeFileName(cs::String s) {
    // Path.GetInvalidFileNameChars() as .NET reports it on each platform
    cs::String _invalid;
#ifdef _WIN32
    _invalid += "\"<>|";
    for (int ch = 0; ch < 32; ch++) _invalid += (char)ch;
    _invalid += ":*?\\/";
#else
    _invalid += '\0';
    _invalid += '/';
#endif
    cs::String _r;
    for (char c : s) if (_invalid.find(c) == cs::String::npos) _r += c;
    return _r;
}

}  // namespace mc
