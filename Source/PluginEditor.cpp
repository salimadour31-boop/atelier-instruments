// Atelier Instruments - Copyright (c) 2026 Amirouche Amwanes - AGPL-3.0-or-later
#include "PluginEditor.h"

using namespace juce;

static Colour bg() { return Colour (0xff1d1a17); }
static Colour panel() { return Colour (0xff2a2520); }
static Colour cream() { return Colour (0xfff1e6d2); }

AtelierLookAndFeel::AtelierLookAndFeel()
{
    accent = atelier::thisInstrument == atelier::Piano ? Colour (0xffd9a441)
           : atelier::thisInstrument == atelier::Drums ? Colour (0xffd2643c) : Colour (0xffb98a5a);
    setColour (Slider::textBoxTextColourId, cream());
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (Label::textColourId, cream());
    setColour (TextButton::buttonColourId, panel().brighter (0.15f));
    setColour (TextButton::buttonOnColourId, accent);
    setColour (TextButton::textColourOffId, cream());
}

void AtelierLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float start, float end, Slider&)
{
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6);
    auto r = jmin (b.getWidth(), b.getHeight()) * 0.5f;
    auto c = b.getCentre();
    auto ang = start + pos * (end - start);
    // body
    g.setGradientFill (ColourGradient (Colour (0xff4a4038), c.x, c.y - r, Colour (0xff221e1a), c.x, c.y + r, false));
    g.fillEllipse (c.x - r * 0.78f, c.y - r * 0.78f, r * 1.56f, r * 1.56f);
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawEllipse (c.x - r * 0.78f, c.y - r * 0.78f, r * 1.56f, r * 1.56f, 1.5f);
    // track + value arc
    Path track, val;
    track.addCentredArc (c.x, c.y, r * 0.93f, r * 0.93f, 0, start, end, true);
    val.addCentredArc (c.x, c.y, r * 0.93f, r * 0.93f, 0, start, ang, true);
    g.setColour (Colours::white.withAlpha (0.08f));
    g.strokePath (track, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (val, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
    // pointer
    Path p;
    p.addRoundedRectangle (-1.5f, -r * 0.7f, 3.0f, r * 0.35f, 1.5f);
    g.setColour (cream());
    g.fillPath (p, AffineTransform::rotation (ang).translated (c.x, c.y));
}

void AtelierLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider::SliderStyle, Slider&)
{
    auto cx = (float) x + (float) w * 0.5f;
    g.setColour (Colours::black.withAlpha (0.45f));
    g.fillRoundedRectangle (cx - 3, (float) y, 6, (float) h, 3);
    g.setColour (accent.withAlpha (0.8f));
    g.fillRoundedRectangle (cx - 3, pos, 6, (float) (y + h) - pos, 3);
    g.setGradientFill (ColourGradient (Colour (0xffe8dcc8), cx, pos - 8, Colour (0xff9b8f7d), cx, pos + 8, false));
    g.fillRoundedRectangle (cx - 16, pos - 7, 32, 14, 3);
    g.setColour (Colours::black.withAlpha (0.6f));
    g.drawLine (cx - 12, pos, cx + 12, pos, 1.5f);
}

//==============================================================================
AtelierEditor::AtelierEditor (AtelierProcessor& p) : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addKnob (knobs, "volume", "Volume", false);
    addKnob (knobs, "tune", "Accord", false);
    addKnob (knobs, "velsens", "Velocite", false);
    addKnob (knobs, "reverb", "Reverb", false);
    addKnob (knobs, "room", "Salle", false);

    if (atelier::thisInstrument == atelier::Drums)
    {
        auto names = atelier::drumBusNames();
        for (int i = 0; i < names.size(); ++i) addKnob (faders, "bus" + String (i), names[i], true);
        struct Pad { const char* name; int note; };
        const Pad list[] = { { "Grosse caisse", 36 }, { "Caisse claire", 38 }, { "Rimshot", 40 }, { "Cross-stick", 37 },
                             { "Charley ferme", 42 }, { "Charley pied", 44 }, { "Charley ouvert", 46 }, { "Crash", 49 },
                             { "Tom aigu", 50 }, { "Tom medium", 47 }, { "Tom grave", 45 }, { "Tom basse", 41 },
                             { "Ride", 51 }, { "Ride cloche", 53 }, { "Splash", 55 }, { "Crash 2", 57 } };
        for (auto& pd : list)
        {
            auto* b = pads.add (new TextButton (pd.name));
            b->setTooltip ("Note MIDI " + String (pd.note));
            auto note = pd.note;
            b->onStateChange = [this, b, note] {
                if (b->isDown()) { if (! proc.keyboardState.isNoteOn (10, note)) proc.keyboardState.noteOn (10, note, 0.85f); }
                else if (proc.keyboardState.isNoteOn (10, note)) proc.keyboardState.noteOff (10, note, 0.0f);
            };
            addAndMakeVisible (b);
        }
    }
    else
    {
        addKnob (knobs, "attack", "Attaque", false);
        addKnob (knobs, "decay", "Declin", false);
        addKnob (knobs, "sustain", "Maintien", false);
        addKnob (knobs, "release", "Relachement", false);
        keyboard = std::make_unique<MidiKeyboardComponent> (proc.keyboardState, MidiKeyboardComponent::horizontalKeyboard);
        keyboard->setAvailableRange (atelier::thisInstrument == atelier::Piano ? 21 : 24, atelier::thisInstrument == atelier::Piano ? 108 : 103);
        keyboard->setLowestVisibleKey (36);
        keyboard->setKeyWidth (17.0f);
        keyboard->setColour (MidiKeyboardComponent::keyDownOverlayColourId, lnf.accent.withAlpha (0.7f));
        keyboard->setColour (MidiKeyboardComponent::mouseOverKeyOverlayColourId, lnf.accent.withAlpha (0.3f));
        addAndMakeVisible (*keyboard);
    }
    status.setFont (FontOptions (13.0f));
    status.setColour (Label::textColourId, cream().withAlpha (0.7f));
    addAndMakeVisible (status);
    setSize (880, atelier::thisInstrument == atelier::Drums ? 520 : 400);
    startTimerHz (20);
}

AtelierEditor::~AtelierEditor() { setLookAndFeel (nullptr); }

void AtelierEditor::addKnob (std::vector<Knob>& list, const String& id, const String& text, bool vertical)
{
    Knob k;
    k.slider = std::make_unique<Slider> (vertical ? Slider::LinearVertical : Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
    k.slider->setTextBoxStyle (Slider::TextBoxBelow, false, 76, 18);
    k.slider->setNumDecimalPlacesToDisplay (2);
    k.label = std::make_unique<Label> (String(), text);
    k.label->setJustificationType (Justification::centred);
    k.label->setFont (FontOptions (13.0f, Font::bold));
    k.attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *k.slider);
    addAndMakeVisible (*k.slider);
    addAndMakeVisible (*k.label);
    list.push_back (std::move (k));
}

void AtelierEditor::timerCallback()
{
    status.setText (proc.getStatus() + "   |   voix : " + String (proc.voicesPlaying.load()), dontSendNotification);
    meter = jmax (proc.meterLevel.load(), meter * 0.85f);
    repaint (getWidth() - 40, 18, 24, 50);
}

void AtelierEditor::paint (Graphics& g)
{
    g.fillAll (bg());
    auto top = getLocalBounds().removeFromTop (78).toFloat();
    g.setGradientFill (ColourGradient (Colour (0xff3b2f25), 0, 0, bg(), 0, top.getBottom(), false));
    g.fillRect (top);
    g.setColour (lnf.accent);
    g.fillRect (0.0f, top.getBottom() - 2, (float) getWidth(), 2.0f);

    const char* title = atelier::thisInstrument == atelier::Piano ? "ATELIER PIANO"
                      : atelier::thisInstrument == atelier::Drums ? "ATELIER BATTERIE" : "ATELIER CORDES";
    const char* sub = atelier::thisInstrument == atelier::Piano ? "Piano a queue acoustique  -  echantillons Salamander (CC-BY 3.0)"
                    : atelier::thisInstrument == atelier::Drums ? "Batterie acoustique General MIDI  -  echantillons Virtuosity Drums (CC0)"
                                                                : "Ensemble de cordes  -  echantillons VSCO 2 CE (CC0)";
    g.setColour (cream());
    g.setFont (FontOptions (30.0f, Font::bold));
    g.drawText (title, 24, 12, 600, 36, Justification::left);
    g.setColour (cream().withAlpha (0.65f));
    g.setFont (FontOptions (13.5f));
    g.drawText (sub, 26, 48, 700, 20, Justification::left);
    g.drawText ("v" JucePlugin_VersionString "  -  libre & open source", getWidth() - 330, 48, 270, 20, Justification::right);

    // panels
    g.setColour (panel());
    g.fillRoundedRectangle (16, 92, (float) getWidth() - 32, 150, 8);
    if (atelier::thisInstrument == atelier::Drums)
    {
        g.fillRoundedRectangle (16, 254, 300, (float) getHeight() - 290, 8);
        g.fillRoundedRectangle (328, 254, (float) getWidth() - 344, (float) getHeight() - 290, 8);
        g.setColour (cream().withAlpha (0.5f));
        g.setFont (FontOptions (12.0f, Font::bold));
        g.drawText ("MIXEUR PAR ELEMENT", 28, 258, 280, 16, Justification::left);
        g.drawText ("PADS (mapping General MIDI)", 340, 258, 300, 16, Justification::left);
    }
    // meter
    auto h = jlimit (0.0f, 1.0f, meter) * 46.0f;
    g.setColour (Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle ((float) getWidth() - 34, 20, 10, 46, 3);
    g.setColour (meter > 0.98f ? Colours::red : lnf.accent);
    g.fillRoundedRectangle ((float) getWidth() - 34, 66 - h, 10, h, 3);
}

void AtelierEditor::resized()
{
    auto area = Rectangle<int> (24, 100, getWidth() - 48, 136);
    auto w = area.getWidth() / jmax (1, (int) knobs.size());
    for (auto& k : knobs)
    {
        auto cell = area.removeFromLeft (w);
        k.label->setBounds (cell.removeFromTop (18));
        k.slider->setBounds (cell);
    }
    status.setBounds (20, getHeight() - 30, getWidth() - 40, 24);
    if (keyboard != nullptr) keyboard->setBounds (16, 254, getWidth() - 32, getHeight() - 290);

    if (! faders.empty())
    {
        auto fa = Rectangle<int> (24, 276, 284, getHeight() - 316);
        auto fw = fa.getWidth() / (int) faders.size();
        for (auto& f : faders)
        {
            auto cell = fa.removeFromLeft (fw);
            f.label->setFont (FontOptions (10.5f, Font::bold));
            f.label->setBounds (cell.removeFromBottom (16));
            f.slider->setTextBoxStyle (Slider::TextBoxBelow, false, fw, 16);
            f.slider->setBounds (cell);
        }
        auto pa = Rectangle<int> (336, 278, getWidth() - 360, getHeight() - 320);
        int cols = 4, rows = (pads.size() + cols - 1) / cols;
        auto pw = pa.getWidth() / cols, ph = pa.getHeight() / rows;
        for (int i = 0; i < pads.size(); ++i)
            pads[i]->setBounds (pa.getX() + (i % cols) * pw + 3, pa.getY() + (i / cols) * ph + 3, pw - 6, ph - 6);
    }
}

AudioProcessorEditor* AtelierProcessor::createEditor() { return new AtelierEditor (*this); }
