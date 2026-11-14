#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct HelpBundle_TabRow {
    double Start = 0;
    cs::List<double> Stops;
};

struct HelpBundle_Link {
    int32_t Start = 0;
    int32_t Length = 0;
    int32_t To = 0;
};

struct HelpBundle_Topic {
    cs::String Title{};
    cs::String Text{};
    cs::List<cs::Ref<HelpBundle_TabRow>> Tabs;
    cs::List<cs::Ref<HelpBundle_Link>> Links;
    cs::Nullable<bool> Contents{};
};

struct HelpBundle_Report {
    int32_t LinksInFile = 0;
    cs::List<cs::String> Unresolved;
    cs::List<cs::String> Misplaced;
};

struct HelpBundle {
    using TabRow = HelpBundle_TabRow;
    using Link = HelpBundle_Link;
    using Topic = HelpBundle_Topic;
    using Report = HelpBundle_Report;

    static int32_t ContentsIndex(cs::List<cs::Ref<HelpBundle_Topic>> topics);

    inline static cs::Ref<cs::JsonSerializerOptions> Json = [] { auto _o = cs::New<cs::JsonSerializerOptions>(); _o->WriteIndented = false; _o->DefaultIgnoreCondition = cs::Json::JsonIgnoreCondition::WhenWritingNull; return _o; }();

    static cs::String Serialize(cs::List<cs::Ref<HelpBundle_Topic>> topics);
    static cs::List<cs::Ref<HelpBundle_Topic>> Deserialize(cs::String json);

    // stand-ins while lines are put together: a tab between cells, and a jump's start, the end of
    // its target and its end
    static constexpr char Cell = '\x01';
    static constexpr char LinkOpen = '\x03';
    static constexpr char LinkName = '\x04';
    static constexpr char LinkClose = '\x05';

    static cs::List<cs::Ref<HelpBundle_Topic>> FromHelpFile(cs::Ref<WinHelpFile> hlp);
    static cs::List<cs::Ref<HelpBundle_Topic>> FromHelpFile(cs::Ref<WinHelpFile> hlp, cs::Ref<HelpBundle_Report>& report);
    static int32_t Twips(int32_t v);
    static int32_t Points(int32_t twips);
    static cs::String TextOf(cs::Ref<HelpTopicData> topic, cs::List<cs::Ref<HelpBundle_TabRow>> tabs, cs::Func<int32_t(cs::Ref<HelpRun>)> target, int32_t& linkCount);
    static cs::String Shown(cs::String s);
    static cs::String Cells(cs::List<cs::Ref<cs::StringBuilder>> cells);
    static cs::String TrimSpace(cs::String s);
    static cs::List<Tup_Item1_Item2_56bef> Paragraph(cs::Ref<HelpParagraphData> p, cs::Func<int32_t(cs::Ref<HelpRun>)> target, int32_t& count);
    static cs::Ref<HelpBundle_TabRow> TabRowOf(cs::Ref<HelpParagraphFormat> f, int32_t tabCount, bool& anySpan);
    static cs::Ref<HelpBundle_TabRow> Stops(int32_t start, cs::List<int32_t> widths, cs::Func<bool(int32_t)> own);
    static Tup_Item1_Item2_56bef TableRow(cs::Ref<HelpTableData> row, cs::Func<int32_t(cs::Ref<HelpRun>)> target, int32_t& count);
    static cs::List<Tup_Start_Length_To_550fe> Place(cs::String marked, cs::String plain, cs::String title, cs::List<cs::String> misplaced);
};

}  // namespace mc
