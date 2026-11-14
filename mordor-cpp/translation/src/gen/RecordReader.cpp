#include "core.h"

namespace mc {

RecordReader::RecordReader(cs::String filename, int32_t recordLength) {
    _recordLength = recordLength;
    _buffer = cs::List<uint8_t>::New(recordLength == 0 ? 65536 : recordLength);
    // File.OpenRead emulated: the stream holds the whole file and a position
    _file = cs::New<cs::Stream>();
    _file->Data = cs::File::ReadAllBytes(filename);
    _file->Position = 0;
}

void RecordReader::Read(int32_t length) {
    if (length < 0) length = _recordLength;
    _cursor = 0;
    if (length == 0) return;
    int32_t total = 0;
    while (total < length) {
        // FileStream.Read(_buffer, total, length - total)
        auto& f = cs::deref(_file);
        int32_t want = length - total;
        if (total + want > _buffer.Length()) throw cs::ArgumentException("Offset and length were out of bounds for the array");
        int32_t avail = f.Data.Count() - f.Position;
        if (avail < 0) avail = 0;
        int32_t n = want < avail ? want : avail;
        for (int32_t k = 0; k < n; k++) _buffer[total + k] = f.Data[f.Position + k];
        f.Position += n;
        if (n == 0) break; // EOF; VB files pad, tolerate short final read
        total += n;
    }
    // zero any unread tail so stale bytes don't leak between records
    for (int32_t i = total; i < length; i++) _buffer[i] = 0;
}

void RecordReader::Seek(int64_t recnum) {
    int64_t pos = (int64_t)_recordLength * (recnum - 1);
    auto& f = cs::deref(_file);
    if (pos < 0) throw cs::IOException("An attempt was made to move the position before the beginning of the stream.");
    f.Position = (int)pos;
}

uint8_t RecordReader::GetByte() {
    uint8_t v = _buffer[_cursor];
    _cursor += 1;
    return v;
}

int16_t RecordReader::GetWord() {
    int16_t v = (int16_t)(uint16_t)(_buffer[_cursor] | (_buffer[_cursor + 1] << 8));
    _cursor += 2;
    return v;
}

uint16_t RecordReader::GetUWord() {
    uint16_t v = (uint16_t)(_buffer[_cursor] | (_buffer[_cursor + 1] << 8));
    _cursor += 2;
    return v;
}

int32_t RecordReader::GetDword() {
    int32_t v = (int32_t)((uint32_t)_buffer[_cursor]
              | ((uint32_t)_buffer[_cursor + 1] << 8)
              | ((uint32_t)_buffer[_cursor + 2] << 16)
              | ((uint32_t)_buffer[_cursor + 3] << 24));
    _cursor += 4;
    return v;
}

int64_t RecordReader::GetInt64() {
    uint64_t v = 0;
    for (int32_t i = 7; i >= 0; i--) v = (v << 8) + _buffer[_cursor + i];
    _cursor += 8;
    return (int64_t)v;
}

float RecordReader::GetFloat() {
    if (_cursor < 0 || _cursor + 4 > _buffer.Length()) throw cs::ArgumentOutOfRangeException("startIndex");
    float v = cs::BinaryPrimitives::ReadSingleLittleEndian(_buffer.Raw().data() + _cursor);
    _cursor += 4;
    return v;
}

int64_t RecordReader::GetCurrency() { return GetInt64(); }

int64_t RecordReader::GetIntCurrency() { return GetInt64() / 10000; }

cs::String RecordReader::GetString(int32_t length) {
    if (length == 0) length = GetUWord();
    if (_cursor < 0 || length < 0 || _cursor + length > _buffer.Length()) throw cs::ArgumentOutOfRangeException("count");
    cs::String s = cs::Encoding::Latin1GetString(_buffer.Raw().data() + _cursor, length);
    _cursor += length;
    return s;
}

cs::List<int16_t> RecordReader::GetWords(int32_t count) {
    auto a = cs::List<int16_t>::New(count);
    for (int32_t i = 0; i < count; i++) a[i] = GetWord();
    return a;
}

void RecordReader::Dispose() { _file = nullptr; }

}  // namespace mc
