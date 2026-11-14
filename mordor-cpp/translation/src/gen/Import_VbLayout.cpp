#include "core.h"

namespace mc {

// ---------------------------------------------------------------- VbField

VbField::VbField(cs::String name, VbKind kind, int32_t upper, int32_t stringLength, cs::Ref<VbType> type) {
    Name = name; Kind = kind; Upper = upper; StringLength = stringLength; Type = type;
}

int32_t VbField::Count() { return Upper < 0 ? 1 : Upper + 1; }

int32_t VbField::ElementSize() {
    switch (Kind) {
        case VbKind::Integer: return 2;
        case VbKind::Long: return 4;
        case VbKind::Single: return 4;
        case VbKind::Double: return 8;
        case VbKind::Currency: return 8;
        case VbKind::FixedString: return StringLength;
        default: return cs::deref(Type).Size;
    }
}

int32_t VbField::Size() { return Count() * ElementSize(); }

cs::String VbField::ToString() {
    cs::String kindText;
    if (Kind == VbKind::FixedString) kindText = "String * " + cs::ToStr(StringLength);
    else if (Kind == VbKind::Udt) kindText = cs::deref(Type).Name;
    else kindText = ToStr(Kind);
    return Name + (Upper >= 0 ? "(" + cs::ToStr(Upper) + ")" : cs::String("")) + " As " + kindText;
}

// ---------------------------------------------------------------- VbType

VbType::VbType(cs::String name, cs::List<cs::Ref<VbField>> fields) {
    Name = name;
    Fields = fields;
    int32_t at = 0;
    for (auto& f : fields) { f->Offset = at; at += f->Size(); }
    Size = at;
}

cs::Ref<VbField> VbType::Field(cs::String name) {
    auto r = Fields.FirstOrDefault([&](auto& f) { return f->Name == name; });
    if (!r) throw cs::ArgumentException(Name + " has no field " + name);
    return r;
}

Tup_Offset_Field_ed9f9 VbType::Locate(cs::String path) {
    cs::Ref<VbType> type = nullptr;
    VbType* self = this;
    int32_t offset = 0;
    cs::Ref<VbField> last = nullptr;
    bool first = true;
    for (auto& part : cs::Str::Split(path, '.')) {
        VbType* cur = first ? self : type.get();
        first = false;
        if (cur == nullptr) throw cs::ArgumentException(path + ": " + cs::deref(last).Name + " is not a record");
        cs::String name = part;
        int32_t index = 0;
        int32_t paren = cs::Str::IndexOf(part, '(');
        if (paren >= 0) {
            name = cs::Str::Substring(part, 0, paren);
            int32_t close = cs::Str::IndexOf(part, ')');
            index = cs::Parse::Parse<int32_t>(cs::Str::Substring(part, paren + 1, close - (paren + 1)));
        }
        auto f = cur->Field(name);
        if (index < 0 || index >= f->Count()) throw cs::ArgumentOutOfRangeException(path + ": " + f->Name + " is declared (" + cs::ToStr(f->Upper) + ")");
        offset += f->Offset + index * f->ElementSize();
        last = f;
        type = f->Type;
    }
    return Tup_Offset_Field_ed9f9{offset, last};
}

int32_t VbType::OffsetOf(cs::String path) { return Locate(path).Offset; }

// ---------------------------------------------------------------- VbRecord

VbRecord::VbRecord(cs::List<uint8_t> data, cs::Ref<VbType> type, int32_t base) { _data = data; Type = type; _base = base; }

cs::List<uint8_t> VbRecord::Bytes(int32_t offset, int32_t length) {
    int32_t at = _base + offset;
    if (at + length <= _data.Length()) {
        if (at < 0 || length < 0) throw cs::ArgumentOutOfRangeException("Bytes");
        return _data.GetRange(at, length);
    }
    auto padded = cs::List<uint8_t>::New(length);
    if (at < _data.Length()) {
        for (int32_t i = 0; i < _data.Length() - at; i++) padded[i] = _data[at + i];
    }
    return padded;
}

cs::List<uint8_t> VbRecord::At(cs::String path, VbKind want) {
    auto loc = cs::deref(Type).Locate(path);
    auto offset = loc.Offset;
    auto field = loc.Field;
    if (field->Kind != want) throw cs::InvalidOperationException(Type->Name + "." + path + " is " + ToStr(field->Kind) + ", read as " + ToStr(want));
    return Bytes(offset, field->ElementSize());
}

int16_t VbRecord::Integer(cs::String path) {
    auto b = At(path, VbKind::Integer);
    if (b.Count() < 2) throw cs::ArgumentOutOfRangeException("length");
    return cs::BinaryPrimitives::ReadInt16LittleEndian(b.Raw().data());
}

int32_t VbRecord::Long(cs::String path) {
    auto b = At(path, VbKind::Long);
    if (b.Count() < 4) throw cs::ArgumentOutOfRangeException("length");
    return cs::BinaryPrimitives::ReadInt32LittleEndian(b.Raw().data());
}

float VbRecord::Single(cs::String path) {
    auto b = At(path, VbKind::Single);
    if (b.Count() < 4) throw cs::ArgumentOutOfRangeException("length");
    return cs::BinaryPrimitives::ReadSingleLittleEndian(b.Raw().data());
}

double VbRecord::Double(cs::String path) {
    auto b = At(path, VbKind::Double);
    if (b.Count() < 8) throw cs::ArgumentOutOfRangeException("length");
    return cs::BinaryPrimitives::ReadDoubleLittleEndian(b.Raw().data());
}

VbCurrency VbRecord::Currency(cs::String path) {
    auto b = At(path, VbKind::Currency);
    if (b.Count() < 8) throw cs::ArgumentOutOfRangeException("length");
    return VbCurrency::FromRaw(cs::BinaryPrimitives::ReadInt64LittleEndian(b.Raw().data()));
}

cs::String VbRecord::FixedString(cs::String path) { return VbFile::Text(At(path, VbKind::FixedString)); }

VbRecord VbRecord::Sub(cs::String path) {
    auto loc = cs::deref(Type).Locate(path);
    auto offset = loc.Offset;
    auto field = loc.Field;
    if (field->Kind != VbKind::Udt) throw cs::InvalidOperationException(Type->Name + "." + path + " is not a record");
    return VbRecord(_data, field->Type, _base + offset);
}

bool VbRecord::IsZero() {
    for (auto& b : Bytes(0, cs::deref(Type).Size)) if (b != 0) return false;
    return true;
}

// ---------------------------------------------------------------- VbFile

VbFile::VbFile(cs::String path, int32_t recordLength, cs::List<uint8_t> data) { Path = path; RecordLength = recordLength; Data = data; }

cs::Ref<VbFile> VbFile::Open(cs::String path, int32_t recordLength) {
    if (!cs::File::Exists(path)) return nullptr;
    cs::List<uint8_t> data;
    {
        data = cs::File::ReadAllBytes(path);
    }
    return cs::New<VbFile>(path, recordLength, data);
}

int64_t VbFile::Length() { return Data.Length(); }

int32_t VbFile::RecordCount() { return cs::Wrap<int32_t>((int64_t)Data.Length() + RecordLength - 1) / RecordLength; }

int32_t VbFile::LastRecordLength() {
    return Data.Length() == 0 ? 0 : (int32_t)(Data.Length() - (int64_t)(RecordCount() - 1) * RecordLength);
}

cs::List<uint8_t> VbFile::Record(int32_t n) {
    auto r = cs::List<uint8_t>::New(RecordLength);
    int64_t at = (int64_t)(n - 1) * RecordLength;
    if (n >= 1 && at < Data.Length()) {
        int32_t count = (int32_t)cs::Math::Min((int64_t)RecordLength, (int64_t)Data.Length() - at);
        for (int32_t i = 0; i < count; i++) r[i] = Data[(int32_t)(at + i)];
    }
    return r;
}

VbRecord VbFile::Record(int32_t n, cs::Ref<VbType> type) { return VbRecord(Record(n), type); }

bool VbFile::Has(int32_t n) { return n >= 1 && (int64_t)(n - 1) * RecordLength < Data.Length(); }

int16_t VbFile::Integer(int32_t n) {
    auto b = Record(n);
    if (b.Count() < 2) throw cs::ArgumentOutOfRangeException("length");
    return cs::BinaryPrimitives::ReadInt16LittleEndian(b.Raw().data());
}

cs::String VbFile::VarString(int32_t n) {
    int32_t _discard = 0;
    return VarString(Record(n), 0, _discard);
}

cs::String VbFile::VarString(cs::List<uint8_t> record, int32_t at, int32_t& next) {
    int32_t len = 0;
    if (at + 2 <= record.Length()) {
        if (at < 0) throw cs::ArgumentOutOfRangeException("start");
        len = cs::BinaryPrimitives::ReadUInt16LittleEndian(record.Raw().data() + at);
    }
    len = cs::Math::Max(0, cs::Math::Min(len, record.Length() - at - 2));
    next = at + 2 + len;
    if (at + 2 < 0 || at + 2 + len > record.Length()) throw cs::ArgumentOutOfRangeException("start");
    return Text(record.GetRange(at + 2, len));
}

cs::String VbFile::Text(cs::List<uint8_t> bytes) { return cs::Encoding::Latin1GetString(bytes); }

cs::String VbFile::RTrim(cs::String s) { return cs::Str::TrimEnd(s, cs::String(" \0", 2)); }

// ---------------------------------------------------------------- VbLayout

cs::Ref<VbField> VbLayout::I(cs::String n, int32_t upper) { return cs::New<VbField>(n, VbKind::Integer, upper); }
cs::Ref<VbField> VbLayout::L(cs::String n, int32_t upper) { return cs::New<VbField>(n, VbKind::Long, upper); }
cs::Ref<VbField> VbLayout::S(cs::String n, int32_t upper) { return cs::New<VbField>(n, VbKind::Single, upper); }
cs::Ref<VbField> VbLayout::C(cs::String n, int32_t upper) { return cs::New<VbField>(n, VbKind::Currency, upper); }
cs::Ref<VbField> VbLayout::F(cs::String n, int32_t length) { return cs::New<VbField>(n, VbKind::FixedString, -1, length); }
cs::Ref<VbField> VbLayout::U(cs::String n, cs::Ref<VbType> t, int32_t upper) { return cs::New<VbField>(n, VbKind::Udt, upper, 0, t); }

cs::Ref<VbType> VbLayout::Slot = cs::New<VbType>("T576E", cs::List<cs::Ref<VbField>>::Of({
    I("Att"), I("Def"), I("Ref"), I("Id"), I("Alignment"), I("Charges"), I("Equipped"), I("IdLevel"), I("Cursed")}));

cs::Ref<VbType> VbLayout::Guild = cs::New<VbType>("T578A", cs::List<cs::Ref<VbField>>::Of({
    I("Level"), C("M579B"), I("M55CB"), I("M57A9"), I("M57B2"), S("Att"), S("Def")}));

cs::Ref<VbType> VbLayout::Companion = cs::New<VbType>("T57BF", cs::List<cs::Ref<VbField>>::Of({
    F("Name", 15), I("Ref"), I("M57D4"), I("Hits"), I("MaxHits"), I("Alignment"), I("Att"), I("Def"),
    I("BindingLevel"), I("IdLevel")}));

cs::Ref<VbType> VbLayout::Window = cs::New<VbType>("T57EF", cs::List<cs::Ref<VbField>>::Of({
    L("M425F"), L("M4258"), L("M4216"), L("M420C"), I("M5801")}));

cs::Ref<VbType> VbLayout::Sanctuary = cs::New<VbType>("T4E6B", cs::List<cs::Ref<VbField>>::Of({
    I("M4E78"), I("M4E7D"), I("Level")}));

cs::Ref<VbType> VbLayout::Character = cs::New<VbType>("T5810", cs::List<cs::Ref<VbField>>::Of({
    F("Name", 30), I("RaceIndex"), I("Alignment"), I("M5829"), S("Age"), I("SpellPoints"), I("DungeonLevel"),
    I("X"), I("Y"), S("Att"), S("Def"), I("Stats", 6), I("StatMods", 6), U("Pack", Slot, 40), U("M5860", Slot, 40),
    I("M586D", 35), I("M56F7"), S("M587C"), S("M588A"), I("Facing"), I("M58A6"), C("M579B"), C("Gold"),
    I("Hits"), I("MaxHits"), C("BankGold"), I("M56AF"), I("CurrentGuild"), U("GuildsJoined", Guild, 15),
    U("Companions", Companion, 4), I("M5700"), I("M58DC"), I("M58E9"), F("M58F9", 30), C("M5906", 8),
    I("M5914", 5), I("Ailments", 7), I("Resistances", 11), L("SpellEffects"), I("SpellResistances", 11),
    I("DeathState"), I("M5943"), F("M594C", 10), U("M5958", Window, 20), I("M5962"), L("M5970"),
    L("ItemEffects"), I("ItemResistances", 11), I("M5997", 1), I("M59A1"), I("M59B0", 10), I("M59BB"),
    I("M59CB"), I("M59D9"), U("Sanctuary", Sanctuary), I("M59F4", 2)}));

cs::Ref<VbType> VbLayout::Shelf = cs::New<VbType>("T5755", cs::List<cs::Ref<VbField>>::Of({
    I("M5766"), I("Id"), I("Align", 4)}));

cs::Ref<VbType> VbLayout::Foe = cs::New<VbType>("T4E8B", cs::List<cs::Ref<VbField>>::Of({
    I("Att"), I("Def"), I("Hits"), I("MaxHits"), I("Alignment"), I("Engaged"), I("Ref"), I("GroupCount"),
    S("M4EE6"), I("IdLevel"), I("M4EF9"), I("KnownAs")}));

cs::Ref<VbType> VbLayout::Fight = cs::New<VbType>("T4F0B", cs::List<cs::Ref<VbField>>::Of({
    I("TreasureClass"), I("M4F24"), C("Gold"), I("ChestTrap"), I("ChestLock")}));

cs::Ref<VbType> VbLayout::Library = cs::New<VbType>("T5BF1", cs::List<cs::Ref<VbField>>::Of({
    I("Id"), I("IdLevel"), C("M5C00"), F("M5C09", 30), I("M5C14"), L("M5C21"), F("M5C2F", 8), I("M5C3D")}));

cs::Ref<VbType> VbLayout::Hall = cs::New<VbType>("T73E9", cs::List<cs::Ref<VbField>>::Of({
    C("M73F5"), F("M73FE", 30), L("M7406")}));

cs::List<cs::Ref<VbType>> VbLayout::All = cs::List<cs::Ref<VbType>>::Of({
    Slot, Guild, Companion, Window, Sanctuary, Character, Shelf, Foe, Fight, Library, Hall});

bool VbLayout::VersionAccepted(cs::String stamp) {
    return stamp == "" || ((int32_t)stamp.size() > 4 ? cs::Str::Substring(stamp, 0, 4) : stamp) == Version;
}

}  // namespace mc
