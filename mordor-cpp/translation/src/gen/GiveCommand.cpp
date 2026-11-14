#include "core.h"

namespace mc {

cs::String GiveCommand::ToTitle(cs::Ref<Character> to) { return "Give to \"" + cs::Str::TrimEnd(to->Name) + "\""; }

cs::List<cs::Ref<Character>> GiveCommand::Offered(cs::List<cs::Ref<Character>> party, cs::Ref<Character> me) {
    return party.Where([&](auto& c) { return c != me; }).ToList();
}

Tup_Tag_Refusal_820db GiveCommand::GoldAnswer(cs::String text, VbCurrency goldOnHand) {
    if (text == "") return Tup_Tag_Refusal_820db{"0", ""};
    double v = Val(text);
    if (v < 1) return Tup_Tag_Refusal_820db{"0", ""};
    if (v > goldOnHand.ToDouble()) return Tup_Tag_Refusal_820db{"", NotEnoughGold};  // Val is a Double: the Currency is compared as one
    return Tup_Tag_Refusal_820db{text, ""};  // the text as typed, not its Val
}

Tup_Tag_Refusal_820db GiveCommand::ItemAnswer(cs::String text, cs::Ref<Character> me, cs::Func<cs::Ref<Item>(int16_t)> lookup) {
    if (text == "" || text == "0") return Tup_Tag_Refusal_820db{"0", ""};
    int32_t slot = Fn131A(text, me, lookup);
    if (slot < 1) return Tup_Tag_Refusal_820db{"", NoItem};  // item #1, the Hands, is slot 0 and so refused too
    auto s = me->Pack()[slot];
    if (s != nullptr && s->Equipped) return Tup_Tag_Refusal_820db{"", ItemEquipped};
    return Tup_Tag_Refusal_820db{text, ""};
}

int32_t GiveCommand::Fn131A(cs::String text, cs::Ref<Character> me, cs::Func<cs::Ref<Item>(int16_t)> lookup) {
    if (text == "") return 0;
    double v = Val(text);
    if (v > INT16_MAX || v < INT16_MIN) return -1;
    int32_t n = cs::CastDouble<int32_t>(cs::Math::Round(v, cs::MidpointRounding::ToEven));
    if (n < 1 || n > Character::PackSize) return -1;
    auto sl = me->Pack()[n - 1];
    if ((sl != nullptr && sl->ItemId != 0) || n == 1) {
        auto named = lookup(sl ? sl->ItemId : Character::HandsItemId);
        if (named != nullptr && named->Name != "") return n - 1;
    }
    return -1;
}

cs::Nullable<int64_t> GiveCommand::GoldFromTag(cs::String tag) { return CLng(tag); }

cs::Nullable<int64_t> GiveCommand::CLng(cs::String s) {
    cs::String t = cs::Str::Trim(s, " \t");
    double v = 0;
    if (cs::Str::StartsWithIgnoreCase(t, "&H") || cs::Str::StartsWithIgnoreCase(t, "&O")) {
        int32_t end = cs::Str::StartsWithIgnoreCase(t, "&H") ? RadixEnd(t, 2, 16) : RadixEnd(t, 2, 8);
        if (end != (int32_t)t.size() || end == 2) return {};
        v = Radix(t, 2, end);
    } else {
        // VB takes D for a Double's exponent as well as E; .NET only E
        cs::String e = cs::Str::Replace(cs::Str::Replace(t, 'D', 'E'), 'd', 'E');
        if (!cs::Parse::TryParse(e, cs::Parse::Float | cs::Parse::AllowThousands, cs::Parse::CurrentCulture(), v) || std::isnan(v) || std::isinf(v))
            return {};  // not a number, or "Infinity"/"NaN" (not numbers to VB), or past a Double
    }
    if (std::isinf(v)) return {};
    double r = cs::Math::Round(v, cs::MidpointRounding::ToEven);
    if (r > INT32_MAX || r < INT32_MIN) return {};  // Overflow, the same trap
    return cs::CastDouble<int64_t>(r);
}

int32_t GiveCommand::RadixEnd(cs::String t, int32_t from, int32_t radix) {
    int32_t i = from;
    while (i < (int32_t)t.size()) {
        auto d = DigitOf(t[i]);
        if (!(d.has_value() && d.value() < radix)) break;
        i++;
    }
    return i;
}

cs::Nullable<int32_t> GiveCommand::DigitOf(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return {};
}

double GiveCommand::Radix(cs::String t, int32_t from, int32_t end) {
    int32_t radix = std::toupper((unsigned char)t[from - 1]) == 'H' ? 16 : 8;
    uint64_t u = 0;
    for (int32_t i = from; i < end; i++) {
        u = u * (uint64_t)radix + (uint64_t)DigitOf(t[i]).value();
        if (u > 0xFFFFFFFFULL) return std::numeric_limits<double>::infinity();
    }
    return u <= 0xFFFF ? (double)(int16_t)(uint16_t)u : (double)(int32_t)(uint32_t)u;
}

int32_t GiveCommand::SlotFromTag(cs::String tag) {
    double v = Val(tag);
    if (v > INT16_MAX || v < INT16_MIN) return 0;
    int32_t n = cs::CastDouble<int32_t>(cs::Math::Round(v, cs::MidpointRounding::ToEven));
    return n > 1 ? n - 1 : 0;
}

cs::List<GiveCommand_Said> GiveCommand::Give(cs::Ref<Character> from, cs::Ref<Character> to, int32_t slot, int64_t gold,
                                             cs::Func<cs::Ref<Item>(int16_t)> lookup,
                                             cs::Func<void(int32_t, IdLevel)> filed) {
    auto said = cs::List<Said>::New();
    if (gold > 0) {
        // sub14B5 has no handler, so an Overflow goes to fn1521's trap (4169-4171), which writes
        // to the error file only and ends the command where it got to: no line, no item
        VbCurrency got{};
        if (!VbCurrency::TryAdd(to->GoldOnHand, VbCurrency::FromRaw(gold * VbCurrency::Scale), got)) return said;
        to->GoldOnHand = got;  // 2931
        VbCurrency left{};
        if (!VbCurrency::TrySubtract(from->GoldOnHand, VbCurrency::FromRaw(gold * VbCurrency::Scale), left)) return said;
        from->GoldOnHand = left;  // 2932
        said.Add(Said{"Give of " + CharStatFile::FormatNumber(gold) + " gold to \"" + cs::Str::TrimEnd(to->Name) + "\" Completed!", false});
    }
    if (slot > 0 && slot < Character::PackSize) {
        auto s = from->Pack()[slot];
        if (s != nullptr && s->ItemId > 0) {
            cs::String no = Party::GiveItem(from, to, s, lookup(s->ItemId), lookup,
                                            [&](cs::String line) { said.Add(Said{line, false}); }, filed);
            if (!no.empty()) { said.Add(Said{no, no == Party::NoFreeSlots(to)}); return said; }
            said.Add(Said{Party::GaveItem(to), false});
        }
    }
    return said;
}

double GiveCommand::Val(cs::String s) {
    auto t = cs::New<cs::StringBuilder>();
    for (char ch : s) if (ch != ' ' && ch != '\t' && ch != '\n') t->Append(ch);
    cs::String x = t->ToString();
    if (cs::Str::StartsWithIgnoreCase(x, "&H") || cs::Str::StartsWithIgnoreCase(x, "&O")) {
        int32_t end = cs::Str::StartsWithIgnoreCase(x, "&H") ? RadixEnd(x, 2, 16) : RadixEnd(x, 2, 8);
        return end == 2 ? 0 : Radix(x, 2, end);
    }
    auto isDigit = [](char c) { return c >= '0' && c <= '9'; };
    int32_t i = 0;
    if (i < (int32_t)x.size() && (x[i] == '-' || x[i] == '+')) i++;
    int32_t digits = 0;
    while (i < (int32_t)x.size() && isDigit(x[i])) { i++; digits++; }
    if (i < (int32_t)x.size() && x[i] == '.') { i++; while (i < (int32_t)x.size() && isDigit(x[i])) { i++; digits++; } }
    if (digits == 0) return 0;
    int32_t mantissa = i;
    if (i < (int32_t)x.size() && (x[i] == 'e' || x[i] == 'E' || x[i] == 'd' || x[i] == 'D')) {
        int32_t j = i + 1;
        if (j < (int32_t)x.size() && (x[j] == '-' || x[j] == '+')) j++;
        int32_t k = j;
        while (k < (int32_t)x.size() && isDigit(x[k])) k++;
        if (k > j) i = k;  // an exponent with no digits is not read
    }
    cs::String num = cs::Str::Substring(x, 0, mantissa) + (i > mantissa ? "E" + cs::Str::Substring(x, mantissa + 1, i - (mantissa + 1)) : "");
    double r = 0; if (!cs::Parse::TryParse(num, cs::Parse::Float, cs::Parse::InvariantCulture(), r)) throw cs::FormatException("The input string '" + num + "' was not in a correct format."); return r;
}

}  // namespace mc
