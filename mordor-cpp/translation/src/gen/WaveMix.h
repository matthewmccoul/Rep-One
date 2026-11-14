#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct IWaveMixDevice {
    virtual ~IWaveMixDevice() = default;
    virtual int32_t OpenWave(cs::String path) = 0;
    virtual void FreeWave(int32_t wave) = 0;
    virtual void FlushChannel(int32_t channel) = 0;
    virtual void Play(int32_t channel, int32_t wave) = 0;
};

struct WaveMixSession {
    static constexpr int32_t CacheSize = 7;
    static constexpr int32_t LastChannel = 7;
    static constexpr float HoldSeconds = 5;

    cs::Ref<IWaveMixDevice> _device;
    cs::Func<float()> _timer;
    cs::Func<cs::String(int32_t)> _pathOf;

    cs::List<int32_t> Wave;
    cs::List<int32_t> Id;
    cs::List<float> Assigned;

    cs::List<int32_t> _open = cs::List<int32_t>::Of({0});

    int32_t LastHanded = 0;

    WaveMixSession(cs::Ref<IWaveMixDevice> device, cs::Func<float()> timer, cs::Func<cs::String(int32_t)> pathOf, int32_t sounds = 72);

    cs::List<int32_t> OpenSounds();

    static float VbTimer();

    void Play(int32_t sound, WaveMixRoutine routine);

    int32_t FxVolume = 0;

    cs::Func<void(int32_t)> WaveOutSetVolume;

    void SetVolume();
    void DeInitialize();
    void Sub1A31(int32_t p, WaveMixRoutine mode);
    void Sub1A3F();
    int32_t Fn19F4();
};

struct WaveMixChannels_Playing {
    int32_t Wave = 0;
    cs::List<int16_t> Data = cs::List<int16_t>::New();
    int32_t Pos = 0;
};

struct WaveMixChannels : IWaveMixDevice {
    using Playing = WaveMixChannels_Playing;

    static constexpr int32_t Channels = 8;
    static constexpr int32_t Rate = 22050;

    std::shared_ptr<void> _gate = std::make_shared<int>(0);
    cs::Func<cs::List<int16_t>(cs::String)> _load;
    cs::Dictionary<int32_t, cs::List<int16_t>> _waves = cs::Dictionary<int32_t, cs::List<int16_t>>::New();
    cs::List<cs::List<cs::Ref<WaveMixChannels_Playing>>> _queues = cs::Enumerable::Range(0, Channels).Select([](auto&) { return cs::List<cs::Ref<WaveMixChannels_Playing>>::New(); }).ToArray();
    int32_t _nextHandle = 1;
    bool _active = true;

    WaveMixChannels(cs::Func<cs::List<int16_t>(cs::String)> load);

    int32_t OpenWave(cs::String path) override;
    void FreeWave(int32_t wave) override;
    void FlushChannel(int32_t channel) override;
    void Play(int32_t channel, int32_t wave) override;

    bool Active();
    void set_Active(bool value);

    bool _closed = false;

    void CloseSession();
    void OpenSession();
    bool Closed();
    cs::List<int32_t> Queued(int32_t channel);
    bool Busy();
    void Mix(cs::List<int16_t> output);

    int32_t _deviceVolume = -1;

    int32_t DeviceVolume();
    void set_DeviceVolume(int32_t value);
};

struct WaveVolume {
    static constexpr int32_t Default = -65538;
    static constexpr int32_t SliderMax = 32767;
    static int32_t FromSlider(int32_t value);
    static int32_t ToSlider(int32_t raw);
};

struct WavePcm {
    static cs::List<int16_t> Decode(cs::List<uint8_t> file, int32_t rate = WaveMixChannels::Rate);
    static cs::String Ascii(cs::List<uint8_t> b, int32_t at);
};

}  // namespace mc
