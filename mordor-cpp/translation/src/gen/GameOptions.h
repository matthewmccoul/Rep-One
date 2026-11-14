#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct GameOptions {
    inline static const cs::String DefaultProportionalFont = "Courier New";
    static constexpr int32_t DefaultProportionalSize = 9;
    inline static const cs::String DefaultNonProportionalFont = "MS Sans Serif";
    static constexpr int32_t DefaultNonProportionalSize = 8;
    inline static const cs::String Section = "Loading Information";
    inline static const cs::String Fonts = "Fonts";

    bool No3d = false;
    bool NoMusic = false;
    bool NoFX = false;
    bool NoStretch = false;
    bool SoundCanvas = false;
    bool NoDing = false;
    bool NoLoadAutomap = false;
    bool NoToolbar = false;
    bool NoPartyFX = false;
    bool NoHelpSound = false;
    bool NoAutoSave = false;
    bool NoBlinkCursor = false;

    cs::String LastBackup = "";

    int32_t MusicVolume = 0;
    int32_t FXVolume = 0;  // set to WaveVolume::Default in the constructor
    int32_t Brightness = 0;

    cs::String ProportionalFont = DefaultProportionalFont;
    int32_t ProportionalSize = DefaultProportionalSize;
    cs::String NonProportionalFont = DefaultNonProportionalFont;
    int32_t NonProportionalSize = DefaultNonProportionalSize;

    GameOptions();

    cs::String LastBackupLine();
    static cs::String PathFor(cs::String gameDir);
    static cs::Ref<GameOptions> Load(cs::String gameDir);
    void ReadFonts(cs::Ref<IniLayers> ini);
    void Save(cs::String gameDir);
    void DefaultFonts();
};

struct PortSettings {
    inline static const cs::String FileName = "settings.ini";
    inline static const cs::String Section = "Loading Information";

    cs::String NotFirstTime = "";
    cs::String Limited = "";
    cs::String LastBackup = "";
    cs::String Character = "";
    bool HasCharacter = false;
    cs::Ref<FramePlacement> Frame;

    bool Welcomed();
    bool LimitationNoticeSeen();
    static cs::String PathFor(cs::String savesDir);
    static cs::Ref<PortSettings> Load(cs::String savesDir);
    static bool Write(cs::String savesDir, cs::String key, cs::String value);
};

struct FramePlacement {
    static constexpr double TitleStrip = 30;

    int32_t Left = 0;
    int32_t Top = 0;
    double Width = 0;
    double Height = 0;
    bool Maximised = false;

    FramePlacement() = default;
    FramePlacement(int32_t Left, int32_t Top, double Width, double Height, bool Maximised)
        : Left(Left), Top(Top), Width(Width), Height(Height), Maximised(Maximised) {}

    bool operator==(const FramePlacement& o) const {
        return Left == o.Left && Top == o.Top && Width == o.Width && Height == o.Height && Maximised == o.Maximised;
    }
    bool operator!=(const FramePlacement& o) const { return !(*this == o); }

    cs::String ToString();
    bool TitleShownOn(int32_t areaX, int32_t areaY, int32_t areaWidth, int32_t areaHeight, double scale);
    static cs::Ref<FramePlacement> Parse(cs::String text);
};

struct IniLayers {
    cs::Ref<Ini> _first;
    cs::Ref<Ini> _second;
    IniLayers(cs::Ref<Ini> first, cs::Ref<Ini> second);
    cs::Ref<Ini> For(cs::String section, cs::String key);
    cs::String Text(cs::String section, cs::String key, cs::String fallback = cs::String(""));
    bool Flag(cs::String section, cs::String key);
    int32_t Number(cs::String section, cs::String key, int32_t fallback);
};

struct Ini {
    cs::List<cs::String> _lines = cs::List<cs::String>::New();

    static cs::Ref<Ini> Parse(cs::List<cs::String> lines);
    static cs::Ref<Ini> Read(cs::String path);
    bool Write(cs::String path);
    cs::String Text(cs::String section, cs::String key, cs::String fallback = cs::String(""));
    bool Flag(cs::String section, cs::String key);
    bool Has(cs::String section, cs::String key);
    void Remove(cs::String section, cs::String key);
    int32_t Number(cs::String section, cs::String key, int32_t fallback);
    void Set(cs::String section, cs::String key, cs::String value);
    int32_t FindSection(cs::String section);
    int32_t Find(cs::String section, cs::String key);
};

}  // namespace mc
