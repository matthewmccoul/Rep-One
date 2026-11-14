#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Lzma_RangeDecoder;
struct Lzma_OutWindow;
struct Lzma_Decoder;
struct Lzma_Decoder_LenDecoder;
struct Lzma_Decoder_LiteralDecoder;

struct Lzma_Base_State {
    uint32_t Index = 0;
    void Init();
    void UpdateChar();
    void UpdateMatch();
    void UpdateRep();
    void UpdateShortRep();
    bool IsCharState();
};

struct Lzma_Base {
    using State = Lzma_Base_State;
    static constexpr uint32_t kNumRepDistances = 4;
    static constexpr uint32_t kNumStates = 12;
    static constexpr int32_t kNumPosSlotBits = 6;
    static constexpr uint32_t kNumLenToPosStatesBits = 2;
    static constexpr uint32_t kNumLenToPosStates = 1u << (int32_t)kNumLenToPosStatesBits;
    static constexpr uint32_t kMatchMinLen = 2;
    static uint32_t GetLenToPosState(uint32_t len);
    static constexpr int32_t kNumAlignBits = 4;
    static constexpr uint32_t kStartPosModelIndex = 4;
    static constexpr uint32_t kEndPosModelIndex = 14;
    static constexpr uint32_t kNumFullDistances = 1u << ((int32_t)kEndPosModelIndex / 2);
    static constexpr int32_t kNumPosStatesBitsMax = 4;
    static constexpr uint32_t kNumPosStatesMax = 1u << kNumPosStatesBitsMax;
    static constexpr int32_t kNumLowLenBits = 3;
    static constexpr int32_t kNumMidLenBits = 3;
    static constexpr int32_t kNumHighLenBits = 8;
    static constexpr uint32_t kNumLowLenSymbols = 1u << kNumLowLenBits;
    static constexpr uint32_t kNumMidLenSymbols = 1u << kNumMidLenBits;
};

struct Lzma_RangeDecoder {
    static constexpr uint32_t kTopValue = 1u << 24;
    uint32_t Range = 0;
    uint32_t Code = 0;
    cs::List<uint8_t> _in = cs::List<uint8_t>::New();
    int32_t _pos = 0;
    int32_t _end = 0;
    int32_t Position();
    uint8_t ReadByte();
    void Init(cs::List<uint8_t> input, int32_t offset, int32_t length);
    uint32_t DecodeDirectBits(int32_t numTotalBits);
};

struct Lzma_BitDecoder {
    static constexpr int32_t kNumBitModelTotalBits = 11;
    static constexpr uint32_t kBitModelTotal = 1u << kNumBitModelTotalBits;
    static constexpr int32_t kNumMoveBits = 5;
    uint32_t Prob = 0;
    void Init();
    uint32_t Decode(cs::Ref<Lzma_RangeDecoder> rangeDecoder);
};

struct Lzma_BitTreeDecoder {
    cs::List<Lzma_BitDecoder> Models;
    int32_t NumBitLevels = 0;
    Lzma_BitTreeDecoder() = default;
    Lzma_BitTreeDecoder(int32_t numBitLevels);
    void Init();
    uint32_t Decode(cs::Ref<Lzma_RangeDecoder> rangeDecoder);
    uint32_t ReverseDecode(cs::Ref<Lzma_RangeDecoder> rangeDecoder);
    static uint32_t ReverseDecode(cs::List<Lzma_BitDecoder> Models, uint32_t startIndex, cs::Ref<Lzma_RangeDecoder> rangeDecoder, int32_t NumBitLevels);
};

struct Lzma_OutWindow {
    cs::List<uint8_t> _buffer = cs::List<uint8_t>::New();
    uint32_t _pos = 0;
    uint32_t _windowSize = 0;
    uint32_t _streamPos = 0;
    cs::Ref<cs::Stream> _stream = cs::New<cs::Stream>();
    void Create(uint32_t windowSize);
    void Init(cs::Ref<cs::Stream> stream);
    void Flush();
    void CopyBlock(uint32_t distance, uint32_t len);
    void PutByte(uint8_t b);
    uint8_t GetByte(uint32_t distance);
};

struct Lzma_Decoder_LiteralDecoder_Decoder2 {
    cs::List<Lzma_BitDecoder> m_Decoders;
    void Create();
    void Init();
    uint8_t DecodeNormal(cs::Ref<Lzma_RangeDecoder> rangeDecoder);
    uint8_t DecodeWithMatchByte(cs::Ref<Lzma_RangeDecoder> rangeDecoder, uint8_t matchByte);
};

struct Lzma_Decoder_LiteralDecoder {
    using Decoder2 = Lzma_Decoder_LiteralDecoder_Decoder2;
    cs::List<Lzma_Decoder_LiteralDecoder_Decoder2> m_Coders = cs::List<Lzma_Decoder_LiteralDecoder_Decoder2>::New();
    int32_t m_NumPrevBits = 0;
    int32_t m_NumPosBits = 0;
    uint32_t m_PosMask = 0;
    void Create(int32_t numPosBits, int32_t numPrevBits);
    void Init();
    uint32_t GetState(uint32_t pos, uint8_t prevByte);
    uint8_t DecodeNormal(cs::Ref<Lzma_RangeDecoder> rangeDecoder, uint32_t pos, uint8_t prevByte);
    uint8_t DecodeWithMatchByte(cs::Ref<Lzma_RangeDecoder> rangeDecoder, uint32_t pos, uint8_t prevByte, uint8_t matchByte);
};

struct Lzma_Decoder_LenDecoder {
    Lzma_BitDecoder m_Choice{};
    Lzma_BitDecoder m_Choice2{};
    cs::List<Lzma_BitTreeDecoder> m_LowCoder = cs::List<Lzma_BitTreeDecoder>::New((int32_t)Lzma_Base::kNumPosStatesMax);
    cs::List<Lzma_BitTreeDecoder> m_MidCoder = cs::List<Lzma_BitTreeDecoder>::New((int32_t)Lzma_Base::kNumPosStatesMax);
    Lzma_BitTreeDecoder m_HighCoder = Lzma_BitTreeDecoder(Lzma_Base::kNumHighLenBits);
    uint32_t m_NumPosStates = 0;
    void Create(uint32_t numPosStates);
    void Init();
    uint32_t Decode(cs::Ref<Lzma_RangeDecoder> rangeDecoder, uint32_t posState);
};

struct Lzma_Decoder {
    using LenDecoder = Lzma_Decoder_LenDecoder;
    using LiteralDecoder = Lzma_Decoder_LiteralDecoder;
    cs::Ref<Lzma_OutWindow> m_OutWindow = cs::New<Lzma_OutWindow>();
    cs::Ref<Lzma_RangeDecoder> m_RangeDecoder = cs::New<Lzma_RangeDecoder>();
    cs::List<Lzma_BitDecoder> m_IsMatchDecoders = cs::List<Lzma_BitDecoder>::New((int32_t)(Lzma_Base::kNumStates << Lzma_Base::kNumPosStatesBitsMax));
    cs::List<Lzma_BitDecoder> m_IsRepDecoders = cs::List<Lzma_BitDecoder>::New((int32_t)Lzma_Base::kNumStates);
    cs::List<Lzma_BitDecoder> m_IsRepG0Decoders = cs::List<Lzma_BitDecoder>::New((int32_t)Lzma_Base::kNumStates);
    cs::List<Lzma_BitDecoder> m_IsRepG1Decoders = cs::List<Lzma_BitDecoder>::New((int32_t)Lzma_Base::kNumStates);
    cs::List<Lzma_BitDecoder> m_IsRepG2Decoders = cs::List<Lzma_BitDecoder>::New((int32_t)Lzma_Base::kNumStates);
    cs::List<Lzma_BitDecoder> m_IsRep0LongDecoders = cs::List<Lzma_BitDecoder>::New((int32_t)(Lzma_Base::kNumStates << Lzma_Base::kNumPosStatesBitsMax));
    cs::List<Lzma_BitTreeDecoder> m_PosSlotDecoder = cs::List<Lzma_BitTreeDecoder>::New((int32_t)Lzma_Base::kNumLenToPosStates);
    cs::List<Lzma_BitDecoder> m_PosDecoders = cs::List<Lzma_BitDecoder>::New((int32_t)(Lzma_Base::kNumFullDistances - Lzma_Base::kEndPosModelIndex));
    Lzma_BitTreeDecoder m_PosAlignDecoder = Lzma_BitTreeDecoder(Lzma_Base::kNumAlignBits);
    cs::Ref<Lzma_Decoder_LenDecoder> m_LenDecoder = cs::New<Lzma_Decoder_LenDecoder>();
    cs::Ref<Lzma_Decoder_LenDecoder> m_RepLenDecoder = cs::New<Lzma_Decoder_LenDecoder>();
    cs::Ref<Lzma_Decoder_LiteralDecoder> m_LiteralDecoder = cs::New<Lzma_Decoder_LiteralDecoder>();
    uint32_t m_DictionarySize = 0;
    uint32_t m_DictionarySizeCheck = 0;
    uint32_t m_PosStateMask = 0;
    Lzma_Decoder();
    void SetDictionarySize(uint32_t dictionarySize, int32_t outSize);
    void SetLiteralProperties(int32_t lp, int32_t lc);
    void SetPosBitsProperties(int32_t pb);
    void Init(cs::List<uint8_t> input, int32_t inputOffset, int32_t inputLength, cs::Ref<cs::Stream> output);
    cs::List<uint8_t> Code(cs::List<uint8_t> input, int32_t inputOffset, int32_t inputLength, int64_t outSize, int32_t limit, int32_t& consumed);
    void SetDecoderProperties(cs::List<uint8_t> properties, int32_t outSize);
};

struct Lzma {
    using Base = Lzma_Base;
    using RangeDecoder = Lzma_RangeDecoder;
    using BitDecoder = Lzma_BitDecoder;
    using BitTreeDecoder = Lzma_BitTreeDecoder;
    using OutWindow = Lzma_OutWindow;
    using Decoder = Lzma_Decoder;
    static cs::List<uint8_t> Decode(cs::List<uint8_t> props, cs::List<uint8_t> input, int32_t inputOffset, int32_t inputLength, int32_t size, int32_t& consumed);
    static cs::List<uint8_t> DecodeToEnd(cs::List<uint8_t> props, cs::List<uint8_t> input, int32_t inputOffset, int32_t inputLength, int32_t limit, int32_t& consumed);
};

}  // namespace mc
