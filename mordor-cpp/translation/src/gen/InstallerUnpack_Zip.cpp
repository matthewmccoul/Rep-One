#include "core.h"

namespace mc {

cs::Ref<InstallerUnpack_Contents> InstallerUnpack::ReadLegacyZip(cs::List<uint8_t> bytes) {
    if (bytes.Length() > MaxInstaller) throw cs::InvalidDataException("the ZIP is too large");
    auto zip = cs::ZipArchive::Open(bytes);
    int32_t entriesLeft = MaxRecords, bytesLeft = MaxUnpacked;
    auto outer = ZipIndex(zip, entriesLeft);
    // Dictionary(StringComparer.OrdinalIgnoreCase): keys are compared upper-cased; the first spelling is kept
    auto games = outer.Pairs().Where([&](auto& p) { return cs::Str::EqualsIgnoreCase(cs::Str::Split(p.Key, '/').Last(), "MORDOR.WIP"); }).ToArray();
    if (games.Length() != 1) throw cs::InvalidDataException("choose the M1FULL.zip package containing one MORDOR.WIP");
    cs::String prefix = cs::Str::Substring(games[0].Key, 0, (int32_t)games[0].Key.size() - (int32_t)cs::String("MORDOR.WIP").size());
    auto files = cs::Dictionary<cs::String, cs::Ref<InstallerUnpack_Entry>>::New();   // keyed upper-case (OrdinalIgnoreCase)
    auto read = cs::HashSet<cs::String>::New();                                       // upper-case (OrdinalIgnoreCase)
    for (cs::String name : {cs::String("MORDOR.WIP"), cs::String("WAVES.WIP")})
    {
        cs::String path = prefix + name;
        cs::ZipArchiveEntry archive;
        bool found = false;
        for (auto& kv : outer) if (cs::Str::EqualsIgnoreCase(kv.Key, path)) { archive = kv.Value; found = true; break; }
        if (!found) continue; // audio is optional
        read.Add(cs::Str::ToUpperInvariant(path));
        auto payload = ZipBytes(archive, bytesLeft);
        auto inner = cs::ZipArchive::Open(payload);
        for (auto& kv : ZipIndex(inner, entriesLeft))
        {
            cs::String relative = kv.Key; auto entry = kv.Value;
            CheckName(relative); // these packages contain flat files, not install paths
            auto data = ZipBytes(entry, bytesLeft);
            if (!files.TryAdd(cs::Str::ToUpperInvariant(relative), cs::New<InstallerUnpack_Entry>(relative, data)))
                throw cs::InvalidDataException("two ZIP entries would both be " + relative);
        }
    }
    return cs::New<InstallerUnpack_Contents>(files.Values().OrderBy([&](auto& e) { return cs::Str::ToUpperInvariant(e->RelativePath); }).ToArray(),
        outer.Keys().Where([&](auto& p) { return !read.Contains(cs::Str::ToUpperInvariant(p)); }).OrderBy([&](auto& p) { return cs::Str::ToUpperInvariant(p); }).ToArray());
}

cs::Dictionary<cs::String, cs::ZipArchiveEntry> InstallerUnpack::ZipIndex(cs::Ref<cs::ZipArchive> zip, int32_t& entriesLeft) {
    if (zip->Entries.Count() > entriesLeft) throw cs::InvalidDataException("the ZIP contains too many entries");
    entriesLeft -= zip->Entries.Count();
    // Dictionary(StringComparer.OrdinalIgnoreCase): the C++ map keeps the first spelling; seen holds the upper-cased keys
    auto result = cs::Dictionary<cs::String, cs::ZipArchiveEntry>::New();
    auto seen = cs::HashSet<cs::String>::New();
    for (auto& entry : zip->Entries)
    {
        cs::String path = cs::Str::Replace(entry.FullName, "\\", "/");
        bool directory = cs::Str::EndsWith(path, "/");
        auto parts = cs::Str::Split(directory ? cs::Str::Substring(path, 0, (int32_t)path.size() - 1) : path, '/');
        if (parts.Length() > MaxDepth) throw cs::InvalidDataException("ZIP folders are nested too deeply");
        for (auto& part : parts) CheckName(part);
        if (!directory) {
            if (!seen.Add(cs::Str::ToUpperInvariant(path))) throw cs::InvalidDataException("the ZIP repeats " + path);
            result.Add(path, entry);
        }
    }
    return result;
}

cs::List<uint8_t> InstallerUnpack::ZipBytes(cs::ZipArchiveEntry entry, int32_t& bytesLeft) {
    if (entry.Length < 0 || entry.Length > bytesLeft)
        throw cs::InvalidDataException("the ZIP expands beyond the unpacking limit");
    bytesLeft -= (int32_t)entry.Length;
    auto source = entry.OpenAndRead(entry.Length);
    if (source.Count() < entry.Length) throw cs::EndOfStreamException("Unable to read beyond the end of the stream.");   // ReadExactly
    if (source.Count() > entry.Length) throw cs::InvalidDataException("a ZIP entry is longer than declared");
    auto result = source;
    uint32_t crc = 0xFFFFFFFF;
    for (auto b : result) crc = ZipCrcTable[(crc ^ b) & 255] ^ (crc >> 8);
    if (~crc != entry.Crc32) throw cs::InvalidDataException("a ZIP entry failed its checksum");
    return result;
}

}  // namespace mc
