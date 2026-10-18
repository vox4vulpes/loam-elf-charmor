#pragma once

#include <array>
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

// The web version's :root palette.
namespace LoamColours
{
    inline const juce::Colour panel       { 0xffe9e1cc };
    inline const juce::Colour panelRaised { 0xfff2ecd9 };
    inline const juce::Colour groove      { 0xffc7bb9c };
    inline const juce::Colour grooveDark  { 0xffa99c79 };
    inline const juce::Colour ink         { 0xff2a241d };
    inline const juce::Colour inkSoft     { 0x9e2a241d };
    inline const juce::Colour mustard     { 0xffc08f17 };
    inline const juce::Colour teal        { 0xff336f6b };
    inline const juce::Colour coral       { 0xffb85540 };
    inline const juce::Colour lavender    { 0xff8a78a3 };
    inline const juce::Colour sage        { 0xff5f7344 };
    inline const juce::Colour readoutBg   { 0xff171308 };
    inline const juce::Colour readoutInk  { 0xfff0a830 };
    inline const juce::Colour shadow      { 0x591e190f };
    inline const juce::Colour modRed      { 0xffd33b2c };
    inline const juce::Colour modGreen    { 0xff3a9a4a };
    inline const juce::Colour screenTeal  { 0xe67ec4be };
}

class LoamLookAndFeel : public juce::LookAndFeel_V4
{
public:
    LoamLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool isMouseOver, bool isDown) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool isHighlighted, bool isDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

// A knob as on the web panel: dial, label, value. Click the value to type one;
// in CUST it gets a button to put it on the mod wheel and red/green dots.
class CharmorKnob : public juce::Component
{
public:
    CharmorKnob (LoamCharmorProcessor&, const juce::String& paramId, juce::Colour accent);
    void resized() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void refresh();   // the editor's timer: CUST state and the heard position

private:
    struct ModButton : public juce::Component
    {
        std::function<void()> onClick;
        int state = 0;   // 0 off, 1 on (green), 2 reversed (red)
        void paint (juce::Graphics&) override;
        void mouseUp (const juce::MouseEvent& e) override { if (contains (e.getPosition()) && onClick) onClick(); }
    };

    juce::String format (double v) const;
    void updateReadout();
    void cycleBinding();
    void startTyping();
    juce::Rectangle<int> dialBounds() const;
    juce::Point<float> dotPosition (float normalised) const;

    LoamCharmorProcessor& processor;
    juce::String paramId;
    const charmor::KnobDef* def;
    juce::RangedAudioParameter* param;
    juce::Slider slider;
    juce::Label label, readout;
    juce::AudioProcessorValueTreeState::SliderAttachment attachment;
    ModButton modButton;
    std::unique_ptr<juce::TextEditor> typing;
    bool modded = false, customShown = false;
    float shownHeard = -1.0f;
    int draggingDot = 0;
};

// A row of buttons for a choice parameter (timbres, filter type, LFO shape).
class ChoiceButtons : public juce::Component
{
public:
    ChoiceButtons (juce::AudioProcessorValueTreeState&, const juce::String& paramId, juce::Colour accent);
    void resized() override;

private:
    juce::OwnedArray<juce::TextButton> buttons;
    std::unique_ptr<juce::ParameterAttachment> attachment;
};

// The spectrum screen: harmonic bars and the filter (with the phaser's notches).
class SpectrumView : public juce::Component
{
public:
    explicit SpectrumView (LoamCharmorProcessor& p) : processor (p) {}
    void paint (juce::Graphics&) override;
    void refresh();
    juce::String caption;

private:
    LoamCharmorProcessor& processor;
    CharmorEngine::Snapshot snap;
    bool live = false;
    int quietTicks = 0;
};

// An LFO's or an envelope's shape, with a dot where the newest note is on it.
class GraphView : public juce::Component
{
public:
    enum Kind { lfo1, lfo2, ampEnv, filterEnv, env2, env3 };
    GraphView (LoamCharmorProcessor& p, Kind k, juce::Colour c) : processor (p), kind (k), colour (c) {}
    void paint (juce::Graphics&) override;
    void refresh (const CharmorEngine::Snapshot&, bool live);

private:
    void paintLfo (juce::Graphics&, const charmor::Params&);
    void paintEnv (juce::Graphics&, const charmor::Params&);
    LoamCharmorProcessor& processor;
    Kind kind;
    juce::Colour colour;
    CharmorEngine::Snapshot snap;
    bool live = false;
};

// The IMAGE: the picture in amber, time left to right, harmonics bottom to top.
class ImageView : public juce::Component, public juce::FileDragAndDropTarget
{
public:
    explicit ImageView (LoamCharmorProcessor& p) : processor (p) {}
    std::function<void (const juce::File&)> onFile;
    void paint (juce::Graphics&) override;
    void refresh (double playPos);   // -1 for none
    void rebuild();
    bool isInterestedInFileDrag (const juce::StringArray&) override { return true; }
    void fileDragEnter (const juce::StringArray&, int, int) override { dragOver = true; repaint(); }
    void fileDragExit (const juce::StringArray&) override { dragOver = false; repaint(); }
    void filesDropped (const juce::StringArray& files, int, int) override;
    int getTopRow() const { return topRow; }

private:
    LoamCharmorProcessor& processor;
    juce::Image picture;
    int topRow = charmor::imageRows;
    double pos = -1;
    bool dragOver = false;
    const charmor::Image* shown = nullptr;
};

// Two tracker-style octaves on the computer keys, Space as the sustain pedal.
class LoamKeyboard : public juce::MidiKeyboardComponent
{
public:
    explicit LoamKeyboard (juce::MidiKeyboardState&);
    void setBaseNote (int midiNote);
    std::function<void (bool)> onSustainKey;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;
    void focusLost (FocusChangeType) override;
    static constexpr int numKeys = 29;   // C to E two octaves up
    void drawWhiteNote (int, juce::Graphics&, juce::Rectangle<float>, bool, bool, juce::Colour, juce::Colour) override;
    void drawBlackNote (int, juce::Graphics&, juce::Rectangle<float>, bool, bool, juce::Colour) override;

private:
    juce::String capFor (int midiNoteNumber) const;
    void setSpaceDown (bool);
    int baseNote = 48;
    bool spaceDown = false;
};

// A pitch or mod wheel beside the keys (as on Loam Eight Sub).
class WheelView : public juce::Component
{
public:
    WheelView (std::atomic<float>& value, bool bipolar);
    juce::String centreText;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent& e) override { setFromY (e.position.y); }
    void mouseDrag (const juce::MouseEvent& e) override { setFromY (e.position.y); }
    void mouseUp (const juce::MouseEvent&) override;
    void refresh();

private:
    void setFromY (float y);
    std::atomic<float>& value;
    bool bipolar;
    float shown = -2.0f;
};

class EgretView : public juce::Component, public juce::SettableTooltipClient
{
public:
    EgretView();
    std::function<void()> onClick;
    void paint (juce::Graphics&) override;
    void mouseUp (const juce::MouseEvent& e) override { if (contains (e.getPosition()) && onClick) onClick(); }

private:
    std::unique_ptr<juce::Drawable> drawable;
};

// One page under the screen (SOUND, MOD, IMAGE, FX): modules painted behind their controls.
class PageView : public juce::Component
{
public:
    struct Module { juce::String title; juce::Colour accent; juce::Rectangle<int> bounds; };
    std::vector<Module> modules;
    std::vector<std::pair<juce::Rectangle<int>, juce::String>> unitTitles;   // FX units' names
    void paint (juce::Graphics&) override;
};

// The whole panel at its native 900 px width; the editor scales it to the window.
class PanelView : public juce::Component
{
public:
    std::function<void (juce::Graphics&)> painter;
    void paint (juce::Graphics& g) override { if (painter) painter (g); }
};

class LoamCharmorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit LoamCharmorEditor (LoamCharmorProcessor&);
    ~LoamCharmorEditor() override;
    void resized() override;

private:
    void timerCallback() override;
    void paintPanel (juce::Graphics&);
    void layoutPanel();
    void showPage (int);
    void refreshPresetBox();
    void savePreset();
    void deletePreset();
    void loadPictureFile (const juce::File&);
    void loadSoundFile (const juce::File&);
    void loadAnyFile (const juce::File&);
    CharmorKnob& knob (const juce::String& id);

    LoamCharmorProcessor& audioProcessor;
    LoamLookAndFeel lnf;
    juce::TooltipWindow tooltipWindow { this, 600 };
    PanelView panel;

    // header and screen
    EgretView egret;
    juce::String brandSub;
    SpectrumView spectrum;
    juce::Rectangle<int> headerArea, screenBay, presetsArea, keybedBay, sustainLight;
    int shownVoices = -1, shownNote = -2;
    bool shownSustain = false;

    // presets
    juce::ComboBox presetBox;
    juce::TextButton presetPrev { "<" }, presetNext { ">" }, presetSave { "SAVE" }, presetDelete { "DELETE" };
    juce::TextEditor presetName;
    juce::Label presetStatus;
    juce::StringArray userPresets;
    juce::String shownPresetSig;
    bool deleteArmed = false;

    // pages
    std::array<juce::TextButton, 4> tabs;
    std::array<PageView, 4> pages;
    int page = 0;
    std::map<juce::String, std::unique_ptr<CharmorKnob>> knobs;
    juce::OwnedArray<ChoiceButtons> choiceRows;
    juce::OwnedArray<GraphView> graphs;
    juce::OwnedArray<juce::Label> smallLabels;
    juce::OwnedArray<juce::TextButton> toggleChips;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ButtonAttachment> buttonAttachments;
    juce::OwnedArray<juce::ComboBox> matrixBoxes;
    juce::OwnedArray<juce::AudioProcessorValueTreeState::ComboBoxAttachment> comboAttachments;
    juce::OwnedArray<juce::ToggleButton> fxSwitches;

    // image page
    ImageView imageView { audioProcessor };
    juce::TextButton loadImage { "LOAD IMAGE" }, loadSound { "LOAD SOUND" }, clearImage { "CLEAR" };
    juce::Label imageStatus, imageHelp;
    std::unique_ptr<juce::FileChooser> chooser;

    // keyboard and wheels
    LoamKeyboard keyboard;
    juce::TextButton octaveDown { "-" }, octaveUp { "+" }, bendUp { "+" }, bendDown { "-" }, modModeButton;
    juce::Label octaveLabel, modWheelLabel;
    WheelView pitchWheel, modWheel;
    std::unique_ptr<juce::ParameterAttachment> modModeAttachment, bendRangeAttachment;
    int octaveShift = 0, modMode = 0;
    bool keyboardFocused = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamCharmorEditor)
};
