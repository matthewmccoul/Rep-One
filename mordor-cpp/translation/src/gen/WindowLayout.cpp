#include "core.h"

namespace mc {

bool WindowRect::HasSize() { return Width > 0 && Height > 0; }

cs::List<Tup_W_H_83d8d> WindowLayout::BuildDefaults() {
    auto d = cs::List<Tup_W_H_83d8d>::New(Count);
    auto Twips = [&](GameWindow w, int32_t width, int32_t height) {
        d[static_cast<int32_t>(w)] = Tup_W_H_83d8d{(double)width / TwipsPerPixel, (double)height / TwipsPerPixel};
    };

    Twips(GameWindow::Character, 4110, 5520);
    Twips(GameWindow::ObjectsAndSpells, 4185, 3510);
    Twips(GameWindow::Messages, 7125, 1560);
    Twips(GameWindow::CompanionStore, 6630, 4485);
    Twips(GameWindow::City, 7320, 3450);
    Twips(GameWindow::GeneralStore, 7290, 4785);
    Twips(GameWindow::Guild, 5340, 3825);
    Twips(GameWindow::Dungeon, 7320, 3450);
    Twips(GameWindow::Unnamed, 3900, 1875);
    Twips(GameWindow::Bank, 6315, 4350);
    Twips(GameWindow::Morgue, 3600, 2310);
    Twips(GameWindow::Group, 4485, 1890);
    Twips(GameWindow::Seer, 5145, 2160);
    // the automap's entry is written in pixels, not twips
    d[static_cast<int32_t>(GameWindow::Automap)] = Tup_W_H_83d8d{458, 493};
    return d;
}

Tup_W_H_83d8d WindowLayout::Default(int32_t window) {
    return window >= 0 && window < Count ? _defaults[window] : Tup_W_H_83d8d{0, 0};
}

Tup_W_H_83d8d WindowLayout::Default(GameWindow window) { return Default(static_cast<int32_t>(window)); }

Tup_W_H_83d8d WindowLayout::Minimum(GameWindow window) {
    auto _d = Default(window);
    double w = _d.W, h = _d.H;
    if (window == GameWindow::Character || window == GameWindow::ObjectsAndSpells || window == GameWindow::Messages ||
        window == GameWindow::Group || window == GameWindow::Dungeon)
        return Tup_W_H_83d8d{w / 2, h / 2};
    if (window == GameWindow::Automap) return Tup_W_H_83d8d{w / 4, h / 4};
    return Tup_W_H_83d8d{0, 0};
}

bool WindowLayout::KeepsAspect(GameWindow window) {
    return window == GameWindow::Character || window == GameWindow::ObjectsAndSpells || window == GameWindow::Group ||
           window == GameWindow::Dungeon;
}

double WindowLayout::AspectHeight(GameWindow window, double width) {
    auto _d = Default(window);
    double w = _d.W, h = _d.H;
    return w > 0 ? h * (width / w) : 0;
}

Tup_X_Y_7f282 WindowLayout::ScaleFor(double screenWidth, double screenHeight, bool largeFonts) {
    double f = largeFonts ? LargeFontDivisor : 1.0;
    double x = (screenWidth / BaselineWidth) / f;
    double y = (screenHeight / BaselineHeight) / f;
    return Tup_X_Y_7f282{cs::Math::Max(1.0, x), cs::Math::Max(1.0, y)};
}

bool WindowLayout::ListedInWindowControl(int32_t window) {
    if (window == static_cast<int32_t>(GameWindow::Unnamed)) return false;
    if (window == static_cast<int32_t>(GameWindow::Automap)) return false;
    if (window == static_cast<int32_t>(GameWindow::GeneralStore)) return false;
    if (window == static_cast<int32_t>(GameWindow::Morgue)) return false;
    if (window == static_cast<int32_t>(GameWindow::Guild)) return false;
    if (window == static_cast<int32_t>(GameWindow::Seer)) return false;
    if (window == static_cast<int32_t>(GameWindow::Bank)) return false;
    return true;
}

bool WindowLayout::PlacementSavedToWholeParty(GameWindow which) { return which != GameWindow::Automap; }

bool WindowLayout::AdjustedByControlPanel(int32_t window) {
    return window != static_cast<int32_t>(GameWindow::Unnamed) && window != static_cast<int32_t>(GameWindow::Automap);
}

bool WindowLayout::Prepare(cs::Ref<Character> leader, double scaleWidth, double scaleHeight) {
    auto table = leader->NormaliseWindows();
    bool blank = true;
    for (auto& r : table) if (r->HasSize()) { blank = false; break; }
    if (blank) Seed(table, scaleWidth, scaleHeight, false);
    for (auto& r : table) if (r->Left != 0 || r->Top != 0) return false;
    return true;
}

Tup_Left_Top_c9dd6 WindowLayout::KeptOnScreen(double left, double top, double screenWidth, double screenHeight) {
    return Tup_Left_Top_c9dd6{left > screenWidth ? 0 : left, top > screenHeight ? 0 : top};
}

cs::List<cs::Ref<WindowRect>> WindowLayout::Fresh() {
    auto w = cs::List<cs::Ref<WindowRect>>::New(Count);
    for (int32_t i = 0; i < Count; i++) w[i] = cs::New<WindowRect>();
    return w;
}

void WindowLayout::Seed(cs::List<cs::Ref<WindowRect>> windows, double screenWidth, double screenHeight, bool largeFonts) {
    auto _s = ScaleFor(screenWidth, screenHeight, largeFonts);
    double sx = _s.X, sy = _s.Y;
    for (int32_t i : SeededByCharacterCreation) {
        auto _d = Default(i);
        double w = _d.W, h = _d.H;
        windows[i]->Width = w * sx;
        windows[i]->Height = h * sy;
    }
    auto _a = Default(GameWindow::Automap);
    double aw = _a.W, ah = _a.H;
    windows[static_cast<int32_t>(GameWindow::Automap)]->Width = aw / AutomapSeedDivisor;
    windows[static_cast<int32_t>(GameWindow::Automap)]->Height = ah / AutomapSeedDivisor;
}

void WindowLayout::Arrange(cs::List<cs::Ref<WindowRect>> windows, double frameWidth, double frameHeight,
                           double captionHeight, double frameBorderHeight, bool partyWidened) {
    const double px = 1, fifty = 50.0 / TwipsPerPixel;
    auto ch = windows[static_cast<int32_t>(GameWindow::Character)];
    auto dn = windows[static_cast<int32_t>(GameWindow::Dungeon)];
    ch->Left = px; ch->Top = px; ch->State = 0;
    auto city = windows[static_cast<int32_t>(GameWindow::City)];
    city->Left = ch->Left + ch->Width + px; city->Top = px; city->State = 0;
    dn->Left = ch->Left + ch->Width + px; dn->Top = ch->Top;
    auto os = windows[static_cast<int32_t>(GameWindow::ObjectsAndSpells)];
    os->Left = dn->Left + dn->Width - os->Width;
    os->Top = dn->Top + dn->Height + fifty + px; os->State = 0;
    auto gr = windows[static_cast<int32_t>(GameWindow::Group)];
    gr->Left = px; gr->Top = ch->Height + px * 2; gr->State = 0;
    double below = (captionHeight + frameBorderHeight) * (px * 2.5);
    auto ms = windows[static_cast<int32_t>(GameWindow::Messages)];
    ms->Left = frameWidth - ms->Width - px * 8;
    ms->Top = frameHeight - below - ms->Height; ms->State = 0;
    auto am = windows[static_cast<int32_t>(GameWindow::Automap)];
    am->Left = ch->Left + ch->Width + px;
    // never into Objects & Spells beside it: on a small screen it goes only as far as there is room
    if (partyWidened) am->Left = cs::Math::Max(am->Left, cs::Math::Min(gr->Left + gr->Width + px, os->Left - am->Width - px));
    am->Top = dn->Top + dn->Height + fifty + px; am->State = 0;
}

void WindowLayout::AdjustToDisplay(cs::List<cs::Ref<WindowRect>> windows, double screenWidth, double screenHeight,
                                   bool largeFonts, cs::Nullable<int32_t> only) {
    auto _s = ScaleFor(screenWidth, screenHeight, largeFonts);
    double sx = _s.X, sy = _s.Y;
    for (int32_t i = 0; i < Count; i++) {
        if (only.has_value() && only.value() != i) continue;
        if (!AdjustedByControlPanel(i)) continue;
        auto _d = Default(i);
        double w = _d.W, h = _d.H;
        if (w <= 0) continue;
        windows[i]->State = 0;
        windows[i]->Width = w * sx;
        windows[i]->Height = h * sy;
    }
}

void WindowLayout::Reset(cs::List<cs::Ref<WindowRect>> windows, cs::Nullable<int32_t> only) {
    for (int32_t i = 0; i < Count; i++) {
        if (only.has_value() && only.value() != i) continue;
        auto _d = Default(i);
        double w = _d.W, h = _d.H;
        if (w <= 0) continue;
        windows[i]->State = 0;
        windows[i]->Width = w;
        windows[i]->Height = h;
    }
}

MdiScroll_BarsResult MdiScroll::Bars(cs::List<MdiScroll_Child> children, double areaW, double areaH, double bar,
                                     cs::Nullable<Tup_X_Y_7f282> keep) {
    double minX = 0, minY = 0, maxX = std::numeric_limits<double>::lowest(), maxY = std::numeric_limits<double>::lowest();
    for (auto& c : children) {
        if (!c.Shown || c.Minimised) continue;
        if (c.Maximised) return MdiScroll_BarsResult{false, false, 0, 0, 0, 0};
        minX = cs::Math::Min(minX, c.X); minY = cs::Math::Min(minY, c.Y);
        maxX = cs::Math::Max(maxX, c.X + c.Width); maxY = cs::Math::Max(maxY, c.Y + c.Height);
    }
    const double slack = 0.5;
    bool h = minX < -slack || maxX > areaW + slack;
    bool v = minY < -slack || maxY > areaH + slack;
    if (h && !v) v = maxY > areaH - bar + slack;
    if (v && !h) h = maxX > areaW - bar + slack;
    double rx = cs::Math::Max(0.0, maxX + (v ? bar : 0) - areaW);
    double ry = cs::Math::Max(0.0, maxY + (h ? bar : 0) - areaH);
    if (!h) minX = rx = 0;
    if (!v) minY = ry = 0;
    if (keep.has_value()) {
        double kx = keep.value().X, ky = keep.value().Y;
        if (cs::Math::Abs(kx) > slack) { h = true; minX = cs::Math::Min(minX, kx); rx = cs::Math::Max(rx, kx); }
        if (cs::Math::Abs(ky) > slack) { v = true; minY = cs::Math::Min(minY, ky); ry = cs::Math::Max(ry, ky); }
    }
    return MdiScroll_BarsResult{h, v, minX, rx, minY, ry};
}

}  // namespace mc
