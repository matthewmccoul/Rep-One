#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct FormRect {
    int32_t Left = 0;
    int32_t Top = 0;
    int32_t Width = 0;
    int32_t Height = 0;
    cs::String ToString();
    bool operator==(const FormRect& o) const { return Left == o.Left && Top == o.Top && Width == o.Width && Height == o.Height; }
    bool operator!=(const FormRect& o) const { return !(*this == o); }
};

struct ServiceForms_Control {
    cs::String Name{};
    int32_t Index = 0;
    FormRect Rect{};
    bool operator==(const ServiceForms_Control& o) const { return Name == o.Name && Index == o.Index && Rect == o.Rect; }
    bool operator!=(const ServiceForms_Control& o) const { return !(*this == o); }
};

struct ServiceForms_Guild {
    static constexpr int32_t Resource = 20;
    static constexpr int32_t ClientW = 5220;
    static constexpr int32_t ClientH = 3420;
    inline static FormRect Elastic = FormRect{-15, -15, 5250, 3450};
    inline static FormRect lblWelcome = FormRect{60, 45, 5100, 285};
    inline static FormRect GuildMasterCaption = FormRect{165, 420, 1710, 195};
    inline static FormRect lblGuildMaster = FormRect{165, 630, 3000, 240};
    inline static FormRect StatsCaption = FormRect{165, 900, 1710, 210};
    inline static FormRect StatsBox = FormRect{165, 1125, 3000, 585};
    inline static FormRect lblStatName0 = FormRect{75, 75, 390, 210};
    inline static FormRect lblStat0 = FormRect{45, 270, 375, 210};
    inline static FormRect lblWelcomeInfo = FormRect{165, 1740, 3000, 600};
    inline static FormRect GuildsCaption = FormRect{3375, 420, 1710, 195};
    inline static FormRect lstGuilds = FormRect{3360, 630, 1755, 1980};
    inline static FormRect cmdMake = FormRect{180, 2415, 1450, 300};
    inline static FormRect cmdInfo = FormRect{1725, 2415, 1450, 300};
    inline static FormRect cmdVisit = FormRect{3375, 2715, 1755, 285};
    inline static FormRect cmdJoin = FormRect{180, 2730, 1455, 300};
    inline static FormRect cmdLog = FormRect{1725, 2730, 1455, 300};
    inline static FormRect cmdLibrary = FormRect{180, 3060, 3000, 300};
    inline static FormRect cmdExit = FormRect{4050, 3045, 1080, 315};

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", 0, Elastic}, {"elaElastic", 1, StatsBox},
        {"lblWelcome", -1, lblWelcome}, {"Label2", 1, GuildMasterCaption},
        {"lblGuildMaster", -1, lblGuildMaster}, {"Label2", 0, StatsCaption},
        {"lblStatName", 0, lblStatName0}, {"lblStat", 0, lblStat0},
        {"lblWelcomeInfo", -1, lblWelcomeInfo}, {"Label2", 2, GuildsCaption},
        {"lstGuilds", -1, lstGuilds}, {"cmdMake", -1, cmdMake}, {"cmdInfo", -1, cmdInfo},
        {"cmdVisit", -1, cmdVisit}, {"cmdJoin", -1, cmdJoin}, {"cmdLog", -1, cmdLog},
        {"cmdLibrary", -1, cmdLibrary}, {"cmdExit", -1, cmdExit},
    });
};

struct ServiceForms_Bank {
    static constexpr int32_t Resource = 18;
    static constexpr int32_t ClientW = 6195;
    static constexpr int32_t ClientH = 3945;
    inline static FormRect Elastic = FormRect{0, -15, 6210, 3960};
    inline static FormRect Welcome = FormRect{1650, 30, 2940, 300};
    inline static FormRect AccountCaption = FormRect{105, 330, 1485, 210};
    inline static FormRect StatusBox = FormRect{120, 585, 5970, 300};
    inline static FormRect lblStatus = FormRect{75, 45, 5850, 225};
    inline static FormRect DepositCaption = FormRect{195, 990, 1485, 210};
    inline static FormRect DepositAmount = FormRect{210, 1245, 1845, 300};
    inline static FormRect DepositAll = FormRect{2130, 1245, 555, 300};
    inline static FormRect WithdrawCaption = FormRect{210, 1755, 1485, 210};
    inline static FormRect WithdrawAmount = FormRect{210, 2010, 1845, 300};
    inline static FormRect WithdrawAll = FormRect{2130, 2010, 555, 300};
    inline static FormRect PartyCaption = FormRect{105, 2475, 1485, 210};
    inline static FormRect PartyBox = FormRect{120, 2730, 2625, 780};
    inline static FormRect PoolDeposit = FormRect{60, 60, 1215, 300};
    inline static FormRect PartyDeposit = FormRect{1350, 60, 1215, 300};
    inline static FormRect PartyPoolDeposit = FormRect{60, 405, 2505, 300};
    inline static FormRect ItemsCaption = FormRect{2835, 990, 2115, 210};
    inline static FormRect ItemsBox = FormRect{2850, 1230, 3075, 2280};
    inline static FormRect lstItems = FormRect{60, 60, 2940, 1785};
    inline static FormRect cmdInfo = FormRect{45, 1920, 1530, 300};
    inline static FormRect cmdExit = FormRect{4425, 3585, 1530, 330};

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", 0, Elastic}, {"lblLabel", 0, Welcome}, {"lblLabel", 1, AccountCaption},
        {"elaElastic", 1, StatusBox}, {"lblStatus", -1, lblStatus},
        {"lblLabel", 2, DepositCaption}, {"txtAmount", 0, DepositAmount}, {"cmdCommand", 0, DepositAll},
        {"lblLabel", 3, WithdrawCaption}, {"txtAmount", 1, WithdrawAmount}, {"cmdCommand", 1, WithdrawAll},
        {"lblLabel", 4, PartyCaption}, {"elaElastic", 2, PartyBox}, {"cmdCommand", 2, PoolDeposit},
        {"cmdCommand", 3, PartyDeposit}, {"cmdCommand", 4, PartyPoolDeposit},
        {"lblLabel", 5, ItemsCaption}, {"elaElastic", 3, ItemsBox}, {"lstItems", -1, lstItems},
        {"cmdInfo", -1, cmdInfo}, {"cmdExit", -1, cmdExit},
    });
};

struct ServiceForms_Morgue {
    static constexpr int32_t Resource = 28;
    static constexpr int32_t ClientW = 3480;
    static constexpr int32_t ClientH = 1905;
    inline static FormRect Elastic = FormRect{0, -15, 3495, 1935};
    inline static FormRect Welcome = FormRect{45, 60, 3195, 330};
    inline static FormRect RaiseCharacter = FormRect{135, 465, 1590, 300};
    inline static FormRect HireRescuers = FormRect{1770, 465, 1590, 300};
    inline static FormRect GrabBody = FormRect{135, 810, 1590, 300};
    inline static FormRect RaiseCompanion = FormRect{135, 1170, 3225, 300};
    inline static FormRect cmdExit = FormRect{2265, 1560, 1095, 300};

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", -1, Elastic}, {"Label1", -1, Welcome}, {"cmdCommand", 0, RaiseCharacter},
        {"cmdCommand", 3, HireRescuers}, {"cmdCommand", 1, GrabBody}, {"cmdCommand", 2, RaiseCompanion},
        {"cmdExit", -1, cmdExit},
    });
};

struct ServiceForms_Confinement {
    static constexpr int32_t Resource = 60;
    static constexpr int32_t ClientW = 6510;
    static constexpr int32_t ClientH = 4080;
    inline static FormRect Elastic = FormRect{-30, -15, 6540, 4395};
    inline static FormRect BuyCaption = FormRect{3015, 75, 1575, 225};
    inline static FormRect BindCaption = FormRect{105, 75, 1575, 225};
    inline static FormRect IdCaption = FormRect{120, 1440, 1575, 405};
    inline static FormRect cmdExit = FormRect{5130, 3720, 1305, 330};
    inline static FormRect BindBox = FormRect{120, 330, 2805, 780};
    inline static FormRect BindComp = FormRect{30, 120, 570, 210};
    inline static FormRect txtBind = FormRect{660, 75, 2055, 285};
    inline static FormRect BindCost = FormRect{90, 450, 450, 210};
    inline static FormRect lblBindCost = FormRect{645, 435, 1170, 255};
    inline static FormRect cmdBind = FormRect{1920, 420, 795, 300};
    inline static FormRect IdBox = FormRect{120, 1890, 2760, 1785};
    inline static FormRect IdComp = FormRect{60, 120, 540, 210};
    inline static FormRect txtID = FormRect{630, 75, 2070, 285};
    inline static FormRect lblValueNum = FormRect{15, 450, 1065, 195};
    inline static FormRect lblValue = FormRect{1140, 420, 1545, 255};
    inline static FormRect cmdRealign0 = FormRect{90, 720, 255, 255};
    inline static FormRect cmdRealign1 = FormRect{330, 720, 255, 255};
    inline static FormRect cmdRealign2 = FormRect{570, 720, 255, 255};
    inline static FormRect cmdIDInfo = FormRect{1140, 720, 750, 300};
    inline static FormRect cmdSell = FormRect{1935, 720, 750, 300};
    inline static FormRect lblIDLevel = FormRect{45, 1140, 1845, 210};
    inline static FormRect cmdID = FormRect{1950, 1065, 720, 300};
    inline static FormRect IdCostCaption = FormRect{285, 1440, 780, 195};
    inline static FormRect lblIDCost = FormRect{1125, 1410, 1545, 255};
    inline static FormRect BuyBox = FormRect{3030, 315, 3390, 3360};
    inline static FormRect CreatureHead = FormRect{120, 30, 915, 195};
    inline static FormRect GneHead = FormRect{2370, 30, 585, 195};
    inline static FormRect elaComp0 = FormRect{90, 270, 2205, 225};  // the rows, each 225 below (FRMCITYC.BAS:498-503)
    inline static FormRect elaCompInfo0 = FormRect{2370, 270, 585, 225};
    inline static FormRect vsbComps = FormRect{3030, 255, 270, 2100};
    inline static FormRect elaAlignments = FormRect{420, 780, 1905, 690};
    inline static FormRect SelectAlignment = FormRect{165, 60, 1590, 195};
    inline static FormRect cmdAlign0 = FormRect{300, 285, 375, 300};
    inline static FormRect cmdAlign1 = FormRect{750, 285, 375, 300};
    inline static FormRect cmdAlign2 = FormRect{1200, 285, 375, 300};
    inline static FormRect CompanionCaption = FormRect{60, 2430, 930, 195};
    inline static FormRect lblSelectedComp = FormRect{1050, 2400, 2265, 255};
    inline static FormRect lblCost = FormRect{120, 2730, 870, 195};
    inline static FormRect lblSelectedCompCost = FormRect{1050, 2700, 990, 255};
    inline static FormRect cmdBuy = FormRect{2085, 2700, 600, 255};
    inline static FormRect cmdInfo = FormRect{2715, 2700, 600, 255};
    inline static FormRect SearchCaption = FormRect{135, 3045, 1605, 210};
    inline static FormRect txtSearch = FormRect{1710, 3000, 1605, 285};

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", 0, Elastic}, {"lblLabel", 0, BuyCaption}, {"lblLabel", 1, BindCaption},
        {"lblLabel", 2, IdCaption}, {"cmdExit", -1, cmdExit},
        {"elaElastic", 3, BindBox}, {"lblInfo", 6, BindComp}, {"txtBind", -1, txtBind},
        {"lblInfo", 0, BindCost}, {"lblBindCost", -1, lblBindCost}, {"cmdBind", -1, cmdBind},
        {"elaElastic", 2, IdBox}, {"lblLabel", 8, IdComp}, {"txtID", -1, txtID},
        {"lblValueNum", -1, lblValueNum}, {"lblValue", -1, lblValue},
        {"cmdRealign", 0, cmdRealign0}, {"cmdRealign", 1, cmdRealign1}, {"cmdRealign", 2, cmdRealign2},
        {"cmdIDInfo", -1, cmdIDInfo}, {"cmdSell", -1, cmdSell}, {"lblIDLevel", -1, lblIDLevel},
        {"cmdID", -1, cmdID}, {"lblLabel", 9, IdCostCaption}, {"lblIDCost", -1, lblIDCost},
        {"elaElastic", 1, BuyBox}, {"lblLabel", 3, CreatureHead}, {"lblLabel", 4, GneHead},
        {"elaComp", 0, elaComp0}, {"elaCompInfo", 0, elaCompInfo0}, {"vsbComps", -1, vsbComps},
        {"elaAlignments", -1, elaAlignments}, {"Label11", -1, SelectAlignment},
        {"cmdAlign", 0, cmdAlign0}, {"cmdAlign", 1, cmdAlign1}, {"cmdAlign", 2, cmdAlign2},
        {"lblLabel", 5, CompanionCaption}, {"lblSelectedComp", -1, lblSelectedComp},
        {"lblCost", -1, lblCost}, {"lblSelectedCompCost", -1, lblSelectedCompCost},
        {"cmdBuy", -1, cmdBuy}, {"cmdInfo", -1, cmdInfo}, {"lblLabel", 7, SearchCaption},
        {"txtSearch", -1, txtSearch},
    });
};

struct ServiceForms_Store {
    static constexpr int32_t Resource = 16;
    static constexpr int32_t ClientW = 7170;
    static constexpr int32_t ClientH = 4380;
    inline static FormRect Elastic = FormRect{-15, -15, 7200, 4410};
    inline static FormRect BuyCaption = FormRect{3060, 90, 1215, 240};
    inline static FormRect IdCaption = FormRect{135, 2280, 2550, 240};
    inline static FormRect UncurseCaption = FormRect{135, 90, 1365, 210};
    inline static FormRect CombineCaption = FormRect{135, 1155, 1410, 210};
    inline static FormRect cmdExit = FormRect{6015, 3930, 1050, 300};
    inline static FormRect UncurseBox = FormRect{120, 315, 2805, 810};
    inline static FormRect UncurseItem = FormRect{60, 150, 450, 180};
    inline static FormRect txtUncurse = FormRect{570, 105, 2160, 285};
    inline static FormRect UncurseCost = FormRect{60, 495, 450, 165};
    inline static FormRect lblUncurse = FormRect{570, 465, 1170, 255};
    inline static FormRect cmdUncurse = FormRect{1800, 465, 930, 255};
    inline static FormRect CombineBox = FormRect{120, 1365, 2805, 840};
    inline static FormRect CombineItem = FormRect{75, 135, 465, 180};
    inline static FormRect txtCombine = FormRect{570, 120, 2160, 285};
    inline static FormRect CombineItems = FormRect{75, 510, 465, 165};
    inline static FormRect lblCombine = FormRect{585, 480, 1290, 255};
    inline static FormRect cmdCombine = FormRect{1950, 480, 780, 255};
    inline static FormRect IdBox = FormRect{120, 2520, 2805, 1800};
    inline static FormRect IdItem = FormRect{30, 165, 510, 180};
    inline static FormRect txtID = FormRect{600, 120, 2130, 285};
    inline static FormRect lblValueNum = FormRect{45, 495, 1065, 195};
    inline static FormRect lblValue = FormRect{1170, 465, 1545, 255};
    inline static FormRect cmdRealign0 = FormRect{90, 765, 255, 255};
    inline static FormRect cmdRealign1 = FormRect{330, 765, 255, 255};
    inline static FormRect cmdRealign2 = FormRect{570, 765, 255, 255};
    inline static FormRect cmdIDInfo = FormRect{1170, 765, 750, 255};
    inline static FormRect cmdSell = FormRect{1965, 765, 750, 255};
    inline static FormRect lblIDLevel = FormRect{75, 1185, 1845, 210};
    inline static FormRect cmdID = FormRect{1980, 1155, 720, 255};
    inline static FormRect IdCostCaption = FormRect{315, 1485, 780, 195};
    inline static FormRect lblIDCost = FormRect{1155, 1455, 1545, 255};
    inline static FormRect BuyBox = FormRect{3045, 315, 4020, 3555};
    inline static FormRect ItemHead = FormRect{135, 75, 570, 180};
    inline static FormRect UgneHead = FormRect{2790, 75, 810, 195};
    inline static FormRect elaItem0 = FormRect{105, 300, 2655, 210};  // nine rows, each 210 below
    inline static FormRect elaItemInfo0 = FormRect{2835, 300, 780, 210};
    inline static FormRect vsbItems = FormRect{3690, 285, 270, 2115};
    inline static FormRect elaAlignments = FormRect{930, 1005, 1905, 675};
    inline static FormRect SelectAlignment = FormRect{90, 60, 1755, 195};
    inline static FormRect cmdAlign0 = FormRect{90, 285, 375, 300};
    inline static FormRect cmdAlign1 = FormRect{525, 285, 375, 300};
    inline static FormRect cmdAlign2 = FormRect{960, 285, 375, 300};
    inline static FormRect cmdAlign3 = FormRect{1425, 285, 375, 300};
    inline static FormRect SelectedItemCaption = FormRect{75, 2475, 510, 195};
    inline static FormRect elaSelectedItem = FormRect{675, 2460, 3000, 225};
    inline static FormRect lblCost = FormRect{90, 2775, 870, 195};
    inline static FormRect elaSelectedItemCost = FormRect{1080, 2760, 1275, 225};
    inline static FormRect cmdBuy = FormRect{2445, 2745, 600, 255};
    inline static FormRect cmdInfo = FormRect{3090, 2745, 600, 255};
    inline static FormRect SearchCaption = FormRect{150, 3210, 1605, 210};
    inline static FormRect txtSearch = FormRect{1725, 3165, 1995, 285};

    static FormRect ListArea();

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", 0, Elastic}, {"Label1", 0, BuyCaption}, {"Label1", 1, IdCaption},
        {"Label1", 2, UncurseCaption}, {"Label1", 3, CombineCaption}, {"cmdExit", -1, cmdExit},
        {"elaElastic", 1, UncurseBox}, {"Label2", 0, UncurseItem}, {"txtUncurse", -1, txtUncurse},
        {"Label3", -1, UncurseCost}, {"lblUncurse", -1, lblUncurse}, {"cmdUncurse", -1, cmdUncurse},
        {"elaElastic", 2, CombineBox}, {"Label2", 1, CombineItem}, {"txtCombine", -1, txtCombine},
        {"Label6", -1, CombineItems}, {"lblCombine", -1, lblCombine}, {"cmdCommand", 0, cmdCombine},
        {"elaElastic", 3, IdBox}, {"Label2", 2, IdItem}, {"txtID", -1, txtID},
        {"lblValueNum", -1, lblValueNum}, {"lblValue", -1, lblValue},
        {"cmdRealign", 0, cmdRealign0}, {"cmdRealign", 1, cmdRealign1}, {"cmdRealign", 2, cmdRealign2},
        {"cmdIDInfo", -1, cmdIDInfo}, {"cmdSell", -1, cmdSell}, {"lblIDLevel", -1, lblIDLevel},
        {"cmdID", -1, cmdID}, {"Label5", -1, IdCostCaption}, {"lblIDCost", -1, lblIDCost},
        {"elaElastic", 4, BuyBox}, {"Label9", 0, ItemHead}, {"Label9", 1, UgneHead},
        {"elaItem", 0, elaItem0}, {"elaItemInfo", 0, elaItemInfo0}, {"vsbItems", -1, vsbItems},
        {"elaAlignments", -1, elaAlignments}, {"Label11", -1, SelectAlignment},
        {"cmdAlign", 0, cmdAlign0}, {"cmdAlign", 1, cmdAlign1}, {"cmdAlign", 2, cmdAlign2}, {"cmdAlign", 3, cmdAlign3},
        {"Label12", 0, SelectedItemCaption}, {"elaSelectedItem", -1, elaSelectedItem}, {"lblCost", -1, lblCost},
        {"elaSelectedItemCost", -1, elaSelectedItemCost}, {"cmdBuy", -1, cmdBuy}, {"cmdInfo", -1, cmdInfo},
        {"Label10", -1, SearchCaption}, {"txtSearch", -1, txtSearch},
    });
};

struct ServiceForms_Seer {
    static constexpr int32_t Resource = 52;
    static constexpr int32_t ClientW = 5025;
    static constexpr int32_t ClientH = 1755;
    inline static FormRect Elastic = FormRect{-15, -15, 5055, 1785};
    inline static FormRect Welcome = FormRect{90, 60, 4905, 300};
    inline static FormRect OptionsCaption = FormRect{150, 405, 1725, 210};
    inline static FormRect OptionsBox = FormRect{150, 645, 4800, 510};
    inline static FormRect Character = FormRect{60, 75, 1500, 360};
    inline static FormRect Monster = FormRect{1650, 75, 1500, 360};
    inline static FormRect Item = FormRect{3240, 75, 1500, 360};
    inline static FormRect Exit = FormRect{3765, 1335, 1185, 360};

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", 0, Elastic}, {"Label1", -1, Welcome}, {"lblLabel", -1, OptionsCaption},
        {"elaElastic", 1, OptionsBox}, {"cmdCommand", 0, Character}, {"cmdCommand", 2, Monster},
        {"cmdCommand", 3, Item}, {"cmdCommand", 1, Exit},
    });
};

struct ServiceForms_Library {
    static constexpr int32_t Resource = 56;
    static constexpr int32_t ClientW = 7710;
    static constexpr int32_t ClientH = 4410;
    inline static FormRect Elastic = FormRect{-15, -15, 7740, 4440};
    inline static FormRect BrowseCaption = FormRect{90, 90, 1245, 225};
    inline static FormRect OptionsBox = FormRect{105, 345, 1215, 645};
    inline static FormRect optMonsters = FormRect{45, 45, 1100, 270};
    inline static FormRect optItems = FormRect{45, 330, 1100, 270};
    inline static FormRect picMonster = FormRect{1560, 105, 900, 900};
    inline static FormRect lstList = FormRect{90, 1095, 2370, 2175};
    inline static FormRect txtInfo = FormRect{2565, 90, 5070, 3795};
    inline static FormRect SearchCaption = FormRect{90, 3345, 1305, 210};
    inline static FormRect txtSearch = FormRect{105, 3600, 2385, 285};
    inline static FormRect lblEntries = FormRect{75, 4050, 6015, 210};
    inline static FormRect cmdExit = FormRect{6165, 3990, 1485, 315};

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", 0, Elastic}, {"elaElastic", 1, OptionsBox},
        {"optOption", 1, optMonsters}, {"optOption", 0, optItems},
        {"cmdCommand", 0, cmdExit}, {"lstList", -1, lstList}, {"txtInfo", -1, txtInfo},
        {"txtSearch", -1, txtSearch}, {"picMonster", -1, picMonster}, {"Label1", -1, BrowseCaption},
        {"lblEntries", -1, lblEntries}, {"lblLabel", -1, SearchCaption},
    });
};

struct ServiceForms_GuildLog {
    static constexpr int32_t Resource = 48;
    static constexpr int32_t ClientW = 7755;
    static constexpr int32_t ClientH = 2490;
    inline static FormRect Elastic = FormRect{-15, -15, 7785, 2520};
    inline static FormRect lstLog = FormRect{90, 75, 7620, 1980};
    inline static FormRect lblEntries = FormRect{90, 2115, 1560, 225};
    inline static FormRect cmdClean = FormRect{4305, 2125, 1755, 315};
    inline static FormRect cmdDone = FormRect{6150, 2125, 1560, 315};

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", -1, Elastic}, {"cmdClean", -1, cmdClean}, {"lstLog", -1, lstLog},
        {"cmdDone", -1, cmdDone}, {"lblEntries", -1, lblEntries},
    });
};

struct ServiceForms_Hall {
    static constexpr int32_t Resource = 58;
    static constexpr int32_t ClientW = 7665;
    static constexpr int32_t ClientH = 5055;
    inline static FormRect Elastic = FormRect{-15, -15, 7695, 5085};
    inline static FormRect Heading = FormRect{195, 45, 7305, 330};
    inline static FormRect RecordsBox = FormRect{195, 390, 3510, 4200};
    inline static FormRect lblRecordStat0 = FormRect{60, 15, 3450, 240};
    inline static FormRect lblRecordChar0 = FormRect{30, 255, 3435, 435};
    inline static FormRect HistoryBox = FormRect{3930, 390, 3555, 4200};
    inline static FormRect lblHistoryStat0 = FormRect{60, 15, 3465, 240};
    inline static FormRect lblHistoryChar0 = FormRect{60, 255, 3435, 435};
    inline static FormRect cmdExit = FormRect{5415, 4665, 2085, 345};

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", 0, Elastic}, {"elaElastic", 2, HistoryBox},
        {"lblHistoryChar", 0, lblHistoryChar0}, {"lblHistoryStat", 0, lblHistoryStat0},
        {"elaElastic", 1, RecordsBox}, {"lblRecordStat", 0, lblRecordStat0}, {"lblRecordChar", 0, lblRecordChar0},
        {"cmdCommand", 0, cmdExit}, {"lblLabel", 1, Heading},
    });
};

struct ServiceForms_Companions {
    static constexpr int32_t Resource = 10;
    static constexpr int32_t ClientW = 4065;
    static constexpr int32_t ClientH = 3105;
    inline static FormRect Elastic = FormRect{60, 270, 3705, 2205};
    inline static FormRect CarryInfo = FormRect{90, 30, 3330, 210};
    inline static FormRect Kick0 = FormRect{3495, 30, 270, 255};
    inline static FormRect Picture0 = FormRect{60, 45, 750, 750};
    inline static FormRect Picture1 = FormRect{2910, 300, 750, 750};
    inline static FormRect Picture2 = FormRect{60, 1125, 750, 750};
    inline static FormRect Picture3 = FormRect{2910, 1395, 750, 750};
    inline static FormRect Name0 = FormRect{885, 60, 1995, 225};
    inline static FormRect Name1 = FormRect{840, 600, 1995, 225};
    inline static FormRect Name2 = FormRect{885, 1140, 1995, 225};
    inline static FormRect Name3 = FormRect{840, 1680, 1995, 225};
    inline static FormRect Hits0 = FormRect{900, 315, 795, 225};
    inline static FormRect Hits1 = FormRect{2025, 855, 810, 225};
    inline static FormRect Hits2 = FormRect{900, 1395, 810, 225};
    inline static FormRect Hits3 = FormRect{2055, 1935, 780, 225};

    static bool RightHand(int32_t slot);

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        {"elaElastic", 0, Elastic}, {"lblCarryInfo", -1, CarryInfo}, {"cmdCompanion", 0, Kick0},
        {"picCompanion", 0, Picture0}, {"picCompanion", 1, Picture1}, {"picCompanion", 2, Picture2}, {"picCompanion", 3, Picture3},
        {"lblCompName", 0, Name0}, {"lblCompName", 1, Name1}, {"lblCompName", 2, Name2}, {"lblCompName", 3, Name3},
        {"lblCompHits", 0, Hits0}, {"lblCompHits", 1, Hits1}, {"lblCompHits", 2, Hits2}, {"lblCompHits", 3, Hits3},
    });
};

struct ServiceForms {
    using Control = ServiceForms_Control;
    using Guild = ServiceForms_Guild;
    using Bank = ServiceForms_Bank;
    using Morgue = ServiceForms_Morgue;
    using Confinement = ServiceForms_Confinement;
    using Store = ServiceForms_Store;
    using Seer = ServiceForms_Seer;
    using Library = ServiceForms_Library;
    using GuildLog = ServiceForms_GuildLog;
    using Hall = ServiceForms_Hall;
    using Companions = ServiceForms_Companions;
};

}  // namespace mc
