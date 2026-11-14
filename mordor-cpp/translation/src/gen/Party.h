#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Party_ShortfallGift {
    cs::Ref<Character> Giver{};
    VbCurrency Amount{};
};

struct Party_Roster {
    cs::String Leader{};
    cs::List<cs::String> Members = cs::List<cs::String>::New();
};

struct Party {
    using BankAnswer = Party_BankAnswer;
    using ShortfallGift = Party_ShortfallGift;
    using Roster = Party_Roster;

    static constexpr int32_t MaxSize = 4;
    inline static const cs::String NobodyToPoolTo = "Nobody else in party to Pool to!";
    inline static const cs::String CannotPoolInCombat = "Cannot pool gold while in combat!";
    inline static const cs::String NobodyToGiveTo = "Nobody in party to give to!";
    inline static const cs::String ReloadTitle = "Save Party?";
    inline static const cs::String Disbanding = "Disbanding Party...";
    inline static const cs::String AutoloadPrefix = "AUTOLOAD PARTY: ";

    cs::List<cs::Ref<Character>> Members = cs::List<cs::Ref<Character>>::New();
    int32_t SelectedIndex = 0;
    cs::Dictionary<cs::Ref<Character>, cs::Ref<CombatOrder>> Orders = cs::Dictionary<cs::Ref<Character>, cs::Ref<CombatOrder>>::New();

    Party(cs::List<cs::Ref<Character>> members);

    cs::Ref<Character> Selected();
    cs::Ref<Character> Leader();
    int32_t Size();
    cs::List<cs::Ref<Character>> Able();
    bool AnyAble();
    cs::String ReloadQuestion();

    cs::List<Tup_owner_companion_66faa> Targets();
    cs::String WhyCannotJoin(cs::Ref<Character> c);
    cs::List<cs::Ref<Character>> HereToJoin(cs::List<cs::Ref<Character>> everyone, int32_t level, int32_t area, int32_t floors,
                                            cs::Func<int32_t(cs::Ref<Character>)> savedArea);
    static bool SamePlace(cs::Ref<Character> a, cs::Ref<Character> b);
    static void ShareLostFlags(cs::Ref<Character> first, cs::Ref<Character> newcomer);
    static bool Opposed(Alignment a, Alignment b);
    static cs::String PoolCompleted(cs::Ref<Character> to);
    VbCurrency PoolGold();
    static bool AllOrNothing(cs::List<cs::Ref<Character>> who, cs::Func<void()> move);
    static bool PoolToBank(cs::List<cs::Ref<Character>> members, cs::Ref<Character> to);
    static VbCurrency PoolOnHandToBank(cs::List<cs::Ref<Character>> members, cs::Ref<Character> to);
    static bool PoolAndDeposit(cs::List<cs::Ref<Character>> members, cs::Ref<Character> to);
    static VbCurrency PartyDeposit(cs::List<cs::Ref<Character>> members);
    static Party_BankAnswer BankTyped(cs::Ref<Character> ch, cs::String text, bool deposit);
    static cs::String BankAllText(VbCurrency balance);
    static cs::List<cs::Ref<Party_ShortfallGift>> PlanShortfall(cs::List<cs::Ref<Character>> members, cs::Ref<Character> payer, int64_t cost,
                                                                 cs::List<cs::Ref<Character>> from = {});
    static cs::List<cs::Ref<Party_ShortfallGift>> CoverShortfall(cs::List<cs::Ref<Character>> members, cs::Ref<Character> payer, int64_t cost,
                                                                  cs::List<cs::Ref<Character>> from = {});
    static VbCurrency GiveGold(cs::Ref<Character> from, cs::Ref<Character> to, VbCurrency amount);
    static cs::String GaveGold(int64_t amount, cs::Ref<Character> to);
    static cs::String TransferGold(cs::Ref<Character> from, cs::Ref<Character> to, int64_t amount);
    static cs::String SwapItems(cs::Ref<Character> from, int32_t slotFrom, cs::Ref<Character> to, int32_t slotTo, cs::Func<cs::Ref<Item>(int16_t)> item);
    static cs::String NoFreeSlots(cs::Ref<Character> to);
    static cs::String GiveItem(cs::Ref<Character> from, cs::Ref<Character> to, cs::Ref<InventorySlot> slot, cs::Ref<Item> it,
                               cs::Func<cs::Ref<Item>(int16_t)> lookup = {}, cs::Func<void(cs::String)> identified = {},
                               cs::Func<void(int32_t, IdLevel)> filed = {});
    static cs::String GaveItem(cs::Ref<Character> to);
    static cs::String NoFreeSlotsFromVault(cs::Ref<Character> to);
    static cs::String GiveFromVault(cs::Ref<Character> from, int32_t bankSlot, cs::Ref<Character> to,
                                    cs::Func<cs::Ref<Item>(int16_t)> lookup = {}, cs::Func<void(cs::String)> identified = {},
                                    cs::Func<void(int32_t, IdLevel)> filed = {});
    static void ReadGiven(cs::Ref<Character> to, cs::Ref<InventorySlot> slot, cs::Ref<Item> it, cs::Func<cs::Ref<Item>(int16_t)> lookup,
                          cs::Func<void(cs::String)> identified, cs::Func<void(int32_t, IdLevel)> filed);
    bool Add(cs::Ref<Character> c);
    bool Remove(cs::Ref<Character> c);
    cs::Ref<Character> LeaderAfterRemoving(cs::Ref<Character> c);
    static cs::String NowLeader(cs::Ref<Character> c);
    cs::List<cs::String> RemoveTheDead(cs::Func<void(cs::Ref<Character>)> save = {}, cs::Func<void(cs::Ref<Character>)> setDead = {});
    bool Select(int32_t index);
    bool SelectNextAble();
    cs::String Swap(int32_t a, int32_t b);
    void MoveTo(int32_t from, int32_t to);
    cs::Ref<CombatOrder> OrderFor(cs::Ref<Character> c);
    void ClearOrders();
    cs::List<cs::Ref<Character>> ApplyDefaultOrders(cs::List<cs::Ref<Spell>> spells = {});
    void RestInCity();
    void PlaceAt(int32_t level, int32_t x, int32_t y);
    void SaveAll(cs::String savesDir);
    static cs::String RosterPath(cs::String savesDir, cs::String leader);
    bool SaveParty(cs::String savesDir, bool remember, cs::Func<void(cs::String)> errorLog = {});
    static cs::Ref<Party> LoadFor(cs::Ref<Character> leader, cs::String savesDir, cs::Func<void(cs::String)> say = {},
                                  cs::Func<cs::Ref<Monster>(cs::Ref<Companion>)> monsters = {}, cs::Func<int32_t(cs::Ref<Character>)> areaOf = {});
};

}  // namespace mc
