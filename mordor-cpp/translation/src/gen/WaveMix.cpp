#include "core.h"

namespace mc {

// ------------------------------------------------------------------ WaveMixSession

WaveMixSession::WaveMixSession(cs::Ref<IWaveMixDevice> device, cs::Func<float()> timer, cs::Func<cs::String(int32_t)> pathOf, int32_t sounds) {
    _device = device; _timer = timer; _pathOf = pathOf;
    Wave = cs::List<int32_t>::New(sounds); Id = cs::List<int32_t>::New(sounds); Assigned = cs::List<float>::New(sounds);
}

cs::List<int32_t> WaveMixSession::OpenSounds() { return _open.Skip(1).ToList(); }

float WaveMixSession::VbTimer() {
    auto now = cs::DateTime::Now();
    return (float)((double)(now.ticks % 864000000000LL) / 1e7);  // DateTime.Now.TimeOfDay.TotalSeconds
}

void WaveMixSession::Play(int32_t sound, WaveMixRoutine routine) {
    if (sound < 0 || sound >= Wave.Length()) return;
    Sub1A31(sound, routine);
    switch (routine) {
        case WaveMixRoutine::Sub09DE:  // MUSIC.BAS:146-167
            if (Wave[sound] == 0) return;
            SetVolume();
            _device->FlushChannel(Id[sound]);
            _device->Play(Id[sound], Wave[sound]);
            break;
        case WaveMixRoutine::Sub086D:  // MUSIC.BAS:207-227
            if (Wave[sound] == 0) return;
            SetVolume();
            _device->Play(Id[sound], Wave[sound]);
            break;
        default:  // MUSIC.BAS:238-246, no test of M7208
            _device->Play(Id[sound], Wave[sound]);
            break;
    }
}

void WaveMixSession::SetVolume() {
    if (FxVolume != 0) { if (WaveOutSetVolume) WaveOutSetVolume(FxVolume); }
}

void WaveMixSession::DeInitialize() {
    for (int32_t n = 0; n < Wave.Length(); n++)
        if (Wave[n] != 0) {
            _device->FreeWave(Wave[n]);
            Wave[n] = 0;
        }
}

void WaveMixSession::Sub1A31(int32_t p, WaveMixRoutine mode) {
    cs::String path = _pathOf(p);
    if (path.empty()) {
        Wave[p] = 0; Id[p] = 0; Assigned[p] = 0;
        return;
    }
    if (Wave[p] == 0) {
        if (_open.Count() - 1 == CacheSize) Sub1A3F();
        else _open.Add(0);
        int32_t slot = _open.Count() - 1;
        Wave[p] = _device->OpenWave(path);
        _open[slot] = p;
        if (mode == WaveMixRoutine::Sub09DE) {
            Assigned[p] = _timer();
            Id[p] = Fn19F4();
        } else Assigned[p] = 0;
    }
    bool hand = true;
    if (mode == WaveMixRoutine::Sub08D1) {
        Id[p] = LastHanded;
        return;
    }
    if (mode == WaveMixRoutine::Sub09DE && Assigned[p] > 0 && Assigned[p] + HoldSeconds > _timer())
        hand = false;
    if ((mode == WaveMixRoutine::Sub09DE && hand) || mode == WaveMixRoutine::Sub086D) {
        Id[p] = Fn19F4();
        if (mode == WaveMixRoutine::Sub09DE) Assigned[p] = _timer();
    }
}

void WaveMixSession::Sub1A3F() {
    int32_t first = _open[1];
    if (Wave[first] != 0) {
        _device->FreeWave(Wave[first]);
        Wave[first] = 0;
        Assigned[first] = 0;
    }
    for (int32_t i = 1; i <= CacheSize - 1; i++) _open[i] = _open[i + 1];
}

int32_t WaveMixSession::Fn19F4() {
    int32_t tries = 0;
    bool busy;
    do {
        busy = false;
        LastHanded++;
        if (LastHanded > LastChannel) LastHanded = 1;
        if (tries > 7) break;
        for (int32_t i = 1; i < _open.Count(); i++) {
            int32_t s = _open[i];
            if (Assigned[s] > 0 && Id[s] == LastHanded) {
                if (Assigned[s] + HoldSeconds > _timer()) busy = true;
                else Assigned[s] = 0;
                break;
            }
        }
        tries++;
    } while (busy);
    return LastHanded;
}

// ------------------------------------------------------------------ WaveMixChannels

WaveMixChannels::WaveMixChannels(cs::Func<cs::List<int16_t>(cs::String)> load) { _load = load; }

int32_t WaveMixChannels::OpenWave(cs::String path) {
    cs::List<int16_t> data;
    try { data = _load(path); } catch (...) { data = nullptr; }
    if (data == nullptr) return 0;
    {
        if (_closed) return 0;
        int32_t h = _nextHandle++;
        _waves[h] = data;
        return h;
    }
}

void WaveMixChannels::FreeWave(int32_t wave) {
    {
        _waves.Remove(wave);
        for (auto& q : _queues) q.RemoveAll([&](const cs::Ref<Playing>& p) { return p->Wave == wave; });
    }
}

void WaveMixChannels::FlushChannel(int32_t channel) {
    if (channel < 0 || channel >= Channels) return;
    _queues[channel].Clear();
}

void WaveMixChannels::Play(int32_t channel, int32_t wave) {
    if (channel < 0 || channel >= Channels) return;  // "An invalid channel (i.e. > 7)" (MODULE13.BAS:24)
    {
        cs::List<int16_t> data{};
        if (!_active || !_waves.TryGetValue(wave, data)) return;
        _queues[channel].Add([&] { auto _o = cs::New<Playing>(); _o->Wave = wave; _o->Data = data; return _o; }());
    }
}

bool WaveMixChannels::Active() { return _active; }

void WaveMixChannels::set_Active(bool value) {
    {
        _active = value;
        if (!value) for (auto& q : _queues) q.Clear();
    }
}

void WaveMixChannels::CloseSession() {
    {
        _closed = true;
        _waves.Clear();
        for (auto& q : _queues) q.Clear();
    }
}

void WaveMixChannels::OpenSession() {
    { _closed = false; _active = true; }
}

bool WaveMixChannels::Closed() { return _closed; }

cs::List<int32_t> WaveMixChannels::Queued(int32_t channel) {
    return _queues[channel].Select([&](const cs::Ref<Playing>& p) { return p->Wave; }).ToArray();
}

bool WaveMixChannels::Busy() {
    return _queues.Any([&](const cs::List<cs::Ref<Playing>>& q) { return q.Count() > 0; });
}

void WaveMixChannels::Mix(cs::List<int16_t> output) {
    cs::List<int32_t> sum = cs::List<int32_t>::New(output.Length());
    sum.Fill(0);  // Span.Clear zeroes
    {
        for (auto& q : _queues) {
            int32_t i = 0;
            while (i < output.Length() && q.Count() > 0) {
                auto head = q[0];
                int32_t n = cs::Math::Min(output.Length() - i, head->Data.Length() - head->Pos);
                for (int32_t k = 0; k < n; k++) sum[i + k] += head->Data[head->Pos + k];
                head->Pos += n; i += n;
                if (head->Pos >= head->Data.Length()) q.RemoveAt(0);
            }
        }
    }
    // WaveMix sums and clips; the wave device then plays the result at its own volume
    int32_t level = DeviceVolume() & 0xFFFF;
    for (int32_t i = 0; i < output.Length(); i++) {
        int32_t clipped = cs::Math::Clamp<int32_t>(sum[i], (int32_t)INT16_MIN, (int32_t)INT16_MAX);
        output[i] = cs::Wrap<int16_t>(level == 0xFFFF ? clipped : (int32_t)((int64_t)clipped * level / 0xFFFF));
    }
}

int32_t WaveMixChannels::DeviceVolume() { return _deviceVolume; }

void WaveMixChannels::set_DeviceVolume(int32_t value) { _deviceVolume = value; }

// ------------------------------------------------------------------ WaveVolume

int32_t WaveVolume::FromSlider(int32_t value) {
    cs::String h = cs::Str::PadLeft(cs::Format(cs::Math::Clamp<int32_t>(value, 0, SliderMax) * 2, "X"), 4, '0');
    return cs::Wrap<int32_t>((long long)(uint32_t)std::stoul(h + h, nullptr, 16));
}

int32_t WaveVolume::ToSlider(int32_t raw) {
    return raw == 0 ? 0 : cs::CastDouble<int32_t>(cs::Math::Round((raw & 0xFFFF) / 2.0, cs::MidpointRounding::ToEven));
}

// ------------------------------------------------------------------ WavePcm

cs::List<int16_t> WavePcm::Decode(cs::List<uint8_t> file, int32_t rate) {
    if (file.Length() < 12 || Ascii(file, 0) != "RIFF" || Ascii(file, 8) != "WAVE") return nullptr;
    int32_t at = 12;
    int32_t format = 0, channels = 0, sampleRate = 0, bits = 0;
    int32_t dataAt = -1, dataLen = 0;
    const uint8_t* raw = file.Raw().data();
    while (at + 8 <= file.Length()) {
        cs::String id = Ascii(file, at);
        int32_t len = cs::BinaryPrimitives::ReadInt32LittleEndian(raw + at + 4);
        if (len < 0) return nullptr;
        int32_t body = at + 8;
        if (id == "fmt " && body + 16 <= file.Length()) {
            format = cs::BinaryPrimitives::ReadUInt16LittleEndian(raw + body);
            channels = cs::BinaryPrimitives::ReadUInt16LittleEndian(raw + body + 2);
            sampleRate = cs::BinaryPrimitives::ReadInt32LittleEndian(raw + body + 4);
            bits = cs::BinaryPrimitives::ReadUInt16LittleEndian(raw + body + 14);
        } else if (id == "data") {
            dataAt = body;
            dataLen = cs::Math::Min(len, file.Length() - body);
            break;
        }
        at = cs::Wrap<int32_t>((long long)body + len + (len & 1));
    }
    if (format != 1 || dataAt < 0 || channels < 1 || sampleRate <= 0 || (bits != 8 && bits != 16)) return nullptr;

    int32_t bytesPer = bits / 8;
    int32_t frames = dataLen / (bytesPer * channels);
    auto mono = cs::List<int16_t>::New(frames);
    for (int32_t f = 0; f < frames; f++) {
        int32_t total = 0;
        for (int32_t c = 0; c < channels; c++) {
            int32_t o = dataAt + (f * channels + c) * bytesPer;
            total += bits == 8 ? (file[o] - 128) * 256 : cs::BinaryPrimitives::ReadInt16LittleEndian(raw + o);  // (x - 128) << 8
        }
        mono[f] = cs::Wrap<int16_t>(total / channels);
    }
    if (sampleRate == rate) return mono;

    int32_t outFrames = (int32_t)((int64_t)frames * rate / sampleRate);
    auto res = cs::List<int16_t>::New(outFrames);
    double step = (double)sampleRate / rate;
    for (int32_t i = 0; i < outFrames; i++) {
        double pos = i * step;
        int32_t a = cs::CastDouble<int32_t>(pos);
        int32_t b = cs::Math::Min(a + 1, frames - 1);
        double t = pos - a;
        res[i] = cs::Wrap<int16_t>(cs::CastDouble<int32_t>(cs::Math::Round(mono[a] + (mono[b] - mono[a]) * t)));
    }
    return res;
}

cs::String WavePcm::Ascii(cs::List<uint8_t> b, int32_t at) {
    return at + 4 <= b.Length() ? cs::Encoding::Latin1GetString(b.Raw().data() + at, 4) : cs::String("");
}

}  // namespace mc
