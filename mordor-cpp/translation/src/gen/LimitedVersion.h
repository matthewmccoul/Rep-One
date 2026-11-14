#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct LimitedVersion {
    inline static bool On = false;

    static void Apply(cs::Ref<GameFilesCheck> check);

    inline static Tup_Date_Price_b4588 _program{};

    static const int32_t LastLevel;
    static const int32_t DeepestLevel;

    static cs::String FrameCaption();
    static bool SaysGoodbye();
    static bool ShowsOrderingInfo();
    static bool ShowsPublicBox();

    inline static const cs::String PublicBoxTitle = "Public Version";
    inline static const cs::String PublicBoxText =
        cs::String("This is the PUBLIC version of Mordor 1.1, so please feel free to give it to your friends and distribute it as you wish!\n\n")
        + "Order the full commercial release now for hundreds of additional monsters, items and over a dozen more levels to explore!\n\n"
        + "For information on Ordering, see the About/Ordering Information section from the Main Menu!";

    static bool ShowsLimitationNotice();

    inline static const cs::String LimitationNoticeTitle = "Limitation Notice";

    static cs::String LimitationNoticeText();
    static bool CloseBoxAsksToExit();
    static bool AboutReadsOrdering();

    inline static const cs::String ChuteCannotBeTaken = "You cannot take this Chute in the Limited Version.";
    inline static const cs::String ChuteDoesNothing = "There is a chute here, but it doesn't do anything in the Limited Version.";
    inline static const cs::String TeleporterGoesNowhere = "..but it doesn't go anywhere in the limited version...";

    static cs::String LevelDenied(int32_t level);
    static Tup_Level_Said_08d2b Clamp(int32_t level);
    static bool ChuteIsDeadToTake(int32_t level);
    static bool ChuteIsDeadUnderfoot(int32_t level);
    static bool TeleporterIsDead(int32_t z2);

    static cs::String LastEdited();

    inline static const cs::String FullLastEdited = "7/20/95";
    inline static const cs::String SharewareLastEdited = "7/6/95";

    static cs::String Price();

    inline static const cs::String FullPrice = "$39.99";
    inline static const cs::String SharewarePrice = "$39.95";

    static Tup_Date_Price_b4588 ReadProgram(cs::String gameDir);
    static Tup_Date_Price_b4588 FromProgram(cs::List<uint8_t> exe);

    inline static const cs::String OrderTitle = "Mordor Ordering Information";
    static cs::String OrderInfo();
    static cs::String OrderInfoAt(cs::String price);

    inline static const cs::String OrderHow =
        cs::String("To order, call TDA! at (800) 624-2101 between 9AM and 4PM PST Monday through Friday and ask to order MORDOR!  Be sure to specify either 3.5\" disks or CD-ROM.\n\n")
        + "You can also contact the company at the following EMail addresses:\n"
        + "Internet: 102033.242@compuserve.com\n"
        + "CompuServe: 102033,242\n"
        + "(Remember to include the proper Address & Credit Card information if you order via EMail!!)\n\n"
        + "Street address:\n"
        + "TDA!\n"
        + "720 132nd St SW #202\n"
        + "Everett, WA 98204\n"
        + "(206) 742-4145\n\n"
        + "Order the full version of MORDOR now & see just how far you can take adventure!";
};

}  // namespace mc
