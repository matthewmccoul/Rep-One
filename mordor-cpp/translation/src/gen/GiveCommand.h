#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GiveCommand_Said {
    cs::String Text{};
    bool Box = false;
    bool operator==(const GiveCommand_Said& o) const { return Text == o.Text && Box == o.Box; }
    bool operator!=(const GiveCommand_Said& o) const { return !(*this == o); }
};

struct GiveCommand {
    using Said = GiveCommand_Said;

    inline static const cs::String NameTitle = "Give to Character";
    inline static const cs::String NameLabel = "Character's Name?";
    inline static const cs::String GoldLabel = "How much Gold?";
    inline static const cs::String ItemLabel = "Which Item #?";
    inline static const cs::String NotEnoughGold = "Not enough gold!";
    inline static const cs::String NoItem = "No Item to give!";
    inline static const cs::String ItemEquipped = "Item is Equipped!";
    inline static const cs::String Sorry = "SORRY!";
    static constexpr int32_t GoldMaxLength = 10;
    static constexpr int32_t ItemMaxLength = 3;

    static cs::String ToTitle(cs::Ref<Character> to);
    static cs::List<cs::Ref<Character>> Offered(cs::List<cs::Ref<Character>> party, cs::Ref<Character> me);
    static Tup_Tag_Refusal_820db GoldAnswer(cs::String text, VbCurrency goldOnHand);
    static Tup_Tag_Refusal_820db ItemAnswer(cs::String text, cs::Ref<Character> me, cs::Func<cs::Ref<Item>(int16_t)> lookup);
    static int32_t Fn131A(cs::String text, cs::Ref<Character> me, cs::Func<cs::Ref<Item>(int16_t)> lookup);
    static cs::Nullable<int64_t> GoldFromTag(cs::String tag);
    static cs::Nullable<int64_t> CLng(cs::String s);
    static int32_t RadixEnd(cs::String t, int32_t from, int32_t radix);
    static cs::Nullable<int32_t> DigitOf(char c);
    static double Radix(cs::String t, int32_t from, int32_t end);
    static int32_t SlotFromTag(cs::String tag);
    static cs::List<GiveCommand_Said> Give(cs::Ref<Character> from, cs::Ref<Character> to, int32_t slot, int64_t gold,
                                           cs::Func<cs::Ref<Item>(int16_t)> lookup,
                                           cs::Func<void(int32_t, IdLevel)> filed = {});
    static double Val(cs::String s);
};

}  // namespace mc
