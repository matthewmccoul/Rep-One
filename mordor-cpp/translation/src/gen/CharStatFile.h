#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CharStatFile_Piece {
    int32_t Row = 0;
    int32_t Col = 0;
    cs::String Text{};
    bool Bold = false;
    bool Italic = false;
    bool OnScreen();
    bool operator==(const CharStatFile_Piece& o) const {
        return Row == o.Row && Col == o.Col && Text == o.Text && Bold == o.Bold && Italic == o.Italic;
    }
    bool operator!=(const CharStatFile_Piece& o) const { return !(*this == o); }
};

struct CharStatFile {
    using Piece = CharStatFile_Piece;
    inline static const cs::String Version = "1.1";
    inline static const cs::String FileName = "CHARSTAT.TXT";
    static cs::String Write(cs::Ref<Character> c, cs::String gameDir, cs::Func<cs::Ref<Item>(int16_t)> item);
    static cs::String Write(cs::Ref<Character> c, cs::List<cs::String> places, cs::Func<cs::Ref<Item>(int16_t)> item);
    static cs::String Compose(cs::List<CharStatFile_Piece> pieces);
    static cs::String Str(int64_t n);
    static cs::String FormatNumber(int64_t n);
    static cs::String FormatNumber(VbCurrency v);
    static cs::String CStrSingle(float v);
    static cs::List<CharStatFile_Piece> Paint(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item, bool toFile, cs::DateTime now);
};

}  // namespace mc
