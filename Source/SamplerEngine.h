// Atelier Instruments - SFZ-style sample player engine
// Copyright (c) 2026 Amirouche Amwanes - AGPL-3.0-or-later (see LICENSE)
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <memory>
#include <vector>
#include <array>

namespace atelier
{
/** Samples are stored as 16-bit integers to keep memory use reasonable. */
struct SampleData
{
    std::vector<int16_t> frames; // interleaved stereo
    int numFrames = 0;
    double sampleRate = 44100.0;

    inline float get (int ch, int i) const noexcept
    {
        i = juce::jlimit (0, numFrames - 1, i);
        return (float) frames[(size_t) (i * 2 + ch)] * (1.0f / 32768.0f);
    }
};

struct Region
{
    juce::String samplePath;
    std::shared_ptr<SampleData> sample;
    int lokey = 0, hikey = 127, keycenter = 60, lovel = 1, hivel = 127;
    int seqLength = 1, seqPosition = 1;
    float tuneCents = 0.0f, volumeDb = 0.0f, pan = 0.0f; // pan -100..100
    int group = 0, offBy = 0, bus = 0;
    bool oneShot = false, loop = false;
    int loopStart = 0, loopEnd = 0;
    float attack = 0.001f, decay = 0.0f, sustain = 1.0f, release = 0.3f;
    float veltrack = 1.0f; // 0..1
};

struct Instrument
{
    std::vector<Region> regions;
    int numSamples = 0;
    juce::String error;
};

/** Parses the subset of SFZ used by our instruments and loads the samples.
    Supported opcodes: sample, key, lokey, hikey, pitch_keycenter, lovel, hivel,
    seq_length, seq_position, tune, transpose, volume, pan, group, off_by,
    loop_mode (one_shot / loop_continuous / no_loop), loop_start, loop_end,
    ampeg_attack/decay/sustain/release, amp_veltrack, bus (Atelier extension). */
std::unique_ptr<Instrument> loadSfz (const juce::File& sfzFile);

struct EngineParams
{
    float masterGain = 1.0f;
    float tuneSemis = 0.0f;
    float attack = -1, decay = -1, sustain = -1, release = -1; // <0 = use region values
    float velSens = 1.0f;
    std::array<float, 8> busGain { 1, 1, 1, 1, 1, 1, 1, 1 };
};

class SamplerEngine
{
public:
    static constexpr int maxVoices = 96;
    static constexpr int numBuses = 8;

    void prepare (double sampleRate, int maxBlock);
    void setInstrument (std::shared_ptr<Instrument> inst); // call from message thread
    bool hasInstrument() const { return std::atomic_load (&instrument) != nullptr; }

    /** Renders and ADDS into output (stereo). */
    void process (juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi, const EngineParams& p);
    void allNotesOff();
    int activeVoiceCount() const;

private:
    struct Voice
    {
        const Region* region = nullptr;
        int note = -1;
        double pos = 0, inc = 1;
        float gainL = 1, gainR = 1;
        juce::ADSR env;
        bool active = false, keyDown = false, sustained = false;
        uint64_t age = 0;
    };

    void noteOn (int note, int vel, const EngineParams& p);
    void noteOff (int note);
    void startVoice (const Region& r, int note, int vel, const EngineParams& p);
    void releaseVoice (Voice& v, float seconds = -1);
    void renderVoices (int start, int num, const EngineParams& p);

    std::shared_ptr<Instrument> instrument, current;
    std::array<Voice, maxVoices> voices;
    std::array<int, 128> rrCounter {};
    juce::AudioBuffer<float> busBuffer; // numBuses * 2 channels
    double sr = 44100.0;
    uint64_t ageCounter = 0;
    bool sustainPedal = false;
};
} // namespace atelier
