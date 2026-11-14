#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GameArtResult {
    cs::String Dir{};
    cs::String Error{};
    cs::List<cs::String> Notes;
    bool Ok();
};

struct GameArt_Source {
    cs::String Name{};
    bool Required = false;
    cs::String Why{};
};

struct GameArt_Picture {
    cs::String Name{};
    int32_t Width = 0;
    int32_t Height = 0;
    cs::String From{};
};

struct GameArt {
    using Source = GameArt_Source;
    using Picture = GameArt_Picture;

    static constexpr int32_t Version = 3;

    inline static const cs::String HelpFile = "help.json";
    inline static const cs::String ManifestFile = "extracted.json";

    inline static cs::List<cs::Ref<GameArt_Source>> Sources = [] {
        auto src = [](cs::String name, bool required, cs::String why) {
            auto _o = cs::New<GameArt_Source>();
            _o->Name = name;
            _o->Required = required;
            _o->Why = why;
            return _o;
        };
        return cs::List<cs::Ref<GameArt_Source>>::Of({
            src("MORDOR.EXE", true, "the menu, credits, toolbar and city pictures"),
            src("MG11.DLL", true, "the dungeon, monster and automap pictures"),
            src("MORDOR.HLP", false, "the help"),
            src("cityw1.png", false, "the city's building buttons, as the 1.1 install shows them"),
        });
    }();

    inline static cs::List<Tup_Name_X_Y_FormPicture_b016c> Town = cs::List<Tup_Name_X_Y_FormPicture_b016c>::Of({
        Tup_Name_X_Y_FormPicture_b016c{"town_store.png", 12, 27, 6},
        Tup_Name_X_Y_FormPicture_b016c{"town_morgue.png", 130, 27, 4},
        Tup_Name_X_Y_FormPicture_b016c{"town_guild.png", 249, 27, 2},
        Tup_Name_X_Y_FormPicture_b016c{"town_dungeon.png", 368, 27, 1},
        Tup_Name_X_Y_FormPicture_b016c{"town_confinement.png", 12, 128, 7},
        Tup_Name_X_Y_FormPicture_b016c{"town_seer.png", 130, 128, 3},
        Tup_Name_X_Y_FormPicture_b016c{"town_bank.png", 249, 128, 5},
    });

    inline static cs::List<cs::String> Toolbar = cs::List<cs::String>::Of({
        "tool_page.png", "tool_join.png", "tool_automap.png", "tool_controlpanel.png", "tool_dropbody.png",
        "tool_pickup.png", "tool_give.png", "tool_experience.png", "tool_help.png",
    });

    inline static cs::List<Tup_Resource_Name_Width_Height_f35c3> Mg11 = cs::List<Tup_Resource_Name_Width_Height_f35c3>::Of({
        Tup_Resource_Name_Width_Height_f35c3{"MAZE", "maze_corridor.png", 1250, 200},
        Tup_Resource_Name_Width_Height_f35c3{"DOOR", "maze_overlays.png", 200, 79},
        Tup_Resource_Name_Width_Height_f35c3{"BLNK", "maze_backdrop.png", 100, 100},
        Tup_Resource_Name_Width_Height_f35c3{"FOG", "maze_dark.png", 50, 50},
        Tup_Resource_Name_Width_Height_f35c3{"WATR", "maze_water.png", 50, 20},
        Tup_Resource_Name_Width_Height_f35c3{"QSND", "maze_quicksand.png", 50, 18},
        Tup_Resource_Name_Width_Height_f35c3{"PICS", "automap-pics.png", 405, 15},
        Tup_Resource_Name_Width_Height_f35c3{"MISC", "combat-misc.png", 270, 45},
        Tup_Resource_Name_Width_Height_f35c3{"ASMO", "asmodeus.png", 100, 100},
        Tup_Resource_Name_Width_Height_f35c3{"MONS", "monsters.png", 600, 540},
    });

    static cs::List<cs::Ref<GameArt_Picture>> BuildList();

    inline static cs::List<cs::Ref<GameArt_Picture>> Pictures = BuildList();

    static cs::String CacheRoot();
    static cs::String Find(cs::String gameDir, cs::String name);
    static cs::String Key(cs::String gameDir);
    static cs::Ref<GameArtResult> Prepare(cs::String gameDir, cs::String cacheRoot = {});
    static bool IsReady(cs::String gameDir, cs::String cacheRoot = {});
    static bool Complete(cs::String dir);
    static cs::String HelpTitle(cs::String cacheDir);
    static cs::List<cs::String> ReadNotes(cs::String manifest);
    static void TryDelete(cs::String dir);
    static cs::List<cs::String> Extract(cs::String gameDir, cs::String outDir);
};

}  // namespace mc
