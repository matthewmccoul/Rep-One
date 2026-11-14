#include "core.h"

namespace mc {

namespace {
// BitConverter / AsSpan with .NET's range checks
void WHF_Check(const cs::List<uint8_t>& b, int32_t at, int32_t n) {
    if (at < 0 || at >= b.Count()) throw cs::ArgumentOutOfRangeException("startIndex");
    if (at > b.Count() - n) throw cs::ArgumentException("Destination array is not long enough");
}
int32_t WHF_ToInt32(const cs::List<uint8_t>& b, int32_t at) { WHF_Check(b, at, 4); return cs::BinaryPrimitives::ReadInt32LittleEndian(b.Raw().data() + at); }
uint32_t WHF_ToUInt32(const cs::List<uint8_t>& b, int32_t at) { WHF_Check(b, at, 4); return cs::BinaryPrimitives::ReadUInt32LittleEndian(b.Raw().data() + at); }
int16_t WHF_ToInt16(const cs::List<uint8_t>& b, int32_t at) { WHF_Check(b, at, 2); return cs::BinaryPrimitives::ReadInt16LittleEndian(b.Raw().data() + at); }
uint16_t WHF_ToUInt16(const cs::List<uint8_t>& b, int32_t at) { WHF_Check(b, at, 2); return cs::BinaryPrimitives::ReadUInt16LittleEndian(b.Raw().data() + at); }
cs::List<uint8_t> WHF_Slice(const cs::List<uint8_t>& b, int32_t start, int32_t len) {
    if (start < 0 || len < 0 || (int64_t)start + len > b.Count()) throw cs::ArgumentOutOfRangeException("start");
    return cs::List<uint8_t>(std::vector<uint8_t>(b.Raw().begin() + start, b.Raw().begin() + start + len));
}
}  // namespace

cs::List<cs::String> WinHelpFile::InternalFiles() { return _files.Keys(); }

WinHelpFile::WinHelpFile(cs::List<uint8_t> f) {
    _f = f;
    if (f.Length() < 16 || WHF_ToInt32(f, 0) != Magic) throw cs::InvalidDataException("not a Windows help file");
    int32_t dir = WHF_ToInt32(f, 4);
    for (auto& kv : BTree(f, dir + 9, CString, 4))
        _files[*std::static_pointer_cast<cs::String>(kv.Key)] = WHF_ToInt32(f, kv.Value);
    auto sys = File_("|SYSTEM");
    if (WHF_ToUInt16(sys, 0) != 0x036C) throw cs::InvalidDataException("the help file's |SYSTEM does not read");
    Minor = WHF_ToUInt16(sys, 2);
    if (Minor > 16) {
        Flags = WHF_ToUInt16(sys, 10);
        for (int32_t p = 12; p + 4 <= sys.Length();) {
            int32_t type = WHF_ToUInt16(sys, p), len = WHF_ToUInt16(sys, p + 2);
            if (type == 1 && p + 4 + len <= sys.Length()) Title = cs::Str::TrimEnd(Latin(sys, p + 4, len), '\0');
            if (type == 3 && len >= 4 && p + 8 <= sys.Length()) ContentsOffset = WHF_ToInt32(sys, p + 4);
            p += 4 + len;
        }
    } else
        Title = cs::Str::TrimEnd(Latin(sys, 12, cs::Math::Max(0, sys.Length() - 12)), '\0');
}

cs::Ref<WinHelpFile> WinHelpFile::Read(cs::String path) { return cs::New<WinHelpFile>(cs::File::ReadAllBytes(path)); }

cs::Ref<WinHelpFile> WinHelpFile::Parse(cs::List<uint8_t> bytes) { return cs::New<WinHelpFile>(bytes); }

bool WinHelpFile::Has(cs::String name) { return _files.ContainsKey(name); }

cs::List<uint8_t> WinHelpFile::File_(cs::String name) {
    int32_t off{};
    if (!_files.TryGetValue(name, off)) throw cs::InvalidDataException("the help file has no " + name);
    int32_t used = WHF_ToInt32(_f, off + 4);
    if (cs::Wrap<int32_t>((int64_t)off + 9 + used) > _f.Length()) throw cs::InvalidDataException("the help file's " + name + " is cut short");
    return WHF_Slice(_f, off + 9, used);
}

cs::String WinHelpFile::Latin(cs::List<uint8_t> b, int32_t at, int32_t len) { return cs::Encoding::Latin1GetString(WHF_Slice(b, at, len)); }

Tup_Item1_Item2_f2a28 WinHelpFile::CString(cs::List<uint8_t> b, int32_t p) {
    int32_t e = p;
    while (e < b.Length() && b[e] != 0) e++;
    return Tup_Item1_Item2_f2a28{std::make_shared<cs::String>(Latin(b, p, e - p)), e + 1};
}

cs::List<Tup_Key_Value_d318a> WinHelpFile::BTree(cs::List<uint8_t> b, int32_t at, cs::Func<Tup_Item1_Item2_f2a28(cs::List<uint8_t>, int32_t)> key, int32_t valueSize) {
    if (WHF_ToUInt16(b, at) != 0x293B) throw cs::InvalidDataException("a help file B+ tree does not read");
    int32_t pageSize = WHF_ToUInt16(b, at + 4);
    int32_t root = WHF_ToInt16(b, at + 26);
    int32_t levels = WHF_ToInt16(b, at + 32);
    int32_t pages = at + 38;
    int32_t page = root;
    for (int32_t l = 1; l < levels; l++) page = WHF_ToInt16(b, pages + page * pageSize + 4);
    auto list = cs::List<Tup_Key_Value_d318a>::New();
    auto seen = cs::HashSet<int32_t>::New();
    while (page != -1 && seen.Add(page)) {
        int32_t p = pages + page * pageSize;
        int32_t n = WHF_ToInt16(b, p + 2);
        int32_t next = WHF_ToInt16(b, p + 6);
        p += 8;
        for (int32_t i = 0; i < n; i++) {
            auto kq = key(b, p);
            auto k = kq.Item1;
            auto q = kq.Item2;
            list.Add(Tup_Key_Value_d318a{k, q});
            p = q + valueSize;
        }
        page = next;
    }
    return list;
}

cs::Dictionary<uint32_t, int32_t> WinHelpFile::Contexts() {
    auto map = cs::Dictionary<uint32_t, int32_t>::New();
    if (!Has("|CONTEXT")) return map;
    auto c = File_("|CONTEXT");
    for (auto& kv : BTree(c, 0, [](cs::List<uint8_t> b, int32_t p) { return Tup_Item1_Item2_f2a28{std::make_shared<uint32_t>(WHF_ToUInt32(b, p)), p + 4}; }, 4))
        map[*std::static_pointer_cast<uint32_t>(kv.Key)] = WHF_ToInt32(c, kv.Value);
    return map;
}

cs::List<Tup_Offset_Title_63b9b> WinHelpFile::Titles() {
    auto list = cs::List<Tup_Offset_Title_63b9b>::New();
    if (!Has("|TTLBTREE")) return list;
    auto t = File_("|TTLBTREE");
    for (auto& kv : BTree(t, 0, [](cs::List<uint8_t> b, int32_t p) { int32_t e = p + 4; while (e < b.Length() && b[e] != 0) e++; return Tup_Item1_Item2_f2a28{std::make_shared<int32_t>(WHF_ToInt32(b, p)), e + 1}; }, 0)) {
        auto k = kv.Key;
        int32_t v = kv.Value;
        int32_t s = v - 1;
        while (s > 0 && t[s - 1] != 0) s--;
        list.Add(Tup_Offset_Title_63b9b{*std::static_pointer_cast<int32_t>(k), Latin(t, s, v - 1 - s)});
    }
    return list;
}

cs::List<uint8_t> WinHelpFile::Lz77(cs::List<uint8_t> b, int32_t start, int32_t end) {
    auto o = cs::List<uint8_t>::New();
    int32_t p = start;
    while (p < end) {
        int32_t flags = b[p++];
        for (int32_t bit = 0; bit < 8 && p < end; bit++, flags >>= 1) {
            if ((flags & 1) != 0) {
                if (p + 1 >= end) return o.ToArray();
                int32_t w = b[p] | (b[p + 1] << 8);
                p += 2;
                int32_t len = (w >> 12) + 3, from = o.Count() - ((w & 0xFFF) + 1);
                for (int32_t k = 0; k < len; k++) o.Add(from + k >= 0 ? o[from + k] : (uint8_t)0);
            } else
                o.Add(b[p++]);
        }
    }
    return o.ToArray();
}

cs::List<cs::Ref<HelpTopicData>> WinHelpFile::Topics() {
    auto topic = File_("|TOPIC");
    bool compressed = Minor > 16 && (Flags & 0xC) != 0;
    int32_t blockSize = Minor <= 16 ? 2048 : (Flags & 8) != 0 ? 2048 : 4096;
    // the decompressed blocks, end to end; a TOPICPOS is a block number and an offset in it
    int32_t blocks = (topic.Length() + blockSize - 1) / blockSize;
    auto data = cs::List<cs::List<uint8_t>>::New();
    for (int32_t b = 0; b < blocks; b++) {
        int32_t s = b * blockSize + 12, e = cs::Math::Min(topic.Length(), (b + 1) * blockSize);
        data.Add(s >= e ? cs::List<uint8_t>::New() : compressed ? Lz77(topic, s, e) : WHF_Slice(topic, s, e - s));
    }
    int32_t posShift = Minor <= 16 ? 0 : 14;  // 3.0 positions are plain file offsets
    auto stream = cs::List<uint8_t>::New();
    auto blockStart = cs::List<int32_t>::New(blocks);
    for (int32_t b = 0; b < blocks; b++) { blockStart[b] = stream.Count(); stream.AddRange(data[b]); }
    auto S = stream.ToArray();
    auto StreamPos = [&](int32_t topicPos) -> int32_t {
        if (posShift == 0) { int32_t blk = topicPos / blockSize; return blk < blocks ? blockStart[blk] + topicPos % blockSize - 12 : -1; }
        int32_t block = topicPos >> posShift, off = (topicPos & ((1 << posShift) - 1)) - 12;
        return block < blocks && off >= 0 ? blockStart[block] + off : -1;
    };
    auto BlockOf = [&](int32_t streamPos) -> int32_t {
        // Array.BinarySearch
        int32_t b = 0;
        {
            int32_t lo = 0, hi = blockStart.Length() - 1;
            bool found = false;
            while (lo <= hi) {
                int32_t i = lo + ((hi - lo) >> 1);
                int32_t c = blockStart[i] < streamPos ? -1 : (blockStart[i] > streamPos ? 1 : 0);
                if (c == 0) { b = i; found = true; break; }
                if (c < 0) lo = i + 1; else hi = i - 1;
            }
            if (!found) b = ~lo;
        }
        if (b < 0) b = ~b - 1;
        while (b + 1 < blocks && blockStart[b + 1] == blockStart[b] && blockStart[b] <= streamPos) b++;
        return b;
    };

    auto phrases = Phrases();
    auto topics = cs::List<cs::Ref<HelpTopicData>>::New();
    cs::Ref<HelpTopicData> cur = nullptr;
    int32_t pos = StreamPos(WHF_ToInt32(topic, 4));
    int32_t charBlock = -1, chars = 0;
    auto visited = cs::HashSet<int32_t>::New();
    while (pos >= 0 && pos + 21 <= S.Length() && visited.Add(pos)) {
        int32_t recSize = WHF_ToInt32(S, pos);
        int32_t dataLen2 = WHF_ToInt32(S, pos + 4);
        int32_t next = WHF_ToInt32(S, pos + 12);
        int32_t dataLen1 = WHF_ToInt32(S, pos + 16);
        int32_t type = S[pos + 20];
        if (recSize < 21 || dataLen1 < 21 || pos + recSize > S.Length()) throw cs::InvalidDataException("a help topic record does not read");
        auto d1 = WHF_Slice(S, pos + 21, dataLen1 - 21);
        auto stored = WHF_Slice(S, pos + dataLen1, recSize - dataLen1);
        auto d2 = dataLen2 > stored.Length() ? Unphrase(stored, dataLen2, phrases) : stored;

        int32_t block = BlockOf(pos);
        if (block != charBlock) { charBlock = block; chars = 0; }
        int32_t offset = (block << 15) + chars;
        if (type == 2) {
            int32_t e = 0;
            while (e < d2.Length() && d2[e] != 0) e++;
            cur = cs::New<HelpTopicData>(offset, Win1252(d2, 0, e));
            topics.Add(cur);
        } else if (type == 0x20 || type == 0x23) {
            if (cur == nullptr) { cur = cs::New<HelpTopicData>(offset, ""); topics.Add(cur); }
            chars += Display(d1, d2, type == 0x23, cur);
        }
        if (next == -1) break;
        pos = StreamPos(next);
    }
    return topics;
}

cs::List<cs::String> WinHelpFile::Phrases() {
    if (!Has("|Phrases")) return nullptr;
    auto p = File_("|Phrases");
    int32_t n = WHF_ToUInt16(p, 0);
    bool packed = Minor > 16;
    int32_t offsets = packed ? 8 : 4;
    auto o = cs::List<int32_t>::New(n + 1);
    for (int32_t i = 0; i <= n; i++) o[i] = WHF_ToUInt16(p, offsets + i * 2);
    cs::List<uint8_t> text;
    int32_t baseAt;
    if (packed) {
        int32_t start = offsets + (n + 1) * 2;
        text = Lz77(p, start, p.Length());
        baseAt = o[0];
    } else { text = p; baseAt = 0; }
    auto list = cs::List<cs::String>::New(n);
    for (int32_t i = 0; i < n; i++) {
        int32_t s = o[i] - baseAt, e = o[i + 1] - baseAt;
        list[i] = s >= 0 && e <= text.Length() && e >= s ? Win1252(text, s, e - s) : "";
    }
    return list;
}

cs::List<uint8_t> WinHelpFile::Unphrase(cs::List<uint8_t> b, int32_t size, cs::List<cs::String> phrases) {
    if (phrases == nullptr) return b;
    auto o = cs::List<uint8_t>::New();
    for (int32_t i = 0; i < b.Length() && o.Count() < size; i++) {
        int32_t c = b[i];
        if (c == 0 || c > 0x10 || i + 1 >= b.Length()) { o.Add((uint8_t)c); continue; }
        int32_t n = 256 * (c - 1) + b[++i];
        int32_t phrase = n / 2;
        if (phrase < phrases.Length()) {
            // Latin1.GetBytes turns the C# cp1252 chars above U+00FF into '?'
            for (char ch : phrases[phrase]) {
                uint8_t u = (uint8_t)ch;
                bool high = u >= 0x80 && u < 0xA0 && u != 0x81 && u != 0x8D && u != 0x8F && u != 0x90 && u != 0x9D;
                o.Add(high ? (uint8_t)'?' : u);
            }
        }
        if ((n & 1) != 0) o.Add((uint8_t)' ');
    }
    return o.ToArray();
}

int32_t WinHelpFile::ScanWord(cs::List<uint8_t> b, int32_t& p) {
    if ((b[p] & 1) != 0) { int32_t v = WHF_ToUInt16(b, p) >> 1; p += 2; return v; }
    return b[p++] >> 1;
}

int32_t WinHelpFile::ScanInt(cs::List<uint8_t> b, int32_t& p) {
    if ((b[p] & 1) != 0) { int32_t v = (WHF_ToUInt16(b, p) >> 1) - 0x4000; p += 2; return v; }
    return (b[p++] >> 1) - 0x40;
}

int64_t WinHelpFile::ScanLong(cs::List<uint8_t> b, int32_t& p) {
    if ((b[p] & 1) != 0) { int64_t v = (int64_t)(WHF_ToUInt32(b, p) >> 1) - 0x40000000LL; p += 4; return v; }
    int64_t w = (WHF_ToUInt16(b, p) >> 1) - 0x4000;
    p += 2;
    return w;
}

int32_t WinHelpFile::Display(cs::List<uint8_t> d1, cs::List<uint8_t> d2, bool table, cs::Ref<HelpTopicData> topic) {
    int32_t p = 0;
    ScanLong(d1, p);                        // topic size
    int32_t length = ScanWord(d1, p);       // characters in the record
    cs::Ref<HelpTableData> row = nullptr;
    if (table) {
        int32_t cols = d1[p++];
        int32_t tableType = d1[p++];
        if (tableType == 0 || tableType == 2) p += 2;  // minimum table width
        auto columns = cs::List<Tup_Gap_Width_b1cfb>::New(cols);
        // each column a pair of words; the first column's first word is the row's left edge
        for (int32_t c = 0; c < cols; c++) columns[c] = Tup_Gap_Width_b1cfb{WHF_ToInt16(d1, p + c * 4 + 2), WHF_ToInt16(d1, p + c * 4)};
        p += cols * 4;
        row = cs::New<HelpTableData>(columns);
        topic->Items.Add(row);
    }
    int32_t tp = 0;
    while (p < d1.Length()) {
        int32_t column = -1;
        if (table) {
            column = WHF_ToInt16(d1, p);
            p += 2;
            if (column == -1) break;
            p += 3;
        }
        p += 2;  // two bytes of nothing known
        p += 2;  // a paragraph id
        int32_t bits = WHF_ToUInt16(d1, p);
        p += 2;
        auto fmt = cs::New<HelpParagraphFormat>();
        if ((bits & 0x0001) != 0) ScanLong(d1, p);
        if ((bits & 0x0002) != 0) fmt->SpaceAbove = ScanInt(d1, p);
        if ((bits & 0x0004) != 0) fmt->SpaceBelow = ScanInt(d1, p);
        if ((bits & 0x0008) != 0) fmt->LineSpacing = ScanInt(d1, p);
        if ((bits & 0x0010) != 0) fmt->LeftIndent = ScanInt(d1, p);
        if ((bits & 0x0020) != 0) fmt->RightIndent = ScanInt(d1, p);
        if ((bits & 0x0040) != 0) fmt->FirstIndent = ScanInt(d1, p);
        if ((bits & 0x0100) != 0) p += 3;  // border: flags and width
        if ((bits & 0x0200) != 0) {
            int32_t n = ScanInt(d1, p);
            for (int32_t i = 0; i < n; i++) {
                int32_t stop = ScanWord(d1, p);
                int32_t kind = (stop & 0x4000) != 0 ? ScanWord(d1, p) : 0;
                fmt->TabStops.Add(Tup_Position_Kind_b5653{stop & 0x3FFF, kind});
            }
        }
        fmt->Alignment = (bits & 0x0400) != 0 ? 1 : (bits & 0x0800) != 0 ? 2 : 0;

        auto para = cs::New<HelpParagraphData>(fmt, column);
        AddParagraph(topic, row, para);
        bool done = false;
        while (!done) {
            int32_t s = tp;
            while (tp < d2.Length() && d2[tp] != 0) tp++;
            if (tp > s) para->Runs.Add(cs::New<HelpRun>(HelpRunKind::Text, Win1252(d2, s, tp - s)));
            tp++;
            if (p >= d1.Length()) return length;
            int32_t c = d1[p++];
            switch (c) {
                case 0xFF: done = true; break;
                case 0x20: p += 4; break;  // vfld
                case 0x21: p += 2; break;  // dtype
                case 0x80: p += 2; break;  // font change
                case 0x81: para->Runs.Add(cs::New<HelpRun>(HelpRunKind::LineBreak)); break;
                case 0x82:
                    para->Ended = true;
                    para = cs::New<HelpParagraphData>(fmt, column);
                    AddParagraph(topic, row, para);
                    break;
                case 0x83: para->Runs.Add(cs::New<HelpRun>(HelpRunKind::Tab)); break;
                case 0x86: case 0x87: case 0x88: {
                    int32_t kind = d1[p++];
                    int64_t size = ScanLong(d1, p);
                    if (kind == 0x22) ScanWord(d1, p);  // hotspot count
                    p += cs::Wrap<int32_t>(size);
                    para->Runs.Add(cs::New<HelpRun>(HelpRunKind::Picture));
                    break;
                }
                case 0x89: para->Runs.Add(cs::New<HelpRun>(HelpRunKind::LinkEnd)); break;
                case 0x8B: para->Runs.Add(cs::New<HelpRun>(HelpRunKind::Text, " ")); break;
                case 0x8C: para->Runs.Add(cs::New<HelpRun>(HelpRunKind::Text, "-")); break;
                case 0xC8: case 0xCC:
                    p += 2 + WHF_ToInt16(d1, p);
                    para->Runs.Add(cs::New<HelpRun>(HelpRunKind::LinkStart, cs::String(""), cs::Nullable<uint32_t>{}, cs::Nullable<int32_t>{}, false, true));
                    break;
                case 0xE0: case 0xE1:
                    para->Runs.Add(cs::New<HelpRun>(HelpRunKind::LinkStart, cs::String(""), cs::Nullable<uint32_t>{}, cs::Nullable<int32_t>(WHF_ToInt32(d1, p)), c == 0xE0));
                    p += 4;
                    break;
                case 0xE2: case 0xE3: case 0xE6: case 0xE7:
                    para->Runs.Add(cs::New<HelpRun>(HelpRunKind::LinkStart, cs::String(""), cs::Nullable<uint32_t>(WHF_ToUInt32(d1, p)), cs::Nullable<int32_t>{}, c == 0xE2 || c == 0xE6));
                    p += 4;
                    break;
                case 0xEA: case 0xEB: case 0xEE: case 0xEF: {
                    int32_t len = WHF_ToInt16(d1, p);
                    int32_t kind = d1[p + 2];
                    uint32_t hash = WHF_ToUInt32(d1, p + 3);
                    // kinds 4 and 6 name another help file: not a jump inside this one
                    para->Runs.Add(cs::New<HelpRun>(HelpRunKind::LinkStart, cs::String(""), (kind == 4 || kind == 6) ? cs::Nullable<uint32_t>{} : cs::Nullable<uint32_t>(hash), cs::Nullable<int32_t>{}, c == 0xEA || c == 0xEE, false, kind == 4 || kind == 6));
                    p += 2 + len;
                    break;
                }
                default:
                    throw cs::InvalidDataException("unknown help formatting command 0x" + cs::Format(c, "X2"));
            }
        }
        if (!table) break;
    }
    return length;
}

void WinHelpFile::AddParagraph(cs::Ref<HelpTopicData> topic, cs::Ref<HelpTableData> row, cs::Ref<HelpParagraphData> para) {
    if (row != nullptr) row->Cells.Add(para);
    else topic->Items.Add(para);
}

cs::String WinHelpFile::Win1252(cs::List<uint8_t> b, int32_t at, int32_t len) {
    auto c = cs::List<char>::New(len);
    for (int32_t i = 0; i < len; i++) {
        uint8_t x = b[at + i];
        c[i] = x >= 0x80 && x < 0xA0 ? Cp1252High[x - 0x80] : (char)x;
    }
    return cs::Str::FromChars(c);
}

HelpParagraphData::HelpParagraphData(cs::Ref<HelpParagraphFormat> format, int32_t column) { Format = format; Column = column; }

HelpTableData::HelpTableData(cs::List<Tup_Gap_Width_b1cfb> columns) { Columns = columns; }

HelpTopicData::HelpTopicData(int32_t offset, cs::String title) { Offset = offset; Title = title; }

}  // namespace mc
