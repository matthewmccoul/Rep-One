#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct RecordReader {
    cs::List<uint8_t> _buffer;
    cs::Ref<cs::Stream> _file;
    int32_t _recordLength = 0;
    int32_t _cursor = 0;

    RecordReader(cs::String filename, int32_t recordLength);
    void Read(int32_t length = -1);
    void Seek(int64_t recnum);
    uint8_t GetByte();
    int16_t GetWord();
    uint16_t GetUWord();
    int32_t GetDword();
    int64_t GetInt64();
    float GetFloat();
    int64_t GetCurrency();
    int64_t GetIntCurrency();
    cs::String GetString(int32_t length = 0);
    cs::List<int16_t> GetWords(int32_t count);
    void Dispose();
};

}  // namespace mc
