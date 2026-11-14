#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct MessageLook {
    static MessageLook Plain;
    MessageKind Kind{};
    cs::Nullable<Tup_R_G_B_5bdd2> Colour{};

    bool operator==(const MessageLook& o) const {
        if (Kind != o.Kind) return false;
        if (Colour.has_value() != o.Colour.has_value()) return false;
        if (Colour.has_value()) {
            const auto& a = Colour.value();
            const auto& b = o.Colour.value();
            if (a.R != b.R || a.G != b.G || a.B != b.B) return false;
        }
        return true;
    }
    bool operator!=(const MessageLook& o) const { return !(*this == o); }
};

inline MessageLook MessageLook::Plain = MessageLook{};

struct MessageList {
    static constexpr int32_t Kept = 20;

    static bool Add(cs::List<cs::String> lines, cs::String line, int32_t kept = Kept);
    static bool Add(cs::List<cs::String> lines, cs::List<MessageLook> looks, cs::String line, MessageLook look, int32_t kept = Kept);
};

}  // namespace mc
