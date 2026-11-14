#include "core.h"

namespace mc {

namespace {
// .NET's char.IsWhiteSpace / string.Trim set for Latin-1 (includes NEL and the non-breaking space)
const cs::String WhiteSpace = " \t\n\v\f\r\x85\xa0";
bool IsWhiteSpace(char c) { return WhiteSpace.find(c) != cs::String::npos; }
cs::String TrimWs(const cs::String& s) { return cs::Str::Trim(s, WhiteSpace); }
cs::String TrimEndWs(const cs::String& s) { return cs::Str::TrimEnd(s, WhiteSpace); }

// object items are std::shared_ptr<void>, which keeps no runtime type to test against
template <class T> cs::Ref<T> ItemAs(const std::shared_ptr<void>& o) {
    (void)o;
    throw cs::NotSupportedException("not translated: type test on an object item (shared_ptr<void> keeps no runtime type)");
}
}  // namespace

int32_t HelpBundle::ContentsIndex(cs::List<cs::Ref<HelpBundle_Topic>> topics) {
    for (int32_t i = 0; i < topics.Count(); i++) if (topics[i]->Contents.has_value() && topics[i]->Contents.value() == true) return i;
    return 0;
}

cs::String HelpBundle::Serialize(cs::List<cs::Ref<HelpBundle_Topic>> topics) {
    (void)topics;
    throw cs::NotSupportedException("not translated: System.Text.Json serialization");
}

cs::List<cs::Ref<HelpBundle_Topic>> HelpBundle::Deserialize(cs::String json) {
    (void)json;
    throw cs::NotSupportedException("not translated: System.Text.Json deserialization");
}

cs::List<cs::Ref<HelpBundle_Topic>> HelpBundle::FromHelpFile(cs::Ref<WinHelpFile> hlp) {
    cs::Ref<HelpBundle_Report> _;
    return FromHelpFile(hlp, _);
}

cs::List<cs::Ref<HelpBundle_Topic>> HelpBundle::FromHelpFile(cs::Ref<WinHelpFile> hlp, cs::Ref<HelpBundle_Report>& report) {
    auto data = hlp->Topics();
    // the topics' offsets, which jumps lead to: the title index has them, in topic order
    auto titles = hlp->Titles();
    auto starts = cs::List<int32_t>::New();
    for (int32_t i = 0; i < data.Count(); i++) starts.Add(i < titles.Count() ? titles[i].Offset : data[i]->Offset);
    auto contexts = hlp->Contexts();
    auto TopicAt = [&](int32_t offset) -> int32_t {
        int32_t best = -1;
        for (int32_t i = 0; i < starts.Count(); i++) if (starts[i] <= offset && (best < 0 || starts[i] >= starts[best])) best = i;
        return best;
    };
    cs::Func<int32_t(cs::Ref<HelpRun>)> Target = [&](cs::Ref<HelpRun> r) -> int32_t {
        if (r->Hash.has_value()) {
            uint32_t h = r->Hash.value();
            int32_t off{};
            return contexts.TryGetValue(h, off) ? TopicAt(off) : -1;
        }
        if (r->Offset.has_value()) { int32_t o = r->Offset.value(); return TopicAt(o); }
        return -1;
    };

    // a trailing record with no topic text at all (MORDOR.HLP ends with an untitled one) is no topic
    while (data.Count() > 0 && (int32_t)data[data.Count() - 1]->Title.size() == 0
           && !data[data.Count() - 1]->Items.Any([&](auto& o) {
                  auto p = ItemAs<HelpParagraphData>(o);
                  return p != nullptr && p->Runs.Any([&](auto& r) { return r->Kind == HelpRunKind::Text; });
              }))
        data.RemoveAt(data.Count() - 1);

    auto topics = cs::List<cs::Ref<HelpBundle_Topic>>::New();
    auto unresolved = cs::List<cs::String>::New();
    auto misplaced = cs::List<cs::String>::New();
    int32_t inFile = 0;
    for (auto& t : data) {
        auto tabs = cs::List<cs::Ref<HelpBundle_TabRow>>::New();
        int32_t linkCount = 0;
        cs::String text = TextOf(t, tabs, nullptr, linkCount);
        inFile += linkCount;
        int32_t _d = 0;
        cs::String marked = TextOf(t, cs::List<cs::Ref<HelpBundle_TabRow>>::New(), Target, _d);
        auto links = cs::List<cs::Ref<HelpBundle_Link>>::New();
        for (auto& pl : Place(marked, text, t->Title, misplaced)) {
            int32_t start = pl.Start, length = pl.Length, to = pl.To;
            if (to >= 0 && to < data.Count()) links.Add([&] { auto _o = cs::New<HelpBundle_Link>(); _o->Start = start; _o->Length = length; _o->To = to; return _o; }());
            else unresolved.Add(t->Title + ": \"" + cs::Str::Substring(text, start, length) + "\"");
        }
        topics.Add([&] {
            auto _o = cs::New<HelpBundle_Topic>();
            _o->Title = t->Title;
            _o->Text = text;
            _o->Tabs = tabs.Count() > 0 ? tabs : cs::List<cs::Ref<HelpBundle_TabRow>>(nullptr);
            _o->Links = links.Count() > 0 ? links : cs::List<cs::Ref<HelpBundle_Link>>(nullptr);
            return _o;
        }());
    }
    // the topic WinHelp opens the file at: |SYSTEM's Contents topic, or the first without one
    int32_t contents = hlp->ContentsOffset.has_value() ? TopicAt(hlp->ContentsOffset.value()) : 0;
    if (contents < 0 || contents >= topics.Count()) contents = 0;
    if (topics.Count() > 0) {
        auto _o = cs::New<HelpBundle_Topic>(*topics[contents]);
        _o->Contents = true;
        topics[contents] = _o;
    }
    report = [&] { auto _o = cs::New<HelpBundle_Report>(); _o->LinksInFile = inFile; _o->Unresolved = unresolved; _o->Misplaced = misplaced; return _o; }();
    return topics;
}

int32_t HelpBundle::Twips(int32_t v) { return v == 0 ? 0 : 10 * v - 3; }

int32_t HelpBundle::Points(int32_t twips) { return twips / 20; }

cs::String HelpBundle::TextOf(cs::Ref<HelpTopicData> topic, cs::List<cs::Ref<HelpBundle_TabRow>> tabs, cs::Func<int32_t(cs::Ref<HelpRun>)> target, int32_t& linkCount) {
    int32_t count = 0;
    // each paragraph or row: its line(s) as the export put them on the page, and its tab row
    auto raw = cs::List<Tup_Item1_Item2_56bef>::New();
    for (auto& item : topic->Items) {
        auto p = ItemAs<HelpParagraphData>(item);
        if (p != nullptr) {
            if (!p->Ended && p->Runs.Count() == 0) continue;      // a record's empty tail
            for (auto& line : Paragraph(p, target, count)) raw.Add(line);
        } else {
            auto row = ItemAs<HelpTableData>(item);
            if (row != nullptr) raw.Add(TableRow(row, target, count));
        }
        raw.Add(Tup_Item1_Item2_56bef{"", nullptr});           // the line break between two paragraphs
    }
    linkCount = count;

    auto lines = cs::List<cs::String>::New();
    auto rows = cs::List<cs::Ref<HelpBundle_TabRow>>::New();
    for (auto& rr : raw) {
        cs::String r = rr.Item1;
        auto row = rr.Item2;
        cs::String l = TrimWs(r);
        lines.Add((int32_t)cs::Str::Trim(TrimWs(l), Cell).size() == 0 ? cs::String("") : TrimEndWs(cs::Str::TrimEnd(TrimEndWs(l), Cell)));
        rows.Add(row);
    }
    auto outLines = cs::List<cs::String>::New();
    for (int32_t i = 0; i < lines.Count(); i++) {
        cs::String l = lines[i];
        if ((int32_t)l.size() == 0) {
            // the break between two paragraphs is not a blank paragraph, so the rows of one
            // table stay together instead of gaining a gap each
            bool separator = !cs::Str::Contains(raw[i].Item1, '\xa0') && !cs::Str::Contains(raw[i].Item1, Cell);
            if (separator && outLines.Count() > 0 && cs::Str::Contains(outLines[outLines.Count() - 1], '\t') && i + 1 < lines.Count() && cs::Str::Contains(lines[i + 1], Cell)) continue;
            if (outLines.Count() > 0 && (int32_t)outLines[outLines.Count() - 1].size() > 0) outLines.Add("");
        } else {
            if (cs::Str::Contains(l, Cell)) tabs.Add(rows[i] ? rows[i] : [&] { auto _o = cs::New<HelpBundle_TabRow>(); _o->Start = 0; _o->Stops = cs::Enumerable::Empty<double>(); return _o; }());
            outLines.Add(cs::Str::Replace(l, Cell, '\t'));
        }
    }
    return TrimWs(cs::Str::Join("\n", outLines));
}

cs::String HelpBundle::Shown(cs::String s) { return cs::Str::Replace(s, '\xa0', ' '); }

cs::String HelpBundle::Cells(cs::List<cs::Ref<cs::StringBuilder>> cells) {
    return cs::Str::Join(cs::String(1, Cell), cells.Select([&](auto& c) { return TrimSpace(c->ToString()); }));
}

cs::String HelpBundle::TrimSpace(cs::String s) { return TrimWs(s); }

cs::List<Tup_Item1_Item2_56bef> HelpBundle::Paragraph(cs::Ref<HelpParagraphData> p, cs::Func<int32_t(cs::Ref<HelpRun>)> target, int32_t& count) {
    auto runs = p->Runs;
    auto f = p->Format;
    // a tab that opens a hanging-indent line before any text is the indent, not a cell
    int32_t first = runs.FindIndex([&](auto& r) { return r->Kind == HelpRunKind::Text || r->Kind == HelpRunKind::Tab; });
    bool skipFirstTab = f->FirstIndent < 0 && first >= 0 && runs[first]->Kind == HelpRunKind::Tab && f->TabStops.Count() == 0
                        && runs.Skip(first + 1).Any([&](auto& r) { return r->Kind == HelpRunKind::Text && (int32_t)TrimWs(r->Text).size() > 0; });
    auto cells = cs::List<cs::Ref<cs::StringBuilder>>::Of({cs::New<cs::StringBuilder>()});
    auto lines = cs::List<Tup_Item1_Item2_56bef>::New();
    int32_t tabCount = 0;
    bool hasText = false;
    for (int32_t i = 0; i < runs.Count(); i++) {
        auto r = runs[i];
        switch (r->Kind) {
            case HelpRunKind::Text: cells[cells.Count() - 1]->Append(Shown(r->Text)); hasText = true; break;
            case HelpRunKind::Tab:
                if (skipFirstTab && i == first) break;
                cells.Add(cs::New<cs::StringBuilder>());
                tabCount++;
                break;
            case HelpRunKind::LinkStart:
                if (r->Macro || r->External) break;
                count++;
                if (target) cells[cells.Count() - 1]->Append(LinkOpen).Append(target(r)).Append(LinkName);
                break;
            case HelpRunKind::LinkEnd:
                if (target) cells[cells.Count() - 1]->Append(LinkClose);
                break;
            case HelpRunKind::LineBreak:
                lines.Add(Tup_Item1_Item2_56bef{cells.Count() > 1 ? Cells(cells) : cells[0]->ToString(), nullptr});
                cells = cs::List<cs::Ref<cs::StringBuilder>>::Of({cs::New<cs::StringBuilder>()});
                break;
            default: break;
        }
    }
    if (tabCount == 0) {
        // an empty paragraph was a non-breaking space on the page; a picture alone, nothing
        cs::String line = cells[0]->ToString();
        if (!hasText && !runs.Any([&](auto& r) { return r->Kind == HelpRunKind::Picture; })) line = "\xa0";
        lines.Add(Tup_Item1_Item2_56bef{line, nullptr});
        return lines;
    }
    bool anySpan = false;
    auto row = TabRowOf(f, tabCount, anySpan);
    // a line whose every span came out narrower than nothing (a stop behind the indent) was no
    // table to the bundle: its tabs are the two spaces the export wrote for each
    lines.Add(anySpan ? Tup_Item1_Item2_56bef{Cells(cells), row}
                      : Tup_Item1_Item2_56bef{cs::Str::Join("  ", cells.Select([&](auto& c) { return c->ToString(); })), nullptr});
    return lines;
}

cs::Ref<HelpBundle_TabRow> HelpBundle::TabRowOf(cs::Ref<HelpParagraphFormat> f, int32_t tabCount, bool& anySpan) {
    int32_t ti = Points(Twips(f->FirstIndent));
    int32_t start = Points(Twips(f->LeftIndent) + Twips(f->FirstIndent));
    auto widths = cs::List<int32_t>::New();
    int32_t prev = 0;
    for (int32_t i = 0; i < tabCount; i++) {
        int32_t stop = i < f->TabStops.Count()
            ? Points(Twips(f->TabStops[i].Position))
            : ((i == 0 ? start : prev) / 72 + 1) * 72;
        widths.Add(i == 0 ? stop - start : stop - prev - start);
        prev = stop;
    }
    anySpan = widths.Any([&](auto& w) { return w >= 0; });
    return Stops(start, widths, [&](int32_t i) { return i == 0 && ti < 0; });
}

cs::Ref<HelpBundle_TabRow> HelpBundle::Stops(int32_t start, cs::List<int32_t> widths, cs::Func<bool(int32_t)> own) {
    double at = start, defaultWidth = -1;
    auto stops = cs::List<double>::New();
    for (int32_t i = 0; i < widths.Count(); i++) {
        at += widths[i];
        double inch = at / 72;
        bool isDefault = !own(i) && at > 0 && (cs::Math::Abs(inch - cs::Math::Round(inch)) < 0.5 / 72 || widths[i] == defaultWidth);
        if (isDefault) defaultWidth = widths[i];
        stops.Add(isDefault ? -1 : at);
    }
    return [&] { auto _o = cs::New<HelpBundle_TabRow>(); _o->Start = start; _o->Stops = stops.ToArray(); return _o; }();
}

Tup_Item1_Item2_56bef HelpBundle::TableRow(cs::Ref<HelpTableData> row, cs::Func<int32_t(cs::Ref<HelpRun>)> target, int32_t& count) {
    auto cols = row->Columns;
    int32_t n = cols.Length();
    auto cells = cs::Enumerable::Range(0, n).Select([&](int32_t) { return cs::New<cs::StringBuilder>(); }).ToList();
    for (auto& p : row->Cells) {
        if (p->Column < 0 || p->Column >= n) continue;
        auto sb = cells[p->Column];
        for (auto& r : p->Runs) {
            switch (r->Kind) {
                case HelpRunKind::Text: sb->Append(Shown(r->Text)); break;
                case HelpRunKind::Tab: sb->Append(' '); break;
                case HelpRunKind::LinkStart:
                    if (r->Macro || r->External) break;
                    count++;
                    if (target) sb->Append(LinkOpen).Append(target(r)).Append(LinkName);
                    break;
                case HelpRunKind::LinkEnd: if (target) sb->Append(LinkClose); break;
                default: break;
            }
        }
    }
    int32_t g0 = cols[0].Gap, g1 = n > 1 ? cols[1].Gap : cols[0].Gap;
    int32_t start = g0 / 2 - g1 / 2;
    auto widths = cs::List<int32_t>::New();
    int32_t sum = 0, edge = 0;
    for (int32_t i = 0; i < n; i++) {
        sum += cols[i].Width + cols[i].Gap;
        int32_t next = (10 * sum + g0 + g1) / 20;
        widths.Add(next - (i == 0 ? 0 : edge) - start);
        edge = next;
    }
    // every cell is a span, the last one too, and each was written with its indent
    return Tup_Item1_Item2_56bef{Cells(cells), Stops(start, widths, [&](int32_t) { return true; })};
}

cs::List<Tup_Start_Length_To_550fe> HelpBundle::Place(cs::String marked, cs::String plain, cs::String title, cs::List<cs::String> misplaced) {
    auto found = cs::List<Tup_Start_Length_To_550fe>::New();
    int32_t j = 0, start = -1, to = -1;
    for (int32_t i = 0; i < (int32_t)marked.size(); i++) {
        char c = marked[i];
        if (c == LinkOpen) {
            int32_t end = cs::Str::IndexOf(marked, LinkName, i);
            to = cs::Parse::Parse<int32_t>(cs::Str::Substring(marked, i + 1, end - i - 1));
            i = end;
            if (start >= 0) misplaced.Add(title + ": a link inside another");
            start = j;
            continue;
        }
        if (c == LinkClose) {
            if (start < 0) { misplaced.Add(title + ": a link end with no start"); continue; }
            int32_t s = start, e = j;
            while (s < e && IsWhiteSpace(plain[s])) s++;
            while (e > s && IsWhiteSpace(plain[e - 1])) e--;
            if (e > s) found.Add(Tup_Start_Length_To_550fe{s, e - s, to});
            else misplaced.Add(title + ": an empty link");
            start = -1;
            continue;
        }
        if (j < (int32_t)plain.size() && plain[j] == c) { j++; continue; }
        if (IsWhiteSpace(c)) continue;
        misplaced.Add(title + ": the marked and plain texts part at " + cs::ToStr(j));
        return found;
    }
    if (start >= 0) misplaced.Add(title + ": a link with no end");
    if (j != (int32_t)plain.size()) misplaced.Add(title + ": the marked text ends early, at " + cs::ToStr(j) + " of " + cs::ToStr((int32_t)plain.size()));
    return found;
}

}  // namespace mc
