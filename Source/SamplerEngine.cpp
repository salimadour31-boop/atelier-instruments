// Atelier Instruments - SFZ-style sample player engine
// Copyright (c) 2026 Amirouche Amwanes - AGPL-3.0-or-later (see LICENSE)
#include "SamplerEngine.h"
#include <map>
#include <cmath>

namespace atelier
{
using Opcodes = std::map<juce::String, juce::String>;

static void applyOpcodes (Region& r, const Opcodes& o)
{
    for (auto& [k, v] : o)
    {
        if (k == "sample") r.samplePath = v.replaceCharacter ('\\', '/');
        else if (k == "key") { r.lokey = r.hikey = r.keycenter = v.getIntValue(); }
        else if (k == "lokey") r.lokey = v.getIntValue();
        else if (k == "hikey") r.hikey = v.getIntValue();
        else if (k == "pitch_keycenter") r.keycenter = v.getIntValue();
        else if (k == "lovel") r.lovel = v.getIntValue();
        else if (k == "hivel") r.hivel = v.getIntValue();
        else if (k == "seq_length") r.seqLength = juce::jmax (1, v.getIntValue());
        else if (k == "seq_position") r.seqPosition = juce::jmax (1, v.getIntValue());
        else if (k == "tune") r.tuneCents += v.getFloatValue();
        else if (k == "transpose") r.tuneCents += 100.0f * v.getFloatValue();
        else if (k == "volume") r.volumeDb = v.getFloatValue();
        else if (k == "pan") r.pan = v.getFloatValue();
        else if (k == "group") r.group = v.getIntValue();
        else if (k == "off_by") r.offBy = v.getIntValue();
        else if (k == "bus") r.bus = juce::jlimit (0, SamplerEngine::numBuses - 1, v.getIntValue());
        else if (k == "loop_mode") { r.oneShot = (v == "one_shot"); r.loop = (v == "loop_continuous" || v == "loop_sustain"); }
        else if (k == "loop_start") r.loopStart = v.getIntValue();
        else if (k == "loop_end") r.loopEnd = v.getIntValue();
        else if (k == "ampeg_attack") r.attack = v.getFloatValue();
        else if (k == "ampeg_decay") r.decay = v.getFloatValue();
        else if (k == "ampeg_sustain") r.sustain = v.getFloatValue() / 100.0f;
        else if (k == "ampeg_release") r.release = v.getFloatValue();
        else if (k == "amp_veltrack") r.veltrack = v.getFloatValue() / 100.0f;
    }
}

std::unique_ptr<Instrument> loadSfz (const juce::File& sfzFile)
{
    auto inst = std::make_unique<Instrument>();
    if (! sfzFile.existsAsFile()) { inst->error = "SFZ introuvable : " + sfzFile.getFullPathName(); return inst; }

    // strip comments, tokenize
    juce::StringArray lines;
    lines.addLines (sfzFile.loadFileAsString());
    juce::StringArray tokens;
    for (auto line : lines)
    {
        auto c = line.indexOf ("//");
        if (c >= 0) line = line.substring (0, c);
        tokens.addTokens (line.replace ("<", " <").replace (">", "> "), " \t", "");
    }
    tokens.removeEmptyStrings();

    Opcodes global, group, region;
    juce::String header;
    std::vector<Opcodes> regionDefs;
    auto flush = [&] { if (header == "<region>") { Opcodes m = global; for (auto& kv : group) m[kv.first] = kv.second; for (auto& kv : region) m[kv.first] = kv.second; regionDefs.push_back (m); } };

    for (auto& t : tokens)
    {
        if (t.startsWithChar ('<'))
        {
            flush();
            header = t;
            region.clear();
            if (header == "<group>") group.clear();
            if (header == "<global>") { global.clear(); group.clear(); }
            continue;
        }
        auto eq = t.indexOfChar ('=');
        if (eq <= 0) continue;
        auto k = t.substring (0, eq), v = t.substring (eq + 1);
        if (header == "<region>") region[k] = v;
        else if (header == "<group>") group[k] = v;
        else if (header == "<global>" || header == "<control>") global[k] = v;
    }
    flush();

    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::map<juce::String, std::shared_ptr<SampleData>> cache;
    auto base = sfzFile.getParentDirectory();

    for (auto& def : regionDefs)
    {
        Region r;
        applyOpcodes (r, def);
        if (r.samplePath.isEmpty()) continue;
        auto& cached = cache[r.samplePath];
        if (cached == nullptr)
        {
            auto f = base.getChildFile (r.samplePath);
            std::unique_ptr<juce::AudioFormatReader> reader (fm.createReaderFor (f));
            if (reader == nullptr) { inst->error = "Echantillon illisible : " + r.samplePath; continue; }
            auto n = (int) reader->lengthInSamples;
            juce::AudioBuffer<float> tmp ((int) reader->numChannels, n);
            reader->read (&tmp, 0, n, 0, true, true);
            auto sd = std::make_shared<SampleData>();
            sd->numFrames = n;
            sd->sampleRate = reader->sampleRate;
            sd->frames.resize ((size_t) n * 2);
            for (int i = 0; i < n; ++i)
                for (int ch = 0; ch < 2; ++ch)
                {
                    auto s = tmp.getSample (juce::jmin (ch, tmp.getNumChannels() - 1), i);
                    sd->frames[(size_t) (i * 2 + ch)] = (int16_t) juce::jlimit (-32768, 32767, (int) std::lround (s * 32767.0f));
                }
            cached = sd;
            ++inst->numSamples;
        }
        r.sample = cached;
        if (r.loopEnd <= r.loopStart || r.loopEnd >= r.sample->numFrames) r.loop = r.loop && false;
        inst->regions.push_back (r);
    }
    if (inst->regions.empty() && inst->error.isEmpty()) inst->error = "Aucune region dans " + sfzFile.getFileName();
    return inst;
}

//==============================================================================
void SamplerEngine::prepare (double sampleRate, int maxBlock)
{
    sr = sampleRate;
    busBuffer.setSize (numBuses * 2, juce::jmax (1, maxBlock));
    for (auto& v : voices) { v.active = false; v.env.setSampleRate (sr); }
}

void SamplerEngine::setInstrument (std::shared_ptr<Instrument> inst)
{
    std::atomic_store (&instrument, std::move (inst));
}

int SamplerEngine::activeVoiceCount() const
{
    int n = 0;
    for (auto& v : voices) n += v.active ? 1 : 0;
    return n;
}

void SamplerEngine::allNotesOff()
{
    for (auto& v : voices) if (v.active) releaseVoice (v, 0.05f);
    sustainPedal = false;
}

void SamplerEngine::releaseVoice (Voice& v, float seconds)
{
    if (seconds >= 0)
    {
        auto prm = v.env.getParameters();
        prm.release = seconds;
        v.env.setParameters (prm);
    }
    v.keyDown = false;
    v.sustained = false;
    v.env.noteOff();
}

void SamplerEngine::startVoice (const Region& r, int note, int vel, const EngineParams& p)
{
    // pick a free voice, else steal the oldest (prefer releasing voices)
    Voice* target = nullptr;
    for (auto& v : voices) if (! v.active) { target = &v; break; }
    if (target == nullptr)
    {
        // steal: released voices first, then the oldest one
        auto score = [] (const Voice& v) { return (v.keyDown ? ((uint64_t) 1 << 62) : 0) + v.age; };
        target = &voices[0];
        for (auto& v : voices) if (score (v) < score (*target)) target = &v;
    }
    auto& v = *target;
    v.region = &r;
    v.note = note;
    v.pos = 0;
    auto semis = (float) (note - r.keycenter) + r.tuneCents / 100.0f + p.tuneSemis;
    v.inc = (r.sample->sampleRate / sr) * std::pow (2.0, semis / 12.0);

    auto vn = (float) vel / 127.0f;
    auto t = juce::jlimit (0.0f, 1.0f, r.veltrack * p.velSens);
    auto g = ((1.0f - t) + t * vn * vn) * juce::Decibels::decibelsToGain (r.volumeDb);
    auto panN = juce::jlimit (-1.0f, 1.0f, r.pan / 100.0f);
    v.gainL = g * std::sqrt (0.5f * (1.0f - panN)) * juce::MathConstants<float>::sqrt2;
    v.gainR = g * std::sqrt (0.5f * (1.0f + panN)) * juce::MathConstants<float>::sqrt2;

    juce::ADSR::Parameters prm;
    prm.attack = p.attack >= 0 ? p.attack : r.attack;
    prm.decay = p.decay >= 0 ? p.decay : r.decay;
    prm.sustain = p.sustain >= 0 ? p.sustain : r.sustain;
    prm.release = r.oneShot ? juce::jmax (0.05f, r.release) : (p.release >= 0 ? p.release : r.release);
    if (prm.decay <= 0.0f) { prm.decay = 0.001f; prm.sustain = juce::jmax (prm.sustain, 0.0f); if (p.decay < 0) prm.sustain = 1.0f; }
    prm.attack = juce::jmax (0.0005f, prm.attack);
    v.env.setSampleRate (sr);
    v.env.setParameters (prm);
    v.env.reset();
    v.env.noteOn();
    v.active = true;
    v.keyDown = true;
    v.sustained = false;
    v.age = ++ageCounter;
}

void SamplerEngine::noteOn (int note, int vel, const EngineParams& p)
{
    if (current == nullptr) return;
    auto counter = rrCounter[(size_t) note]++;

    // choke groups (off_by) and piano-style re-strike damping
    std::vector<int> groupsStarted;
    for (auto& r : current->regions)
        if (note >= r.lokey && note <= r.hikey && vel >= r.lovel && vel <= r.hivel
            && (int) (counter % (unsigned) r.seqLength) + 1 == r.seqPosition)
        {
            if (r.group != 0) groupsStarted.push_back (r.group);
            // re-strike: release older voices on the same key (max 3 kept for one-shots)
            int sameKey = 0;
            for (auto& v : voices)
                if (v.active && v.note == note)
                {
                    if (! r.oneShot) releaseVoice (v, 0.12f);
                    else if (++sameKey >= 3) releaseVoice (v, 0.08f);
                }
            startVoice (r, note, vel, p);
        }
    for (auto g : groupsStarted)
        for (auto& v : voices)
            if (v.active && v.region->offBy == g && v.age != ageCounter && v.region->group != g)
                releaseVoice (v, 0.06f);
}

void SamplerEngine::noteOff (int note)
{
    for (auto& v : voices)
        if (v.active && v.note == note && v.keyDown && ! v.region->oneShot)
        {
            if (sustainPedal) { v.keyDown = false; v.sustained = true; }
            else releaseVoice (v);
        }
}

void SamplerEngine::renderVoices (int start, int num, const EngineParams&)
{
    for (auto& v : voices)
    {
        if (! v.active) continue;
        auto& r = *v.region;
        auto& s = *r.sample;
        auto* L = busBuffer.getWritePointer (r.bus * 2) + start;
        auto* R = busBuffer.getWritePointer (r.bus * 2 + 1) + start;
        for (int i = 0; i < num; ++i)
        {
            if (r.loop && v.pos >= (double) r.loopEnd) v.pos -= (double) (r.loopEnd - r.loopStart);
            if (v.pos >= (double) (s.numFrames - 2)) { v.active = false; break; }
            auto idx = (int) v.pos;
            auto fr = (float) (v.pos - idx);
            // 4-point Hermite interpolation
            float out[2];
            for (int ch = 0; ch < 2; ++ch)
            {
                auto xm1 = s.get (ch, idx - 1), x0 = s.get (ch, idx), x1 = s.get (ch, idx + 1), x2 = s.get (ch, idx + 2);
                auto c1 = 0.5f * (x1 - xm1);
                auto c2 = xm1 - 2.5f * x0 + 2.0f * x1 - 0.5f * x2;
                auto c3 = 0.5f * (x2 - xm1) + 1.5f * (x0 - x1);
                out[ch] = ((c3 * fr + c2) * fr + c1) * fr + x0;
            }
            auto e = v.env.getNextSample();
            L[i] += out[0] * v.gainL * e;
            R[i] += out[1] * v.gainR * e;
            v.pos += v.inc;
            if (! v.env.isActive()) { v.active = false; break; }
        }
    }
}

void SamplerEngine::process (juce::AudioBuffer<float>& output, const juce::MidiBuffer& midi, const EngineParams& p)
{
    auto loaded = std::atomic_load (&instrument);
    if (loaded != current)
    {
        for (auto& v : voices) v.active = false; // never point into a freed instrument
        current = loaded;
    }
    const int n = output.getNumSamples();
    if (busBuffer.getNumSamples() < n) busBuffer.setSize (numBuses * 2, n, false, false, true);
    busBuffer.clear (0, n);

    int pos = 0;
    for (const auto meta : midi)
    {
        auto m = meta.getMessage();
        auto t = juce::jlimit (0, n, meta.samplePosition);
        if (t > pos) { renderVoices (pos, t - pos, p); pos = t; }
        if (m.isNoteOn()) noteOn (m.getNoteNumber(), m.getVelocity(), p);
        else if (m.isNoteOff()) noteOff (m.getNoteNumber());
        else if (m.isSustainPedalOn()) sustainPedal = true;
        else if (m.isSustainPedalOff())
        {
            sustainPedal = false;
            for (auto& v : voices) if (v.active && v.sustained) releaseVoice (v);
        }
        else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff();
    }
    if (pos < n) renderVoices (pos, n - pos, p);

    for (int b = 0; b < numBuses; ++b)
        for (int ch = 0; ch < juce::jmin (2, output.getNumChannels()); ++ch)
            output.addFrom (ch, 0, busBuffer, b * 2 + ch, 0, n, p.busGain[(size_t) b] * p.masterGain);
}
} // namespace atelier
