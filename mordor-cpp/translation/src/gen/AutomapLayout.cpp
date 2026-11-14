#include "core.h"

namespace mc {

bool AutomapLayout::Narrow(double formWidth) { return formWidth < WindowLayout::Default(GameWindow::Automap).W; }

bool AutomapLayout::Shallow(double formHeight) { return formHeight < WindowLayout::Default(GameWindow::Automap).H; }

bool AutomapLayout::ColumnNumbers(double formWidth, double formHeight) { return !Narrow(formWidth) && !Shallow(formHeight); }

bool AutomapLayout::RowNumbers(double formWidth, double formHeight) { return !Shallow(formHeight) && !Narrow(formWidth); }

double AutomapLayout::Across(double formWidth, double room, int32_t levelWidth, cs::Nullable<int32_t> x) {
    if (!Narrow(formWidth)) return 0;
    double at = (x.has_value() ? (double)x.value() : (levelWidth / 2.0 - 1)) * Pitch + Cell / 4.0;
    return cs::Math::Round(room / 2 - at);
}

double AutomapLayout::Down(double formHeight, double room, int32_t levelHeight, cs::Nullable<int32_t> y) {
    if (!Shallow(formHeight)) return 0;
    double yy = y.has_value() ? (double)y.value() : (levelHeight / 2.0 - 1);  // VB3 "/" is a float division
    double at = (levelHeight - 1 - yy) * Pitch + Cell / 4.0;
    return cs::Math::Round(room / 2 - at);
}

int32_t AutomapLayout::OpeningLevel(int32_t dungeonLevel, bool lostLevel) {
    return !lostLevel && dungeonLevel != 0 ? dungeonLevel : 1;
}

bool AutomapLayout::CursorRecentres(int32_t dungeonLevel, bool lost, int32_t shownLevel) {
    return dungeonLevel > 0 && !lost && dungeonLevel == shownLevel;
}

bool AutomapLayout::CityShownAtStart(int32_t dungeonLevel, bool dead) { return dungeonLevel == 0 && !dead; }

bool AutomapLayout::CityLoadsOnReturn(bool dead, bool exitedToMainMenu) { return !dead && !exitedToMainMenu; }

double AutomapLayout::Zoom(double roomW, double roomH, double mapW, double mapH, double renderScaling) {
    if (!(mapW > 0) || !(mapH > 0) || !(roomW > 0) || !(roomH > 0)) return 1;
    double rs = renderScaling > 0 && !std::isinf(renderScaling) ? renderScaling : 1;
    double fit = cs::Math::Min((roomW - SnapSlack / rs) / mapW, (roomH - SnapSlack / rs) / mapH);
    return fit > 1 ? fit : 1;
}

cs::List<int32_t> AutomapLayout::DeviceEdges(int32_t pixels, double scale) {
    if (pixels < 0) pixels = 0;
    double s = scale > 0 && !std::isinf(scale) ? scale : 1;
    double step = Pitch * s;
    int32_t p = cs::Math::Max(Pitch, cs::CastDouble<int32_t>(cs::Math::Floor(step + 1e-9)));
    auto off = cs::List<int32_t>::New(Pitch + 1);
    for (int32_t o = 0; o <= Pitch; o++) off[o] = cs::CastDouble<int32_t>(cs::Math::Floor(o * (double)p / Pitch + 0.5));
    int32_t wide = WideOffset(off);
    auto edges = cs::List<int32_t>::New(pixels + 1);
    for (int32_t i = 0; i <= pixels; i++)
    {
        int32_t c = i / Pitch, o = i % Pitch;
        int32_t start = cs::CastDouble<int32_t>(cs::Math::Floor(c * step + 0.5));
        int32_t extra = cs::CastDouble<int32_t>(cs::Math::Floor((c + 1) * step + 0.5)) - start - p;
        edges[i] = start + off[o] + (o > wide ? extra : 0);
    }
    return edges;
}

int32_t AutomapLayout::WideOffset(cs::List<int32_t> off) {
    int32_t best = Pitch / 2, bestW = INT32_MAX, bestD = INT32_MAX;
    for (int32_t o = 1; o < Pitch; o++)
    {
        int32_t w = off[o + 1] - off[o], d = cs::Math::Abs(o - Pitch / 2);
        if (w < bestW || (w == bestW && d < bestD)) { best = o; bestW = w; bestD = d; }
    }
    return best;
}

double AutomapLayout::Snap(double v, double renderScaling) {
    double rs = renderScaling > 0 && !std::isinf(renderScaling) ? renderScaling : 1;
    return cs::Math::Round(v * rs) / rs;
}

double AutomapLayout::Centred(double room, double size, double renderScaling) {
    double rs = renderScaling > 0 && !std::isinf(renderScaling) ? renderScaling : 1;
    return cs::Math::Max(0.0, cs::Math::Round((room - size) / 2 * rs) / rs);
}

double AutomapLayout::Centred(double room, double size, double lead, double tail, double renderScaling) {
    double rs = renderScaling > 0 && !std::isinf(renderScaling) ? renderScaling : 1;
    double at = (room - (size - lead - tail)) / 2 - lead;
    at = cs::Math::Min(at, room - size);
    return cs::Math::Max(0.0, cs::Math::Round(at * rs) / rs);
}

Tup_X_Y_7f282 AutomapLayout::ToMap(double x, double y, double left, double top, double scale) {
    double s = scale > 0 ? scale : 1;
    return Tup_X_Y_7f282{(x - left) / s, (y - top) / s};
}

bool AutomapLayout::OnMaze(double mapX, double mapY, double gutterLeft, int32_t mazeW, int32_t mazeH) {
    return mapX >= gutterLeft && mapX < gutterLeft + mazeW && mapY >= 0 && mapY < mazeH;
}

Tup_X_Y_9ed49 AutomapLayout::SquareAt(double mazeX, double mazeY, int32_t levelWidth, int32_t levelHeight) {
    int32_t x = cs::Math::Clamp<int32_t>(cs::CastDouble<int32_t>(cs::Math::Floor(mazeX / Pitch)), 0, cs::Math::Max(0, levelWidth - 1));
    int32_t row = cs::Math::Clamp<int32_t>(cs::CastDouble<int32_t>(cs::Math::Floor(mazeY / Pitch)), 0, cs::Math::Max(0, levelHeight - 1));
    return Tup_X_Y_9ed49{x, levelHeight - 1 - row};
}

}  // namespace mc
