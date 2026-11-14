#include "core.h"

namespace mc {

namespace {
// BitConverter.ToInt32 / ToUInt16 with .NET's argument checks
int32_t BcToInt32(const cs::List<uint8_t>& b, int32_t at) {
    if (at < 0 || at >= b.Length()) throw cs::ArgumentOutOfRangeException("startIndex");
    if (at > b.Length() - 4) throw cs::ArgumentException("Destination array is not long enough");
    return cs::BinaryPrimitives::ReadInt32LittleEndian(b.Raw().data() + at);
}
uint16_t BcToUInt16(const cs::List<uint8_t>& b, int32_t at) {
    if (at < 0 || at >= b.Length()) throw cs::ArgumentOutOfRangeException("startIndex");
    if (at > b.Length() - 2) throw cs::ArgumentException("Destination array is not long enough");
    return cs::BinaryPrimitives::ReadUInt16LittleEndian(b.Raw().data() + at);
}
}  // namespace

RgbaImage::RgbaImage(int32_t width, int32_t height, cs::List<uint32_t> pixels) {
    if (width <= 0 || height <= 0) throw cs::ArgumentOutOfRangeException("an image has to have some size");
    Width = width;
    Height = height;
    Pixels = pixels ? pixels : cs::List<uint32_t>::New(cs::Wrap<int32_t>((int64_t)width * height));
    if (Pixels.Length() != cs::Wrap<int32_t>((int64_t)width * height)) throw cs::ArgumentException("pixel count does not match the size");
}

uint32_t RgbaImage::Index(int32_t x, int32_t y) {
    return Pixels[y * Width + x];
}

void RgbaImage::SetIndex(int32_t x, int32_t y, uint32_t value) {
    Pixels[y * Width + x] = value;
}

cs::Ref<RgbaImage> RgbaImage::Crop(int32_t x, int32_t y, int32_t width, int32_t height) {
    if (x < 0 || y < 0 || x + width > Width || y + height > Height)
        throw cs::ArgumentOutOfRangeException(cs::ToStr(width) + "x" + cs::ToStr(height) + " at (" + cs::ToStr(x) + "," + cs::ToStr(y) + ") is outside a " + cs::ToStr(Width) + "x" + cs::ToStr(Height) + " image");
    auto o = cs::New<RgbaImage>(width, height);
    for (int32_t r = 0; r < height; r++)
        for (int32_t k = 0; k < width; k++) o->Pixels[r * width + k] = Pixels[(y + r) * Width + x + k];
    return o;
}

cs::Ref<RgbaImage> RgbaImage::ScaleNearest(int32_t width, int32_t height) {
    auto o = cs::New<RgbaImage>(width, height);
    for (int32_t y = 0; y < height; y++) {
        int32_t sy = (int32_t)((int64_t)y * Height / height);
        for (int32_t x = 0; x < width; x++) o->Pixels[y * width + x] = Pixels[sy * Width + (int32_t)((int64_t)x * Width / width)];
    }
    return o;
}

bool RgbaImage::SamePixels(cs::Ref<RgbaImage> other, cs::String& difference) {
    difference = "";
    if (other->Width != Width || other->Height != Height) {
        difference = cs::ToStr(Width) + "x" + cs::ToStr(Height) + " against " + cs::ToStr(other->Width) + "x" + cs::ToStr(other->Height);
        return false;
    }
    int32_t bad = 0, first = -1;
    for (int32_t i = 0; i < Pixels.Length(); i++) {
        uint32_t a = Pixels[i], b = other->Pixels[i];
        if (a == b || ((a >> 24) == 0 && (b >> 24) == 0)) continue;
        if (bad++ == 0) first = i;
    }
    if (bad == 0) return true;
    difference = cs::ToStr(bad) + " pixel(s) differ, the first at (" + cs::ToStr(first % Width) + "," + cs::ToStr(first / Width) + "): " + cs::Format(Pixels[first], "X8") + " against " + cs::Format(other->Pixels[first], "X8");
    return false;
}

cs::Ref<RgbaImage> Dib::DecodeBmpFile(cs::List<uint8_t> b, int32_t offset) {
    if (offset + 14 > b.Length() || b[offset] != 'B' || b[offset + 1] != 'M')
        throw cs::InvalidDataException("not a bitmap file (no BM signature)");
    int32_t bits = BcToInt32(b, offset + 10);
    return Decode(b, offset + 14, offset + bits);
}

cs::Ref<RgbaImage> Dib::Decode(cs::List<uint8_t> b, int32_t offset, int32_t pixelOffset) {
    if (offset + 12 > b.Length()) throw cs::InvalidDataException("the bitmap header is cut short");
    int32_t hs = BcToInt32(b, offset);
    bool core = hs == 12;
    if (!core && hs < 40) throw cs::InvalidDataException("unknown bitmap header size " + cs::ToStr(hs));
    int32_t w = core ? (int32_t)BcToUInt16(b, offset + 4) : BcToInt32(b, offset + 4);
    int32_t h = core ? (int32_t)BcToUInt16(b, offset + 6) : BcToInt32(b, offset + 8);
    int32_t bpp = BcToUInt16(b, offset + (core ? 10 : 14));
    int32_t comp = core ? 0 : BcToInt32(b, offset + 16);
    int32_t used = core ? 0 : BcToInt32(b, offset + 32);
    if (w <= 0 || w > 8192 || h == 0 || cs::Math::Abs(h) > 8192) throw cs::InvalidDataException("implausible bitmap size " + cs::ToStr(w) + "x" + cs::ToStr(h));
    int32_t colours = bpp <= 8 ? (used > 0 ? used : 1 << bpp) : 0;
    int32_t entry = core ? 3 : 4;
    auto pal = cs::List<uint32_t>::New(cs::Math::Max(colours, 1 << cs::Math::Min(bpp, 8)));
    for (int32_t i = 0; i < colours; i++) {
        int32_t q = offset + hs + i * entry;
        if (q + 3 > b.Length()) throw cs::InvalidDataException("the bitmap palette is cut short");
        pal[i] = 0xFF000000u | ((uint32_t)b[q + 2] << 16) | ((uint32_t)b[q + 1] << 8) | b[q];
    }
    for (int32_t i = colours; i < pal.Length(); i++) pal[i] = 0xFF000000u;
    int32_t px = pixelOffset >= 0 ? pixelOffset : offset + hs + colours * entry;
    bool bottomUp = h > 0;
    int32_t height = cs::Math::Abs(h);
    auto img = cs::New<RgbaImage>(w, height);

    if (comp == 1 || comp == 2) {
        DecodeRle(b, px, comp == 1 ? 8 : 4, w, height, pal, img, bottomUp);
        return img;
    }
    if (comp != 0) throw cs::InvalidDataException("bitmap compression " + cs::ToStr(comp) + " is not read");
    int32_t stride = ((w * bpp + 31) / 32) * 4;
    for (int32_t y = 0; y < height; y++) {
        int32_t row = px + (bottomUp ? height - 1 - y : y) * stride;
        if (row + (w * bpp + 7) / 8 > b.Length()) throw cs::InvalidDataException("the bitmap's pixels are cut short");
        for (int32_t x = 0; x < w; x++) {
            uint32_t v;
            if (bpp == 1) v = pal[(b[row + x / 8] >> (7 - x % 8)) & 1];
            else if (bpp == 4) v = pal[(b[row + x / 2] >> (x % 2 == 0 ? 4 : 0)) & 15];
            else if (bpp == 8) v = pal[b[row + x]];
            else if (bpp == 24) v = 0xFF000000u | ((uint32_t)b[row + x * 3 + 2] << 16) | ((uint32_t)b[row + x * 3 + 1] << 8) | b[row + x * 3];
            else if (bpp == 32) v = 0xFF000000u | ((uint32_t)b[row + x * 4 + 2] << 16) | ((uint32_t)b[row + x * 4 + 1] << 8) | b[row + x * 4];
            else throw cs::InvalidDataException(cs::ToStr(bpp) + " bits a pixel is not read");
            img->Pixels[y * w + x] = v;
        }
    }
    return img;
}

void Dib::DecodeRle(cs::List<uint8_t> b, int32_t p, int32_t bits, int32_t w, int32_t h, cs::List<uint32_t> pal, cs::Ref<RgbaImage> img, bool bottomUp) {
    auto idx = cs::List<uint8_t>::New(w * h);
    int32_t x = 0, y = 0;
    auto Put = [&](int32_t v) { if (x < w && y < h) idx[y * w + x] = (uint8_t)v; x++; };
    while (p + 1 < b.Length()) {
        int32_t n = b[p++];
        int32_t c = b[p++];
        if (n > 0) {
            for (int32_t k = 0; k < n; k++) Put(bits == 8 ? c : (k % 2 == 0 ? c >> 4 : c & 15));
            continue;
        }
        if (c == 0) { x = 0; y++; continue; }
        if (c == 1) break;
        if (c == 2) { x += b[p]; y += b[p + 1]; p += 2; continue; }
        if (bits == 8) {
            for (int32_t k = 0; k < c; k++) Put(b[p + k]);
            p += c + (c & 1);
        } else {
            int32_t bytes = (c + 1) / 2;
            for (int32_t k = 0; k < c; k++) Put(k % 2 == 0 ? b[p + k / 2] >> 4 : b[p + k / 2] & 15);
            p += bytes + (bytes & 1);
        }
    }
    for (int32_t yy = 0; yy < h; yy++) {
        int32_t src = bottomUp ? h - 1 - yy : yy;
        for (int32_t xx = 0; xx < w; xx++) img->Pixels[yy * w + xx] = pal[idx[src * w + xx]];
    }
}

cs::Ref<RgbaImage> Dib::DecodeIcon(cs::List<uint8_t> b, int32_t offset) {
    int32_t hs = BcToInt32(b, offset);
    if (hs != 40) throw cs::InvalidDataException("unknown icon header size " + cs::ToStr(hs));
    int32_t w = BcToInt32(b, offset + 4);
    int32_t h2 = BcToInt32(b, offset + 8);
    int32_t bpp = BcToUInt16(b, offset + 14);
    int32_t h = h2 / 2;
    int32_t used = BcToInt32(b, offset + 32);
    int32_t colours = bpp <= 8 ? (used > 0 ? used : 1 << bpp) : 0;
    int32_t xorAt = offset + hs + colours * 4;
    int32_t xorStride = ((w * bpp + 31) / 32) * 4;
    int32_t andAt = xorAt + xorStride * h;
    int32_t andStride = ((w + 31) / 32) * 4;

    // the colour half alone, read as a DIB of the icon's own height
    auto header = cs::List<uint8_t>::New(hs + colours * 4 + xorStride * h);
    for (int32_t k = 0; k < header.Length(); k++) header[k] = b[offset + k];
    if (header.Length() < 12) throw cs::ArgumentException("Destination array was not long enough");
    cs::BinaryPrimitives::WriteInt32LittleEndian(header.Raw().data() + 8, h);
    auto img = Decode(header, 0);
    for (int32_t y = 0; y < h; y++) {
        int32_t row = andAt + (h - 1 - y) * andStride;
        for (int32_t x = 0; x < w; x++)
            if (((b[row + x / 8] >> (7 - x % 8)) & 1) != 0) img->Pixels[y * w + x] = 0;
    }
    return img;
}

cs::List<uint8_t> Png::Encode(cs::Ref<RgbaImage> img) {
    (void)img;
    throw cs::NotSupportedException("not translated: PNG encoding (MemoryStream/ZLibStream)");
}

cs::Ref<RgbaImage> Png::Decode(cs::List<uint8_t> f) {
    (void)f;
    throw cs::NotSupportedException("not translated: PNG decoding (MemoryStream/ZLibStream)");
}

int32_t Png::Paeth(int32_t a, int32_t b, int32_t c) {
    int32_t p = a + b - c, pa = cs::Math::Abs(p - a), pb = cs::Math::Abs(p - b), pc = cs::Math::Abs(p - c);
    return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

int32_t Png::ReadBigEndian(cs::List<uint8_t> f, int32_t p) {
    return (int32_t)(((uint32_t)f[p] << 24) | ((uint32_t)f[p + 1] << 16) | ((uint32_t)f[p + 2] << 8) | (uint32_t)f[p + 3]);
}

void Png::BigEndian(cs::List<uint8_t> b, int32_t p, int32_t v) {
    b[p] = (uint8_t)(v >> 24); b[p + 1] = (uint8_t)(v >> 16); b[p + 2] = (uint8_t)(v >> 8); b[p + 3] = (uint8_t)v;
}

void Png::Chunk(cs::Ref<std::iostream> o, cs::String type, cs::List<uint8_t> data) {
    auto len = cs::List<uint8_t>::New(4);
    BigEndian(len, 0, data.Length());
    cs::deref(o).write((const char*)len.Raw().data(), (std::streamsize)len.Count());
    auto typed = cs::List<uint8_t>::New(4 + data.Length());
    cs::Encoding::ASCIIGetBytes(type).CopyTo(typed, 0);
    data.CopyTo(typed, 4);
    cs::deref(o).write((const char*)typed.Raw().data(), (std::streamsize)typed.Count());
    BigEndian(len, 0, (int32_t)Crc32(typed));
    cs::deref(o).write((const char*)len.Raw().data(), (std::streamsize)len.Count());
}

uint32_t Png::Crc32(cs::List<uint8_t> b) {
    if (!_crcTable) _crcTable = cs::Enumerable::Range(0, 256).Select([&](auto& n) {
        uint32_t c = (uint32_t)n;
        for (int32_t k = 0; k < 8; k++) c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        return c;
    }).ToArray();
    auto t = _crcTable;
    uint32_t crc = 0xFFFFFFFFu;
    for (auto& x : b) crc = t[(int32_t)((crc ^ x) & 0xFF)] ^ (crc >> 8);
    return ~crc;
}

}  // namespace mc
