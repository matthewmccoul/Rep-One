#include "core.h"

namespace mc {

// ---- Lzma

cs::List<uint8_t> Lzma::Decode(cs::List<uint8_t> props, cs::List<uint8_t> input, int32_t inputOffset, int32_t inputLength, int32_t size, int32_t& consumed) {
    if (size < 0) throw cs::InvalidDataException("LZMA: a negative size");
    auto d = cs::New<Lzma_Decoder>();
    d->SetDecoderProperties(props, size);
    return d->Code(input, inputOffset, inputLength, size, size, consumed);
}

cs::List<uint8_t> Lzma::DecodeToEnd(cs::List<uint8_t> props, cs::List<uint8_t> input, int32_t inputOffset, int32_t inputLength, int32_t limit, int32_t& consumed) {
    if (limit < 0) throw cs::InvalidDataException("LZMA: a negative limit");
    auto d = cs::New<Lzma_Decoder>();
    d->SetDecoderProperties(props, limit);
    return d->Code(input, inputOffset, inputLength, -1, limit, consumed);
}

// ---- LzmaBase.cs

void Lzma_Base_State::Init() { Index = 0; }

void Lzma_Base_State::UpdateChar() {
    if (Index < 4) Index = 0;
    else if (Index < 10) Index -= 3;
    else Index -= 6;
}

void Lzma_Base_State::UpdateMatch() { Index = (uint32_t)(Index < 7 ? 7 : 10); }
void Lzma_Base_State::UpdateRep() { Index = (uint32_t)(Index < 7 ? 8 : 11); }
void Lzma_Base_State::UpdateShortRep() { Index = (uint32_t)(Index < 7 ? 9 : 11); }
bool Lzma_Base_State::IsCharState() { return Index < 7; }

uint32_t Lzma_Base::GetLenToPosState(uint32_t len) {
    len -= kMatchMinLen;
    if (len < kNumLenToPosStates) return len;
    return kNumLenToPosStates - 1;
}

// ---- RangeCoder.cs

int32_t Lzma_RangeDecoder::Position() { return _pos; }

uint8_t Lzma_RangeDecoder::ReadByte() {
    if (_pos >= _end) throw cs::InvalidDataException("LZMA: the compressed data ends too soon");
    int32_t p = _pos++;
    return _in[p];
}

void Lzma_RangeDecoder::Init(cs::List<uint8_t> input, int32_t offset, int32_t length) {
    if (offset < 0 || length < 0 || offset > input.Length() || length > input.Length() - offset)
        throw cs::InvalidDataException("LZMA: the compressed data is not where it is said to be");
    _in = input;
    _pos = offset;
    _end = offset + length;
    Code = 0;
    Range = 0xFFFFFFFFu;
    for (int32_t i = 0; i < 5; i++)
        Code = (Code << 8) | ReadByte();
}

uint32_t Lzma_RangeDecoder::DecodeDirectBits(int32_t numTotalBits) {
    uint32_t range = Range;
    uint32_t code = Code;
    uint32_t result = 0;
    for (int32_t i = numTotalBits; i > 0; i--) {
        range >>= 1;
        uint32_t t = (code - range) >> 31;
        code -= range & (t - 1);
        result = (result << 1) | (1u - t);
        if (range < kTopValue) {
            code = (code << 8) | ReadByte();
            range <<= 8;
        }
    }
    Range = range;
    Code = code;
    return result;
}

// ---- RangeCoderBit.cs

void Lzma_BitDecoder::Init() { Prob = kBitModelTotal >> 1; }

uint32_t Lzma_BitDecoder::Decode(cs::Ref<Lzma_RangeDecoder> rangeDecoder) {
    uint32_t newBound = (rangeDecoder->Range >> kNumBitModelTotalBits) * Prob;
    if (rangeDecoder->Code < newBound) {
        rangeDecoder->Range = newBound;
        Prob += (kBitModelTotal - Prob) >> kNumMoveBits;
        if (rangeDecoder->Range < Lzma_RangeDecoder::kTopValue) {
            rangeDecoder->Code = (rangeDecoder->Code << 8) | rangeDecoder->ReadByte();
            rangeDecoder->Range <<= 8;
        }
        return 0;
    } else {
        rangeDecoder->Range -= newBound;
        rangeDecoder->Code -= newBound;
        Prob -= Prob >> kNumMoveBits;
        if (rangeDecoder->Range < Lzma_RangeDecoder::kTopValue) {
            rangeDecoder->Code = (rangeDecoder->Code << 8) | rangeDecoder->ReadByte();
            rangeDecoder->Range <<= 8;
        }
        return 1;
    }
}

// ---- RangeCoderBitTree.cs

Lzma_BitTreeDecoder::Lzma_BitTreeDecoder(int32_t numBitLevels) {
    NumBitLevels = numBitLevels;
    Models = cs::List<Lzma_BitDecoder>::New(1 << numBitLevels);
}

void Lzma_BitTreeDecoder::Init() {
    for (uint32_t i = 1; i < (uint32_t)(1 << NumBitLevels); i++)
        Models[(int32_t)i].Init();
}

uint32_t Lzma_BitTreeDecoder::Decode(cs::Ref<Lzma_RangeDecoder> rangeDecoder) {
    uint32_t m = 1;
    for (int32_t bitIndex = NumBitLevels; bitIndex > 0; bitIndex--)
        m = (m << 1) + Models[(int32_t)m].Decode(rangeDecoder);
    return m - ((uint32_t)1 << NumBitLevels);
}

uint32_t Lzma_BitTreeDecoder::ReverseDecode(cs::Ref<Lzma_RangeDecoder> rangeDecoder) {
    uint32_t m = 1;
    uint32_t symbol = 0;
    for (int32_t bitIndex = 0; bitIndex < NumBitLevels; bitIndex++) {
        uint32_t bit = Models[(int32_t)m].Decode(rangeDecoder);
        m <<= 1;
        m += bit;
        symbol |= bit << bitIndex;
    }
    return symbol;
}

uint32_t Lzma_BitTreeDecoder::ReverseDecode(cs::List<Lzma_BitDecoder> Models, uint32_t startIndex, cs::Ref<Lzma_RangeDecoder> rangeDecoder, int32_t NumBitLevels) {
    uint32_t m = 1;
    uint32_t symbol = 0;
    for (int32_t bitIndex = 0; bitIndex < NumBitLevels; bitIndex++) {
        uint32_t bit = Models[(int32_t)(startIndex + m)].Decode(rangeDecoder);
        m <<= 1;
        m += bit;
        symbol |= bit << bitIndex;
    }
    return symbol;
}

// ---- LzOutWindow.cs

void Lzma_OutWindow::Create(uint32_t windowSize) {
    if (_windowSize != windowSize)
        _buffer = cs::List<uint8_t>::New((int32_t)windowSize);
    _windowSize = windowSize;
    _pos = 0;
    _streamPos = 0;
}

void Lzma_OutWindow::Init(cs::Ref<cs::Stream> stream) {
    _stream = stream;
    _streamPos = 0;
    _pos = 0;
}

void Lzma_OutWindow::Flush() {
    uint32_t size = _pos - _streamPos;
    if (size == 0) return;
    // MemoryStream.Write(_buffer, _streamPos, size): the stream is only ever appended to
    for (int32_t i = 0; i < (int32_t)size; i++)
        _stream->Data.Add(_buffer[(int32_t)_streamPos + i]);
    _stream->Position += (int32_t)size;
    if (_pos >= _windowSize) _pos = 0;
    _streamPos = _pos;
}

void Lzma_OutWindow::CopyBlock(uint32_t distance, uint32_t len) {
    uint32_t pos = _pos - distance - 1;
    if (pos >= _windowSize) pos += _windowSize;
    for (; len > 0; len--) {
        if (pos >= _windowSize) pos = 0;
        uint32_t dst = _pos++;
        uint32_t src = pos++;
        _buffer[(int32_t)dst] = _buffer[(int32_t)src];
        if (_pos >= _windowSize) Flush();
    }
}

void Lzma_OutWindow::PutByte(uint8_t b) {
    uint32_t dst = _pos++;
    _buffer[(int32_t)dst] = b;
    if (_pos >= _windowSize) Flush();
}

uint8_t Lzma_OutWindow::GetByte(uint32_t distance) {
    uint32_t pos = _pos - distance - 1;
    if (pos >= _windowSize) pos += _windowSize;
    return _buffer[(int32_t)pos];
}

// ---- LzmaDecoder.cs: LenDecoder

void Lzma_Decoder_LenDecoder::Create(uint32_t numPosStates) {
    for (uint32_t posState = m_NumPosStates; posState < numPosStates; posState++) {
        m_LowCoder[(int32_t)posState] = Lzma_BitTreeDecoder(Lzma_Base::kNumLowLenBits);
        m_MidCoder[(int32_t)posState] = Lzma_BitTreeDecoder(Lzma_Base::kNumMidLenBits);
    }
    m_NumPosStates = numPosStates;
}

void Lzma_Decoder_LenDecoder::Init() {
    m_Choice.Init();
    for (uint32_t posState = 0; posState < m_NumPosStates; posState++) {
        m_LowCoder[(int32_t)posState].Init();
        m_MidCoder[(int32_t)posState].Init();
    }
    m_Choice2.Init();
    m_HighCoder.Init();
}

uint32_t Lzma_Decoder_LenDecoder::Decode(cs::Ref<Lzma_RangeDecoder> rangeDecoder, uint32_t posState) {
    if (m_Choice.Decode(rangeDecoder) == 0)
        return m_LowCoder[(int32_t)posState].Decode(rangeDecoder);
    uint32_t symbol = Lzma_Base::kNumLowLenSymbols;
    if (m_Choice2.Decode(rangeDecoder) == 0)
        symbol += m_MidCoder[(int32_t)posState].Decode(rangeDecoder);
    else {
        symbol += Lzma_Base::kNumMidLenSymbols;
        symbol += m_HighCoder.Decode(rangeDecoder);
    }
    return symbol;
}

// ---- LzmaDecoder.cs: LiteralDecoder

void Lzma_Decoder_LiteralDecoder_Decoder2::Create() { m_Decoders = cs::List<Lzma_BitDecoder>::New(0x300); }

void Lzma_Decoder_LiteralDecoder_Decoder2::Init() {
    for (int32_t i = 0; i < 0x300; i++) m_Decoders[i].Init();
}

uint8_t Lzma_Decoder_LiteralDecoder_Decoder2::DecodeNormal(cs::Ref<Lzma_RangeDecoder> rangeDecoder) {
    uint32_t symbol = 1;
    do
        symbol = (symbol << 1) | m_Decoders[(int32_t)symbol].Decode(rangeDecoder);
    while (symbol < 0x100);
    return (uint8_t)symbol;
}

uint8_t Lzma_Decoder_LiteralDecoder_Decoder2::DecodeWithMatchByte(cs::Ref<Lzma_RangeDecoder> rangeDecoder, uint8_t matchByte) {
    uint32_t symbol = 1;
    do {
        uint32_t matchBit = (uint32_t)(matchByte >> 7) & 1u;
        matchByte = (uint8_t)(matchByte << 1);
        uint32_t bit = m_Decoders[(int32_t)(((1 + matchBit) << 8) + symbol)].Decode(rangeDecoder);
        symbol = (symbol << 1) | bit;
        if (matchBit != bit) {
            while (symbol < 0x100)
                symbol = (symbol << 1) | m_Decoders[(int32_t)symbol].Decode(rangeDecoder);
            break;
        }
    } while (symbol < 0x100);
    return (uint8_t)symbol;
}

void Lzma_Decoder_LiteralDecoder::Create(int32_t numPosBits, int32_t numPrevBits) {
    if (m_Coders.Length() > 0 && m_NumPrevBits == numPrevBits && m_NumPosBits == numPosBits)
        return;
    m_NumPosBits = numPosBits;
    m_PosMask = ((uint32_t)1 << numPosBits) - 1;
    m_NumPrevBits = numPrevBits;
    uint32_t numStates = (uint32_t)1 << (m_NumPrevBits + m_NumPosBits);
    m_Coders = cs::List<Lzma_Decoder_LiteralDecoder_Decoder2>::New((int32_t)numStates);
    for (uint32_t i = 0; i < numStates; i++)
        m_Coders[(int32_t)i].Create();
}

void Lzma_Decoder_LiteralDecoder::Init() {
    uint32_t numStates = (uint32_t)1 << (m_NumPrevBits + m_NumPosBits);
    for (uint32_t i = 0; i < numStates; i++)
        m_Coders[(int32_t)i].Init();
}

uint32_t Lzma_Decoder_LiteralDecoder::GetState(uint32_t pos, uint8_t prevByte) {
    return ((pos & m_PosMask) << m_NumPrevBits) + (uint32_t)(prevByte >> (8 - m_NumPrevBits));
}

uint8_t Lzma_Decoder_LiteralDecoder::DecodeNormal(cs::Ref<Lzma_RangeDecoder> rangeDecoder, uint32_t pos, uint8_t prevByte) {
    return m_Coders[(int32_t)GetState(pos, prevByte)].DecodeNormal(rangeDecoder);
}

uint8_t Lzma_Decoder_LiteralDecoder::DecodeWithMatchByte(cs::Ref<Lzma_RangeDecoder> rangeDecoder, uint32_t pos, uint8_t prevByte, uint8_t matchByte) {
    return m_Coders[(int32_t)GetState(pos, prevByte)].DecodeWithMatchByte(rangeDecoder, matchByte);
}

// ---- LzmaDecoder.cs: Decoder

Lzma_Decoder::Lzma_Decoder() {
    m_DictionarySize = 0xFFFFFFFFu;
    for (int32_t i = 0; (uint32_t)i < Lzma_Base::kNumLenToPosStates; i++)
        m_PosSlotDecoder[i] = Lzma_BitTreeDecoder(Lzma_Base::kNumPosSlotBits);
}

void Lzma_Decoder::SetDictionarySize(uint32_t dictionarySize, int32_t outSize) {
    if (m_DictionarySize != dictionarySize) {
        m_DictionarySize = dictionarySize;
        m_DictionarySizeCheck = cs::Math::Max(m_DictionarySize, (uint32_t)1);
        // the SDK's Math.Max(m_DictionarySizeCheck, 1 << 12), but no larger than the output can reach back
        uint32_t blockSize = cs::Math::Max(cs::Math::Min(m_DictionarySizeCheck, (uint32_t)cs::Math::Max(outSize, (int32_t)1)), (uint32_t)(1 << 12));
        m_OutWindow->Create(blockSize);
    }
}

void Lzma_Decoder::SetLiteralProperties(int32_t lp, int32_t lc) {
    if (lp > 8) throw cs::InvalidDataException("LZMA: bad properties");
    if (lc > 8) throw cs::InvalidDataException("LZMA: bad properties");
    m_LiteralDecoder->Create(lp, lc);
}

void Lzma_Decoder::SetPosBitsProperties(int32_t pb) {
    if (pb > Lzma_Base::kNumPosStatesBitsMax) throw cs::InvalidDataException("LZMA: bad properties");
    uint32_t numPosStates = (uint32_t)1 << pb;
    m_LenDecoder->Create(numPosStates);
    m_RepLenDecoder->Create(numPosStates);
    m_PosStateMask = numPosStates - 1;
}

void Lzma_Decoder::Init(cs::List<uint8_t> input, int32_t inputOffset, int32_t inputLength, cs::Ref<cs::Stream> output) {
    m_RangeDecoder->Init(input, inputOffset, inputLength);
    m_OutWindow->Init(output);

    uint32_t i;
    for (i = 0; i < Lzma_Base::kNumStates; i++) {
        for (uint32_t j = 0; j <= m_PosStateMask; j++) {
            uint32_t index = (i << Lzma_Base::kNumPosStatesBitsMax) + j;
            m_IsMatchDecoders[(int32_t)index].Init();
            m_IsRep0LongDecoders[(int32_t)index].Init();
        }
        m_IsRepDecoders[(int32_t)i].Init();
        m_IsRepG0Decoders[(int32_t)i].Init();
        m_IsRepG1Decoders[(int32_t)i].Init();
        m_IsRepG2Decoders[(int32_t)i].Init();
    }

    m_LiteralDecoder->Init();
    for (i = 0; i < Lzma_Base::kNumLenToPosStates; i++)
        m_PosSlotDecoder[(int32_t)i].Init();
    for (i = 0; i < Lzma_Base::kNumFullDistances - Lzma_Base::kEndPosModelIndex; i++)
        m_PosDecoders[(int32_t)i].Init();

    m_LenDecoder->Init();
    m_RepLenDecoder->Init();
    m_PosAlignDecoder.Init();
}

cs::List<uint8_t> Lzma_Decoder::Code(cs::List<uint8_t> input, int32_t inputOffset, int32_t inputLength, int64_t outSize, int32_t limit, int32_t& consumed) {
    bool toEnd = outSize < 0;
    // new MemoryStream(capacity): the capacity is only a hint
    auto output = cs::New<cs::Stream>();
    output->Data = cs::Bytes::New();
    Init(input, inputOffset, inputLength, output);

    Lzma_Base_State state{};
    state.Init();
    uint32_t rep0 = 0, rep1 = 0, rep2 = 0, rep3 = 0;

    uint64_t nowPos64 = 0;
    uint64_t outSize64 = toEnd ? (uint64_t)(int64_t)limit : (uint64_t)outSize;
    if (nowPos64 < outSize64) {
        if (m_IsMatchDecoders[(int32_t)(state.Index << Lzma_Base::kNumPosStatesBitsMax)].Decode(m_RangeDecoder) != 0)
            throw cs::InvalidDataException("LZMA: the data does not decode");
        state.UpdateChar();
        uint8_t b = m_LiteralDecoder->DecodeNormal(m_RangeDecoder, 0, 0);
        m_OutWindow->PutByte(b);
        nowPos64++;
    }
    while (toEnd || nowPos64 < outSize64) {
        uint32_t posState = (uint32_t)nowPos64 & m_PosStateMask;
        if (m_IsMatchDecoders[(int32_t)((state.Index << Lzma_Base::kNumPosStatesBitsMax) + posState)].Decode(m_RangeDecoder) == 0) {
            if (nowPos64 >= outSize64) throw cs::InvalidDataException("LZMA: the data is larger than " + cs::ToStr(limit) + " bytes");
            uint8_t b;
            uint8_t prevByte = m_OutWindow->GetByte(0);
            if (!state.IsCharState())
                b = m_LiteralDecoder->DecodeWithMatchByte(m_RangeDecoder, (uint32_t)nowPos64, prevByte, m_OutWindow->GetByte(rep0));
            else
                b = m_LiteralDecoder->DecodeNormal(m_RangeDecoder, (uint32_t)nowPos64, prevByte);
            m_OutWindow->PutByte(b);
            state.UpdateChar();
            nowPos64++;
        } else {
            uint32_t len;
            if (m_IsRepDecoders[(int32_t)state.Index].Decode(m_RangeDecoder) == 1) {
                if (m_IsRepG0Decoders[(int32_t)state.Index].Decode(m_RangeDecoder) == 0) {
                    if (m_IsRep0LongDecoders[(int32_t)((state.Index << Lzma_Base::kNumPosStatesBitsMax) + posState)].Decode(m_RangeDecoder) == 0) {
                        // the SDK lacks this check; a stream may not reach back before its own first byte
                        if (rep0 >= nowPos64) throw cs::InvalidDataException("LZMA: the data does not decode");
                        if (nowPos64 >= outSize64) throw cs::InvalidDataException("LZMA: the data is larger than " + cs::ToStr(limit) + " bytes");
                        state.UpdateShortRep();
                        m_OutWindow->PutByte(m_OutWindow->GetByte(rep0));
                        nowPos64++;
                        continue;
                    }
                } else {
                    uint32_t distance;
                    if (m_IsRepG1Decoders[(int32_t)state.Index].Decode(m_RangeDecoder) == 0)
                        distance = rep1;
                    else {
                        if (m_IsRepG2Decoders[(int32_t)state.Index].Decode(m_RangeDecoder) == 0)
                            distance = rep2;
                        else {
                            distance = rep3;
                            rep3 = rep2;
                        }
                        rep2 = rep1;
                    }
                    rep1 = rep0;
                    rep0 = distance;
                }
                len = m_RepLenDecoder->Decode(m_RangeDecoder, posState) + Lzma_Base::kMatchMinLen;
                state.UpdateRep();
            } else {
                rep3 = rep2;
                rep2 = rep1;
                rep1 = rep0;
                len = Lzma_Base::kMatchMinLen + m_LenDecoder->Decode(m_RangeDecoder, posState);
                state.UpdateMatch();
                uint32_t posSlot = m_PosSlotDecoder[(int32_t)Lzma_Base::GetLenToPosState(len)].Decode(m_RangeDecoder);
                if (posSlot >= Lzma_Base::kStartPosModelIndex) {
                    int32_t numDirectBits = (int32_t)((posSlot >> 1) - 1);
                    rep0 = (2u | (posSlot & 1u)) << numDirectBits;
                    if (posSlot < Lzma_Base::kEndPosModelIndex)
                        rep0 += Lzma_BitTreeDecoder::ReverseDecode(m_PosDecoders, rep0 - posSlot - 1, m_RangeDecoder, numDirectBits);
                    else {
                        rep0 += m_RangeDecoder->DecodeDirectBits(numDirectBits - Lzma_Base::kNumAlignBits) << Lzma_Base::kNumAlignBits;
                        rep0 += m_PosAlignDecoder.ReverseDecode(m_RangeDecoder);
                    }
                } else
                    rep0 = posSlot;
            }
            // the SDK's TrainSize is 0 here: nothing is decoded on top of earlier data
            if (rep0 >= nowPos64 || rep0 >= m_DictionarySizeCheck) {
                // the end marker: the end, or, with the size known, an end too soon
                if (rep0 == 0xFFFFFFFFu && toEnd) break;
                throw cs::InvalidDataException(rep0 == 0xFFFFFFFFu ? "LZMA: the data ends before its stated size" : "LZMA: the data does not decode");
            }
            if (nowPos64 + len > outSize64)
                throw cs::InvalidDataException(toEnd ? "LZMA: the data is larger than " + cs::ToStr(limit) + " bytes" : "LZMA: the data runs past its stated size");
            m_OutWindow->CopyBlock(rep0, len);
            nowPos64 += len;
        }
    }
    m_OutWindow->Flush();
    consumed = m_RangeDecoder->Position() - inputOffset;
    return output->Data.ToArray();
}

void Lzma_Decoder::SetDecoderProperties(cs::List<uint8_t> properties, int32_t outSize) {
    if (properties.Length() < 5) throw cs::InvalidDataException("LZMA: bad properties");
    int32_t lc = properties[0] % 9;
    int32_t remainder = properties[0] / 9;
    int32_t lp = remainder % 5;
    int32_t pb = remainder / 5;
    if (pb > Lzma_Base::kNumPosStatesBitsMax) throw cs::InvalidDataException("LZMA: bad properties");
    uint32_t dictionarySize = 0;
    for (int32_t i = 0; i < 4; i++)
        dictionarySize += ((uint32_t)properties[1 + i]) << (i * 8);
    SetDictionarySize(dictionarySize, outSize);
    SetLiteralProperties(lp, lc);
    SetPosBitsProperties(pb);
}

}  // namespace mc
