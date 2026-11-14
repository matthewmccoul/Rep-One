#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct UnpackResult {
    cs::String Dir{};
    cs::String Message{};
    cs::Ref<GameFilesCheck> Check;
    cs::List<cs::String> Files;
    cs::List<cs::String> Skipped;
    cs::String Detail{};

    UnpackResult() = default;
    UnpackResult(cs::String Dir, cs::String Message, cs::Ref<GameFilesCheck> Check, cs::List<cs::String> Files, cs::List<cs::String> Skipped, cs::String Detail = {})
        : Dir(Dir), Message(Message), Check(Check), Files(Files), Skipped(Skipped), Detail(Detail) {}

    bool Ok();
};

struct InstallerUnpack_Entry {
    cs::String RelativePath{};
    cs::List<uint8_t> Data;

    InstallerUnpack_Entry() = default;
    InstallerUnpack_Entry(cs::String RelativePath, cs::List<uint8_t> Data) : RelativePath(RelativePath), Data(Data) {}
};

struct InstallerUnpack_Contents {
    cs::List<cs::Ref<InstallerUnpack_Entry>> Files;
    cs::List<cs::String> Skipped;

    InstallerUnpack_Contents() = default;
    InstallerUnpack_Contents(cs::List<cs::Ref<InstallerUnpack_Entry>> Files, cs::List<cs::String> Skipped) : Files(Files), Skipped(Skipped) {}
};

struct InstallerUnpack_Record {
    int32_t Offset = 0;
    int32_t Length = 0;

    bool operator==(const InstallerUnpack_Record& o) const { return Offset == o.Offset && Length == o.Length; }
    bool operator!=(const InstallerUnpack_Record& o) const { return !(*this == o); }
};

struct InstallerUnpack_Folder {
    uint64_t Key = 0;
    uint64_t Parent = 0;
    cs::String Internal{};
    cs::String Name{};

    InstallerUnpack_Folder() = default;
    InstallerUnpack_Folder(uint64_t Key, uint64_t Parent, cs::String Internal, cs::String Name) : Key(Key), Parent(Parent), Internal(Internal), Name(Name) {}
};

struct InstallerUnpack_FileEntry {
    uint64_t Key = 0;
    uint64_t Folder = 0;
    int64_t Size = 0;
    cs::String Name{};

    InstallerUnpack_FileEntry() = default;
    InstallerUnpack_FileEntry(uint64_t Key, uint64_t Folder, int64_t Size, cs::String Name) : Key(Key), Folder(Folder), Size(Size), Name(Name) {}
};

struct InstallerUnpack {
    using Entry = InstallerUnpack_Entry;
    using Contents = InstallerUnpack_Contents;
    using Record = InstallerUnpack_Record;
    using Folder = InstallerUnpack_Folder;
    using FileEntry = InstallerUnpack_FileEntry;

    inline static const cs::String NotAnInstaller = "This file isn't a Mordor installer OpenDejenol can read.";
    inline static const cs::String NotAnInstallerHint = "Choose M1FULL.zip, MordorSetup.exe or MordorDemoSetup.exe, as downloaded from Decklin's Domain.";

    inline static cs::List<cs::String> InstallerNames = cs::List<cs::String>::Of({"MordorSetup.exe", "M1FULL.zip", "MordorDemoSetup.exe"});

    inline static const cs::String EnvRoot = "MORDOR_UNPACK_DIR";

    // caps on a user-chosen file: the full installer is 6 MB and unpacks to 14 MB in 310 records
    static constexpr int64_t MaxInstaller = 64LL << 20;
    static constexpr int32_t MaxUnpacked = 128 << 20;
    static constexpr int32_t MaxRecords = 10000;
    static constexpr int32_t MaxNameLength = 128;
    static constexpr int32_t MaxDepth = 16;

    static constexpr uint64_t ProjectKey = 0xFFFFFFFFFFFF0002ULL;

    inline static const cs::String InstallDir = "INSTALLDIR";
    inline static const cs::String System16 = "System16Folder";

    // compared case-insensitively (StringComparer.OrdinalIgnoreCase in the C#)
    inline static cs::HashSet<cs::String> Flattened = cs::HashSet<cs::String>::From(cs::List<cs::String>::Of({"DATA", "MUSIC", "images"}));

    inline static const cs::String EnvDownloads = "MORDOR_DOWNLOADS";

    static cs::String Root();
    static cs::String EditionDir(cs::String root, bool full);
    static cs::Ref<InstallerUnpack_Contents> Read(cs::String installer);
    static cs::Ref<InstallerUnpack_Contents> Parse(cs::List<uint8_t> exe);
    static cs::List<uint8_t> Unwrap(cs::List<uint8_t> exe);
    static cs::Ref<InstallerUnpack_Contents> ReadArchive(cs::List<uint8_t> arc);
    static Tup_Start_Length_e3d7c Archive(cs::List<uint8_t> f);
    static cs::List<uint8_t> Payload(cs::List<uint8_t> f, int32_t start, int32_t length);
    static cs::Dictionary<uint64_t, InstallerUnpack_Record> Records(cs::List<uint8_t> arc);
    static Tup_Item1_Item2_b8a70 Project(cs::List<uint8_t> a, int32_t start, int32_t length);
    static bool ReadName(cs::List<uint8_t> a, int32_t& at, int32_t end, cs::String& s);
    static bool IsStoredName(cs::String stored, cs::String name);
    static cs::Ref<InstallerUnpack_Contents> Layout(cs::List<uint8_t> arc, cs::Dictionary<uint64_t, InstallerUnpack_Record> records, cs::Dictionary<uint64_t, cs::Ref<InstallerUnpack_Folder>> folders, cs::List<cs::Ref<InstallerUnpack_FileEntry>> files);
    static cs::List<cs::String> PathOf(cs::Ref<InstallerUnpack_FileEntry> file, cs::Dictionary<uint64_t, cs::Ref<InstallerUnpack_Folder>> folders, uint64_t install);
    static cs::String Describe(cs::Ref<InstallerUnpack_FileEntry> file, cs::Dictionary<uint64_t, cs::Ref<InstallerUnpack_Folder>> folders);
    static void CheckName(cs::String name);
    static bool IsPlainName(cs::String name);
    static cs::Ref<UnpackResult> Unpack(cs::String installer, cs::String root = {});
    static void TryDelete(cs::String dir);
    static cs::List<cs::String> Unpacked(cs::String root = {});
    static cs::List<cs::String> InDownloads(cs::String downloads = {});
    static cs::String DownloadsFolder();
    static cs::String KnownDownloads();
    static int32_t SHGetKnownFolderPath(cs::Guid& id, uint32_t flags, intptr_t token, intptr_t& path);
    static bool Tag(cs::List<uint8_t> a, int32_t at, cs::String tag);
    static int32_t U16(cs::List<uint8_t> a, int32_t at);
    static int32_t I32(cs::List<uint8_t> a, int32_t at);
    static uint64_t U64(cs::List<uint8_t> a, int32_t at);

    // InstallerUnpack.Zip.cs
    inline static cs::List<uint32_t> ZipCrcTable;
    static cs::Ref<InstallerUnpack_Contents> ReadLegacyZip(cs::List<uint8_t> bytes);
    static cs::Dictionary<cs::String, cs::ZipArchiveEntry> ZipIndex(cs::Ref<cs::ZipArchive> zip, int32_t& entriesLeft);
    static cs::List<uint8_t> ZipBytes(cs::ZipArchiveEntry entry, int32_t& bytesLeft);
};

}  // namespace mc
