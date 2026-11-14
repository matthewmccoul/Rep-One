#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct LookAround_Entry {
    cs::String Name{};
    cs::String Guild{};
    int32_t Level = 0;
    int32_t Hits = 0;
    bool CountedDead = false;
    bool operator==(const LookAround_Entry& o) const {
        return Name == o.Name && Guild == o.Guild && Level == o.Level && Hits == o.Hits && CountedDead == o.CountedDead;
    }
    bool operator!=(const LookAround_Entry& o) const { return !(*this == o); }
};

struct LookAround_Listed {
    int32_t Hits = 0;
    bool Dead = false;
    bool operator==(const LookAround_Listed& o) const { return Hits == o.Hits && Dead == o.Dead; }
    bool operator!=(const LookAround_Listed& o) const { return !(*this == o); }
};

struct LookAround_ListedParty {
    cs::Dictionary<cs::Ref<Character>, LookAround_Listed> _rows = cs::Dictionary<cs::Ref<Character>, LookAround_Listed>::New();
    void Sync(cs::List<cs::Ref<Character>> party);
    cs::Nullable<LookAround_Listed> For(cs::Ref<Character> c, cs::Ref<Character> shown);
};

struct LookAround {
    using Entry = LookAround_Entry;
    using Listed = LookAround_Listed;
    using ListedParty = LookAround_ListedParty;

    // returns (Here, Living, Dead)
    static std::tuple<cs::List<LookAround_Entry>, int32_t, int32_t> Walk(cs::Ref<Character> leader, cs::List<cs::Ref<Character>> roster,
        cs::Func<std::tuple<int32_t, int32_t>(cs::Ref<Character>)> placeOf, bool everywhere = false,
        cs::Func<cs::Nullable<LookAround_Listed>(cs::Ref<Character>)> listed = {});
    static cs::List<cs::Ref<Character>> Roster(cs::List<cs::Ref<Character>> party, cs::List<cs::Ref<Character>> saved);
    static cs::List<CharStatFile_Piece> Paint(cs::List<LookAround_Entry> here);
    static cs::String Readout(cs::Ref<Character> shown, cs::Ref<Character> leader, int32_t x, int32_t y, int32_t level, Facing facing, int32_t living, int32_t dead);
};

}  // namespace mc
