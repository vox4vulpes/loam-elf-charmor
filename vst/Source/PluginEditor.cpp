#include "PluginEditor.h"

using namespace LoamColours;
using namespace charmor;

namespace
{
    // The panel at the web page's size; the window scales it.
    constexpr int panelWidth = 900, panelHeight = 1130;
    constexpr float defaultScale = 0.82f;
    constexpr int pad = 20, gap = 12, knobW = 52, knobH = 66, titleH = 26;
    constexpr int pageY = 364, pageH = 594, keybedY = 970, keybedH = 140;

    juce::Font sansFont (float height, bool bold = false)
    {
        return juce::Font (juce::FontOptions (height, bold ? juce::Font::bold : juce::Font::plain));
    }
    juce::Font monoFont (float height)
    {
        return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), height, juce::Font::plain));
    }
    juce::String utf8 (const char* text) { return juce::String (juce::CharPointer_UTF8 (text)); }
    juce::String quoted (const juce::String& s) { return utf8 ("\xe2\x80\x9c") + s + utf8 ("\xe2\x80\x9d"); }

    constexpr const char* egretSvg = R"svg(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" width="24" height="24"> <path d="M14.5 4.2c1.6-.3 2.8.6 2.8 1.8 0 .9-.6 1.5-1.4 1.7-.4 1.7-.2 3.4.6 4.8 1 1.8.5 4-1.6 5.2-1.9 1.1-4.6 1-6.4-.4-1.3-1-1.9-2.4-1.7-3.6 2.2.8 4.1.4 5.4-.9.9-1 .9-2.4.3-3.8-.6-1.4-.4-3.2 2-4.8z" fill="#fbf8ef" stroke="#2a241d" stroke-width=".8" stroke-linejoin="round"/> <path d="M17.2 5.4l4.6 1.1-4.4.6z" fill="#2a241d"/> <circle cx="15.6" cy="5.2" r=".45" fill="#2a241d"/> <path d="M14.2 4.4c-1.2-.6-2-.3-2.6.4" fill="none" stroke="#2a241d" stroke-width=".5"/> <path d="M10.8 17.6l-.6 4.2M12.6 17.7l.4 4.1" stroke="#2a241d" stroke-width=".9" stroke-linecap="round"/> <path d="M9.3 21.9h1.8M12.3 21.9h1.8" stroke="#e8c32a" stroke-width="1.4" stroke-linecap="round"/> </svg>)svg";
    juce::String subtitleText()  { return utf8 ("\xe5\xb0\x8f\xe9\xb7\xba \xc2\xb7 ADDITIVE SYNTHESIZER"); }
    juce::String egretNameText() { return utf8 ("\xe3\x82\xb3\xe3\x82\xb5\xe3\x82\xae \xc2\xb7 KOSAGI \xc2\xb7 LITTLE EGRET"); }

    // Six columns, as the web grid: x and width of `span` columns starting at `col`.
    juce::Rectangle<int> cols (int col, int span, int y, int h)
    {
        const auto w = (panelWidth - 2 * pad - 5 * gap) / 6.0;
        const auto x = pad + col * (w + gap);
        return { juce::roundToInt (x), y, juce::roundToInt (span * w + (span - 1) * gap), h };
    }
    juce::Rectangle<int> content (juce::Rectangle<int> module) { return module.reduced (12).withTrimmedTop (titleH); }

    // Knobs side by side, centred, at most 8 px apart.
    void layoutKnobs (juce::Rectangle<int> area, const std::vector<juce::Component*>& list)
    {
        const auto n = (int) list.size();
        const auto spacing = n > 1 ? juce::jmin (8, (area.getWidth() - n * knobW) / (n - 1)) : 0;
        auto row = area.withSizeKeepingCentre (n * knobW + (n - 1) * spacing, knobH).withY (area.getY());
        for (auto* c : list)
        {
            c->setBounds (row.removeFromLeft (knobW));
            row.removeFromLeft (spacing);
        }
    }

    juce::String noteName (int note) { return juce::MidiMessage::getMidiNoteName (note, true, true, 4); }
}

//==============================================================================
LoamLookAndFeel::LoamLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, panel);
    setColour (juce::ComboBox::outlineColourId, grooveDark);
    setColour (juce::ComboBox::textColourId, ink);
    setColour (juce::ComboBox::arrowColourId, ink);
    setColour (juce::ComboBox::focusedOutlineColourId, teal);
    setColour (juce::PopupMenu::backgroundColourId, panelRaised);
    setColour (juce::PopupMenu::textColourId, ink);
    setColour (juce::PopupMenu::headerTextColourId, inkSoft);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, mustard);
    setColour (juce::PopupMenu::highlightedTextColourId, panelRaised);
    setColour (juce::TextButton::textColourOffId, ink);
    setColour (juce::TextButton::textColourOnId, panelRaised);
    setColour (juce::TextButton::buttonOnColourId, mustard);
    setColour (juce::TextEditor::backgroundColourId, panel);
    setColour (juce::TextEditor::outlineColourId, grooveDark);
    setColour (juce::TextEditor::focusedOutlineColourId, teal);
    setColour (juce::TextEditor::textColourId, ink);
    setColour (juce::TextEditor::highlightColourId, mustard.withAlpha (0.35f));
    setColour (juce::CaretComponent::caretColourId, ink);
    setColour (juce::Label::textColourId, inkSoft);
    setColour (juce::TooltipWindow::backgroundColourId, panelRaised);
    setColour (juce::TooltipWindow::textColourId, ink);
    setColour (juce::TooltipWindow::outlineColourId, grooveDark);
}

void LoamLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                                        float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto area = juce::Rectangle<int> (x, y, width, height).toFloat();
    const auto radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 2.0f;
    const auto centre = area.getCentre();
    const auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const auto arcRadius = radius - 3.0f;
    const juce::PathStrokeType arcStroke (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

    g.setColour (grooveDark.withAlpha (0.25f));
    g.fillEllipse (juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre));
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour (grooveDark);
    g.strokePath (track, arcStroke);
    juce::Path value;
    value.addCentredArc (centre.x, centre.y, arcRadius, arcRadius, 0.0f, rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (value, arcStroke);

    const auto capRadius = radius * 0.72f;
    const auto cap = juce::Rectangle<float> (capRadius * 2.0f, capRadius * 2.0f).withCentre (centre);
    g.setColour (shadow);
    g.fillEllipse (cap.translated (0.0f, 1.5f));
    // The highlight sits straight up, at 12 o'clock: the knob's 50% position.
    g.setGradientFill (juce::ColourGradient (panelRaised, centre.x, centre.y - capRadius * 0.5f,
                                             groove, centre.x, centre.y + capRadius * 1.3f, true));
    g.fillEllipse (cap);
    juce::Path pointer;
    pointer.addRoundedRectangle (-1.5f, -capRadius * 0.85f, 3.0f, capRadius * 0.55f, 1.5f);
    g.setColour (accent);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre));
}

void LoamLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool isMouseOver, bool isDown)
{
    const auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
    const auto on = button.getToggleState();
    const auto accent = button.findColour (juce::TextButton::buttonOnColourId);
    const auto alpha = button.isEnabled() ? 1.0f : 0.45f;
    auto fill = on ? accent : panel;
    if (! on && button.isEnabled() && (isMouseOver || isDown))
        fill = fill.brighter (0.15f);
    g.setColour (fill.withMultipliedAlpha (alpha));
    g.fillRoundedRectangle (bounds, 6.0f);
    g.setColour ((on ? accent : grooveDark).withMultipliedAlpha (alpha));
    g.drawRoundedRectangle (bounds, 6.0f, 1.0f);
}

void LoamLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool)
{
    // Slide switch, like the web effect units'.
    const auto track = button.getLocalBounds().toFloat().withSizeKeepingCentre (32.0f, 18.0f);
    const auto on = button.getToggleState();
    g.setColour (on ? button.findColour (juce::ToggleButton::tickColourId) : grooveDark);
    g.fillRoundedRectangle (track, 9.0f);
    const auto thumb = juce::Rectangle<float> (14.0f, 14.0f).withCentre ({ on ? track.getRight() - 9.0f : track.getX() + 9.0f, track.getCentreY() });
    g.setColour (shadow);
    g.fillEllipse (thumb.translated (0.0f, 1.0f));
    g.setColour (panelRaised);
    g.fillEllipse (thumb);
}

juce::Font LoamLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return sansFont (11.0f, true); }
juce::Font LoamLookAndFeel::getComboBoxFont (juce::ComboBox&) { return sansFont (11.5f, true); }
void LoamLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    LookAndFeel_V4::positionComboBoxText (box, label);
    label.setMinimumHorizontalScale (0.6f);
}

//==============================================================================
CharmorKnob::CharmorKnob (LoamCharmorProcessor& p, const juce::String& id, juce::Colour accent)
    : processor (p), paramId (id), def (LoamCharmorProcessor::findKnob (id)), param (p.apvts.getParameter (id)),
      attachment (p.apvts, id, slider)
{
    addChildComponent (modButton);
    modButton.onClick = [this] { cycleBinding(); };
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
    slider.setMouseDragSensitivity (150);   // the web knob: 150 px for the whole range
    slider.setWantsKeyboardFocus (false);
    slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    slider.onValueChange = [this] { updateReadout(); };
    addAndMakeVisible (slider);

    label.setText (juce::String (def->label).toUpperCase(), juce::dontSendNotification);
    label.setFont (sansFont (9.0f, true));
    readout.setFont (monoFont (10.0f));
    for (auto* l : { &label, &readout })
    {
        l->setJustificationType (juce::Justification::centred);
        l->setBorderSize ({});
        addAndMakeVisible (*l);
    }
    label.setInterceptsMouseClicks (false, false);
    readout.setMouseCursor (juce::MouseCursor::IBeamCursor);
    readout.setTooltip ("Click to type a value");
    readout.addMouseListener (this, false);
    updateReadout();
}

juce::String CharmorKnob::format (double v) const
{
    switch (def->unit)
    {
        case Unit::hz:        return v >= 1000 ? juce::String (v / 1000, 1) + "k" : juce::String (juce::roundToInt (v)) + "Hz";
        case Unit::plusMinus: return (v > 0.5 ? "+" : "") + juce::String (juce::roundToInt (v));
        case Unit::semitones: return (v > 0.5 ? "+" : "") + juce::String (juce::roundToInt (v)) + "st";
        case Unit::hzFine:    return juce::String (v, v < 1 ? 2 : 1) + "Hz";
        case Unit::perOct:    return juce::String (v, v < 2 ? 2 : 1);
        case Unit::seconds:   return juce::String (v, 1) + "s";
        case Unit::dB:        return juce::String (juce::roundToInt (v)) + "dB";
        case Unit::ratio:     return juce::String (v, v < 10 ? 1 : 0) + ":1";
        case Unit::ms:        return juce::String (juce::roundToInt (v)) + "ms";
        case Unit::cents:     return juce::String (juce::roundToInt (v)) + utf8 ("\xc2\xa2");
        case Unit::pct:       return juce::String (juce::roundToInt (v)) + "%";
        case Unit::none:      break;
    }
    return juce::String (juce::roundToInt (v));
}

void CharmorKnob::updateReadout()
{
    const auto v = modded ? (double) param->convertFrom0to1 (shownHeard) : slider.getValue();
    readout.setText (format (v), juce::dontSendNotification);
}

juce::Rectangle<int> CharmorKnob::dialBounds() const { return getLocalBounds().withTrimmedBottom (26).reduced (1); }

void CharmorKnob::resized()
{
    auto area = getLocalBounds();
    readout.setBounds (area.removeFromBottom (13));
    label.setBounds (area.removeFromBottom (13));
    slider.setBounds (area.reduced (1));
    modButton.setBounds (getLocalBounds().removeFromTop (14).removeFromRight (14));
    if (typing != nullptr)
        typing->setBounds (readout.getBounds().expanded (2, 1));
}

void CharmorKnob::ModButton::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat().reduced (1.5f);
    const auto fill = state == 1 ? modGreen : state == 2 ? modRed : panel;
    g.setColour (fill);
    g.fillEllipse (r);
    g.setColour (state == 0 ? grooveDark : fill);
    g.drawEllipse (r, 1.5f);
}

// Off -> on (red where the knob is, green at the far end) -> reversed (dots swapped) -> off.
void CharmorKnob::cycleBinding()
{
    auto b = processor.getBinding (paramId);
    if (! b.bound) { const auto now = param->getValue(); b = { true, false, now, now < 0.95f ? 1.0f : 0.0f }; }
    else if (! b.reversed) b = { true, true, b.green, b.red };
    else b = {};
    processor.setBinding (paramId, b);
    shownHeard = -1.0f;
    refresh();
}

void CharmorKnob::refresh()
{
    const auto custom = processor.isCustomActive();
    const auto b = processor.getBinding (paramId);
    const auto nowModded = custom && b.bound;
    modButton.setVisible (custom);
    const auto state = ! b.bound ? 0 : (b.reversed ? 2 : 1);
    if (state != modButton.state) { modButton.state = state; modButton.repaint(); }

    // While the wheel drives it, the dial shows where it's heard; its own value stays underneath.
    slider.setVisible (! nowModded);
    const auto heard = processor.getHeardNormalised (paramId);
    if (nowModded != modded || nowModded != customShown || heard != shownHeard)
    {
        modded = nowModded;
        customShown = nowModded;
        shownHeard = heard;
        updateReadout();
        repaint();
    }
}

juce::Point<float> CharmorKnob::dotPosition (float normalised) const
{
    const auto area = dialBounds().toFloat();
    const auto radius = juce::jmin (area.getWidth(), area.getHeight()) * 0.5f - 5.0f;
    const auto angle = juce::MathConstants<float>::pi * (1.25f + 1.5f * normalised);
    return area.getCentre() + juce::Point<float> (std::sin (angle), -std::cos (angle)) * radius;
}

void CharmorKnob::paint (juce::Graphics& g)
{
    if (! modded)
        return;
    const auto area = dialBounds();
    getLookAndFeel().drawRotarySlider (g, area.getX(), area.getY(), area.getWidth(), area.getHeight(), shownHeard,
                                       juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, slider);
}

void CharmorKnob::paintOverChildren (juce::Graphics& g)
{
    const auto b = processor.getBinding (paramId);
    if (! (processor.isCustomActive() && b.bound))
        return;
    for (const auto& [norm, colour] : { std::pair { b.red, modRed }, std::pair { b.green, modGreen } })
    {
        const auto dot = juce::Rectangle<float> (9.0f, 9.0f).withCentre (dotPosition (norm));
        g.setColour (shadow);
        g.fillEllipse (dot.translated (0.0f, 1.0f));
        g.setColour (colour);
        g.fillEllipse (dot);
        g.setColour (panelRaised);
        g.drawEllipse (dot, 1.5f);
    }
}

void CharmorKnob::mouseDown (const juce::MouseEvent& e)
{
    draggingDot = 0;
    if (e.eventComponent == &readout)
        return;
    if (e.eventComponent != this || ! modded)
        return;
    const auto b = processor.getBinding (paramId);
    const auto dRed = e.position.getDistanceFrom (dotPosition (b.red)), dGreen = e.position.getDistanceFrom (dotPosition (b.green));
    if (juce::jmin (dRed, dGreen) <= 9.0f)
        draggingDot = dRed <= dGreen ? 1 : 2;
}

void CharmorKnob::mouseDrag (const juce::MouseEvent& e)
{
    if (e.eventComponent != this || draggingDot == 0)
        return;
    const auto centre = dialBounds().toFloat().getCentre();
    auto deg = juce::radiansToDegrees (std::atan2 (e.position.x - centre.x, centre.y - e.position.y));
    if (deg > 135.0f)       deg = deg > 157.5f ? -135.0f : 135.0f;   // the gap at the bottom: snap to the nearer end
    else if (deg < -135.0f) deg = deg < -157.5f ? 135.0f : -135.0f;
    auto b = processor.getBinding (paramId);
    (draggingDot == 1 ? b.red : b.green) = juce::jlimit (0.0f, 1.0f, (deg + 135.0f) / 270.0f);
    processor.setBinding (paramId, b);
    refresh();
    repaint();
}

void CharmorKnob::mouseUp (const juce::MouseEvent& e)
{
    draggingDot = 0;
    if (e.eventComponent == &readout && readout.contains (e.getEventRelativeTo (&readout).getPosition()))
        startTyping();
}

// A typed value: a number with an optional unit -- 4.5k, 4500, 250ms, 1.5s, +7, 30% --
// clamped to the knob's range. Enter or clicking away sets it, Escape keeps the old one.
void CharmorKnob::startTyping()
{
    if (typing != nullptr)
        return;
    typing = std::make_unique<juce::TextEditor>();
    typing->setFont (monoFont (10.0f));
    typing->setJustification (juce::Justification::centred);
    typing->setIndents (1, 0);
    typing->setColour (juce::TextEditor::outlineColourId, teal);
    typing->setText (juce::String (std::round (slider.getValue() * 100) / 100), false);
    addAndMakeVisible (*typing);
    resized();
    typing->grabKeyboardFocus();
    typing->selectAll();

    auto finish = [this] (bool apply)
    {
        if (typing == nullptr || typing->getProperties().contains ("done"))
            return;
        typing->getProperties().set ("done", true);   // Escape, then the focus going: only the first counts
        auto text = typing->getText().trim().toLowerCase().replaceCharacter (',', '.');
        juce::MessageManager::callAsync ([safe = juce::Component::SafePointer<CharmorKnob> (this)] { if (safe != nullptr) safe->typing.reset(); });
        if (! apply)
            return;
        const auto number = text.initialSectionContainingOnly ("+-.0123456789");
        const auto unit = text.substring (number.length()).trim();
        if (! number.containsAnyOf ("0123456789"))
            return;
        static const juce::StringArray units { "", "k", "khz", "hz", "ms", "s", "%", "st", "db", ":1", "c", utf8 ("\xc2\xa2"), "/oct" };
        if (! units.contains (unit))
            return;
        auto v = number.getDoubleValue();
        if (unit == "k" || unit == "khz") v *= 1000;
        if (def->unit == Unit::ms && unit == "s") v *= 1000;
        if (def->unit == Unit::seconds && unit == "ms") v /= 1000;
        if (def->step > 0) v = std::round (v / def->step) * def->step;
        v = juce::jlimit (def->min, def->max, v);
        param->beginChangeGesture();
        param->setValueNotifyingHost (param->convertTo0to1 ((float) v));
        param->endChangeGesture();
    };
    typing->onReturnKey = [finish] { finish (true); };
    typing->onEscapeKey = [finish] { finish (false); };
    typing->onFocusLost = [finish] { finish (true); };
}

//==============================================================================
ChoiceButtons::ChoiceButtons (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, juce::Colour accent)
{
    auto* p = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id));
    for (int i = 0; i < p->choices.size(); ++i)
    {
        auto* b = buttons.add (new juce::TextButton (p->choices[i]));
        b->setColour (juce::TextButton::buttonOnColourId, accent);
        b->setWantsKeyboardFocus (false);
        b->onClick = [this, i] { attachment->setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    attachment = std::make_unique<juce::ParameterAttachment> (*p, [this] (float v)
    {
        for (int i = 0; i < buttons.size(); ++i)
            buttons[i]->setToggleState (i == juce::roundToInt (v), juce::dontSendNotification);
    });
    attachment->sendInitialUpdate();
}

void ChoiceButtons::resized()
{
    auto area = getLocalBounds();
    const auto w = (area.getWidth() - (buttons.size() - 1) * 4) / (float) buttons.size();
    for (int i = 0; i < buttons.size(); ++i)
        buttons[i]->setBounds (juce::roundToInt (i * (w + 4)), 0, juce::roundToInt (w), area.getHeight());
}

//==============================================================================
namespace
{
    constexpr double fMin = 20, fMax = 20000, dbRange = 72;
    float xOf (double f, float w) { return (float) (std::log (f / fMin) / std::log (fMax / fMin)) * w; }
    float yOf (double a, float h)
    {
        const auto db = 20 * std::log10 (std::fmax (1e-9, a));
        return h * (1.0f - (float) std::fmax (0.0, (db + dbRange) / (dbRange + 12)));
    }
}

void SpectrumView::refresh()
{
    const auto s = processor.getSnapshot();
    if (s.note >= 0 && s.voices > 0)
    {
        snap = s;
        live = true;
        quietTicks = 0;
        caption = "LIVE " + utf8 ("\xc2\xb7 ") + noteName (s.note) + utf8 (" \xc2\xb7 ") + juce::String (s.count)
                  + (s.stretch != 0 ? " PARTIALS" : " HARMONICS");
    }
    else if (++quietTicks > 10)
    {
        live = false;
        caption = "PREVIEW " + utf8 ("\xc2\xb7") + " C4";
    }
    repaint();
}

void SpectrumView::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto W = bounds.getWidth(), H = bounds.getHeight();
    g.setColour (readoutBg);
    g.fillRoundedRectangle (bounds, 6.0f);

    g.setColour (readoutInk.withAlpha (0.08f));
    for (double f = 32.7; f < fMax; f *= 2)
        g.drawVerticalLine (juce::roundToInt (xOf (f, W)), 0.0f, H);

    const auto p = processor.heardParams();
    double f0 = 261.63, cutoff, stretch, lfo = 0;
    std::vector<double> amps;
    if (live)
    {
        f0 = snap.f0; cutoff = snap.cutoff; stretch = snap.stretch; lfo = snap.lfo;
        amps.assign (snap.amps.begin(), snap.amps.begin() + snap.count + 1);
    }
    else
    {
        // Idle: what C4 would sound like right now, at the top of its envelopes.
        cutoff = effectiveCutoff (p, f0, p.fS / 100);
        stretch = prismStretch (p, p.fS / 100);
        int K = 1;
        while (K < maxPartials && f0 * partialRatio (K + 1, stretch) < 20000) ++K;
        Amps out {}, scratch {}, col {};
        const Amps* img = nullptr;
        if (auto image = processor.getImage(); image != nullptr && p.imgAmount > 0)
        {
            for (int k = 1; k <= maxPartials; ++k) col[(size_t) k] = image->amps[(size_t) (k - 1) * (size_t) image->cols];
            img = &col;
        }
        voiceAmps (p, f0, K, cutoff, stretch, 0, 0, out, scratch, img);
        amps.assign (out.begin(), out.begin() + K + 1);
    }

    double peak = 0;
    for (size_t k = 1; k < amps.size(); ++k) peak = std::fmax (peak, std::abs (amps[k]));
    const auto scale = peak > 0 ? 1 / peak : 1.0;
    for (size_t k = 1; k < amps.size(); ++k)
    {
        const auto a = std::abs (amps[k]) * scale;
        if (a < 1e-5) continue;
        const auto x = xOf (f0 * partialRatio ((int) k, stretch), W), y = yOf (a, H);
        g.setColour (readoutInk.withAlpha (0.25f));
        g.fillRect (x - 2.5f, y, 5.0f, H - y);   // the glow
        g.setColour (readoutInk);
        g.fillRect (x - 1.0f, y, 2.0f, H - y);
    }

    // the shaping curve: the filter, with the phaser's notches
    juce::Path curve;
    for (float px = 0; px <= W; px += 2)
    {
        const auto f = fMin * std::pow (fMax / fMin, px / W);
        const auto y = yOf (filterGain (p.ftype, f, cutoff, p.res) * phaserGain (p, f, lfo) / 4, H);
        if (px == 0) curve.startNewSubPath (px, y); else curve.lineTo (px, y);
    }
    g.setColour (screenTeal);
    g.strokePath (curve, juce::PathStrokeType (1.5f));
}

//==============================================================================
namespace
{
    double rndHeld (long n) { const auto x = std::sin ((n + 1) * 78.233) * 43758.5453; return (x - std::floor (x)) * 2 - 1; }

    void dot (juce::Graphics& g, float x, float y, juce::Colour c)
    {
        const auto r = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ x, y });
        g.setColour (juce::Colour (0xfffbf8ef));
        g.fillEllipse (r);
        g.setColour (c);
        g.drawEllipse (r, 1.5f);
    }

    void graphLabel (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, juce::Justification j)
    {
        g.setColour (readoutInk.withAlpha (0.55f));
        g.setFont (monoFont (9.0f));
        g.drawText (text, r, j, false);
    }
}

void GraphView::refresh (const CharmorEngine::Snapshot& s, bool isLive)
{
    snap = s;
    live = isLive;
    repaint();
}

void GraphView::paint (juce::Graphics& g)
{
    g.setColour (readoutBg);
    g.fillRoundedRectangle (getLocalBounds().toFloat(), 6.0f);
    const auto p = processor.heardParams();
    if (kind == lfo1 || kind == lfo2) paintLfo (g, p); else paintEnv (g, p);
}

void GraphView::paintLfo (juce::Graphics& g, const Params& p)
{
    const auto W = (float) getWidth(), H = (float) getHeight();
    const auto first = kind == lfo1;
    const auto rate = first ? p.l1Rate : p.l2Rate, delay = (first ? p.l1Delay : p.l2Delay) / 1000;
    const auto shape = first ? p.l1Shape : p.l2Shape;
    const auto retrig = first ? p.l1Retrig : p.l2Retrig;
    const auto T = std::fmin (20.0, std::fmax (2 / rate, delay + 1.5 / rate));
    const auto mid = H / 2, amp = H / 2 - 6;
    auto at = [&] (double t)
    {
        const auto fade = delay > 0 ? std::fmin (1.0, t / delay) : 1.0;
        const auto cyc = t * rate;
        return lfoShape (shape, cyc - std::floor (cyc), rndHeld ((long) std::floor (cyc))) * fade;
    };

    g.setColour (readoutInk.withAlpha (0.12f));
    g.drawHorizontalLine (juce::roundToInt (mid), 0.0f, W);
    if (delay > 0)   // the fade-in: the wave grows inside these lines
    {
        const auto fx = (float) std::fmin (W, delay / T * W);
        juce::Path fadeLines;
        fadeLines.startNewSubPath (0, mid); fadeLines.lineTo (fx, mid - amp); fadeLines.lineTo (W, mid - amp);
        fadeLines.startNewSubPath (0, mid); fadeLines.lineTo (fx, mid + amp); fadeLines.lineTo (W, mid + amp);
        juce::Path dashed;
        const float dashes[] = { 3.0f, 3.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, fadeLines, dashes, 2);
        g.setColour (juce::Colour (0x73b7a6d4));
        g.fillPath (dashed);
    }
    juce::Path wave;
    for (float px = 0; px <= W; ++px)
    {
        const auto y = mid - (float) at (px / W * T) * amp;
        if (px == 0) wave.startNewSubPath (px, y); else wave.lineTo (px, y);
    }
    g.setColour (colour);
    g.strokePath (wave, juce::PathStrokeType (1.6f));
    graphLabel (g, juce::String (rate, rate < 1 ? 2 : 1) + " Hz" + (retrig ? utf8 (" \xc2\xb7 RETRIG") : juce::String()),
                { 4, H - 13, W - 8, 11 }, juce::Justification::left);
    graphLabel (g, juce::String (T, T < 10 ? 1 : 0) + " s", { 4, H - 13, W - 8, 11 }, juce::Justification::right);

    if (live)
    {
        const auto ph = snap.lfoPhase[first ? 0 : 1];
        const auto t = retrig ? std::fmod (snap.t, T) : ph / rate;
        dot (g, (float) (t / T) * W, mid - (float) at (retrig ? snap.t : t) * amp, colour);
    }
}

// Each stage gets a width by the square root of its time, so a 2 ms attack and
// a 6 s release both stay visible; sustain gets a fixed stretch.
void GraphView::paintEnv (juce::Graphics& g, const Params& p)
{
    const auto W = (float) getWidth(), H = (float) getHeight();
    double A, D, S, R;
    int which;
    switch (kind)
    {
        case ampEnv:    A = p.aA; D = p.aD; S = p.aS; R = p.aR; which = 0; break;
        case filterEnv: A = p.fA; D = p.fD; S = p.fS; R = p.fR; which = 1; break;
        case env2:      A = p.e2A; D = p.e2D; S = p.e2S; R = p.e2R; which = 2; break;
        default:        A = p.e3A; D = p.e3D; S = p.e3S; R = p.e3R; which = 3; break;
    }
    S /= 100;
    auto wa = std::sqrt (A), wd = std::sqrt (D), ws = std::sqrt (600.0), wr = std::sqrt (R);
    const auto k = W / (wa + wd + ws + wr);
    wa *= k; wd *= k; ws *= k; wr *= k;
    const auto top = 8.0f, bottom = H - 14.0f;
    auto y = [&] (double l) { return bottom - (float) l * (bottom - top); };
    auto level = [&] (double x)
    {
        if (x <= wa) return x / wa;
        x -= wa; if (x <= wd) return S + (1 - S) * std::exp (-4.6 * x / wd);
        x -= wd; if (x <= ws) return S;
        x -= ws; return S * std::exp (-4.6 * x / wr);
    };
    juce::Path line, fill;
    fill.startNewSubPath (0, bottom);
    for (float px = 0; px <= W; ++px)
    {
        const auto yy = y (level (px));
        fill.lineTo (px, yy);
        if (px == 0) line.startNewSubPath (px, yy); else line.lineTo (px, yy);
    }
    fill.lineTo (W, bottom);
    fill.closeSubPath();
    g.setColour (colour.withAlpha (0.18f));
    g.fillPath (fill);
    g.setColour (colour);
    g.strokePath (line, juce::PathStrokeType (1.6f));
    g.setColour (readoutInk.withAlpha (0.12f));
    for (auto x : { wa, wa + wd, wa + wd + ws })
        g.drawVerticalLine (juce::roundToInt (x), top, bottom);
    const char* letters[] = { "A", "D", "S", "R" };
    const double starts[] = { 0, wa, wa + wd, wa + wd + ws }, widths[] = { wa, wd, ws, wr };
    for (int i = 0; i < 4; ++i)
        graphLabel (g, letters[i], { (float) starts[i], H - 12, (float) widths[i], 11 }, juce::Justification::centred);

    if (live)
    {
        const auto lv = snap.envLevel[(size_t) which];
        const auto stage = snap.envStage[(size_t) which];
        double x;
        if (stage == 0) x = lv * wa;
        else if (stage == 1) x = lv - S > 1e-3 ? wa + std::fmin (1.0, -std::log ((lv - S) / std::fmax (1e-6, 1 - S)) / 4.6) * wd : wa + wd + ws / 2;
        else x = wa + wd + ws + std::fmin (1.0, -std::log (std::fmax (1e-6, lv) / std::fmax (1e-3, S)) / 4.6) * wr;
        dot (g, (float) juce::jlimit (0.0, (double) W, x), y (lv), colour);
    }
}

//==============================================================================
void ImageView::rebuild()
{
    const auto img = processor.getImage();
    shown = img.get();
    if (img == nullptr)
    {
        picture = {};
        topRow = imageRows;
        repaint();
        return;
    }
    picture = juce::Image (juce::Image::RGB, img->cols, imageRows, false);
    for (int r = 0; r < imageRows; ++r)
        for (int c = 0; c < img->cols; ++c)
        {
            const auto b = img->bytes[(size_t) (r * img->cols + c)] / 255.0f;
            picture.setPixelAt (c, imageRows - 1 - r, juce::Colour::fromFloatRGBA (0.94f * b, 0.66f * b, 0.19f * b, 1.0f));
        }
    // Show the rows up to the highest harmonic in use (at least 16).
    topRow = 16;
    for (int r = imageRows - 1; r >= 16; --r)
    {
        bool lit = false;
        for (int c = 0; c < img->cols && ! lit; ++c) lit = img->bytes[(size_t) (r * img->cols + c)] > 0;
        if (lit) { topRow = juce::jmin (imageRows, r + 2); break; }
    }
    repaint();
}

void ImageView::refresh (double playPos)
{
    if (processor.getImage().get() != shown)
        rebuild();
    if (playPos != pos)
    {
        pos = playPos;
        repaint();
    }
}

void ImageView::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    g.setColour (readoutBg);
    g.fillRoundedRectangle (r, 6.0f);
    if (picture.isValid())
    {
        g.setImageResamplingQuality (juce::Graphics::lowResamplingQuality);
        g.drawImage (picture, 0, 0, getWidth(), getHeight(), 0, imageRows - topRow, picture.getWidth(), topRow);
        if (pos >= 0)
        {
            g.setColour (screenTeal);
            g.fillRect ((float) pos * r.getWidth() - 1.0f, 0.0f, 2.0f, r.getHeight());
        }
    }
    else
    {
        g.setColour (readoutInk.withAlpha (0.6f));
        g.setFont (monoFont (10.0f).withExtraKerningFactor (0.12f));
        g.drawText ("DROP A PICTURE OR A SOUND HERE", r, juce::Justification::centred, false);
    }
    if (dragOver)
    {
        g.setColour (teal);
        g.drawRoundedRectangle (r.reduced (1.0f), 6.0f, 2.0f);
    }
}

void ImageView::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    repaint();
    if (! files.isEmpty() && onFile)
        onFile (juce::File (files[0]));
}

//==============================================================================
namespace { constexpr const char* keyCaps = "zsxdcvgbhnjmq2w3er5t6y7ui9o0p"; }

LoamKeyboard::LoamKeyboard (juce::MidiKeyboardState& state)
    : MidiKeyboardComponent (state, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    clearKeyMappings();
    for (int i = 0; i < numKeys; ++i)
        setKeyPressForNote (juce::KeyPress (keyCaps[i], 0, 0), i);
    setScrollButtonsVisible (false);
    setVelocity (0.9f, false);   // the web keys play at 0.9
    setBaseNote (baseNote);
}

void LoamKeyboard::setBaseNote (int midiNote)
{
    baseNote = midiNote;
    setAvailableRange (baseNote, baseNote + numKeys - 1);
    setLowestVisibleKey (baseNote);
    setKeyPressBaseOctave (baseNote / 12);
    repaint();
}

bool LoamKeyboard::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey)
        return true;
    return MidiKeyboardComponent::keyPressed (key);
}

bool LoamKeyboard::keyStateChanged (bool isKeyDown)
{
    const auto handled = MidiKeyboardComponent::keyStateChanged (isKeyDown);
    const auto space = juce::KeyPress::isKeyCurrentlyDown (juce::KeyPress::spaceKey);
    setSpaceDown (space);
    return handled || space;
}

void LoamKeyboard::focusLost (FocusChangeType cause)
{
    MidiKeyboardComponent::focusLost (cause);
    setSpaceDown (false);
}

void LoamKeyboard::setSpaceDown (bool down)
{
    if (down == spaceDown)
        return;
    spaceDown = down;
    if (onSustainKey)
        onSustainKey (down);
}

juce::String LoamKeyboard::capFor (int midiNoteNumber) const
{
    const auto offset = midiNoteNumber - baseNote;
    return juce::isPositiveAndBelow (offset, numKeys) ? juce::String::charToString (keyCaps[offset]).toUpperCase() : juce::String();
}

void LoamKeyboard::drawWhiteNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver, juce::Colour lineColour, juce::Colour)
{
    MidiKeyboardComponent::drawWhiteNote (note, g, area, isDown, isOver, lineColour, juce::Colours::transparentBlack);
    if (note % 12 == 0)
    {
        g.setColour (coral);
        g.setFont (monoFont (9.0f));
        g.drawText (noteName (note), area.withTrimmedTop (area.getHeight() * getBlackNoteLengthProportion() + 4.0f).withHeight (14.0f),
                    juce::Justification::centred, false);
    }
    g.setColour (inkSoft);
    g.setFont (monoFont (10.0f));
    g.drawText (capFor (note), area.removeFromBottom (18.0f), juce::Justification::centred, false);
}

void LoamKeyboard::drawBlackNote (int note, juce::Graphics& g, juce::Rectangle<float> area, bool isDown, bool isOver, juce::Colour fill)
{
    MidiKeyboardComponent::drawBlackNote (note, g, area, isDown, isOver, fill);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.setFont (monoFont (9.0f));
    g.drawText (capFor (note), area.removeFromBottom (16.0f), juce::Justification::centred, false);
}

//==============================================================================
WheelView::WheelView (std::atomic<float>& v, bool isBipolar) : value (v), bipolar (isBipolar)
{
    setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
}

void WheelView::setFromY (float y)
{
    const auto t = juce::jlimit (0.0f, 1.0f, 1.0f - y / (float) getHeight());
    value = bipolar ? t * 2.0f - 1.0f : t;
    refresh();
}

void WheelView::mouseUp (const juce::MouseEvent&)
{
    if (bipolar) { value = 0.0f; refresh(); }   // a pitch wheel springs back to the centre
}

void WheelView::refresh()
{
    if (const auto v = value.load(); v != shown) { shown = v; repaint(); }
}

void WheelView::paint (juce::Graphics& g)
{
    const auto s = getLocalBounds().toFloat();
    g.setColour (ink.withAlpha (0.35f));
    g.fillRoundedRectangle (s, 5.0f);
    const auto face = s.reduced (3.0f);
    g.setGradientFill (juce::ColourGradient (groove, face.getX(), 0.0f, grooveDark, face.getRight(), 0.0f, false));
    g.fillRoundedRectangle (face, 4.0f);
    const auto t = bipolar ? (shown + 1.0f) * 0.5f : shown;
    const auto y = face.getBottom() - t * face.getHeight();
    g.setColour (ink.withAlpha (0.18f));
    for (float ridge = std::fmod (y - face.getY(), 6.0f); ridge < face.getHeight(); ridge += 6.0f)
        g.drawHorizontalLine (juce::roundToInt (face.getY() + ridge), face.getX() + 2.0f, face.getRight() - 2.0f);
    if (bipolar)
    {
        g.setColour (ink.withAlpha (0.35f));
        g.drawHorizontalLine (juce::roundToInt (face.getCentreY()), face.getX(), face.getRight());
    }
    g.setColour (bipolar ? coral : teal);
    g.fillRoundedRectangle ({ face.getX(), y - 2.0f, face.getWidth(), 4.0f }, 2.0f);
    if (centreText.isNotEmpty())
    {
        const auto badge = juce::Rectangle<float> (face.getWidth() - 2.0f, 16.0f).withCentre (face.getCentre());
        g.setColour (panelRaised.withAlpha (0.9f));
        g.fillRoundedRectangle (badge, 3.0f);
        g.setColour (ink);
        g.setFont (monoFont (11.0f));
        g.drawText (centreText, badge, juce::Justification::centred, false);
    }
}

//==============================================================================
EgretView::EgretView()
{
    drawable = juce::Drawable::createFromImageData (egretSvg, std::strlen (egretSvg));
    setMouseCursor (juce::MouseCursor::PointingHandCursor);
    setTitle ("Kosagi, the little egret");
    setTooltip (utf8 ("\xe5\xb0\x8f\xe9\xb7\xba"));
}

void EgretView::paint (juce::Graphics& g)
{
    if (drawable != nullptr)
        drawable->drawWithin (g, getLocalBounds().toFloat(), juce::RectanglePlacement::centred, 1.0f);
}

void PageView::paint (juce::Graphics& g)
{
    for (const auto& m : modules)
    {
        const auto b = m.bounds.toFloat();
        g.setColour (m.accent);
        g.fillRoundedRectangle (b, 12.0f);
        g.setColour (panelRaised);
        g.fillRoundedRectangle (b.withTrimmedTop (3.0f), 12.0f);
        g.setColour (grooveDark.withAlpha (0.6f));
        g.drawRoundedRectangle (b.withTrimmedTop (3.0f), 12.0f, 1.0f);
        g.setColour (m.accent);
        g.setFont (sansFont (14.0f, true).withExtraKerningFactor (0.12f));
        g.drawText (m.title.toUpperCase(), m.bounds.reduced (12, 0).withTrimmedTop (8).withHeight (16), juce::Justification::centredLeft);
    }
    for (const auto& [r, name] : unitTitles)   // the FX units: a recessed card each, its name on top
    {
        g.setColour (panel);
        g.fillRoundedRectangle (r.toFloat(), 10.0f);
        g.setColour (groove);
        g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10.0f, 1.0f);
        g.setColour (inkSoft);
        g.setFont (sansFont (10.0f, true).withExtraKerningFactor (0.08f));
        g.drawText (name.toUpperCase(), r.reduced (10, 0).withTrimmedTop (10).withHeight (18), juce::Justification::centredLeft);
    }
}

//==============================================================================
CharmorKnob& LoamCharmorEditor::knob (const juce::String& id)
{
    auto& k = knobs[id];
    jassert (k != nullptr);
    return *k;
}

LoamCharmorEditor::LoamCharmorEditor (LoamCharmorProcessor& p)
    : AudioProcessorEditor (p), audioProcessor (p), spectrum (p),
      keyboard (p.keyboardState), pitchWheel (p.bendWheel, true), modWheel (p.modWheel, false)
{
    setLookAndFeel (&lnf);
    panel.painter = [this] (juce::Graphics& g) { paintPanel (g); };
    addAndMakeVisible (panel);

    // ---- header
    brandSub = subtitleText();
    egret.onClick = [this, safe = juce::Component::SafePointer<LoamCharmorEditor> (this)]
    {
        brandSub = egretNameText();
        panel.repaint (headerArea);
        audioProcessor.chirp();
        juce::Timer::callAfterDelay (2600, [safe]
        {
            if (safe != nullptr) { safe->brandSub = subtitleText(); safe->panel.repaint (safe->headerArea); }
        });
    };
    panel.addAndMakeVisible (egret);
    panel.addAndMakeVisible (spectrum);

    // ---- presets
    presetBox.onChange = [this]
    {
        const auto id = presetBox.getSelectedId();
        if (id <= 0 || id == 999) return;
        if (id < 100) audioProcessor.loadFactoryPreset (id - 1);
        else if (! audioProcessor.loadUserPreset (userPresets[id - 100])) presetStatus.setText ("Couldn't read that preset", juce::dontSendNotification);
        refreshPresetBox();
    };
    auto step = [this] (int d)
    {
        const auto total = (int) factoryPresets().size() + userPresets.size();
        auto index = presetBox.getSelectedId() >= 100 ? (int) factoryPresets().size() + presetBox.getSelectedId() - 100
                   : presetBox.getSelectedId() >= 1 && presetBox.getSelectedId() < 100 ? presetBox.getSelectedId() - 1 : 0;
        index = ((index + d) % total + total) % total;
        if (index < (int) factoryPresets().size()) audioProcessor.loadFactoryPreset (index);
        else audioProcessor.loadUserPreset (userPresets[index - (int) factoryPresets().size()]);
        refreshPresetBox();
    };
    presetPrev.onClick = [step] { step (-1); };
    presetNext.onClick = [step] { step (1); };
    presetName.setTextToShowWhenEmpty ("Preset name", inkSoft);
    presetName.setInputRestrictions (32);
    presetName.setFont (sansFont (12.0f));
    presetName.setIndents (8, 6);
    presetName.onReturnKey = [this] { savePreset(); };
    presetSave.onClick = [this] { savePreset(); };
    presetDelete.onClick = [this] { deletePreset(); };
    presetStatus.setFont (monoFont (10.5f));
    presetStatus.setJustificationType (juce::Justification::centredRight);
    for (auto* c : std::initializer_list<juce::Component*> { &presetBox, &presetPrev, &presetNext, &presetName, &presetSave, &presetDelete, &presetStatus })
        panel.addAndMakeVisible (*c);
    for (auto* b : { &presetPrev, &presetNext, &presetSave, &presetDelete })
        b->setColour (juce::TextButton::buttonOnColourId, sage);

    // ---- tabs and pages
    const char* tabNames[] = { "SOUND", "MOD", "IMAGE", "FX" };
    for (int i = 0; i < 4; ++i)
    {
        tabs[(size_t) i].setButtonText (tabNames[i]);
        tabs[(size_t) i].setColour (juce::TextButton::buttonOnColourId, coral);
        tabs[(size_t) i].onClick = [this, i] { showPage (i); };
        tabs[(size_t) i].setWantsKeyboardFocus (false);
        panel.addAndMakeVisible (tabs[(size_t) i]);
        panel.addChildComponent (pages[(size_t) i]);
    }

    auto makeKnob = [this] (int pageIndex, const juce::String& id, juce::Colour accent)
    {
        knobs[id] = std::make_unique<CharmorKnob> (audioProcessor, id, accent);
        pages[(size_t) pageIndex].addAndMakeVisible (*knobs[id]);
    };
    // SOUND
    for (auto id : { "morph", "oddEven", "pluck" }) makeKnob (0, id, mustard);
    for (auto id : { "cutoff", "res", "envAmt", "keytrack", "fA", "fD", "fS", "fR" }) makeKnob (0, id, teal);
    for (auto id : { "aA", "aD", "aS", "aR" }) makeKnob (0, id, coral);
    for (auto id : { "uniVoices", "uniDetune", "uniSpread" }) makeKnob (0, id, lavender);
    for (auto id : { "prism", "prismEnv" }) makeKnob (0, id, coral);
    for (auto id : { "phDepth", "phNotches", "phOffset", "phSpeed" }) makeKnob (0, id, teal);
    for (auto id : { "harmVoices", "harmInterval", "harmLevel" }) makeKnob (0, id, sage);
    for (auto id : { "blurTime", "blurHarm" }) makeKnob (0, id, mustard);
    makeKnob (0, "volume", mustard);
    // MOD
    for (auto id : { "l1Rate", "l1Delay", "l2Rate", "l2Delay" }) makeKnob (1, id, lavender);
    for (auto id : { "e2A", "e2D", "e2S", "e2R", "e3A", "e3D", "e3S", "e3R" }) makeKnob (1, id, coral);
    for (int i = 0; i < modSlots; ++i) makeKnob (1, "modAmt" + juce::String (i), sage);
    // IMAGE
    for (auto id : { "imgAmount", "imgSpeed" }) makeKnob (2, id, teal);
    // FX
    for (auto id : { "distDrive", "distTone", "distMix", "choRate", "choDepth", "choMix", "dlyTime", "dlyFb", "dlyMix",
                     "revSize", "reverb", "compThresh", "compRatio", "compGain" })
        makeKnob (3, id, lavender);

    auto choiceRow = [this] (int pageIndex, const char* id, juce::Colour accent)
    {
        auto* row = choiceRows.add (new ChoiceButtons (audioProcessor.apvts, id, accent));
        pages[(size_t) pageIndex].addAndMakeVisible (row);
        return row;
    };
    choiceRow (0, "waveA", mustard); choiceRow (0, "waveB", mustard); choiceRow (0, "ftype", teal);
    choiceRow (1, "l1Shape", lavender); choiceRow (1, "l2Shape", lavender);

    for (auto text : { "A", "B" })
    {
        auto* l = smallLabels.add (new juce::Label ({}, text));
        l->setFont (monoFont (10.0f));
        pages[0].addAndMakeVisible (l);
    }

    auto chip = [this] (int pageIndex, const char* id, const char* text, juce::Colour accent)
    {
        auto* b = toggleChips.add (new juce::TextButton (text));
        b->setClickingTogglesState (true);
        b->setColour (juce::TextButton::buttonOnColourId, accent);
        b->setWantsKeyboardFocus (false);
        buttonAttachments.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (audioProcessor.apvts, id, *b));
        pages[(size_t) pageIndex].addAndMakeVisible (b);
    };
    chip (1, "l1Retrig", "RETRIG", lavender);
    chip (1, "l2Retrig", "RETRIG", lavender);
    chip (2, "imgLoop", "LOOP", teal);

    const GraphView::Kind graphKinds[] = { GraphView::filterEnv, GraphView::ampEnv, GraphView::lfo1, GraphView::lfo2, GraphView::env2, GraphView::env3 };
    const juce::Colour graphColours[] = { screenTeal, juce::Colour (0xffe08a74), juce::Colour (0xffb7a6d4), juce::Colour (0xffb7a6d4),
                                          juce::Colour (0xffe08a74), juce::Colour (0xffe08a74) };
    for (int i = 0; i < 6; ++i)
    {
        auto* gv = graphs.add (new GraphView (audioProcessor, graphKinds[i], graphColours[i]));
        pages[i < 2 ? 0 : 1].addAndMakeVisible (gv);
    }

    // the mod matrix: source -> target and an amount
    for (int i = 0; i < modSlots; ++i)
        for (auto prefix : { "modSrc", "modDst" })
        {
            const auto id = prefix + juce::String (i);
            auto* box = matrixBoxes.add (new juce::ComboBox());
            auto* param = dynamic_cast<juce::AudioParameterChoice*> (audioProcessor.apvts.getParameter (id));
            box->addItemList (param->choices, 1);
            box->setWantsKeyboardFocus (false);
            comboAttachments.add (new juce::AudioProcessorValueTreeState::ComboBoxAttachment (audioProcessor.apvts, id, *box));
            pages[1].addAndMakeVisible (box);
        }
    for (int i = 0; i < modSlots; ++i)
    {
        auto* arrow = smallLabels.add (new juce::Label ({}, utf8 ("\xe2\x86\x92")));
        arrow->setFont (sansFont (12.0f));
        arrow->setJustificationType (juce::Justification::centred);
        pages[1].addAndMakeVisible (arrow);
    }

    // FX switches
    for (auto id : { "fxDist", "fxCho", "fxDly", "fxRev", "fxComp" })
    {
        auto* sw = fxSwitches.add (new juce::ToggleButton());
        sw->setColour (juce::ToggleButton::tickColourId, lavender);
        sw->setWantsKeyboardFocus (false);
        buttonAttachments.add (new juce::AudioProcessorValueTreeState::ButtonAttachment (audioProcessor.apvts, id, *sw));
        pages[3].addAndMakeVisible (sw);
    }

    // IMAGE page
    imageView.onFile = [this] (const juce::File& f) { loadAnyFile (f); };
    pages[2].addAndMakeVisible (imageView);
    auto choose = [this] (bool sound)
    {
        chooser = std::make_unique<juce::FileChooser> (sound ? "Load a sound" : "Load a picture", juce::File(),
                                                       sound ? "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3" : "*.png;*.jpg;*.jpeg;*.gif");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this, sound] (const juce::FileChooser& fc)
                              {
                                  const auto f = fc.getResult();
                                  if (f.existsAsFile()) { if (sound) loadSoundFile (f); else loadPictureFile (f); }
                              });
    };
    loadImage.onClick = [choose] { choose (false); };
    loadSound.onClick = [choose] { choose (true); };
    clearImage.onClick = [this] { audioProcessor.clearImage(); imageStatus.setText ("No image yet.", juce::dontSendNotification); };
    imageStatus.setFont (monoFont (10.5f));
    imageStatus.setColour (juce::Label::textColourId, ink);
    imageHelp.setFont (sansFont (11.0f));
    imageHelp.setJustificationType (juce::Justification::topLeft);
    imageHelp.setText ("A picture plays left to right over TIME; each row from the bottom is one harmonic (1 to 256), and the brighter "
                       "the pixel, the louder. A sound is analysed into the harmonics of its pitch and plays back at any note. "
                       "AMOUNT mixes it with the timbre.", juce::dontSendNotification);
    for (auto* c : std::initializer_list<juce::Component*> { &loadImage, &loadSound, &clearImage, &imageStatus, &imageHelp })
        pages[2].addAndMakeVisible (*c);

    // ---- keyboard and wheels
    keyboard.onSustainKey = [this] (bool down) { audioProcessor.uiSustain = down; };
    octaveDown.onClick = [this] { octaveShift = juce::jmax (-2, octaveShift - 1); keyboard.setBaseNote (48 + 12 * octaveShift); panel.repaint(); };
    octaveUp.onClick = [this] { octaveShift = juce::jmin (2, octaveShift + 1); keyboard.setBaseNote (48 + 12 * octaveShift); panel.repaint(); };
    octaveLabel.setFont (monoFont (10.0f));
    octaveLabel.setColour (juce::Label::textColourId, ink);
    octaveLabel.setJustificationType (juce::Justification::centred);
    modWheelLabel.setText ("MOD", juce::dontSendNotification);
    modWheelLabel.setFont (monoFont (8.5f));
    modWheelLabel.setColour (juce::Label::textColourId, ink);
    modWheelLabel.setJustificationType (juce::Justification::centred);
    for (auto* b : { &bendUp, &bendDown, &modModeButton })
        b->setColour (juce::TextButton::buttonOnColourId, teal);
    modModeButton.setClickingTogglesState (false);
    modModeButton.setTooltip ("Mod wheel: vibrato, filter, custom (knobs you put on it), or only through the mod matrix");

    modModeAttachment = std::make_unique<juce::ParameterAttachment> (*audioProcessor.apvts.getParameter ("modMode"), [this] (float v)
    {
        modMode = juce::roundToInt (v);
        const char* names[] = { "VIB", "FILT", "CUST", "MTRX" };
        modModeButton.setButtonText (names[juce::jlimit (0, 3, modMode)]);
        modModeButton.setToggleState (true, juce::dontSendNotification);
        for (auto& [id, k] : knobs) k->refresh();
    });
    modModeAttachment->sendInitialUpdate();
    modModeButton.onClick = [this] { modModeAttachment->setValueAsCompleteGesture ((float) ((modMode + 1) % 4)); };

    bendRangeAttachment = std::make_unique<juce::ParameterAttachment> (*audioProcessor.apvts.getParameter ("bendRange"), [this] (float v)
    {
        pitchWheel.centreText = juce::String (juce::roundToInt (v));
        pitchWheel.repaint();
    });
    bendRangeAttachment->sendInitialUpdate();
    bendUp.onClick = [this] { bendRangeAttachment->setValueAsCompleteGesture ((float) juce::jmin (12, pitchWheel.centreText.getIntValue() + 1)); };
    bendDown.onClick = [this] { bendRangeAttachment->setValueAsCompleteGesture ((float) juce::jmax (1, pitchWheel.centreText.getIntValue() - 1)); };

    for (auto* c : std::initializer_list<juce::Component*> { &keyboard, &octaveDown, &octaveUp, &octaveLabel, &bendUp, &bendDown,
                                                             &modModeButton, &modWheelLabel, &pitchWheel, &modWheel })
        panel.addAndMakeVisible (*c);
    for (auto* b : { &octaveDown, &octaveUp, &bendUp, &bendDown, &modModeButton })
        b->setWantsKeyboardFocus (false);

    userPresets = audioProcessor.getUserPresetNames();
    refreshPresetBox();
    if (auto img = audioProcessor.getImage())
        imageStatus.setText (audioProcessor.getImageName() + utf8 (" \xc2\xb7 ") + juce::String (img->cols) + " columns", juce::dontSendNotification);
    else
        imageStatus.setText ("No image yet.", juce::dontSendNotification);

    layoutPanel();
    showPage (0);
    setResizable (true, true);
    setResizeLimits (450, 565, 1350, 1695);
    if (auto* c = getConstrainer())
        c->setFixedAspectRatio ((double) panelWidth / panelHeight);
    setSize (juce::roundToInt (panelWidth * defaultScale), juce::roundToInt (panelHeight * defaultScale));
    startTimerHz (30);
}

LoamCharmorEditor::~LoamCharmorEditor()
{
    audioProcessor.uiSustain = false;
    setLookAndFeel (nullptr);
}

void LoamCharmorEditor::resized()
{
    const auto scale = (float) getWidth() / panelWidth;
    panel.setBounds (0, 0, panelWidth, panelHeight);
    panel.setTransform (juce::AffineTransform::scale (scale));
}

void LoamCharmorEditor::showPage (int index)
{
    page = index;
    for (int i = 0; i < 4; ++i)
    {
        tabs[(size_t) i].setToggleState (i == index, juce::dontSendNotification);
        pages[(size_t) i].setVisible (i == index);
    }
}

//==============================================================================
void LoamCharmorEditor::refreshPresetBox()
{
    const auto name = audioProcessor.getPresetName();
    const auto edited = audioProcessor.isPresetEdited();
    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < (int) factoryPresets().size(); ++i)
        presetBox.addItem (factoryPresets()[(size_t) i].name, i + 1);
    if (! userPresets.isEmpty())
    {
        presetBox.addSectionHeading ("Your presets");
        for (int i = 0; i < userPresets.size(); ++i)
            presetBox.addItem (userPresets[i], 100 + i);
    }
    int selected = 999;
    if (! edited)
    {
        for (int i = 0; i < (int) factoryPresets().size(); ++i)
            if (name == factoryPresets()[(size_t) i].name) selected = i + 1;
        if (const auto i = userPresets.indexOf (name, true); i >= 0) selected = 100 + i;
    }
    if (selected == 999)
        presetBox.addItem (name + " (edited)", 999);
    presetBox.setSelectedId (selected, juce::dontSendNotification);
    const auto isUser = userPresets.contains (name, true);
    presetDelete.setEnabled (isUser);
    if (! isUser) { deleteArmed = false; presetDelete.setButtonText ("DELETE"); }
    shownPresetSig = name + (edited ? "*" : "") + juce::String (userPresets.size());
}

void LoamCharmorEditor::savePreset()
{
    const auto current = audioProcessor.getPresetName();
    auto name = presetName.getText().trim();
    if (name.isEmpty())
        name = userPresets.contains (current, true) ? current : "My preset " + juce::String (userPresets.size() + 1);
    for (const auto& f : factoryPresets())
        if (name == f.name) name << " (mine)";
    const auto existed = userPresets.contains (name, true);
    if (! audioProcessor.saveUserPreset (name))
    {
        presetStatus.setText ("Couldn't save " + quoted (name), juce::dontSendNotification);
        return;
    }
    presetName.clear();
    userPresets = audioProcessor.getUserPresetNames();
    refreshPresetBox();
    presetStatus.setText ((existed ? "Updated " : "Saved ") + quoted (name), juce::dontSendNotification);
    keyboard.grabKeyboardFocus();
}

void LoamCharmorEditor::deletePreset()
{
    const auto name = audioProcessor.getPresetName();
    if (! userPresets.contains (name, true))
        return;
    if (! deleteArmed)   // two-step delete: the first click arms the button
    {
        deleteArmed = true;
        presetDelete.setButtonText ("SURE?");
        presetStatus.setText ("Click again to delete " + quoted (name), juce::dontSendNotification);
        return;
    }
    const auto deleted = audioProcessor.deleteUserPreset (name);
    userPresets = audioProcessor.getUserPresetNames();
    if (deleted)
        audioProcessor.loadFactoryPreset (0);
    refreshPresetBox();
    presetStatus.setText ((deleted ? "Deleted " : "Couldn't delete ") + quoted (name), juce::dontSendNotification);
}

//==============================================================================
void LoamCharmorEditor::loadAnyFile (const juce::File& f)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    if (formats.findFormatForFileExtension (f.getFileExtension()) != nullptr)
        loadSoundFile (f);
    else
        loadPictureFile (f);
}

void LoamCharmorEditor::loadPictureFile (const juce::File& f)
{
    const auto source = juce::ImageFileFormat::loadFrom (f);
    if (! source.isValid())
    {
        imageStatus.setText ("Couldn't read that picture.", juce::dontSendNotification);
        return;
    }
    const auto cols = juce::jlimit (2, 256, source.getWidth());
    juce::Image scaled (juce::Image::ARGB, cols, imageRows, true);
    {
        juce::Graphics g (scaled);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (source, 0, 0, cols, imageRows, 0, 0, source.getWidth(), source.getHeight());
    }
    std::vector<unsigned char> bytes ((size_t) cols * imageRows);
    for (int y = 0; y < imageRows; ++y)
        for (int x = 0; x < cols; ++x)
        {
            const auto c = scaled.getPixelAt (x, y);
            const auto lum = (0.299f * c.getRed() + 0.587f * c.getGreen() + 0.114f * c.getBlue()) * c.getFloatAlpha();
            bytes[(size_t) ((imageRows - 1 - y) * cols + x)] = (unsigned char) juce::roundToInt (lum);
        }
    audioProcessor.setImage (std::move (bytes), cols, f.getFileName());
    imageStatus.setText (f.getFileName() + utf8 (" \xc2\xb7 ") + juce::String (cols) + " columns", juce::dontSendNotification);
}

// A sound: its pitch found by autocorrelation over the loudest stretch, then each
// frame's level at every harmonic of that pitch, as the web version does it.
void LoamCharmorEditor::loadSoundFile (const juce::File& f)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (f));
    if (reader == nullptr)
    {
        imageStatus.setText ("Couldn't read that sound.", juce::dontSendNotification);
        return;
    }
    const auto sr = reader->sampleRate;
    const auto len = (int) juce::jmin ((juce::int64) (sr * 30), reader->lengthInSamples);
    juce::AudioBuffer<float> audio ((int) reader->numChannels, len);
    reader->read (&audio, 0, len, 0, true, true);
    std::vector<float> x ((size_t) len, 0.0f);
    for (int ch = 0; ch < audio.getNumChannels(); ++ch)
        for (int i = 0; i < len; ++i)
            x[(size_t) i] += audio.getSample (ch, i) / (float) audio.getNumChannels();

    // pitch
    const int N = 4096;
    double best = 0;
    int bestAt = 0;
    for (int s = 0; s + N < len; s += 1024)
    {
        double e = 0;
        for (int i = s; i < s + N; i += 4) e += x[(size_t) i] * x[(size_t) i];
        if (e > best) { best = e; bestAt = s; }
    }
    double f0 = 0;
    if (len > N)
    {
        const int lo = (int) (sr / 1000), hi = juce::jmin (N / 2, (int) std::ceil (sr / 50));
        std::vector<double> corr ((size_t) hi + 2, 0.0);
        double maxCorr = 0;
        for (int l = lo; l <= hi + 1; ++l)
        {
            double sum = 0, n1 = 0, n2 = 0;
            for (int i = 0; i + l < N; ++i)
            {
                const auto a = x[(size_t) (bestAt + i)], b = x[(size_t) (bestAt + i + l)];
                sum += a * b; n1 += a * a; n2 += b * b;
            }
            corr[(size_t) l] = sum / (std::sqrt (n1 * n2) + 1e-12);
            if (l <= hi) maxCorr = std::fmax (maxCorr, corr[(size_t) l]);
        }
        for (int l = lo + 1; l <= hi; ++l)   // the first strong peak, not just the biggest (octave errors)
            if (corr[(size_t) l] > corr[(size_t) l - 1] && corr[(size_t) l] >= corr[(size_t) l + 1] && corr[(size_t) l] > 0.8 * maxCorr)
            {
                const auto a = corr[(size_t) l - 1], b = corr[(size_t) l], c = corr[(size_t) l + 1];
                const auto d = (a - c) / (2 * (a - 2 * b + c));
                f0 = sr / (l + (std::isfinite (d) ? d : 0.0));
                break;
            }
    }
    if (f0 <= 0)
    {
        imageStatus.setText ("Couldn't find a pitch in " + f.getFileName() + ".", juce::dontSendNotification);
        return;
    }

    // per-frame harmonic levels
    const auto dur = len / sr;
    const int cols = juce::jlimit (8, 256, juce::roundToInt (dur * 40));
    const int order = f0 < 100 ? 13 : 12, fftN = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> data ((size_t) fftN * 2);
    const auto binHz = sr / fftN;
    const int K = juce::jmin (maxPartials, (int) std::floor (sr / 2 * 0.95 / f0));
    std::vector<double> mags ((size_t) cols * imageRows, 0.0);
    double maxMag = 0;
    for (int c = 0; c < cols; ++c)
    {
        const int centre = (int) std::floor ((c + 0.5) / cols * len);
        std::fill (data.begin(), data.end(), 0.0f);
        for (int i = 0; i < fftN; ++i)
        {
            const int at = centre - fftN / 2 + i;
            data[(size_t) i] = at >= 0 && at < len ? x[(size_t) at] * (float) (0.5 - 0.5 * std::cos (2 * pi * i / (fftN - 1))) : 0.0f;
        }
        fft.performFrequencyOnlyForwardTransform (data.data());
        for (int k = 1; k <= K; ++k)
        {
            const int bin = juce::roundToInt (k * f0 / binHz);
            double m = 0;
            for (int b = bin - 1; b <= bin + 1; ++b)
                if (b > 0 && b < fftN / 2) m = std::fmax (m, (double) data[(size_t) b]);
            mags[(size_t) ((k - 1) * cols + c)] = m;
            maxMag = std::fmax (maxMag, m);
        }
    }
    std::vector<unsigned char> bytes (mags.size());
    for (size_t i = 0; i < bytes.size(); ++i)
    {
        const auto a = maxMag > 0 ? mags[i] / maxMag : 0.0;
        bytes[i] = a <= 1e-3 ? 0 : (unsigned char) juce::jlimit (0, 255, juce::roundToInt (255 * (1 + 20 * std::log10 (a) / 60)));
    }
    if (auto* p = audioProcessor.apvts.getParameter ("imgSpeed"))
        p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0.1, 30.0, dur)));
    if (auto* p = audioProcessor.apvts.getParameter ("imgLoop"))
        p->setValueNotifyingHost (0.0f);
    const auto name = f.getFileName() + utf8 (" \xc2\xb7 ") + noteName (juce::roundToInt (69 + 12 * std::log2 (f0 / 440)))
                      + " (" + juce::String (juce::roundToInt (f0)) + " Hz)";
    audioProcessor.setImage (std::move (bytes), cols, name);
    imageStatus.setText (name + utf8 (" \xc2\xb7 ") + juce::String (cols) + " columns", juce::dontSendNotification);
}

//==============================================================================
void LoamCharmorEditor::timerCallback()
{
    spectrum.refresh();
    const auto snap = audioProcessor.getSnapshot();
    const auto live = snap.note >= 0 && snap.voices > 0;
    for (auto* gv : graphs)
        if (gv->isShowing()) gv->refresh (snap, live);
    if (page == 2)
        imageView.refresh (live ? snap.imgPos : -1.0);
    for (auto& [id, k] : knobs)
        k->refresh();
    pitchWheel.refresh();
    modWheel.refresh();

    const auto voices = audioProcessor.getActiveVoices(), note = audioProcessor.lastNote.load();
    const auto sustain = audioProcessor.sustainActive.load();
    if (voices != shownVoices || note != shownNote || sustain != shownSustain)
    {
        shownVoices = voices; shownNote = note; shownSustain = sustain;
        panel.repaint (headerArea);
        panel.repaint (sustainLight);
    }
    octaveLabel.setText ("OCT " + juce::String (3 + octaveShift), juce::dontSendNotification);

    const auto sig = audioProcessor.getPresetName() + (audioProcessor.isPresetEdited() ? "*" : "") + juce::String (userPresets.size());
    if (sig != shownPresetSig)
        refreshPresetBox();
    panel.repaint (screenBay.withTrimmedTop (screenBay.getHeight() - 20));

    if (! keyboardFocused && keyboard.isShowing())
    {
        keyboard.grabKeyboardFocus();
        keyboardFocused = true;
    }
}

void LoamCharmorEditor::paintPanel (juce::Graphics& g)
{
    g.fillAll (LoamColours::panel);

    // brand: LOAM ELF-cHARMOR, the subtitle and the egret
    auto brand = headerArea.toFloat();
    auto titleRow = brand.removeFromTop (32.0f);
    const auto brandFont = sansFont (30.0f, true);
    const std::pair<juce::String, juce::Colour> words[] = { { "LOAM", ink }, { "ELF-cHARMOR", coral } };
    for (const auto& [text, colour] : words)
    {
        g.setFont (brandFont);
        g.setColour (colour);
        const auto w = juce::GlyphArrangement::getStringWidth (brandFont, text);
        g.drawText (text, titleRow.removeFromLeft (w + 8.0f), juce::Justification::bottomLeft, false);
    }
    g.setColour (inkSoft);
    g.setFont (monoFont (10.0f).withExtraKerningFactor (0.14f));
    g.drawText (brandSub, brand.removeFromTop (18.0f), juce::Justification::centredLeft, false);

    auto readout = [&g] (juce::Rectangle<float> w, const juce::String& label, const juce::String& value)
    {
        g.setColour (readoutBg);
        g.fillRoundedRectangle (w, 6.0f);
        w.reduce (10.0f, 0.0f);
        g.setColour (readoutInk.withAlpha (0.55f));
        g.setFont (monoFont (9.0f).withExtraKerningFactor (0.1f));
        g.drawText (label, w.removeFromLeft (44.0f), juce::Justification::centredLeft, false);
        g.setColour (readoutInk);
        g.setFont (monoFont (15.0f));
        g.drawText (value, w, juce::Justification::centredLeft, false);
    };
    auto readouts = headerArea.toFloat();
    readout (readouts.removeFromRight (104.0f).withSizeKeepingCentre (104.0f, 28.0f), "NOTE", shownNote >= 0 ? noteName (shownNote) : utf8 ("\xe2\x80\x94"));
    readouts.removeFromRight (10.0f);
    readout (readouts.removeFromRight (90.0f).withSizeKeepingCentre (90.0f, 28.0f), "VOICES", juce::String (juce::jmax (0, shownVoices)));

    // the screen bay and its caption
    g.setColour (grooveDark);
    g.fillRoundedRectangle (screenBay.toFloat(), 10.0f);
    g.setColour (ink.withAlpha (0.75f));
    g.setFont (monoFont (9.0f).withExtraKerningFactor (0.15f));
    const auto caption = screenBay.reduced (10, 0).removeFromBottom (18).toFloat();
    g.drawText (spectrum.caption, caption, juce::Justification::centredLeft, false);
    g.drawText ("SPECTRUM", caption, juce::Justification::centredRight, false);

    // the presets module
    const auto pb = presetsArea.toFloat();
    g.setColour (sage);
    g.fillRoundedRectangle (pb, 12.0f);
    g.setColour (panelRaised);
    g.fillRoundedRectangle (pb.withTrimmedTop (3.0f), 12.0f);
    g.setColour (grooveDark.withAlpha (0.6f));
    g.drawRoundedRectangle (pb.withTrimmedTop (3.0f), 12.0f, 1.0f);
    g.setColour (sage);
    g.setFont (sansFont (14.0f, true).withExtraKerningFactor (0.12f));
    g.drawText ("PRESETS", presetsArea.reduced (12, 0).withTrimmedTop (8).withHeight (16), juce::Justification::centredLeft);

    // the keybed bay, with the SUSTAIN light
    g.setColour (grooveDark);
    g.fillRoundedRectangle (keybedBay.toFloat(), 10.0f);
    const auto light = sustainLight.toFloat();
    const auto led = juce::Rectangle<float> (9.0f, 9.0f).withCentre ({ light.getRight() - 6.0f, light.getCentreY() });
    if (shownSustain)
    {
        g.setColour (coral.withAlpha (0.35f));
        g.fillEllipse (led.expanded (3.0f));
    }
    g.setColour (shownSustain ? coral : ink.withAlpha (0.25f));
    g.fillEllipse (led);
    g.setColour (ink);
    g.setFont (monoFont (10.0f).withExtraKerningFactor (0.12f));
    g.drawText ("SUSTAIN", light.withTrimmedRight (16.0f), juce::Justification::centredRight, false);
}

void LoamCharmorEditor::layoutPanel()
{
    headerArea = { pad, pad, panelWidth - 2 * pad, 48 };
    const auto subWidth = juce::GlyphArrangement::getStringWidth (monoFont (10.0f).withExtraKerningFactor (0.14f), subtitleText());
    egret.setBounds (headerArea.getX() + juce::roundToInt (subWidth) + 8, headerArea.getY() + 30, 22, 22);

    screenBay = { pad, 80, panelWidth - 2 * pad, 156 };
    spectrum.setBounds (screenBay.reduced (10).withTrimmedBottom (14));

    presetsArea = { pad, 248, panelWidth - 2 * pad, 64 };
    auto pr = presetsArea.reduced (12).withTrimmedTop (titleH - 6).withHeight (26);
    presetPrev.setBounds (pr.removeFromLeft (30)); pr.removeFromLeft (6);
    presetBox.setBounds (pr.removeFromLeft (220)); pr.removeFromLeft (6);
    presetNext.setBounds (pr.removeFromLeft (30)); pr.removeFromLeft (14);
    presetName.setBounds (pr.removeFromLeft (170)); pr.removeFromLeft (6);
    presetSave.setBounds (pr.removeFromLeft (56)); pr.removeFromLeft (6);
    presetDelete.setBounds (pr.removeFromLeft (64)); pr.removeFromLeft (10);
    presetStatus.setBounds (pr);

    for (int i = 0; i < 4; ++i)
    {
        tabs[(size_t) i].setBounds (pad + i * 82, 324, 76, 28);
        pages[(size_t) i].setBounds (0, pageY, panelWidth, pageH);
        pages[(size_t) i].modules.clear();
        pages[(size_t) i].unitTitles.clear();
    }

    auto& sound = pages[0];
    auto addModule = [] (PageView& pv, const juce::String& title, juce::Colour accent, juce::Rectangle<int> r)
    {
        pv.modules.push_back ({ title, accent, r });
        return content (r);
    };
    auto knobList = [this] (std::initializer_list<const char*> ids)
    {
        std::vector<juce::Component*> list;
        for (auto* id : ids) list.push_back (&knob (id));
        return list;
    };

    // ---- SOUND: timbre | filter | output; filter env | amp env | unison; prism | phaser | harmonizer | blur
    {
        auto t = addModule (sound, "Timbre", mustard, cols (0, 3, 0, 196));
        auto rowA = t.removeFromTop (26); t.removeFromTop (6);
        auto rowB = t.removeFromTop (26); t.removeFromTop (10);
        smallLabels[0]->setBounds (rowA.removeFromLeft (16));
        choiceRows[0]->setBounds (rowA);
        smallLabels[1]->setBounds (rowB.removeFromLeft (16));
        choiceRows[1]->setBounds (rowB);
        layoutKnobs (t, knobList ({ "morph", "oddEven", "pluck" }));

        auto f = addModule (sound, "Filter", teal, cols (3, 2, 0, 196));
        choiceRows[2]->setBounds (f.removeFromTop (26)); f.removeFromTop (10);
        layoutKnobs (f, knobList ({ "cutoff", "res", "envAmt", "keytrack" }));

        layoutKnobs (addModule (sound, "Output", mustard, cols (5, 1, 0, 196)), knobList ({ "volume" }));

        auto fe = addModule (sound, "Filter Env", teal, cols (0, 2, 208, 176));
        graphs[0]->setBounds (fe.removeFromTop (56)); fe.removeFromTop (8);
        layoutKnobs (fe, knobList ({ "fA", "fD", "fS", "fR" }));
        auto ae = addModule (sound, "Amp Env", coral, cols (2, 2, 208, 176));
        graphs[1]->setBounds (ae.removeFromTop (56)); ae.removeFromTop (8);
        layoutKnobs (ae, knobList ({ "aA", "aD", "aS", "aR" }));
        layoutKnobs (addModule (sound, "Unison", lavender, cols (4, 2, 208, 176)), knobList ({ "uniVoices", "uniDetune", "uniSpread" }));

        layoutKnobs (addModule (sound, "Prism", coral, cols (0, 1, 396, 122)), knobList ({ "prism", "prismEnv" }));
        layoutKnobs (addModule (sound, "Phaser", teal, cols (1, 2, 396, 122)), knobList ({ "phDepth", "phNotches", "phOffset", "phSpeed" }));
        layoutKnobs (addModule (sound, "Harmonizer", sage, cols (3, 2, 396, 122)), knobList ({ "harmVoices", "harmInterval", "harmLevel" }));
        layoutKnobs (addModule (sound, "Blur", mustard, cols (5, 1, 396, 122)), knobList ({ "blurTime", "blurHarm" }));
    }

    // ---- MOD: LFO 1 | LFO 2; Env 2 | Env 3; the matrix
    {
        auto& mod = pages[1];
        for (int l = 0; l < 2; ++l)
        {
            auto r = addModule (mod, l == 0 ? "LFO 1" : "LFO 2", lavender, cols (l * 3, 3, 0, 206));
            auto row = r.removeFromTop (24); r.removeFromTop (6);
            toggleChips[l]->setBounds (row.removeFromRight (74)); row.removeFromRight (6);
            choiceRows[3 + l]->setBounds (row);
            graphs[2 + l]->setBounds (r.removeFromTop (56)); r.removeFromTop (6);
            layoutKnobs (r, l == 0 ? knobList ({ "l1Rate", "l1Delay" }) : knobList ({ "l2Rate", "l2Delay" }));
        }
        for (int e = 0; e < 2; ++e)
        {
            auto r = addModule (mod, e == 0 ? "Env 2" : "Env 3", coral, cols (e * 3, 3, 218, 176));
            graphs[4 + e]->setBounds (r.removeFromTop (56)); r.removeFromTop (8);
            layoutKnobs (r, e == 0 ? knobList ({ "e2A", "e2D", "e2S", "e2R" }) : knobList ({ "e3A", "e3D", "e3S", "e3R" }));
        }
        auto m = addModule (mod, "Mod Matrix", sage, cols (0, 6, 406, 188));
        const auto slotW = (m.getWidth() - 2 * 18) / 3;
        for (int i = 0; i < modSlots; ++i)
        {
            auto slot = juce::Rectangle<int> (m.getX() + (i % 3) * (slotW + 18), m.getY() + (i / 3) * (knobH + 10), slotW, knobH);
            knob ("modAmt" + juce::String (i)).setBounds (slot.removeFromRight (knobW));
            slot.removeFromRight (6);
            const auto boxW = (slot.getWidth() - 16) / 2;
            auto boxRow = slot.withSizeKeepingCentre (slot.getWidth(), 24).withY (slot.getY() + 14);
            matrixBoxes[i * 2]->setBounds (boxRow.removeFromLeft (boxW));
            smallLabels[2 + i]->setBounds (boxRow.removeFromLeft (16));
            matrixBoxes[i * 2 + 1]->setBounds (boxRow.removeFromLeft (boxW));
        }
    }

    // ---- IMAGE
    {
        auto r = addModule (pages[2], "Image", teal, cols (0, 6, 0, 300));
        imageView.setBounds (r.removeFromLeft (520).withHeight (240));
        r.removeFromLeft (16);
        auto row = r.removeFromTop (26);
        loadImage.setBounds (row.removeFromLeft (96)); row.removeFromLeft (6);
        loadSound.setBounds (row.removeFromLeft (96)); row.removeFromLeft (6);
        clearImage.setBounds (row.removeFromLeft (64));
        r.removeFromTop (8);
        imageStatus.setBounds (r.removeFromTop (16));
        r.removeFromTop (8);
        auto controls = r.removeFromTop (knobH);
        layoutKnobs (controls.removeFromLeft (2 * knobW + 8), knobList ({ "imgAmount", "imgSpeed" }));
        controls.removeFromLeft (10);
        toggleChips[2]->setBounds (controls.removeFromLeft (64).withSizeKeepingCentre (64, 26));
        r.removeFromTop (8);
        imageHelp.setBounds (r);
    }

    // ---- FX: five units side by side
    {
        auto r = addModule (pages[3], "Effects", lavender, cols (0, 6, 0, 210));
        const char* names[] = { "Dist", "Chorus", "Delay", "Reverb", "Comp" };
        const std::vector<std::vector<const char*>> unitKnobs { { "distDrive", "distTone", "distMix" }, { "choRate", "choDepth", "choMix" },
                                                                { "dlyTime", "dlyFb", "dlyMix" }, { "revSize", "reverb" },
                                                                { "compThresh", "compRatio", "compGain" } };
        const auto unitW = (r.getWidth() - 4 * 10) / 5;
        for (int u = 0; u < 5; ++u)
        {
            const auto unit = juce::Rectangle<int> (r.getX() + u * (unitW + 10), r.getY(), unitW, r.getHeight());
            pages[3].unitTitles.push_back ({ unit, names[u] });
            fxSwitches[u]->setBounds (unit.reduced (8, 0).withTrimmedTop (8).withHeight (22).removeFromRight (40));
            std::vector<juce::Component*> list;
            for (auto* id : unitKnobs[(size_t) u]) list.push_back (&knob (id));
            layoutKnobs (unit.reduced (2, 0).withTrimmedTop (44), list);
        }
    }

    // ---- keyboard bay: octave, sustain light, wheels and keys
    keybedBay = { pad, keybedY, panelWidth - 2 * pad, keybedH };
    auto kb = keybedBay.reduced (8);
    auto top = kb.removeFromTop (24);
    octaveDown.setBounds (top.removeFromLeft (28));
    octaveLabel.setBounds (top.removeFromLeft (64));
    octaveUp.setBounds (top.removeFromLeft (28));
    sustainLight = top.removeFromRight (90);
    kb.removeFromTop (6);
    auto wheels = kb.removeFromLeft (70);
    auto pitchCol = wheels.removeFromLeft (32), modCol = wheels.removeFromRight (32);
    bendUp.setBounds (pitchCol.removeFromTop (18));
    bendDown.setBounds (pitchCol.removeFromBottom (18));
    pitchWheel.setBounds (pitchCol.reduced (5, 3));
    modModeButton.setBounds (modCol.removeFromTop (18));
    modWheelLabel.setBounds (modCol.removeFromBottom (18));
    modWheel.setBounds (modCol.reduced (5, 3));
    kb.removeFromLeft (8);
    keyboard.setBounds (kb);
    keyboard.setKeyWidth ((float) kb.getWidth() / 17.0f);   // 17 white keys, C to E two octaves up
}
