#include "core.h"

namespace mc {

cs::Ref<InstallerUnpack_Contents> InstallerUnpack::ReadLegacyZip(cs::List<uint8_t> bytes) {
    if (bytes.Length() > MaxInstaller) throw cs::InvalidDataException("the ZIP is too large");
    throw cs::NotSupportedException("not translated: ZIP archive reading (System.IO.Compression)");
}

cs::Dictionary<cs::String, cs::ZipArchiveEntry> InstallerUnpack::ZipIndex(cs::Ref<cs::ZipArchive> zip, int32_t& entriesLeft) {
    (void)zip;
    (void)entriesLeft;
    throw cs::NotSupportedException("not translated: ZIP archive entry index (System.IO.Compression, case-insensitive dictionary)");
}

cs::List<uint8_t> InstallerUnpack::ZipBytes(cs::ZipArchiveEntry entry, int32_t& bytesLeft) {
    (void)entry;
    (void)bytesLeft;
    throw cs::NotSupportedException("not translated: ZIP entry decompression (System.IO.Compression)");
}

}  // namespace mc
