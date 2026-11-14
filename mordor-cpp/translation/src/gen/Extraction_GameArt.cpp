#include "core.h"

namespace mc {

bool GameArtResult::Ok() { return !Dir.empty(); }

cs::List<cs::Ref<GameArt_Picture>> GameArt::BuildList() {
    auto pic = [&](cs::String name, int32_t width, int32_t height, cs::String from) {
        auto _o = cs::New<GameArt_Picture>();
        _o->Name = name;
        _o->Width = width;
        _o->Height = height;
        _o->From = from;
        return _o;
    };
    auto l = cs::List<cs::Ref<GameArt_Picture>>::New();
    for (auto& m : Mg11) l.Add(pic(m.Name, m.Width, m.Height, "MG11.DLL, resource MORDOR/" + cs::ToStr(m.Resource)));
    l.Add(pic("menu_background.png", 600, 400, "MORDOR.EXE, frmMenu's picture"));
    l.Add(pic("credits_david.png", 150, 150, "MORDOR.EXE, frmCredits' picture"));
    l.Add(pic("town_exit.png", 120, 42, "MORDOR.EXE, frmCityMain's first picture (the Exit sign)"));
    for (int32_t i = 0; i < Toolbar.Count(); i++) {
        auto t = Toolbar[i];
        l.Add(pic(t, 20, 20, "MORDOR.EXE, frmMain's 20x20 picture " + cs::ToStr(i + 1) + " of 9"));
    }
    l.Add(pic("about_icon.png", 32, 32, "MORDOR.EXE, the program's icon (RT_ICON)"));
    for (auto& t : Town)
        l.Add(pic(t.Name, 103, 75, "cityw1.png at (" + cs::ToStr(t.X) + "," + cs::ToStr(t.Y) + "), else MORDOR.HLP's CITYW1.BMP, else frmCityMain's picture " + cs::ToStr(t.FormPicture + 1) + " scaled"));
    return l;
}

cs::String GameArt::CacheRoot() {
    const char* e = std::getenv("MORDOR_ART_CACHE");
    cs::String env = e ? cs::String(e) : cs::String();
    if (!cs::Str::IsNullOrWhiteSpace(env)) return env;
    // UserFolders.Local falls back to the temporary folder, as this did, with no local application data
    return cs::Path::Combine(UserFolders::Local(), "cache");
}

cs::String GameArt::Find(cs::String gameDir, cs::String name) {
    cs::String direct = cs::Path::Combine(gameDir, name);
    if (cs::File::Exists(direct)) return direct;
    try {
        return cs::Directory::GetFiles(gameDir).FirstOrDefault([&](auto& f) { return cs::Str::EqualsIgnoreCase(cs::Path::GetFileName(f), name); });
    } catch (...) {
        return {};
    }
}

cs::String GameArt::Key(cs::String gameDir) {
    auto all = cs::New<cs::StringBuilder>("mordor-art v" + cs::ToStr(Version) + "\n");
    for (auto& s : Sources)
    {
        cs::String p = Find(gameDir, s->Name);
        if (p.empty()) { all->Append(s->Name).Append(" absent\n"); continue; }
        all->Append(s->Name).Append(" ").Append(cs::Convert::ToHexString(cs::SHA256::HashFile(p))).Append("\n");
    }
    return cs::Str::ToLowerInvariant(cs::Str::Substring(cs::Convert::ToHexString(cs::SHA256::HashData(cs::Encoding::UTF8GetBytes(all->ToString()))), 0, 16));
}

cs::Ref<GameArtResult> GameArt::Prepare(cs::String gameDir, cs::String cacheRoot) {
    auto result = [&](cs::String dir, cs::String error, cs::List<cs::String> notes) {
        auto _o = cs::New<GameArtResult>();
        _o->Dir = dir;
        _o->Error = error;
        _o->Notes = notes;
        return _o;
    };
    try {
        for (auto& s : Sources.Where([&](auto& s) { return s->Required; }))
            if (Find(gameDir, s->Name).empty())
                return result({}, s->Name + " is missing from " + gameDir + ": " + s->Why + " are in it, and the game cannot start without it.", cs::List<cs::String>::New());
        cs::String root = !cacheRoot.empty() ? cacheRoot : CacheRoot();
        cs::String dir = cs::Path::Combine(root, Key(gameDir));
        cs::String manifest = cs::Path::Combine(dir, ManifestFile);
        if (Complete(dir))
            return result(dir, {}, ReadNotes(manifest));

        cs::Directory::CreateDirectory(root);
        // Guid.NewGuid().ToString("N")[..8]: 8 random lowercase hex digits
        cs::String temp = dir + ".part-" + cs::Format(cs::New<cs::Random>()->Next(), "x8");
        cs::Directory::CreateDirectory(temp);
        cs::List<cs::String> notes;
        try {
            notes = Extract(gameDir, temp);
        } catch (...) {
            TryDelete(temp);
            throw;
        }
        if (cs::Directory::Exists(dir)) TryDelete(dir);    // an earlier, unfinished one
        try {
            std::filesystem::rename(temp, dir);
        } catch (const std::filesystem::filesystem_error& e) {
            if (cs::File::Exists(manifest)) TryDelete(temp);  // another start got there first
            else throw cs::IOException(e.what());
        }
        return result(dir, {}, notes);
    } catch (const std::exception& ex) {
        return result({}, "The game's pictures could not be read from " + gameDir + ": " + cs::String(ex.what()), cs::List<cs::String>::New());
    }
}

bool GameArt::IsReady(cs::String gameDir, cs::String cacheRoot) {
    try {
        if (Sources.Any([&](auto& s) { return s->Required && Find(gameDir, s->Name).empty(); })) return false;
        return Complete(cs::Path::Combine(!cacheRoot.empty() ? cacheRoot : CacheRoot(), Key(gameDir)));
    } catch (const cs::IOException&) {
        return false;
    } catch (const cs::UnauthorizedAccessException&) {
        return false;
    } catch (const cs::ArgumentException&) {
        return false;
    } catch (const cs::ArgumentOutOfRangeException&) {
        return false;
    } catch (const cs::NotSupportedException&) {
        return false;
    }
}

bool GameArt::Complete(cs::String dir) {
    cs::String manifest = cs::Path::Combine(dir, ManifestFile);
    if (!cs::File::Exists(manifest) || !Pictures.All([&](auto& p) { return cs::File::Exists(cs::Path::Combine(dir, p->Name)); })) return false;
    try
    {
        auto doc = cs::Json::Parse(cs::File::ReadAllText(manifest));
        if (doc.kind != cs::Json::Node::Object) throw cs::InvalidOperationException("The requested operation requires an element of type 'Object'.");
        auto h = doc.Find("help");
        bool help = h != nullptr && h->kind == cs::Json::Node::True;
        return !help || cs::File::Exists(cs::Path::Combine(dir, HelpFile));
    }
    catch (const cs::JsonException&) { return false; }
}

cs::String GameArt::HelpTitle(cs::String cacheDir) {
    try
    {
        auto doc = cs::Json::Parse(cs::File::ReadAllText(cs::Path::Combine(cacheDir, ManifestFile)));
        if (doc.kind != cs::Json::Node::Object) throw cs::InvalidOperationException("The requested operation requires an element of type 'Object'.");
        auto t = doc.Find("helpTitle");
        return t != nullptr && t->kind == cs::Json::Node::Str && t->text.size() > 0 ? t->text : cs::String();
    }
    catch (const cs::IOException&) { return {}; }
    catch (const cs::JsonException&) { return {}; }
    catch (const cs::UnauthorizedAccessException&) { return {}; }
}

cs::List<cs::String> GameArt::ReadNotes(cs::String manifest) {
    try
    {
        auto doc = cs::Json::Parse(cs::File::ReadAllText(manifest));
        if (doc.kind != cs::Json::Node::Object) throw cs::InvalidOperationException("The requested operation requires an element of type 'Object'.");
        auto n = doc.Find("notes");
        if (n == nullptr) return cs::List<cs::String>::New();
        if (n->kind != cs::Json::Node::Array) throw cs::InvalidOperationException("The requested operation requires an element of type 'Array'.");
        auto r = cs::List<cs::String>::New();
        for (auto& e : n->items) {
            if (e.kind == cs::Json::Node::Null) r.Add("");
            else if (e.kind == cs::Json::Node::Str) r.Add(e.text);
            else throw cs::InvalidOperationException("The requested operation requires an element of type 'String'.");
        }
        return r;
    }
    catch (...) { return cs::List<cs::String>::New(); }
}

void GameArt::TryDelete(cs::String dir) {
    try {
        cs::Directory::Delete(dir, true);
    } catch (...) {
    }
}

cs::List<cs::String> GameArt::Extract(cs::String gameDir, cs::String outDir) {
    auto notes = cs::List<cs::String>::New();
    auto made = cs::Dictionary<cs::String, cs::Ref<RgbaImage>>::New();
    auto Need = [&](cs::String name) -> cs::String {
        cs::String f = Find(gameDir, name);
        if (f.empty()) throw cs::FileNotFoundException(name + " is missing");
        return f;
    };

    auto Put = [&](cs::String name, cs::Func<cs::Ref<RgbaImage>()> make) {
        auto pic = Pictures.First([&](auto& p) { return p->Name == name; });
        cs::Ref<RgbaImage> img;
        auto failed = [&](const char* msg) {
            notes.Add(name + ": blank - " + pic->From + " did not read (" + cs::String(msg) + ")");
            img = cs::New<RgbaImage>(pic->Width, pic->Height);
        };
        try {
            img = make();
            if (img->Width != pic->Width || img->Height != pic->Height)
                throw cs::InvalidDataException("it is " + cs::ToStr(img->Width) + "x" + cs::ToStr(img->Height) + ", not " + cs::ToStr(pic->Width) + "x" + cs::ToStr(pic->Height));
        } catch (const cs::InvalidDataException& ex) {
            failed(ex.what());
        } catch (const cs::ArgumentException& ex) {
            failed(ex.what());
        } catch (const cs::ArgumentOutOfRangeException& ex) {  // an ArgumentException in .NET; cs List indexing throws it
            failed(ex.what());
        } catch (const cs::IndexOutOfRangeException& ex) {
            failed(ex.what());
        } catch (const cs::FileNotFoundException& ex) {
            failed(ex.what());
        }
        made[name] = img;
    };

    // MG11.DLL
    auto mg11 = NeModule::Read(Need("MG11.DLL"));
    int32_t noted = notes.Count();
    for (auto& m : Mg11) {
        auto res = m.Resource;
        auto name = m.Name;
        Put(name, [&]() {
            auto r = mg11->Find("MORDOR", res);
            if (!r) throw cs::InvalidDataException("no resource " + res);
            return Dib::DecodeBmpFile(mg11->Bytes, r->Offset);
        });
    }
    // a picture or two can be left blank; an MG11.DLL with none of them is not the game's
    if (notes.Count() - noted == Mg11.Length()) throw cs::InvalidDataException("MG11.DLL holds none of the game's pictures");

    // MORDOR.EXE
    auto exe = NeModule::Read(Need("MORDOR.EXE"));
    auto FormPicture = [&](cs::String form, int32_t index, int32_t w = 0, int32_t h = 0) -> cs::Ref<RgbaImage> {
        auto pics = exe->FormPictures(form);
        if (w > 0) pics = pics.Where([&](auto& p) { return p->Width == w && p->Height == h; }).ToList();
        if (index >= pics.Count()) throw cs::InvalidDataException(form + " has no picture " + cs::ToStr(index + 1));
        return pics[index];
    };
    Put("menu_background.png", [&]() { return FormPicture("frmMenu", 0); });
    Put("credits_david.png", [&]() { return FormPicture("frmCredits", 0); });
    Put("town_exit.png", [&]() { return FormPicture("frmCityMain", 0); });
    for (int32_t i = 0; i < Toolbar.Length(); i++) {
        int32_t k = i;
        Put(Toolbar[k], [&]() { return FormPicture("frmMain", k, 20, 20); });
    }
    Put("about_icon.png", [&]() {
        auto icon = exe->Resources.FirstOrDefault([&](auto& r) {
            return r->Type == "#" + cs::ToStr(NeModule::RtIcon) && cs::BinaryPrimitives::ReadInt32LittleEndian(&exe->Bytes[r->Offset + 4]) == 32;
        });
        if (!icon) throw cs::InvalidDataException("the program has no 32x32 icon");
        return Dib::DecodeIcon(exe->Bytes, icon->Offset);
    });

    // the town's buttons: the 1.1 install's screenshot of the City window, else the help's copy
    // of it, else the program's own pictures
    cs::Ref<RgbaImage> city = nullptr;
    cs::String shot = Find(gameDir, "cityw1.png");
    if (!shot.empty()) {
        try {
            city = Png::Decode(cs::File::ReadAllBytes(shot));
        } catch (const cs::InvalidDataException& ex) {
            notes.Add("cityw1.png does not read: " + cs::String(ex.what()));
        } catch (const cs::IndexOutOfRangeException& ex) {
            notes.Add("cityw1.png does not read: " + cs::String(ex.what()));
        } catch (const cs::ArgumentOutOfRangeException& ex) {  // cs array indexing's IndexOutOfRange
            notes.Add("cityw1.png does not read: " + cs::String(ex.what()));
        }
    }
    cs::Ref<WinHelpFile> hlp = nullptr;
    cs::String hlpPath = Find(gameDir, "MORDOR.HLP");
    if (!hlpPath.empty()) {
        try {
            hlp = WinHelpFile::Read(hlpPath);
        } catch (const cs::InvalidDataException& ex) {
            notes.Add("MORDOR.HLP does not read: " + cs::String(ex.what()));
        } catch (const cs::IndexOutOfRangeException& ex) {
            notes.Add("MORDOR.HLP does not read: " + cs::String(ex.what()));
        } catch (const cs::ArgumentException& ex) {
            notes.Add("MORDOR.HLP does not read: " + cs::String(ex.what()));
        } catch (const cs::ArgumentOutOfRangeException& ex) {
            notes.Add("MORDOR.HLP does not read: " + cs::String(ex.what()));
        }
    }
    if (city == nullptr && hlp != nullptr && hlp->Has("CITYW1.BMP")) {
        try {
            city = Dib::DecodeBmpFile(hlp->File_("CITYW1.BMP"));
        } catch (const cs::InvalidDataException& ex) {
            notes.Add("MORDOR.HLP's CITYW1.BMP does not read: " + cs::String(ex.what()));
        } catch (const cs::IndexOutOfRangeException& ex) {
            notes.Add("MORDOR.HLP's CITYW1.BMP does not read: " + cs::String(ex.what()));
        } catch (const cs::ArgumentOutOfRangeException& ex) {  // cs array indexing's IndexOutOfRange
            notes.Add("MORDOR.HLP's CITYW1.BMP does not read: " + cs::String(ex.what()));
        }
    }
    if (city != nullptr && (city->Width < 471 || city->Height < 203)) {
        notes.Add("the City screenshot is not the expected 487x229");
        city = nullptr;
    }
    if (city == nullptr) notes.Add("the city's buttons are the program's own pictures, scaled: no screenshot of the City window (cityw1.png) here");
    for (auto& t : Town)
        Put(t.Name, [&]() {
            return city != nullptr
                ? city->Crop(t.X, t.Y, 103, 75)
                : FormPicture("frmCityMain", t.FormPicture)->ScaleNearest(103, 75);
        });

    for (auto& kv : made) cs::File::WriteAllBytes(cs::Path::Combine(outDir, kv.Key), Png::Encode(kv.Value));

    // the help
    if (hlp != nullptr) {
        auto failed = [&](const char* msg) { notes.Add("MORDOR.HLP's topics do not read: " + cs::String(msg)); };
        try {
            cs::Ref<HelpBundle_Report> report;
            auto topics = HelpBundle::FromHelpFile(hlp, report);
            for (auto& u : report->Unresolved) notes.Add("help: a jump to no topic, " + cs::ToStr(u));
            for (auto& m : report->Misplaced) notes.Add("help: " + cs::ToStr(m));
            cs::File::WriteAllText(cs::Path::Combine(outDir, HelpFile), HelpBundle::Serialize(topics));
        } catch (const cs::InvalidDataException& ex) {
            failed(ex.what());
        } catch (const cs::IndexOutOfRangeException& ex) {
            failed(ex.what());
        } catch (const cs::ArgumentException& ex) {
            failed(ex.what());
        } catch (const cs::ArgumentOutOfRangeException& ex) {
            failed(ex.what());
        } catch (const cs::FormatException& ex) {
            failed(ex.what());
        }
    } else if (hlpPath.empty()) {
        notes.Add("no help: MORDOR.HLP is not here");
    }

    // the anonymous manifest object, its properties in declaration order
    bool helpMade = cs::File::Exists(cs::Path::Combine(outDir, HelpFile));
    auto manifest = cs::Json::Node::Obj();
    manifest.Add("version", cs::Json::ToJson(Version));
    manifest.Add("game", cs::Json::ToJson(cs::Path::GetFullPath(gameDir)));
    manifest.Add("made", cs::Json::ToJson(cs::DateTime::UtcNow().ToString("u")));
    auto pics = cs::Json::Node::Arr();
    for (auto& p : Pictures) { auto o = cs::Json::Node::Obj(); o.Add("name", cs::Json::ToJson(p->Name)); o.Add("from", cs::Json::ToJson(p->From)); pics.items.push_back(o); }
    manifest.Add("pictures", pics);
    manifest.Add("help", cs::Json::ToJson(helpMade));
    manifest.Add("helpTitle", helpMade && hlp != nullptr ? cs::Json::ToJson(hlp->Title) : cs::Json::Node());
    manifest.Add("notes", cs::Json::ToJson(notes));
    cs::File::WriteAllText(cs::Path::Combine(outDir, ManifestFile), cs::Json::Write(manifest, true));
    return notes;
}

}  // namespace mc
