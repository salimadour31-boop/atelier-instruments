// Atelier Instruments - Copyright (c) 2026 Amirouche Amwanes - AGPL-3.0-or-later
#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::StringArray atelier::drumBusNames() { return { "Grosse caisse", "Caisse claire", "Charleston", "Toms", "Cymbales" }; }

AtelierProcessor::AtelierProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      juce::Thread ("Atelier sample loader"),
      apvts (*this, nullptr, "ATELIER", createLayout())
{
    startThread();
}

AtelierProcessor::~AtelierProcessor() { stopThread (10000); }

juce::AudioProcessorValueTreeState::ParameterLayout AtelierProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto secs = [] (const String& id, const String& name, float lo, float hi, float def) {
        return std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.0f, 0.4f), def,
                                                      AudioParameterFloatAttributes().withLabel ("s"));
    };
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "volume", 1 }, "Volume", NormalisableRange<float> (-48.0f, 6.0f, 0.1f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "tune", 1 }, "Accord", NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f, AudioParameterFloatAttributes().withLabel ("demi-tons")));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "reverb", 1 }, "Reverb", NormalisableRange<float> (0.0f, 1.0f), atelier::thisInstrument == atelier::Drums ? 0.12f : 0.2f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "room", 1 }, "Taille salle", NormalisableRange<float> (0.0f, 1.0f), 0.6f));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { "velsens", 1 }, "Sensibilite velocite", NormalisableRange<float> (0.0f, 1.5f), 1.0f));

    if (atelier::thisInstrument == atelier::Drums)
    {
        auto names = atelier::drumBusNames();
        for (int i = 0; i < names.size(); ++i)
            l.add (std::make_unique<AudioParameterFloat> (ParameterID { "bus" + String (i), 1 }, names[i], NormalisableRange<float> (-60.0f, 6.0f, 0.1f, 2.5f), 0.0f, AudioParameterFloatAttributes().withLabel ("dB")));
    }
    else
    {
        bool strings = atelier::thisInstrument == atelier::Strings;
        l.add (secs ("attack", "Attaque", 0.0005f, 3.0f, strings ? 0.12f : 0.001f));
        l.add (secs ("decay", "Declin", 0.001f, 10.0f, 0.001f));
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { "sustain", 1 }, "Maintien", NormalisableRange<float> (0.0f, 1.0f), 1.0f));
        l.add (secs ("release", "Relachement", 0.01f, 6.0f, strings ? 0.6f : 0.45f));
    }
    return l;
}

juce::File AtelierProcessor::findInstrumentFolder()
{
    const juce::String folder (ATELIER_SFZ_FOLDER);
    juce::Array<juce::File> candidates;
    // 1) inside the plugin bundle: <bundle>/Contents/<arch or MacOS>/binary -> <bundle>/Contents/Resources
    auto bin = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
    for (auto d = bin.getParentDirectory(); d.exists() && ! d.isRoot(); d = d.getParentDirectory())
    {
        if (d.getFileName() == "Contents") { candidates.add (d.getChildFile ("Resources").getChildFile (folder)); break; }
        if (d.getChildFile ("Contents").isDirectory()) { candidates.add (d.getChildFile ("Contents/Resources").getChildFile (folder)); break; }
    }
    // 2) shared data folders (installer / manual copy)
    candidates.add (juce::File::getSpecialLocation (juce::File::commonApplicationDataDirectory).getChildFile ("Atelier Instruments").getChildFile (folder));
    candidates.add (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("Atelier Instruments").getChildFile (folder));
    candidates.add (juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Atelier Instruments").getChildFile (folder));
    for (auto& c : candidates)
        if (c.getChildFile (ATELIER_SFZ_FILE).existsAsFile()) return c;
    return candidates.getFirst();
}

void AtelierProcessor::run()
{
    auto dir = findInstrumentFolder();
    auto inst = atelier::loadSfz (dir.getChildFile (ATELIER_SFZ_FILE));
    juce::String msg;
    if (inst->regions.empty())
        msg = "Echantillons introuvables. Reinstallez le plugin (voir LISEZMOI). " + inst->error;
    else
        msg = juce::String (inst->numSamples) + " echantillons charges, " + juce::String ((int) inst->regions.size()) + " zones"
              + (inst->error.isNotEmpty() ? " (" + inst->error + ")" : juce::String());
    if (! inst->regions.empty())
    {
        engine.setInstrument (std::shared_ptr<atelier::Instrument> (inst.release()));
        ready = true;
    }
    const juce::ScopedLock sl (statusLock);
    status = msg;
}

void AtelierProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate, samplesPerBlock);
    reverb.setSampleRate (sampleRate);
    reverb.reset();
    masterSmoothed.reset (sampleRate, 0.05);
    keyboardState.reset();
}

bool AtelierProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void AtelierProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);

    auto get = [this] (const char* id) { auto* p = apvts.getRawParameterValue (id); return p != nullptr ? p->load() : -1.0f; };
    atelier::EngineParams p;
    p.masterGain = 1.0f;
    p.tuneSemis = get ("tune");
    p.velSens = get ("velsens");
    if (atelier::thisInstrument == atelier::Drums)
    {
        for (int i = 0; i < 5; ++i) p.busGain[(size_t) i] = juce::Decibels::decibelsToGain (get (("bus" + juce::String (i)).toRawUTF8()), -59.9f);
    }
    else
    {
        p.attack = get ("attack"); p.decay = get ("decay"); p.sustain = get ("sustain"); p.release = get ("release");
        if (p.decay <= 0.0015f) p.decay = -1; // natural decay of the sample
    }
    engine.process (buffer, midi, p);

    juce::Reverb::Parameters rp;
    rp.roomSize = 0.3f + 0.65f * get ("room");
    rp.damping = 0.45f;
    rp.wetLevel = 0.45f * get ("reverb");
    rp.dryLevel = 1.0f - 0.25f * get ("reverb");
    rp.width = 1.0f;
    reverb.setParameters (rp);
    if (buffer.getNumChannels() >= 2)
        reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());

    masterSmoothed.setTargetValue (juce::Decibels::decibelsToGain (get ("volume"), -47.9f));
    masterSmoothed.applyGain (buffer, buffer.getNumSamples());
    meterLevel = buffer.getMagnitude (0, buffer.getNumSamples());
    voicesPlaying = engine.activeVoiceCount();
}

void AtelierProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, destData);
}

void AtelierProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AtelierProcessor(); }
