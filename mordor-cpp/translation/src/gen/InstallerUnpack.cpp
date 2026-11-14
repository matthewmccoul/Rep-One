#include "core.h"

namespace mc {

// Guid.NewGuid().ToString("N")[..12]: twelve lowercase hex digits
static cs::String GuidN12() {
    static cs::Ref<cs::Random> rng = cs::New<cs::Random>();
    const char* hex = "0123456789abcdef";
    cs::String s;
    for (int i = 0; i < 12; i++) s += hex[rng->Next(16)];
    return s;
}

static cs::String GetEnv(const cs::String& name) {
    const char* v = std::getenv(name.c_str());
    return v ? cs::String(v) : cs::String();
}

bool UnpackResult::Ok() { return !Dir.empty(); }

cs::String InstallerUnpack::Root() {
    cs::String env = GetEnv(EnvRoot);
    if (!cs::Str::IsNullOrWhiteSpace(env)) return env;
    return cs::Path::Combine(UserFolders::Local(), "game");
}

cs::String InstallerUnpack::EditionDir(cs::String root, bool full) { return cs::Path::Combine(root, full ? "full" : "shareware"); }

cs::Ref<InstallerUnpack_Contents> InstallerUnpack::Read(cs::String installer) {
    if (!cs::File::Exists(installer)) throw cs::InvalidDataException("there is no such file");
    std::error_code ec;
    int64_t length = (int64_t)std::filesystem::file_size(installer, ec);
    if (length > MaxInstaller) throw cs::InvalidDataException("it is far larger than Mordor's installers");
    return Parse(cs::File::ReadAllBytes(installer));
}

cs::Ref<InstallerUnpack_Contents> InstallerUnpack::Parse(cs::List<uint8_t> exe) {
    return (exe.Count() >= 2 && exe[0] == 'P' && exe[1] == 'K') ? ReadLegacyZip(exe) : ReadArchive(Unwrap(exe));
}

cs::List<uint8_t> InstallerUnpack::Unwrap(cs::List<uint8_t> exe) {
    auto t = Archive(exe);
    auto archive = t.Start;
    auto archiveLength = t.Length;
    return Payload(exe, archive, archiveLength);
}

cs::Ref<InstallerUnpack_Contents> InstallerUnpack::ReadArchive(cs::List<uint8_t> arc) {
    auto records = Records(arc);
    InstallerUnpack_Record project{};
    if (!records.TryGetValue(ProjectKey, project) || !Tag(arc, project.Offset, "tin9"))
        throw cs::InvalidDataException("the archive has no InstallMate project");
    auto pf = Project(arc, project.Offset, project.Length);
    auto folders = pf.Item1;
    auto files = pf.Item2;
    return Layout(arc, records, folders, files);
}

Tup_Start_Length_e3d7c InstallerUnpack::Archive(cs::List<uint8_t> f) {
    if (f.Length() < 0x40 || f[0] != 'M' || f[1] != 'Z') throw cs::InvalidDataException("it is not a Windows program");
    int32_t pe = I32(f, 0x3C);
    if (pe < 0x40 || pe > f.Length() - 24 || !Tag(f, pe, cs::String("PE\0\0", 4))) throw cs::InvalidDataException("it is not a Windows program");
    int32_t sections = U16(f, pe + 6), optional = U16(f, pe + 20);
    int32_t table = pe + 24 + optional;
    if (sections > 96 || table < 0 || (int64_t)table + sections * 40LL > f.Length()) throw cs::InvalidDataException("its section table runs off the end of the file");
    for (int32_t i = 0; i < sections; i++) {
        int32_t s = table + i * 40;
        if (cs::Str::TrimEnd(cs::Encoding::Latin1GetString(f.Raw().data() + s, 8), '\0') != ".tsuarch") continue;
        int64_t raw = (int64_t)(uint32_t)I32(f, s + 16), ptr = (int64_t)(uint32_t)I32(f, s + 20);
        if (ptr + raw > f.Length() || raw < 0x60) throw cs::InvalidDataException("its archive runs off the end of the file");
        int32_t start = (int32_t)ptr;
        if (!Tag(f, start + 0x10, "tiz3") || U16(f, start + 0x14) != 0x3C) throw cs::InvalidDataException("its archive is not an InstallMate tiz3 archive");
        int64_t length = (int64_t)(uint32_t)I32(f, start + 0x20);
        if (length < 0x4D || length > raw) throw cs::InvalidDataException("its archive's length is wrong");
        return Tup_Start_Length_e3d7c{start, (int32_t)length};
    }
    throw cs::InvalidDataException("it has no InstallMate archive (.tsuarch)");
}

cs::List<uint8_t> InstallerUnpack::Payload(cs::List<uint8_t> f, int32_t start, int32_t length) {
    int32_t props = start + 0x10 + 0x38, data = start + 0x10 + 0x3D;
    cs::List<uint8_t> arc;
    int32_t used = 0;
    try { arc = Lzma::DecodeToEnd(f.GetRange(props, 5), f, data, start + length - data, MaxUnpacked, used); }
    catch (const cs::InvalidDataException& ex) { throw cs::InvalidDataException("its archive does not unpack (" + cs::String(ex.what()) + ")"); }
    if (data + used != start + length) throw cs::InvalidDataException("its archive does not end where it says");
    return arc;
}

cs::Dictionary<uint64_t, InstallerUnpack_Record> InstallerUnpack::Records(cs::List<uint8_t> arc) {
    auto map = cs::Dictionary<uint64_t, InstallerUnpack_Record>::New();
    int64_t at = 0;
    while (at < arc.Length()) {
        if (at + 0x40 > arc.Length() || !Tag(arc, (int32_t)at, "tzf3")) throw cs::InvalidDataException("its archive's records do not read");
        int64_t len = cs::BinaryPrimitives::ReadInt64LittleEndian(arc.Raw().data() + ((int32_t)at + 0x10));
        if (len < 0 || len > (int64_t)arc.Length() - at - 0x40) throw cs::InvalidDataException("a record runs off the end of the archive");
        uint64_t key = (uint64_t)cs::BinaryPrimitives::ReadInt64LittleEndian(arc.Raw().data() + ((int32_t)at + 8));
        if (!map.TryAdd(key, InstallerUnpack_Record{(int32_t)at + 0x40, (int32_t)len})) throw cs::InvalidDataException("two records have the same key");
        if (map.Count() > MaxRecords) throw cs::InvalidDataException("the archive has too many records");
        at += 0x40 + len;
    }
    return map;
}

Tup_Item1_Item2_b8a70 InstallerUnpack::Project(cs::List<uint8_t> a, int32_t start, int32_t length) {
    auto folders = cs::Dictionary<uint64_t, cs::Ref<InstallerUnpack_Folder>>::New();
    auto files = cs::List<cs::Ref<InstallerUnpack_FileEntry>>::New();
    int32_t end = start + length;
    for (int32_t i = start; i + 0x60 <= end; i++) {
        if (a[i] == 'f' && a[i + 1] == 'i' && Tag(a, i, "file") && I32(a, i + 4) == 0) {
            int32_t at = i + 0x54;
            cs::String name{};
            cs::String stored{};
            if (!ReadName(a, at, end, name) || !ReadName(a, at, end, stored)) continue;
            if (!IsStoredName(stored, name)) continue;
            files.Add(cs::New<InstallerUnpack_FileEntry>(U64(a, i + 8), U64(a, i + 0x2C), cs::BinaryPrimitives::ReadInt64LittleEndian(a.Raw().data() + (i + 0x3C)), name));
        } else if (a[i] == 'f' && a[i + 1] == 'l' && Tag(a, i, "fldr") && I32(a, i + 4) == 0) {
            // the parent's key and the names follow a run of fields whose length varies with the
            // kind of folder (0x2C for the install's own, 0x28 and 0x24 for Windows' and
            // InstallMate's): the one at which both names read is the one
            for (int32_t p : cs::List<int32_t>::Of({0x2C, 0x28, 0x24})) {
                int32_t at = i + p + 8;
                cs::String inner{};
                cs::String name{};
                if (!ReadName(a, at, end, inner) || !ReadName(a, at, end, name)) continue;
                folders.TryAdd(U64(a, i + 8), cs::New<InstallerUnpack_Folder>(U64(a, i + 8), U64(a, i + p), inner, name));
                break;
            }
        }
    }
    return Tup_Item1_Item2_b8a70{folders, files};
}

bool InstallerUnpack::ReadName(cs::List<uint8_t> a, int32_t& at, int32_t end, cs::String& s) {
    s = cs::String();
    if (at + 4 > end) return false;
    int32_t n = I32(a, at);
    if (n < 1 || n > 260 || at + 4 + n + 4 > end) return false;
    for (int32_t k = 0; k < n; k++) if (a[at + 4 + k] < 0x20 || a[at + 4 + k] > 0x7E) return false;
    if (I32(a, at + 4 + n) != 0) return false;
    s = cs::Encoding::Latin1GetString(a.Raw().data() + (at + 4), n);
    at += 8 + n;
    return true;
}

bool InstallerUnpack::IsStoredName(cs::String stored, cs::String name) {
    if ((int32_t)stored.size() != (int32_t)name.size() + 17 || stored.at(name.size()) != '_') return false;
    cs::String tail = cs::Str::Substring(stored, (int32_t)name.size() + 1);
    for (char c : tail)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return false;
    for (int32_t k = 0; k < (int32_t)name.size(); k++)
        if (stored[k] != name[k] && stored[k] != '_') return false;
    return true;
}

cs::Ref<InstallerUnpack_Contents> InstallerUnpack::Layout(cs::List<uint8_t> arc, cs::Dictionary<uint64_t, InstallerUnpack_Record> records, cs::Dictionary<uint64_t, cs::Ref<InstallerUnpack_Folder>> folders, cs::List<cs::Ref<InstallerUnpack_FileEntry>> files) {
    auto install = folders.Values().Where([&](auto& f) { return f->Internal == InstallDir; }).ToList();
    if (install.Count() != 1) throw cs::InvalidDataException("its project has no install folder");
    // keyed by the upper-cased path: StringComparer.OrdinalIgnoreCase in the C#
    auto taken = cs::Dictionary<cs::String, cs::Ref<InstallerUnpack_Entry>>::New();
    auto skipped = cs::List<cs::String>::New();
    for (auto& file : files) {
        auto path = PathOf(file, folders, install[0]->Key);
        if (path == nullptr) { skipped.Add(Describe(file, folders)); continue; }
        InstallerUnpack_Record rec{};
        if (!records.TryGetValue(file->Key, rec)) throw cs::InvalidDataException(file->Name + " is named but not in the archive");
        if (rec.Length != file->Size) throw cs::InvalidDataException(file->Name + " is not the size the project says");
        cs::String rel = cs::Str::Join(cs::String(1, cs::Path::DirectorySeparatorChar()), path);
        auto data = cs::List<uint8_t>(std::vector<uint8_t>(arc.Raw().begin() + rec.Offset, arc.Raw().begin() + rec.Offset + rec.Length));
        cs::Ref<InstallerUnpack_Entry> already;
        if (taken.TryGetValue(cs::Str::ToUpperInvariant(rel), already)) {
            // the same file put in two places that the port's layout makes one: harmless if the same
            if (already->Data.SequenceEqual(data)) continue;
            throw cs::InvalidDataException("two different files would both be " + rel);
        }
        taken[cs::Str::ToUpperInvariant(rel)] = cs::New<InstallerUnpack_Entry>(rel, data);
    }
    if (taken.Count() == 0) throw cs::InvalidDataException("it installs no files");
    // a record the project names no file for, other than the setup's own: said, so a file the
    // project names in some way this does not read is not lost without a word
    auto named = cs::HashSet<uint64_t>::From(files.Select([&](auto& f) { return f->Key; }));
    int32_t unnamed = records.Keys().Count([&](auto& k) { return (k >> 16) != 0xFFFFFFFFFFFFULL && !named.Contains(k); });
    if (unnamed > 0) skipped.Add(cs::ToStr(unnamed) + " record" + (unnamed == 1 ? "" : "s") + " the project names no file for");
    return cs::New<InstallerUnpack_Contents>(taken.Values().OrderBy([&](auto& e) { return cs::Str::ToUpperInvariant(e->RelativePath); }).ToList(), skipped);
}

cs::List<cs::String> InstallerUnpack::PathOf(cs::Ref<InstallerUnpack_FileEntry> file, cs::Dictionary<uint64_t, cs::Ref<InstallerUnpack_Folder>> folders, uint64_t install) {
    CheckName(file->Name);
    auto parts = cs::List<cs::String>::Of({file->Name});
    uint64_t at = file->Folder;
    for (int32_t depth = 0;; depth++) {
        if (depth > MaxDepth) throw cs::InvalidDataException("its folders go round in a circle");
        if (at == install) break;
        cs::Ref<InstallerUnpack_Folder> folder;
        if (!folders.TryGetValue(at, folder)) return nullptr;
        if (folder->Internal == System16 && depth == 0) return parts;  // the VB3 runtime, beside MORDOR.EXE
        CheckName(folder->Name);
        parts.Insert(0, folder->Name);
        at = folder->Parent;
    }
    // DATA\, MUSIC\ and images\ straight under the install folder: into the top folder
    if (parts.Count() == 2 && Flattened.ToList().Any([&](auto& x) { return cs::Str::EqualsIgnoreCase(x, parts[0]); })) parts.RemoveAt(0);
    return parts;
}

cs::String InstallerUnpack::Describe(cs::Ref<InstallerUnpack_FileEntry> file, cs::Dictionary<uint64_t, cs::Ref<InstallerUnpack_Folder>> folders) {
    cs::Ref<InstallerUnpack_Folder> f;
    return file->Name + (folders.TryGetValue(file->Folder, f) ? " (for " + f->Name + ")" : "");
}

void InstallerUnpack::CheckName(cs::String name) {
    if (!IsPlainName(name)) throw cs::InvalidDataException("it names a file \"" + name + "\" that cannot be written safely");
}

bool InstallerUnpack::IsPlainName(cs::String name) {
    if (cs::Str::IsNullOrWhiteSpace(name) || (int32_t)name.size() > MaxNameLength) return false;
    if (name == "." || name == ".." || cs::Str::EndsWith(name, '.') || cs::Str::EndsWith(name, ' ')) return false;
    for (char c : name)
        if ((unsigned char)c < 0x20 || cs::Str::Contains(cs::String("<>:\"/\\|?*"), c)) return false;
    cs::String stem = cs::Str::ToUpperInvariant(cs::Str::TrimEnd(cs::Str::Split(name, '.')[0]));
    if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" || ((int32_t)stem.size() == 4 && (cs::Str::StartsWith(stem, "COM") || cs::Str::StartsWith(stem, "LPT")) && (stem[3] >= '0' && stem[3] <= '9')))
        return false;
    return !std::filesystem::path(name).has_root_path();
}

cs::Ref<UnpackResult> InstallerUnpack::Unpack(cs::String installer, cs::String root) {
    auto none = cs::List<cs::String>::New();
    cs::Ref<InstallerUnpack_Contents> contents;
    auto notInstaller = [&](const std::exception& ex) {
        return cs::New<UnpackResult>(cs::String(), NotAnInstaller + " " + NotAnInstallerHint, nullptr, none, none, cs::Path::GetFileName(installer) + ": " + cs::String(ex.what()));
    };
    // the C# filter: InvalidDataException, IOException (and its subclasses), UnauthorizedAccessException,
    // ArgumentException (and its subclasses), NotSupportedException
    try { contents = Read(installer); }
    catch (const cs::InvalidDataException& ex) { return notInstaller(ex); }
    catch (const cs::IOException& ex) { return notInstaller(ex); }
    catch (const cs::FileNotFoundException& ex) { return notInstaller(ex); }
    catch (const cs::DirectoryNotFoundException& ex) { return notInstaller(ex); }
    catch (const cs::EndOfStreamException& ex) { return notInstaller(ex); }
    catch (const cs::UnauthorizedAccessException& ex) { return notInstaller(ex); }
    catch (const cs::ArgumentException& ex) { return notInstaller(ex); }
    catch (const cs::ArgumentNullException& ex) { return notInstaller(ex); }
    catch (const cs::ArgumentOutOfRangeException& ex) { return notInstaller(ex); }
    catch (const cs::NotSupportedException& ex) { return notInstaller(ex); }

    cs::String top, work;
    try {
        top = cs::Path::GetFullPath(root.empty() ? Root() : root);
        cs::Directory::CreateDirectory(top);
        work = cs::Path::Combine(top, ".unpacking-" + GuidN12());
        cs::Directory::CreateDirectory(work);
    } catch (const std::exception& ex) {
        return cs::New<UnpackResult>(cs::String(), "The game could not be unpacked: there is no folder for it (" + cs::String(ex.what()) + ").", nullptr, none, none);
    }

    try {
        cs::String trimmed = work;
        if (trimmed.size() > 1 && (trimmed.back() == '/' || trimmed.back() == cs::Path::DirectorySeparatorChar())) trimmed.pop_back();
        cs::String within = trimmed + cs::Path::DirectorySeparatorChar();
        for (auto& e : contents->Files) {
            cs::String dest = cs::Path::GetFullPath(cs::Path::Combine(work, e->RelativePath));
            // the names were checked one by one; this is the belt to those braces
            if (!cs::Str::StartsWithIgnoreCase(dest, within)) throw cs::InvalidDataException("a file would go outside the game folder");
            cs::Directory::CreateDirectory(cs::Path::GetDirectoryName(dest));
            cs::File::WriteAllBytes(dest, e->Data);
        }
        auto check = GameFiles::Check(work);
        if (!check->IsPlayable()) {
            TryDelete(work);
            return cs::New<UnpackResult>(cs::String(), NotAnInstaller + " " + NotAnInstallerHint, check, none, none,
                                         cs::Path::GetFileName(installer) + ": it unpacks, but not to a Mordor to play - " + check->Reason);
        }
        cs::String dir = EditionDir(top, check->IsFull());
        cs::String old{};
        if (cs::Directory::Exists(dir)) {
            old = cs::Path::Combine(top, ".replaced-" + GuidN12());
            std::filesystem::rename(dir, old);
        }
        try { std::filesystem::rename(work, dir); }
        catch (...) {
            if (!old.empty()) try { std::filesystem::rename(old, dir); } catch (...) { }
            throw;
        }
        if (!old.empty()) TryDelete(old);
        auto placed = GameFiles::Check(dir);
        return cs::New<UnpackResult>(dir, "Unpacked " + (!placed->Recognised.empty() ? placed->Recognised : (placed->IsFull() ? cs::String("the full game") : cs::String("the shareware"))) + " into " + dir + ".",
                                     placed, contents->Files.Select([&](auto& f) { return f->RelativePath; }).ToList(), contents->Skipped);
    } catch (const std::exception& ex) {
        TryDelete(work);
        return cs::New<UnpackResult>(cs::String(), "The game could not be unpacked (" + cs::String(ex.what()) + ").", nullptr, none, none);
    }
}

void InstallerUnpack::TryDelete(cs::String dir) {
    try { if (cs::Directory::Exists(dir)) cs::Directory::Delete(dir, true); } catch (...) { }
}

cs::List<cs::String> InstallerUnpack::Unpacked(cs::String root) {
    cs::List<cs::String> _result = cs::List<cs::String>::New();
    cs::String top = root.empty() ? Root() : root;
    _result.Add(EditionDir(top, true));
    _result.Add(EditionDir(top, false));
    return _result;
}

cs::List<cs::String> InstallerUnpack::InDownloads(cs::String downloads) {
    cs::String dir = downloads.empty() ? DownloadsFolder() : downloads;
    auto found = cs::List<cs::String>::New();
    if (cs::Str::IsNullOrEmpty(dir)) return found;
    for (auto& name : InstallerNames) {
        try {
            cs::String p = cs::Path::Combine(dir, name);
            if (cs::File::Exists(p)) found.Add(p);
        } catch (...) { }
    }
    return found;
}

cs::String InstallerUnpack::DownloadsFolder() {
    cs::String env = GetEnv(EnvDownloads);
    if (!cs::Str::IsNullOrWhiteSpace(env)) return env;
#ifdef _WIN32
    {
        cs::String known = KnownDownloads();
        if (!cs::Str::IsNullOrEmpty(known)) return known;
    }
    cs::String profile = GetEnv("USERPROFILE");
#else
    cs::String profile = GetEnv("HOME");
#endif
    return profile == "" ? cs::String() : cs::Path::Combine(profile, "Downloads");
}

cs::String InstallerUnpack::KnownDownloads() {
    // P/Invoke (SHGetKnownFolderPath) not translated: the C#'s failure path returns null
    return {};
}

int32_t InstallerUnpack::SHGetKnownFolderPath(cs::Guid& id, uint32_t flags, intptr_t token, intptr_t& path) {
    (void)id; (void)flags; (void)token; (void)path;
    throw cs::NotSupportedException("not translated: P/Invoke shell32 SHGetKnownFolderPath");
}

bool InstallerUnpack::Tag(cs::List<uint8_t> a, int32_t at, cs::String tag) {
    if (at < 0 || at + (int32_t)tag.size() > a.Length()) return false;
    for (int32_t k = 0; k < (int32_t)tag.size(); k++) if (a[at + k] != (uint8_t)tag[k]) return false;
    return true;
}

int32_t InstallerUnpack::U16(cs::List<uint8_t> a, int32_t at) {
    if (at >= 0 && at + 2 <= a.Length()) return a[at] | (a[at + 1] << 8);
    throw cs::InvalidDataException("it ends too soon");
}

int32_t InstallerUnpack::I32(cs::List<uint8_t> a, int32_t at) {
    if (at >= 0 && at + 4 <= a.Length()) return cs::BinaryPrimitives::ReadInt32LittleEndian(a.Raw().data() + at);
    throw cs::InvalidDataException("it ends too soon");
}

uint64_t InstallerUnpack::U64(cs::List<uint8_t> a, int32_t at) {
    if (at >= 0 && at + 8 <= a.Length()) return (uint64_t)cs::BinaryPrimitives::ReadInt64LittleEndian(a.Raw().data() + at);
    throw cs::InvalidDataException("it ends too soon");
}

}  // namespace mc
