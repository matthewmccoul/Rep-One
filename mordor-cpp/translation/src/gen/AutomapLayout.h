#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct AutomapLayout {
    static constexpr int32_t Cell = 15;
    static constexpr int32_t Pitch = Cell - 1;
    static constexpr double SnapSlack = 4;

    static bool Narrow(double formWidth);
    static bool Shallow(double formHeight);
    static bool ColumnNumbers(double formWidth, double formHeight);
    static bool RowNumbers(double formWidth, double formHeight);
    static double Across(double formWidth, double room, int32_t levelWidth, cs::Nullable<int32_t> x);
    static double Down(double formHeight, double room, int32_t levelHeight, cs::Nullable<int32_t> y);
    static int32_t OpeningLevel(int32_t dungeonLevel, bool lostLevel);
    static bool CursorRecentres(int32_t dungeonLevel, bool lost, int32_t shownLevel);
    static bool CityShownAtStart(int32_t dungeonLevel, bool dead);
    static bool CityLoadsOnReturn(bool dead, bool exitedToMainMenu);
    static double Zoom(double roomW, double roomH, double mapW, double mapH, double renderScaling);
    static cs::List<int32_t> DeviceEdges(int32_t pixels, double scale);
    static int32_t WideOffset(cs::List<int32_t> off);
    static double Snap(double v, double renderScaling);
    static double Centred(double room, double size, double renderScaling);
    static double Centred(double room, double size, double lead, double tail, double renderScaling);
    static Tup_X_Y_7f282 ToMap(double x, double y, double left, double top, double scale);
    static bool OnMaze(double mapX, double mapY, double gutterLeft, int32_t mazeW, int32_t mazeH);
    static Tup_X_Y_9ed49 SquareAt(double mazeX, double mazeY, int32_t levelWidth, int32_t levelHeight);
};

}  // namespace mc
