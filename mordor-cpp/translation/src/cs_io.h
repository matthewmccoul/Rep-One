// System.Security.Cryptography.SHA256, System.IO.Compression (ZLibStream, ZipArchive) and Convert.ToHexString,
// as Mordor.Core uses them. Compression goes through zlib (the library .NET itself wraps; Unreal ships it too).
#pragma once
#include <zlib.h>
namespace cs {

// ---------------------------------------------------------------- SHA-256 (FIPS 180-4)
namespace SHA256 {
inline Bytes HashData(const uint8_t* data, size_t n) {
    static const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
    uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
    auto rotr = [](uint32_t x, int r) { return (x >> r) | (x << (32 - r)); };
    std::vector<uint8_t> m(data, data + n);
    uint64_t bits = (uint64_t)n * 8; m.push_back(0x80);
    while (m.size() % 64 != 56) m.push_back(0);
    for (int i = 7; i >= 0; i--) m.push_back((uint8_t)(bits >> (i * 8)));
    for (size_t off = 0; off < m.size(); off += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) w[i] = (uint32_t)m[off + i * 4] << 24 | (uint32_t)m[off + i * 4 + 1] << 16 | (uint32_t)m[off + i * 4 + 2] << 8 | m[off + i * 4 + 3];
        for (int i = 16; i < 64; i++) { uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3), s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10); w[i] = w[i - 16] + s0 + w[i - 7] + s1; }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25), ch = (e & f) ^ (~e & g), t1 = hh + S1 + ch + K[i] + w[i];
            uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22), mj = (a & b) ^ (a & c) ^ (b & c), t2 = S0 + mj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    Bytes r = Bytes::New(32);
    for (int i = 0; i < 8; i++) for (int k = 0; k < 4; k++) r[i * 4 + k] = (uint8_t)(h[i] >> (24 - k * 8));
    return r;
}
inline Bytes HashData(const Bytes& b) { return HashData(b.Raw().data(), (size_t)b.Count()); }
// SHA256.HashData(stream) over a file opened with File.OpenRead
inline Bytes HashFile(const String& path) { return HashData(File::ReadAllBytes(path)); }
}  // namespace SHA256

namespace Convert {
// Convert.ToHexString: upper-case, two digits a byte
inline String ToHexString(const Bytes& b) { static const char* hx = "0123456789ABCDEF"; String s; for (auto x : b) { s += hx[x >> 4]; s += hx[x & 15]; } return s; }
}

// ---------------------------------------------------------------- zlib streams
namespace Compression {
enum class CompressionLevel { Optimal, Fastest, NoCompression, SmallestSize };
inline int ZLevel(CompressionLevel l) {   // .NET's mapping onto zlib levels
    switch (l) { case CompressionLevel::Optimal: return 6; case CompressionLevel::Fastest: return 1; case CompressionLevel::NoCompression: return 0; case CompressionLevel::SmallestSize: return 9; }
    return 6;
}
// windowBits 15: ZLibStream (zlib header + Adler-32); -15: DeflateStream (raw, as in ZIP entries)
inline Bytes Deflate(const uint8_t* p, size_t n, CompressionLevel level, int windowBits) {
    z_stream z{}; if (deflateInit2(&z, ZLevel(level), Z_DEFLATED, windowBits, 8, Z_DEFAULT_STRATEGY) != Z_OK) throw IOException("zlib deflateInit failed");
    std::vector<uint8_t> out(deflateBound(&z, (uLong)n) + 16);
    z.next_in = const_cast<Bytef*>(p); z.avail_in = (uInt)n; z.next_out = out.data(); z.avail_out = (uInt)out.size();
    int r = deflate(&z, Z_FINISH); size_t len = out.size() - z.avail_out; deflateEnd(&z);
    if (r != Z_STREAM_END) throw IOException("zlib deflate failed");
    out.resize(len); return Bytes(out);
}
// Reads until the compressed stream ends. limit: stop once more than this many bytes have come out (-1: no limit).
inline Bytes Inflate(const uint8_t* p, size_t n, int windowBits, long long limit = -1) {
    z_stream z{}; if (inflateInit2(&z, windowBits) != Z_OK) throw IOException("zlib inflateInit failed");
    std::vector<uint8_t> out; uint8_t buf[65536];
    z.next_in = const_cast<Bytef*>(p); z.avail_in = (uInt)n;
    while (true) {
        z.next_out = buf; z.avail_out = sizeof buf;
        int r = inflate(&z, Z_NO_FLUSH);
        out.insert(out.end(), buf, buf + (sizeof buf - z.avail_out));
        if (r == Z_STREAM_END) break;
        if (r != Z_OK) { inflateEnd(&z); if (r == Z_BUF_ERROR && z.avail_in == 0) break;   // truncated: .NET returns what it has
            throw InvalidDataException("The archive entry was compressed using an unsupported compression method."); }
        if (limit >= 0 && (long long)out.size() > limit) break;
    }
    inflateEnd(&z); return Bytes(out);
}
}  // namespace Compression

// ---------------------------------------------------------------- ZipArchive (read mode)
struct ZipArchive;
struct ZipArchiveEntry {
    std::shared_ptr<const std::vector<uint8_t>> data;   // the whole archive
    String FullName; String Name; long long Length = 0; long long CompressedLength = 0; uint32_t Crc32 = 0;
    int method = 0; size_t localHeader = 0;
    // entry.Open() read to the end, stopping one byte past limit (enough for ReadExactly + the ReadByte check)
    Bytes OpenAndRead(long long limit) const {
        const auto& d = *data;
        if (localHeader + 30 > d.size() || d[localHeader] != 'P' || d[localHeader + 1] != 'K' || d[localHeader + 2] != 3 || d[localHeader + 3] != 4)
            throw InvalidDataException("A local file header is corrupt.");
        size_t nameLen = d[localHeader + 26] | d[localHeader + 27] << 8, extraLen = d[localHeader + 28] | d[localHeader + 29] << 8;
        size_t start = localHeader + 30 + nameLen + extraLen;
        if (start + (size_t)CompressedLength > d.size()) throw InvalidDataException("A local file header is corrupt.");
        if (method == 0) { size_t n = (size_t)std::min<long long>(CompressedLength, limit + 1); return Bytes(std::vector<uint8_t>(d.begin() + (long)start, d.begin() + (long)(start + n))); }
        if (method == 8) return Compression::Inflate(d.data() + start, (size_t)CompressedLength, -15, limit);
        throw InvalidDataException("The archive entry was compressed using an unsupported compression method.");
    }
};
struct ZipArchive {
    List<ZipArchiveEntry> Entries;
    static Ref<ZipArchive> Open(const Bytes& bytes) {   // new ZipArchive(new MemoryStream(bytes), ZipArchiveMode.Read)
        auto d = std::make_shared<const std::vector<uint8_t>>(bytes.Raw().begin(), bytes.Raw().end());
        const auto& b = *d;
        auto u16 = [&](size_t o) { return (uint32_t)(b[o] | b[o + 1] << 8); };
        auto u32 = [&](size_t o) { return (uint32_t)b[o] | (uint32_t)b[o + 1] << 8 | (uint32_t)b[o + 2] << 16 | (uint32_t)b[o + 3] << 24; };
        long long eocd = -1;
        for (long long i = (long long)b.size() - 22; i >= 0 && i >= (long long)b.size() - 22 - 65535; i--)
            if (b[(size_t)i] == 'P' && b[(size_t)i + 1] == 'K' && b[(size_t)i + 2] == 5 && b[(size_t)i + 3] == 6) { eocd = i; break; }
        if (eocd < 0) throw InvalidDataException("End of Central Directory record could not be found.");
        size_t count = u16((size_t)eocd + 10), cdOff = u32((size_t)eocd + 16);
        auto z = New<ZipArchive>(); z->Entries = List<ZipArchiveEntry>::New();
        size_t p = cdOff;
        for (size_t k = 0; k < count; k++) {
            if (p + 46 > b.size() || u32(p) != 0x02014b50) throw InvalidDataException("Central Directory corrupt.");
            ZipArchiveEntry e; e.data = d;
            uint32_t flags = u16(p + 8); e.method = (int)u16(p + 10); e.Crc32 = u32(p + 16);
            e.CompressedLength = u32(p + 20); e.Length = u32(p + 24);
            size_t nl = u16(p + 28), xl = u16(p + 30), cl = u16(p + 32); e.localHeader = u32(p + 42);
            if (p + 46 + nl > b.size()) throw InvalidDataException("Central Directory corrupt.");
            String raw((const char*)b.data() + p + 46, nl);
            e.FullName = (flags & 0x800) || true ? Encoding::Utf8Decode((const uint8_t*)raw.data(), raw.size()) : raw;   // .NET Core: UTF-8 either way
            size_t slash = e.FullName.find_last_of("/\\"); e.Name = slash == String::npos ? e.FullName : e.FullName.substr(slash + 1);
            z->Entries.Add(e);
            p += 46 + nl + xl + cl;
        }
        return z;
    }
};

}  // namespace cs
