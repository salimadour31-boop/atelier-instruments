// Offline render test: plays a few notes through the engine, writes a WAV,
// and fails if the output is silent or clips.
// Usage: atelier_render <file.sfz> <out.wav> <note,vel,start_s,len_s>...
#include "../Source/SamplerEngine.h"
#include <iostream>

int main (int argc, char** argv)
{
    if (argc < 4) { std::cerr << "usage: atelier_render file.sfz out.wav note,vel,start,len ...\n"; return 2; }
    auto inst = atelier::loadSfz (juce::File (juce::String (argv[1])));
    if (inst->regions.empty()) { std::cerr << inst->error << "\n"; return 3; }
    std::cout << "regions=" << inst->regions.size() << " samples=" << inst->numSamples << "\n";

    const double sr = 48000.0; const int block = 256;
    juce::MidiBuffer all; double endT = 0;
    for (int i = 3; i < argc; ++i)
    {
        juce::StringArray f; f.addTokens (argv[i], ",", "");
        int note = f[0].getIntValue(), vel = f[1].getIntValue();
        double st = f[2].getDoubleValue(), len = f[3].getDoubleValue();
        all.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) vel), (int) (st * sr));
        all.addEvent (juce::MidiMessage::noteOff (1, note), (int) ((st + len) * sr));
        endT = juce::jmax (endT, st + len + 2.0);
    }
    const int total = (int) (endT * sr);
    juce::AudioBuffer<float> out (2, total); out.clear();
    atelier::SamplerEngine eng; eng.prepare (sr, block);
    eng.setInstrument (std::shared_ptr<atelier::Instrument> (inst.release()));
    atelier::EngineParams p; p.masterGain = 0.7f;
    for (int s = 0; s < total; s += block)
    {
        int n = juce::jmin (block, total - s);
        juce::MidiBuffer mb; mb.addEvents (all, s, n, -s);
        juce::AudioBuffer<float> view (out.getArrayOfWritePointers(), 2, s, n);
        eng.process (view, mb, p);
    }
    float peak = out.getMagnitude (0, total);
    double rms = 0.5 * (out.getRMSLevel (0, 0, total) + out.getRMSLevel (1, 0, total));
    std::cout << "peak=" << peak << " (" << juce::Decibels::gainToDecibels (peak) << " dBFS) rms=" << juce::Decibels::gainToDecibels ((float) rms) << " dBFS\n";
    juce::File of { juce::String (argv[2]) }; of.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (of.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0))) { os.release(); w->writeFromAudioSampleBuffer (out, 0, total); }
    if (peak < 1.0e-3f) { std::cerr << "FAIL: silent\n"; return 4; }
    if (peak >= 0.999f) { std::cerr << "FAIL: clipping\n"; return 5; }
    std::cout << "OK\n";
    return 0;
}
