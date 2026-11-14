#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct WindowRect {
    double Left = 0;
    double Top = 0;
    double Width = 0;
    double Height = 0;
    int32_t State = 0;
    bool HasSize();
};

struct WindowLayout {
    static constexpr int32_t Count = 20;
    static constexpr int32_t Revision = 1;
    static constexpr int32_t TwipsPerPixel = 15;
    static constexpr double BaselineWidth = 800;
    static constexpr double BaselineHeight = 600;
    static constexpr double LargeFontDivisor = 1.25;
    static constexpr double AutomapSeedDivisor = 2.25;

    static cs::List<Tup_W_H_83d8d> BuildDefaults();
    inline static cs::List<Tup_W_H_83d8d> _defaults = BuildDefaults();

    static Tup_W_H_83d8d Default(int32_t window);
    static Tup_W_H_83d8d Default(GameWindow window);
    static Tup_W_H_83d8d Minimum(GameWindow window);
    static bool KeepsAspect(GameWindow window);
    static double AspectHeight(GameWindow window, double width);
    static Tup_X_Y_7f282 ScaleFor(double screenWidth, double screenHeight, bool largeFonts);
    static bool ListedInWindowControl(int32_t window);
    static bool PlacementSavedToWholeParty(GameWindow which);
    static bool AdjustedByControlPanel(int32_t window);
    static bool Prepare(cs::Ref<Character> leader, double scaleWidth, double scaleHeight);
    static Tup_Left_Top_c9dd6 KeptOnScreen(double left, double top, double screenWidth, double screenHeight);
    static cs::List<cs::Ref<WindowRect>> Fresh();
    static void Seed(cs::List<cs::Ref<WindowRect>> windows, double screenWidth, double screenHeight, bool largeFonts);
    static void Arrange(cs::List<cs::Ref<WindowRect>> windows, double frameWidth, double frameHeight,
                        double captionHeight, double frameBorderHeight, bool partyWidened = false);

    inline static cs::List<int32_t> SeededByCharacterCreation = cs::List<int32_t>::Of({0, 1, 10, 12, 11, 5, 16, 15, 19, 2, 3, 17, 13});

    static void AdjustToDisplay(cs::List<cs::Ref<WindowRect>> windows, double screenWidth, double screenHeight,
                                bool largeFonts, cs::Nullable<int32_t> only = {});
    static void Reset(cs::List<cs::Ref<WindowRect>> windows, cs::Nullable<int32_t> only = {});
};

struct MdiScroll_Child {
    double X = 0;
    double Y = 0;
    double Width = 0;
    double Height = 0;
    bool Shown = false;
    bool Minimised = false;
    bool Maximised = false;
    bool operator==(const MdiScroll_Child& o) const {
        return X == o.X && Y == o.Y && Width == o.Width && Height == o.Height && Shown == o.Shown &&
               Minimised == o.Minimised && Maximised == o.Maximised;
    }
    bool operator!=(const MdiScroll_Child& o) const { return !(*this == o); }
};

// result tuple (bool H, bool V, double MinX, double MaxX, double MinY, double MaxY) of MdiScroll.Bars
struct MdiScroll_BarsResult {
    bool H{};
    bool V{};
    double MinX{};
    double MaxX{};
    double MinY{};
    double MaxY{};
};

struct MdiScroll {
    using Child = MdiScroll_Child;
    static MdiScroll_BarsResult Bars(cs::List<MdiScroll_Child> children, double areaW, double areaH, double bar,
                                     cs::Nullable<Tup_X_Y_7f282> keep = {});
};

}  // namespace mc
