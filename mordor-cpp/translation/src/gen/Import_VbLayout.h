#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct VbField {
    cs::String Name{};
    VbKind Kind{};
    int32_t Upper = 0;
    int32_t StringLength = 0;
    cs::Ref<VbType> Type{};
    int32_t Offset = 0;

    VbField(cs::String name, VbKind kind, int32_t upper = -1, int32_t stringLength = 0, cs::Ref<VbType> type = {});

    int32_t Count();
    int32_t ElementSize();
    int32_t Size();
    cs::String ToString();
};

struct VbType {
    cs::String Name{};
    cs::List<cs::Ref<VbField>> Fields{};
    int32_t Size = 0;

    VbType(cs::String name, cs::List<cs::Ref<VbField>> fields);

    cs::Ref<VbField> Field(cs::String name);
    Tup_Offset_Field_ed9f9 Locate(cs::String path);
    int32_t OffsetOf(cs::String path);
};

struct VbRecord {
    cs::List<uint8_t> _data{};
    int32_t _base = 0;
    cs::Ref<VbType> Type{};

    VbRecord() = default;
    VbRecord(cs::List<uint8_t> data, cs::Ref<VbType> type, int32_t base = 0);

    cs::List<uint8_t> Bytes(int32_t offset, int32_t length);
    cs::List<uint8_t> At(cs::String path, VbKind want);
    int16_t Integer(cs::String path);
    int32_t Long(cs::String path);
    float Single(cs::String path);
    double Double(cs::String path);
    VbCurrency Currency(cs::String path);
    cs::String FixedString(cs::String path);
    VbRecord Sub(cs::String path);
    bool IsZero();
};

struct VbFile {
    cs::String Path{};
    int32_t RecordLength = 0;
    cs::List<uint8_t> Data{};

    VbFile(cs::String path, int32_t recordLength, cs::List<uint8_t> data);

    static cs::Ref<VbFile> Open(cs::String path, int32_t recordLength);
    int64_t Length();
    int32_t RecordCount();
    int32_t LastRecordLength();
    cs::List<uint8_t> Record(int32_t n);
    VbRecord Record(int32_t n, cs::Ref<VbType> type);
    bool Has(int32_t n);
    int16_t Integer(int32_t n);
    cs::String VarString(int32_t n);
    static cs::String VarString(cs::List<uint8_t> record, int32_t at, int32_t& next);
    static cs::String Text(cs::List<uint8_t> bytes);
    static cs::String RTrim(cs::String s);
};

struct VbLayout {
    static cs::Ref<VbField> I(cs::String n, int32_t upper = -1);
    static cs::Ref<VbField> L(cs::String n, int32_t upper = -1);
    static cs::Ref<VbField> S(cs::String n, int32_t upper = -1);
    static cs::Ref<VbField> C(cs::String n, int32_t upper = -1);
    static cs::Ref<VbField> F(cs::String n, int32_t length);
    static cs::Ref<VbField> U(cs::String n, cs::Ref<VbType> t, int32_t upper = -1);

    static cs::Ref<VbType> Slot;
    static cs::Ref<VbType> Guild;
    static cs::Ref<VbType> Companion;
    static cs::Ref<VbType> Window;
    static cs::Ref<VbType> Sanctuary;
    static cs::Ref<VbType> Character;
    static cs::Ref<VbType> Shelf;
    static cs::Ref<VbType> Foe;
    static cs::Ref<VbType> Fight;
    static cs::Ref<VbType> Library;
    static cs::Ref<VbType> Hall;
    static cs::List<cs::Ref<VbType>> All;

    static constexpr int32_t CharactersLen = 2900;
    static constexpr int32_t StoreLen = 16;
    static constexpr int32_t GuildmastersLen = 128;
    static constexpr int32_t AutomapLen = 10;
    static constexpr int32_t RoomsLen = 28;
    static constexpr int32_t PartiesLen = 35;
    static constexpr int32_t LibraryLen = 100;
    static constexpr int32_t HallLen = 45;
    static constexpr int32_t PensLen = 16;

    inline static const cs::String Version = "1.1";

    static bool VersionAccepted(cs::String stamp);
};

}  // namespace mc
