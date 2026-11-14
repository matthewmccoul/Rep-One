#include "core.h"

namespace mc {

// ---------------------------------------------------------------- GameOptions

GameOptions::GameOptions() {
    FXVolume = WaveVolume::Default;
}

cs::String GameOptions::LastBackupLine() {
    return "Last Backup was on " + (LastBackup == "" ? cs::String("(none)") : LastBackup);
}

cs::String GameOptions::PathFor(cs::String gameDir) {
    return cs::Path::Combine(gameDir, "MORDOR.INI");
}

cs::Ref<GameOptions> GameOptions::Load(cs::String gameDir) {
    auto first = Ini::Read(PortSettings::PathFor(Saves::DirFor(gameDir)));
    auto second = Ini::Read(PathFor(gameDir));
    auto ini = cs::New<IniLayers>(first, second);
    auto o = [&] {
        auto _o = cs::New<GameOptions>();
        _o->No3d = ini->Flag(Section, "No3d");
        _o->NoMusic = ini->Flag(Section, "NoMusic");
        _o->NoFX = ini->Flag(Section, "NoFX");
        _o->NoStretch = ini->Flag(Section, "NoStretch");
        _o->SoundCanvas = ini->Flag(Section, "SoundCanvas");
        _o->NoDing = ini->Flag(Section, "NoDing");
        _o->NoLoadAutomap = ini->Flag(Section, "NoLoadAutomap");
        _o->LastBackup = PortSettings::Load(Saves::DirFor(gameDir))->LastBackup;
        _o->NoToolbar = ini->Flag(Section, "NoToolbar");
        _o->NoPartyFX = ini->Flag(Section, "NoPartyFX");
        _o->NoHelpSound = ini->Flag(Section, "NoHelpSound");
        _o->NoAutoSave = ini->Flag(Section, "NoAutoSave");
        _o->NoBlinkCursor = ini->Flag(Section, "NoBlinkCursor");
        _o->MusicVolume = ini->Number(Section, "MusicVolume", 0);
        // no key, "-65538" (MODULE1.BAS:983-987)
        _o->FXVolume = ini->Number(Section, "FXVolume", WaveVolume::Default);
        _o->Brightness = ini->Number(Section, "Brightness", 0);
        return _o;
    }();
    o->ReadFonts(ini);
    return o;
}

void GameOptions::ReadFonts(cs::Ref<IniLayers> ini) {
    ProportionalFont = ini->Text(Fonts, "Pro");
    ProportionalSize = ini->Number(Fonts, "ProSize", DefaultProportionalSize);
    NonProportionalFont = ini->Text(Fonts, "NonPro");
    NonProportionalSize = ini->Number(Fonts, "NonProSize", DefaultNonProportionalSize);
    if (ProportionalFont == "") ProportionalFont = DefaultProportionalFont;
    if (NonProportionalFont == "") NonProportionalFont = DefaultNonProportionalFont;
}

void GameOptions::Save(cs::String gameDir) {
    cs::String saves = Saves::DirFor(gameDir);
    try { cs::Directory::CreateDirectory(saves); } catch (...) { }
    auto ini = Ini::Read(PortSettings::PathFor(saves));
    auto Flag = [&](cs::String key, bool on) { ini->Set(Section, key, on ? "True" : "False"); };
    Flag("No3d", No3d);
    Flag("NoMusic", NoMusic);
    Flag("NoFX", NoFX);
    Flag("NoStretch", NoStretch);
    Flag("SoundCanvas", SoundCanvas);
    Flag("NoDing", NoDing);
    Flag("NoLoadAutomap", NoLoadAutomap);
    Flag("NoToolbar", NoToolbar);
    Flag("NoPartyFX", NoPartyFX);
    Flag("NoHelpSound", NoHelpSound);
    Flag("NoAutoSave", NoAutoSave);
    Flag("NoBlinkCursor", NoBlinkCursor);
    ini->Set(Section, "MusicVolume", cs::ToStr(MusicVolume));
    ini->Set(Section, "FXVolume", cs::ToStr(FXVolume));
    ini->Set(Section, "Brightness", cs::ToStr(Brightness));
    if (ProportionalFont != "") {
        ini->Set(Fonts, "Pro", ProportionalFont);
        ini->Set(Fonts, "ProSize", cs::ToStr(ProportionalSize));
    }
    if (NonProportionalFont != "") {
        ini->Set(Fonts, "NonPro", NonProportionalFont);
        ini->Set(Fonts, "NonProSize", cs::ToStr(NonProportionalSize));
    }
    ini->Write(PortSettings::PathFor(saves));
}

void GameOptions::DefaultFonts() {
    ProportionalFont = DefaultProportionalFont;
    ProportionalSize = DefaultProportionalSize;
    NonProportionalFont = DefaultNonProportionalFont;
    NonProportionalSize = DefaultNonProportionalSize;
}

// ---------------------------------------------------------------- PortSettings

cs::String PortSettings::PathFor(cs::String savesDir) {
    return cs::Path::Combine(savesDir, FileName);
}

bool PortSettings::Welcomed() {
    return cs::Str::Contains(NotFirstTime, 'T');
}

bool PortSettings::LimitationNoticeSeen() {
    return cs::Str::Contains(Limited, 'T');
}

cs::Ref<PortSettings> PortSettings::Load(cs::String savesDir) {
    auto ini = Ini::Read(PathFor(savesDir));
    return [&] {
        auto _o = cs::New<PortSettings>();
        _o->NotFirstTime = ini->Text(Section, "NotFirstTime");
        _o->Limited = ini->Text(Section, "Limited");
        _o->LastBackup = ini->Text(Section, "LastBackup");
        _o->Character = ini->Text(Section, "Character");
        _o->HasCharacter = ini->Has(Section, "Character");
        _o->Frame = FramePlacement::Parse(ini->Text(Section, "Frame"));
        return _o;
    }();
}

bool PortSettings::Write(cs::String savesDir, cs::String key, cs::String value) {
    try { cs::Directory::CreateDirectory(savesDir); } catch (...) { }
    auto ini = Ini::Read(PathFor(savesDir));
    ini->Set(Section, key, value);
    return ini->Write(PathFor(savesDir));
}

// ---------------------------------------------------------------- FramePlacement

cs::String FramePlacement::ToString() {
    return cs::ToStr(Left) + "," + cs::ToStr(Top) + "," +
        cs::ToStr(Width) + "," +
        cs::ToStr(Height) + "," +
        (Maximised ? cs::String("1") : cs::String("0"));
}

bool FramePlacement::TitleShownOn(int32_t areaX, int32_t areaY, int32_t areaWidth, int32_t areaHeight, double scale) {
    if (scale <= 0) scale = 1;
    int64_t w = cs::CastDouble<int64_t>(cs::Math::Round(Width * scale)), h = cs::CastDouble<int64_t>(cs::Math::Round(TitleStrip * scale));
    return Left < (int64_t)areaX + areaWidth && (int64_t)Left + w > areaX
        && Top < (int64_t)areaY + areaHeight && (int64_t)Top + h > areaY;
}

cs::Ref<FramePlacement> FramePlacement::Parse(cs::String text) {
    auto p = cs::Str::Split(text, ',');
    if (p.Length() != 5) return nullptr;
    int32_t l = 0;
    if (!cs::Parse::TryParse<int32_t>(cs::Str::Trim(p[0]), l)) return nullptr;
    int32_t t = 0;
    if (!cs::Parse::TryParse<int32_t>(cs::Str::Trim(p[1]), t)) return nullptr;
    double w = 0;
    if (!cs::Parse::TryParse(cs::Str::Trim(p[2]), cs::Parse::Float, cs::Parse::InvariantCulture(), w) || w < 100) return nullptr;
    double h = 0;
    if (!cs::Parse::TryParse(cs::Str::Trim(p[3]), cs::Parse::Float, cs::Parse::InvariantCulture(), h) || h < 100) return nullptr;
    return cs::New<FramePlacement>(l, t, w, h, cs::Str::Trim(p[4]) == "1");
}

// ---------------------------------------------------------------- IniLayers

IniLayers::IniLayers(cs::Ref<Ini> first, cs::Ref<Ini> second) {
    _first = first;
    _second = second;
}

cs::Ref<Ini> IniLayers::For(cs::String section, cs::String key) {
    return cs::deref(_first).Has(section, key) ? _first : _second;
}

cs::String IniLayers::Text(cs::String section, cs::String key, cs::String fallback) {
    return cs::deref(For(section, key)).Text(section, key, fallback);
}

bool IniLayers::Flag(cs::String section, cs::String key) {
    return cs::deref(For(section, key)).Flag(section, key);
}

int32_t IniLayers::Number(cs::String section, cs::String key, int32_t fallback) {
    return cs::deref(For(section, key)).Number(section, key, fallback);
}

// ---------------------------------------------------------------- Ini

cs::Ref<Ini> Ini::Parse(cs::List<cs::String> lines) {
    auto ini = cs::New<Ini>();
    ini->_lines.AddRange(lines);
    return ini;
}

cs::Ref<Ini> Ini::Read(cs::String path) {
    auto ini = cs::New<Ini>();
    try { if (cs::File::Exists(path)) ini->_lines.AddRange(cs::File::ReadAllLines(path)); }
    catch (...) { }
    return ini;
}

bool Ini::Write(cs::String path) {
    try {
        // File.WriteAllLines: every line followed by a newline
        cs::String text;
        for (auto& line : _lines) { text += line; text += "\n"; }
        cs::File::WriteAllText(path, text);
        return true;
    }
    catch (...) { return false; }
}

cs::String Ini::Text(cs::String section, cs::String key, cs::String fallback) {
    int32_t i = Find(section, key);
    if (i < 0) return fallback;
    int32_t eq = cs::Str::IndexOf(_lines[i], '=');
    return eq < 0 ? fallback : cs::Str::Trim(cs::Str::Substring(_lines[i], eq + 1));
}

bool Ini::Flag(cs::String section, cs::String key) {
    return cs::Str::EqualsIgnoreCase(Text(section, key, "False"), "True");
}

bool Ini::Has(cs::String section, cs::String key) {
    return Find(section, key) >= 0;
}

void Ini::Remove(cs::String section, cs::String key) {
    int32_t i = Find(section, key);
    if (i >= 0) _lines.RemoveAt(i);
}

int32_t Ini::Number(cs::String section, cs::String key, int32_t fallback) {
    int32_t v = 0;
    return cs::Parse::TryParse<int32_t>(cs::Str::Trim(Text(section, key, "")), v) ? v : fallback;
}

void Ini::Set(cs::String section, cs::String key, cs::String value) {
    int32_t i = Find(section, key);
    if (i >= 0) { _lines[i] = key + "=" + value; return; }

    int32_t s = FindSection(section);
    if (s < 0) {
        if (_lines.Count() > 0 && cs::Str::Trim(_lines[_lines.Count() - 1]) != "") _lines.Add("");
        _lines.Add("[" + section + "]");
        _lines.Add(key + "=" + value);
        return;
    }
    // after the last line of that section
    int32_t at = s + 1;
    while (at < _lines.Count() && !cs::Str::StartsWith(cs::Str::TrimStart(_lines[at]), '[')) at++;
    while (at - 1 > s && cs::Str::Trim(_lines[at - 1]) == "") at--;
    _lines.Insert(at, key + "=" + value);
}

int32_t Ini::FindSection(cs::String section) {
    for (int32_t i = 0; i < _lines.Count(); i++) {
        cs::String t = cs::Str::Trim(_lines[i]);
        if (cs::Str::StartsWith(t, '[') && cs::Str::EndsWith(t, ']') &&
            cs::Str::EqualsIgnoreCase(cs::Str::Trim(cs::Str::Substring(t, 1, (int32_t)t.size() - 2)), section))
            return i;
    }
    return -1;
}

int32_t Ini::Find(cs::String section, cs::String key) {
    int32_t s = FindSection(section);
    if (s < 0) return -1;
    for (int32_t i = s + 1; i < _lines.Count(); i++) {
        cs::String t = cs::Str::Trim(_lines[i]);
        if (cs::Str::StartsWith(t, '[')) return -1;
        int32_t eq = cs::Str::IndexOf(t, '=');
        if (eq > 0 && cs::Str::EqualsIgnoreCase(cs::Str::Trim(cs::Str::Substring(t, 0, eq)), key)) return i;
    }
    return -1;
}

}  // namespace mc
