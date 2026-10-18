#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <juce_audio_processors/juce_audio_processors.h>
#include "CharmorEngine.h"
#include "CharmorParams.h"
#include "FxChain.h"

// Loam Elf-cHarmor as a plugin: the web version's additive engine and FX page,
// every knob a parameter, CUST mod-wheel bindings and the IMAGE picture kept
// with the project, factory presets and user presets as files.
class LoamCharmorProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    LoamCharmorProcessor();
    ~LoamCharmorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Loam Elf-cHarmor"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;

    // Wheels and pedal: written by MIDI on the audio thread and by the on-screen
    // controls; whichever moved last wins.
    std::atomic<float> bendWheel { 0.0f }, modWheel { 0.0f };
    std::atomic<bool> uiSustain { false }, sustainActive { false };
    std::atomic<int> lastNote { -1 };
    void chirp() { chirpPending = true; }   // the egret's two little calls

    static juce::NormalisableRange<float> rangeFor (const charmor::KnobDef&);
    static const charmor::KnobDef* findKnob (const juce::String& id);

    //==============================================================================
    // CUST: knobs on the mod wheel, from red (wheel down) to green (wheel up),
    // positions 0..1 along the knob.
    struct Binding { bool bound = false, reversed = false; float red = 0.0f, green = 1.0f; };
    Binding getBinding (const juce::String& id) const;
    void setBinding (const juce::String& id, Binding);
    bool isCustomActive() const;
    float getHeardNormalised (const juce::String& id) const;   // where the knob is heard, 0..1

    // What the voices hear right now (CUST applied): for the editor's previews and graphs.
    charmor::Params heardParams() const;
    CharmorEngine::Snapshot getSnapshot() const { return engine.getSnapshot(); }
    int getActiveVoices() const { return engine.getActiveVoices(); }

    //==============================================================================
    // IMAGE
    void setImage (std::vector<unsigned char> bytes, int cols, const juce::String& name);
    void clearImage() { setImage ({}, 0, {}); }
    std::shared_ptr<const charmor::Image> getImage() const { return std::atomic_load (&image); }
    juce::String getImageName() const { return imageName; }

    //==============================================================================
    // Presets
    void loadFactoryPreset (int index);
    static juce::File getUserPresetFolder();
    juce::StringArray getUserPresetNames() const;
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::String& name);
    bool deleteUserPreset (const juce::String& name);
    juce::String getPresetName() const { return presetName; }
    bool isPresetEdited() const;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void buildParams (charmor::Params&, FxChain::Settings&) const;
    void handleAsyncUpdate() override;   // rebuilds the reverb room off the audio thread
    void resetToDefaults();
    void applyChanges (const juce::String& changes);
    void setParam (const juce::String& id, float value);
    void rememberPreset (const juce::String& name);
    std::unique_ptr<juce::XmlElement> extrasToXml() const;   // CUST and IMAGE
    void extrasFromXml (const juce::XmlElement*);

    CharmorEngine engine;
    FxChain fx;
    bool midiSustain = false;

    struct Knob
    {
        const charmor::KnobDef* def = nullptr;
        juce::RangedAudioParameter* param = nullptr;
        std::atomic<float>* value = nullptr;
        std::function<void (charmor::Params&, FxChain::Settings&, double)> apply;
        std::atomic<bool> bound { false }, reversed { false };
        std::atomic<float> red { 0.0f }, green { 1.0f };
    };
    std::vector<std::unique_ptr<Knob>> knobs;
    std::map<juce::String, Knob*> knobById;
    std::map<juce::String, std::atomic<float>*> choiceValues, toggleValues;
    std::atomic<float>* bendRange = nullptr;
    // Read on the audio thread every block: looked up once, here.
    std::atomic<float> *waveA = nullptr, *waveB = nullptr, *ftype = nullptr, *l1Shape = nullptr, *l2Shape = nullptr, *modMode = nullptr;
    std::atomic<float> *l1Retrig = nullptr, *l2Retrig = nullptr, *imgLoop = nullptr;
    std::atomic<float> *fxDist = nullptr, *fxCho = nullptr, *fxDly = nullptr, *fxRev = nullptr, *fxComp = nullptr;
    std::array<std::atomic<float>*, charmor::modSlots> modSrc {}, modDst {};
    juce::AudioBuffer<float> monoScratch;   // the second channel for a mono output
    std::atomic<bool> chirpPending { false };
    double chirpT = -1, sampleRate = 44100;
    std::array<double, 2> chirpPhase {};

    std::shared_ptr<const charmor::Image> image;
    juce::String imageName, presetName { "Init Saw" };
    std::map<juce::String, float> presetSnapshot;   // normalised values when the preset loaded
    std::atomic<double> wantedReverbSize { 2.4 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoamCharmorProcessor)
};
