#include "core.h"

namespace mc {

namespace {
// Encoding.ASCII.GetString(b, at, n): non-ASCII bytes become '?'
cs::String AsciiString(const cs::List<uint8_t>& b, int32_t at, int32_t n) {
    if (at < 0 || n < 0 || (int64_t)at + n > b.Count()) throw cs::ArgumentOutOfRangeException("count");
    cs::String s;
    for (int32_t k = 0; k < n; k++) {
        uint8_t c = b.Raw()[(size_t)(at + k)];
        s += c > 0x7F ? '?' : (char)c;
    }
    return s;
}
}  // namespace

NeModule::NeModule(cs::List<uint8_t> bytes, cs::List<cs::Ref<NeModule_Resource>> resources) {
    Bytes = bytes;
    Resources = resources;
}

cs::Ref<NeModule> NeModule::Read(cs::String path) { return Parse(cs::File::ReadAllBytes(path)); }

cs::Ref<NeModule> NeModule::Parse(cs::List<uint8_t> f) {
    if (f.Length() < 0x40 || f[0] != 'M' || f[1] != 'Z') throw cs::InvalidDataException("not an MZ program");
    int32_t ne = cs::BinaryPrimitives::ReadInt32LittleEndian(f.Raw().data() + 0x3C);
    if (ne <= 0 || cs::Wrap<int32_t>((int64_t)ne + 0x40) > f.Length() || f[ne] != 'N' || f[ne + 1] != 'E') throw cs::InvalidDataException("not a 16-bit Windows (NE) program");
    auto U16 = [&](int32_t at) -> int32_t {
        if (at + 1 < f.Length()) return f[at] | (f[at + 1] << 8);
        throw cs::InvalidDataException("the resource table runs off the end of the file");
    };
    int32_t table = ne + U16(ne + 0x24);
    int32_t shift = U16(table);
    auto NameOf = [&](int32_t id) -> cs::String {
        if ((id & 0x8000) != 0) return "#" + cs::ToStr(id & 0x7FFF);
        int32_t at = table + id;
        if (at >= f.Length() || at + 1 + f[at] > f.Length()) throw cs::InvalidDataException("a resource name runs off the end of the file");
        return AsciiString(f, at + 1, f[at]);
    };
    auto list = cs::List<cs::Ref<NeModule_Resource>>::New();
    int32_t p = table + 2;
    while (U16(p) != 0) {
        cs::String type = NameOf(U16(p));
        int32_t count = U16(p + 2);
        p += 8;
        for (int32_t i = 0; i < count; i++, p += 12) {
            int32_t off = cs::Wrap<int32_t>((int64_t)((uint32_t)U16(p) << (shift & 31)));
            int32_t len = cs::Wrap<int32_t>((int64_t)((uint32_t)U16(p + 2) << (shift & 31)));
            if (off < 0 || off >= f.Length()) continue;
            auto nm = NameOf(U16(p + 6));
            list.Add([&] { auto _o = cs::New<NeModule_Resource>(); _o->Type = type; _o->Name = nm; _o->Offset = off; _o->Length = cs::Math::Min(len, f.Length() - off); return _o; }());
        }
    }
    return cs::New<NeModule>(f, list);
}

cs::String NeModule::TypeName(int32_t rt) { return "#" + cs::ToStr(rt); }

cs::Ref<NeModule_Resource> NeModule::Find(cs::String type, cs::String name) {
    return Resources.FirstOrDefault([&](auto& r) { return cs::Str::EqualsIgnoreCase(r->Type, type) && cs::Str::EqualsIgnoreCase(r->Name, name); });
}

cs::List<uint8_t> NeModule::Data(cs::Ref<NeModule_Resource> r) {
    auto& rr = cs::deref(r);
    if (rr.Offset < 0 || rr.Length < 0 || (int64_t)rr.Offset + rr.Length > Bytes.Length()) throw cs::ArgumentOutOfRangeException("start");
    return Bytes.GetRange(rr.Offset, rr.Length);
}

cs::List<Tup_Form_Data_43d89> NeModule::VbForms() {
    auto rc = Resources.Where([&](auto& r) { return r->Type == "#" + cs::ToStr(RtRcData) && cs::Str::StartsWith(r->Name, '#'); })
                  .ToDictionary([&](auto& r) { return cs::Parse::Parse<int32_t>(cs::Str::Substring(r->Name, 1)); }, [&](auto& r) { return r; });
    auto forms = cs::List<Tup_Form_Data_43d89>::New();
    for (auto& kv : rc.Pairs().OrderBy([&](auto& kv) { return kv.Key; })) {
        auto id = kv.Key;
        auto data = kv.Value;
        cs::Ref<NeModule_Resource> names{};
        if (!rc.TryGetValue(cs::Wrap<int32_t>((int64_t)id + 1), names) || names->Length == 0) continue;
        int32_t len = Bytes[names->Offset];
        if (len == 0 || len > names->Length - 1) continue;
        cs::String first = AsciiString(Bytes, names->Offset + 1, len);
        if (cs::Str::StartsWith(first, "frm")) forms.Add(Tup_Form_Data_43d89{first, data});
    }
    return forms;
}

cs::List<cs::Ref<RgbaImage>> NeModule::FormPictures(cs::String form) {
    auto _t = VbForms().FirstOrDefault([&](auto& f) { return cs::Str::EqualsIgnoreCase(f.Form, form); });
    auto data = _t.Data;
    auto found = cs::List<cs::Ref<RgbaImage>>::New();
    if (data == nullptr) return found;
    int32_t end = data->Offset + data->Length;
    for (int32_t i = data->Offset; i + 54 <= end; i++) {
        if (Bytes[i] != 'B' || Bytes[i + 1] != 'M' || cs::BinaryPrimitives::ReadInt32LittleEndian(Bytes.Raw().data() + i + 14) != 40) continue;
        int32_t size = cs::BinaryPrimitives::ReadInt32LittleEndian(Bytes.Raw().data() + i + 2);
        if (size < 54 || cs::Wrap<int32_t>((int64_t)i + size) > end) continue;
        cs::Ref<RgbaImage> img;
        try { img = Dib::DecodeBmpFile(Bytes, i); }
        catch (const cs::InvalidDataException&) { continue; }
        found.Add(img);
        i = cs::Wrap<int32_t>((int64_t)i + size - 1);
    }
    return found;
}

}  // namespace mc
