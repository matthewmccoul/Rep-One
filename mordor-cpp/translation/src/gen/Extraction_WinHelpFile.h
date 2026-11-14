#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct WinHelpFile {
    static constexpr int32_t Magic = 0x00035F3F;

    cs::List<uint8_t> _f;
    cs::Dictionary<cs::String, int32_t> _files = cs::Dictionary<cs::String, int32_t>::New();

    int32_t Minor = 0;
    int32_t Flags = 0;
    cs::String Title = "";

    cs::Nullable<int32_t> ContentsOffset{};

    cs::List<cs::String> InternalFiles();

    WinHelpFile(cs::List<uint8_t> f);

    static cs::Ref<WinHelpFile> Read(cs::String path);
    static cs::Ref<WinHelpFile> Parse(cs::List<uint8_t> bytes);
    bool Has(cs::String name);
    cs::List<uint8_t> File_(cs::String name);
    static cs::String Latin(cs::List<uint8_t> b, int32_t at, int32_t len);
    static Tup_Item1_Item2_f2a28 CString(cs::List<uint8_t> b, int32_t p);
    static cs::List<Tup_Key_Value_d318a> BTree(cs::List<uint8_t> b, int32_t at, cs::Func<Tup_Item1_Item2_f2a28(cs::List<uint8_t>, int32_t)> key, int32_t valueSize);
    cs::Dictionary<uint32_t, int32_t> Contexts();
    cs::List<Tup_Offset_Title_63b9b> Titles();
    static cs::List<uint8_t> Lz77(cs::List<uint8_t> b, int32_t start, int32_t end);
    cs::List<cs::Ref<HelpTopicData>> Topics();
    cs::List<cs::String> Phrases();
    static cs::List<uint8_t> Unphrase(cs::List<uint8_t> b, int32_t size, cs::List<cs::String> phrases);
    static int32_t ScanWord(cs::List<uint8_t> b, int32_t& p);
    static int32_t ScanInt(cs::List<uint8_t> b, int32_t& p);
    static int64_t ScanLong(cs::List<uint8_t> b, int32_t& p);
    static int32_t Display(cs::List<uint8_t> d1, cs::List<uint8_t> d2, bool table, cs::Ref<HelpTopicData> topic);
    static void AddParagraph(cs::Ref<HelpTopicData> topic, cs::Ref<HelpTableData> row, cs::Ref<HelpParagraphData> para);

    // cp1252 chars above U+00FF have no Latin-1 form: kept as their cp1252 byte
    inline static cs::List<char> Cp1252High = cs::List<char>::Of({
        '\x80', '\x81', '\x82', '\x83', '\x84', '\x85', '\x86', '\x87', '\x88', '\x89', '\x8a', '\x8b', '\x8c', '\x8d', '\x8e', '\x8f',
        '\x90', '\x91', '\x92', '\x93', '\x94', '\x95', '\x96', '\x97', '\x98', '\x99', '\x9a', '\x9b', '\x9c', '\x9d', '\x9e', '\x9f'});

    static cs::String Win1252(cs::List<uint8_t> b, int32_t at, int32_t len);
};

struct HelpParagraphFormat {
    int32_t LeftIndent = 0, RightIndent = 0, FirstIndent = 0, SpaceAbove = 0, SpaceBelow = 0, LineSpacing = 0;
    int32_t Alignment = 0;
    cs::List<Tup_Position_Kind_b5653> TabStops = cs::List<Tup_Position_Kind_b5653>::New();
};

struct HelpRun {
    HelpRunKind Kind{};
    cs::String Text{};
    cs::Nullable<uint32_t> Hash{};
    cs::Nullable<int32_t> Offset{};
    bool Popup = false;
    bool Macro = false;
    bool External = false;

    HelpRun(HelpRunKind kind = {}, cs::String text = "", cs::Nullable<uint32_t> hash = {}, cs::Nullable<int32_t> offset = {}, bool popup = false, bool macro = false, bool external = false)
        : Kind(kind), Text(text), Hash(hash), Offset(offset), Popup(popup), Macro(macro), External(external) {}
};

struct HelpParagraphData : cs::Object {
    cs::Ref<HelpParagraphFormat> Format;
    int32_t Column = 0;
    cs::List<cs::Ref<HelpRun>> Runs = cs::List<cs::Ref<HelpRun>>::New();
    bool Ended = false;

    HelpParagraphData(cs::Ref<HelpParagraphFormat> format, int32_t column);
};

struct HelpTableData : cs::Object {
    cs::List<Tup_Gap_Width_b1cfb> Columns;
    cs::List<cs::Ref<HelpParagraphData>> Cells = cs::List<cs::Ref<HelpParagraphData>>::New();
    HelpTableData(cs::List<Tup_Gap_Width_b1cfb> columns);
};

struct HelpTopicData {
    int32_t Offset = 0;
    cs::String Title{};
    cs::List<cs::Ref<cs::Object>> Items = cs::List<cs::Ref<cs::Object>>::New();
    HelpTopicData(int32_t offset, cs::String title);
};

}  // namespace mc
