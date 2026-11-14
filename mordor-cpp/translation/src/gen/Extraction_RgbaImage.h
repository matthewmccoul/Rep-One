#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct RgbaImage {
    int32_t Width = 0;
    int32_t Height = 0;
    cs::List<uint32_t> Pixels;

    RgbaImage(int32_t width, int32_t height, cs::List<uint32_t> pixels = {});

    uint32_t Index(int32_t x, int32_t y);
    void SetIndex(int32_t x, int32_t y, uint32_t value);

    cs::Ref<RgbaImage> Crop(int32_t x, int32_t y, int32_t width, int32_t height);
    cs::Ref<RgbaImage> ScaleNearest(int32_t width, int32_t height);
    bool SamePixels(cs::Ref<RgbaImage> other, cs::String& difference);
};

struct Dib {
    static cs::Ref<RgbaImage> DecodeBmpFile(cs::List<uint8_t> b, int32_t offset = 0);
    static cs::Ref<RgbaImage> Decode(cs::List<uint8_t> b, int32_t offset, int32_t pixelOffset = -1);
    static void DecodeRle(cs::List<uint8_t> b, int32_t p, int32_t bits, int32_t w, int32_t h, cs::List<uint32_t> pal, cs::Ref<RgbaImage> img, bool bottomUp);
    static cs::Ref<RgbaImage> DecodeIcon(cs::List<uint8_t> b, int32_t offset);
};

struct Png {
    inline static cs::List<uint8_t> Signature = cs::List<uint8_t>::Of({137, 80, 78, 71, 13, 10, 26, 10});
    inline static cs::List<uint32_t> _crcTable;

    static cs::List<uint8_t> Encode(cs::Ref<RgbaImage> img);
    static cs::Ref<RgbaImage> Decode(cs::List<uint8_t> f);
    static int32_t Paeth(int32_t a, int32_t b, int32_t c);
    static int32_t ReadBigEndian(cs::List<uint8_t> f, int32_t p);
    static void BigEndian(cs::List<uint8_t> b, int32_t p, int32_t v);
    static void Chunk(cs::Ref<std::iostream> o, cs::String type, cs::List<uint8_t> data);
    static uint32_t Crc32(cs::List<uint8_t> b);
};

}  // namespace mc
