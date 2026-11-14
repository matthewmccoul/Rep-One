#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct DesignFont {
    inline static const cs::String ProFace = "Courier New";
    inline static const cs::String NonProFace = "MS Sans Serif";
    static DesignFont Default;

    cs::String Face{};
    double Points = 0;
    bool Bold = false;
    cs::String Tag{};

    DesignFont() = default;
    DesignFont(cs::String Face, double Points, bool Bold = true, cs::String Tag = {});

    DesignFont Plain();
    cs::Nullable<double> FixedPoints();
    cs::String ToString();

    bool operator==(const DesignFont& o) const { return Face == o.Face && Points == o.Points && Bold == o.Bold && Tag == o.Tag; }
    bool operator!=(const DesignFont& o) const { return !(*this == o); }
};

inline DesignFont DesignFont::Default = DesignFont(DesignFont::NonProFace, 8.25);

struct ResolvedFont {
    cs::String Face{};
    double Points = 0;

    ResolvedFont() = default;
    ResolvedFont(cs::String Face, double Points);

    double Pixels();

    bool operator==(const ResolvedFont& o) const { return Face == o.Face && Points == o.Points; }
    bool operator!=(const ResolvedFont& o) const { return !(*this == o); }
};

struct FontRule {
    static bool IsDefault(cs::Ref<GameOptions> o);
    static ResolvedFont Resolve(cs::Ref<GameOptions> o, DesignFont d, FontPass pass);
    static double Sized(double had, double wanted);
    static double Pixels(double points);
};

struct DesignFonts_Entry;

struct DesignFonts_Form : std::enable_shared_from_this<DesignFonts_Form> {
    cs::String Name{};
    int32_t Resource = 0;
    FontPass Pass{};

    DesignFonts_Form(cs::String Name, int32_t Resource, FontPass Pass);

    DesignFonts_Entry Control(cs::String key, DesignFont font);
};

struct DesignFonts_Entry {
    cs::Ref<DesignFonts_Form> Form{};
    cs::String Control{};
    DesignFont Font{};

    DesignFonts_Entry() = default;
    DesignFonts_Entry(cs::Ref<DesignFonts_Form> Form, cs::String Control, DesignFont Font);

    bool operator==(const DesignFonts_Entry& o) const { return Form == o.Form && Control == o.Control && Font == o.Font; }
    bool operator!=(const DesignFonts_Entry& o) const { return !(*this == o); }
};

struct DesignFonts {
    using Form = DesignFonts_Form;
    using Entry = DesignFonts_Entry;

    inline static cs::Ref<DesignFonts_Form> CharStat1 = cs::New<DesignFonts_Form>("frmCharStat1", 8, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> CharStat3 = cs::New<DesignFonts_Form>("frmCharStat3", 10, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> CharStat4 = cs::New<DesignFonts_Form>("frmCharStat4", 12, FontPass::Restyle);
    inline static cs::Ref<DesignFonts_Form> Main = cs::New<DesignFonts_Form>("frmMain", 6, FontPass::Restyle);
    inline static cs::Ref<DesignFonts_Form> CityMain = cs::New<DesignFonts_Form>("frmCityMain", 14, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> CityStore = cs::New<DesignFonts_Form>("frmCityStore", 16, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> CityBank = cs::New<DesignFonts_Form>("frmCityBank", 18, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> CityGuild = cs::New<DesignFonts_Form>("frmCityGuild", 20, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> DungeonMain = cs::New<DesignFonts_Form>("frmDungeonMain", 22, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> Interact = cs::New<DesignFonts_Form>("frmInteract", 26, FontPass::Restyle);
    inline static cs::Ref<DesignFonts_Form> CityMorgue = cs::New<DesignFonts_Form>("frmCityMorgue", 28, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> Exchange = cs::New<DesignFonts_Form>("frmExchange", 38, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> AutoMap = cs::New<DesignFonts_Form>("frmAutoMap", 40, FontPass::Restyle);
    inline static cs::Ref<DesignFonts_Form> Party = cs::New<DesignFonts_Form>("frmParty", 44, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> PartyOptions = cs::New<DesignFonts_Form>("frmPartyOptions", 46, FontPass::Restyle);
    inline static cs::Ref<DesignFonts_Form> CitySeer = cs::New<DesignFonts_Form>("frmCitySeer", 52, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> CityCStore = cs::New<DesignFonts_Form>("frmCityCStore", 60, FontPass::RestyleAndZoom);
    inline static cs::Ref<DesignFonts_Form> GuildLog = cs::New<DesignFonts_Form>("frmGuildLog", 48, FontPass::SizeOnly);
    inline static cs::Ref<DesignFonts_Form> Library = cs::New<DesignFonts_Form>("frmLibrary", 56, FontPass::SizeOnly);
    inline static cs::Ref<DesignFonts_Form> Hall = cs::New<DesignFonts_Form>("frmHall", 58, FontPass::SizeOnly);

    inline static DesignFont StoreCounts = DesignFont(DesignFont::ProFace, 8.25);
    inline static DesignFont StoreSelected = DesignFont("Arial", 8.25, false);
    inline static DesignFont GuildStatName = DesignFont(DesignFont::ProFace, 8.25);
    inline static DesignFont GuildStat = DesignFont(DesignFont::ProFace, 8.25, false);
    inline static DesignFont Welcome = DesignFont(DesignFont::NonProFace, 12, true, ":12");
    inline static DesignFont CompCounts = DesignFont(DesignFont::ProFace, 8.25);
    inline static DesignFont CharStat = DesignFont(DesignFont::ProFace, 8.25, false);
    inline static DesignFont CarryInfo = DesignFont("Arial", 8.25);
    inline static DesignFont Disarm = DesignFont(DesignFont::ProFace, 7.5);
    inline static DesignFont Toolbar = DesignFont(DesignFont::ProFace, 8.25);
    inline static DesignFont ExchangeName = DesignFont(DesignFont::NonProFace, 9.75);
    inline static DesignFont HallHeading = DesignFont(DesignFont::NonProFace, 12);
    inline static DesignFont PlainSans = DesignFont::Default.Plain();

    inline static cs::List<DesignFonts_Entry> All = cs::List<DesignFonts_Entry>::Of({
        CityStore->Control("elaItemInfo(0)", StoreCounts),
        CityStore->Control("Label9(0)", StoreCounts),
        CityStore->Control("Label9(1)", StoreCounts),
        CityStore->Control("elaSelectedItem", StoreSelected),
        CityStore->Control("elaSelectedItemCost", StoreSelected),
        CityStore->Control("elaItem(0)", DesignFont::Default),
        CityStore->Control("Label10", DesignFont::Default),
        CityGuild->Control("lblStatName(0)", GuildStatName),
        CityGuild->Control("lblStat(0)", GuildStat),
        CityGuild->Control("lblWelcome", Welcome),
        CityGuild->Control("lstGuilds", PlainSans),
        CityBank->Control("lblLabel(0)", Welcome),
        CityBank->Control("lstItems", DesignFont::Default),
        CityMorgue->Control("Label1", Welcome),
        CitySeer->Control("Label1", Welcome),
        CityCStore->Control("elaCompInfo(0)", CompCounts),
        CityCStore->Control("elaComp(0)", PlainSans),
        CityCStore->Control("lblLabel(3)", CompCounts),
        CityCStore->Control("lblLabel(4)", CompCounts),
        CharStat1->Control("lblStat(0)", CharStat),
        CharStat1->Control("lblStatInfo(0)", DesignFont::Default),
        CharStat1->Control("lblRaceInfo", DesignFont::Default),
        CharStat1->Control("lblGuildInfo", DesignFont::Default),
        CharStat1->Control("lblSpecial(0)", DesignFont::Default),
        CharStat1->Control("elaGroupInfo", PlainSans),
        CharStat1->Control("picTabPicture", DesignFont::Default),
        CharStat1->Control("lblBuffer(0)", PlainSans),
        CharStat1->Control("tabStatTab", PlainSans),
        CharStat3->Control("lblItem(0)", PlainSans),
        CharStat3->Control("lblItemStatus(0)", DesignFont::Default),
        CharStat3->Control("lblName(0)", PlainSans),
        CharStat3->Control("lblLabel(0)", DesignFont::Default),
        CharStat3->Control("lblCarryInfo", CarryInfo),
        CharStat3->Control("lblCompName(0)", PlainSans),
        CharStat3->Control("tabItemTab", PlainSans),
        CharStat3->Control("cmdCommand(0)", PlainSans),
        CharStat4->Control("lstMessages", DesignFont::Default),
        Main->Control("picToolbar", Toolbar),
        Main->Control("lblExperience", PlainSans),
        Main->Control("lblGold", PlainSans),
        CityMain->Control("lblSelection(0)", DesignFont::Default),
        DungeonMain->Control("lblDisarm", Disarm),
        DungeonMain->Control("lblTrap", DesignFont::Default),
        DungeonMain->Control("lblRoom", PlainSans),
        DungeonMain->Control("lblSquareInfo", DesignFont::Default),
        DungeonMain->Control("lblMonster(0)", DesignFont::Default),
        DungeonMain->Control("cmdAction(0)", PlainSans),
        Party->Control("lblName(0)", PlainSans),
        Party->Control("lblLabel(0)", DesignFont::Default),
        Party->Control("cmdCommand(0)", PlainSans),
        AutoMap->Control("lblInfo", DesignFont::Default),
        AutoMap->Control("lblLevel", PlainSans),
        Interact->Control("cmbCharacter", DesignFont::Default),
        Interact->Control("lblQuestion", DesignFont::Default),
        PartyOptions->Control("lstCharacters", PlainSans),
        Exchange->Control("pnlName(0)", ExchangeName),
        Exchange->Control("pnlItem(0)", DesignFont::Default),
        GuildLog->Control("lstLog", PlainSans),
        Library->Control("lstList", DesignFont::Default),
        Hall->Control("lblLabel(1)", HallHeading),
        Hall->Control("lblRecordStat(0)", DesignFont::Default),
        Hall->Control("lblRecordChar(0)", PlainSans),
        Hall->Control("lblHistoryChar(0)", PlainSans),
    });
};

}  // namespace mc
