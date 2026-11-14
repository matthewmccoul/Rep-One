#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct NeModule_Resource {
    cs::String Type{};
    cs::String Name{};
    int32_t Offset = 0;
    int32_t Length = 0;
    bool operator==(const NeModule_Resource& o) const { return Type == o.Type && Name == o.Name && Offset == o.Offset && Length == o.Length; }
    bool operator!=(const NeModule_Resource& o) const { return !(*this == o); }
};

struct NeModule {
    using Resource = NeModule_Resource;

    static constexpr int32_t RtIcon = 3;
    static constexpr int32_t RtBitmap = 2;
    static constexpr int32_t RtRcData = 10;
    static constexpr int32_t RtGroupIcon = 14;

    cs::List<uint8_t> Bytes;
    cs::List<cs::Ref<NeModule_Resource>> Resources;

    NeModule(cs::List<uint8_t> bytes, cs::List<cs::Ref<NeModule_Resource>> resources);

    static cs::Ref<NeModule> Read(cs::String path);
    static cs::Ref<NeModule> Parse(cs::List<uint8_t> f);
    static cs::String TypeName(int32_t rt);
    cs::Ref<NeModule_Resource> Find(cs::String type, cs::String name);
    cs::List<uint8_t> Data(cs::Ref<NeModule_Resource> r);
    cs::List<Tup_Form_Data_43d89> VbForms();
    cs::List<cs::Ref<RgbaImage>> FormPictures(cs::String form);
};

}  // namespace mc
