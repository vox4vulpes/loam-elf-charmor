#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace charmor;

namespace
{
    const juce::Identifier presetProp ("preset"), custTag ("CUST"), imageTag ("IMAGE");
    constexpr const char* presetTag = "LoamElfCharmorPreset";

    // Where each knob goes: a field of the voices' Params or of the FX settings.
    std::function<void (Params&, FxChain::Settings&, double)> applierFor (const juce::String& id)
    {
        using P = Params;
        using S = FxChain::Settings;
        auto p = [] (double P::* m) { return [m] (P& x, S&, double v) { x.*m = v; }; };
        auto s = [] (double S::* m) { return [m] (P&, S& x, double v) { x.*m = v; }; };
        static const std::map<juce::String, std::function<void (P&, S&, double)>> table {
            { "morph", p (&P::morph) }, { "oddEven", p (&P::oddEven) }, { "pluck", p (&P::pluck) },
            { "cutoff", p (&P::cutoff) }, { "res", p (&P::res) }, { "envAmt", p (&P::envAmt) }, { "keytrack", p (&P::keytrack) },
            { "fA", p (&P::fA) }, { "fD", p (&P::fD) }, { "fS", p (&P::fS) }, { "fR", p (&P::fR) },
            { "aA", p (&P::aA) }, { "aD", p (&P::aD) }, { "aS", p (&P::aS) }, { "aR", p (&P::aR) },
            { "uniVoices", p (&P::uniVoices) }, { "uniDetune", p (&P::uniDetune) }, { "uniSpread", p (&P::uniSpread) },
            { "prism", p (&P::prism) }, { "prismEnv", p (&P::prismEnv) },
            { "phDepth", p (&P::phDepth) }, { "phNotches", p (&P::phNotches) }, { "phOffset", p (&P::phOffset) }, { "phSpeed", p (&P::phSpeed) },
            { "harmVoices", p (&P::harmVoices) }, { "harmInterval", p (&P::harmInterval) }, { "harmLevel", p (&P::harmLevel) },
            { "blurTime", p (&P::blurTime) }, { "blurHarm", p (&P::blurHarm) },
            { "l1Rate", p (&P::l1Rate) }, { "l1Delay", p (&P::l1Delay) }, { "l2Rate", p (&P::l2Rate) }, { "l2Delay", p (&P::l2Delay) },
            { "e2A", p (&P::e2A) }, { "e2D", p (&P::e2D) }, { "e2S", p (&P::e2S) }, { "e2R", p (&P::e2R) },
            { "e3A", p (&P::e3A) }, { "e3D", p (&P::e3D) }, { "e3S", p (&P::e3S) }, { "e3R", p (&P::e3R) },
            { "imgAmount", p (&P::imgAmount) }, { "imgSpeed", p (&P::imgSpeed) },
            { "distDrive", s (&S::distDrive) }, { "distTone", s (&S::distTone) }, { "distMix", s (&S::distMix) },
            { "choRate", s (&S::choRate) }, { "choDepth", s (&S::choDepth) }, { "choMix", s (&S::choMix) },
            { "dlyTime", s (&S::dlyTime) }, { "dlyFb", s (&S::dlyFb) }, { "dlyMix", s (&S::dlyMix) },
            { "revSize", s (&S::revSize) }, { "reverb", s (&S::reverb) },
            { "compThresh", s (&S::compThresh) }, { "compRatio", s (&S::compRatio) }, { "compGain", s (&S::compGain) },
            { "volume", s (&S::volume) },
        };
        if (id.startsWith ("modAmt"))
        {
            const auto slot = (size_t) id.getTrailingIntValue();
            return [slot] (P& x, S&, double v) { x.modAmt[slot] = v; };
        }
        return table.at (id);
    }
}

//==============================================================================
juce::NormalisableRange<float> LoamCharmorProcessor::rangeFor (const KnobDef& k)
{
    const auto mn = (float) k.min, mx = (float) k.max;
    if (k.scale == Scale::log)
        return { mn, mx,
                 [] (float start, float end, float t) { return start * std::pow (end / start, t); },
                 [] (float start, float end, float v) { return std::log (v / start) / std::log (end / start); },
                 nullptr };
    if (k.scale == Scale::curve2)   // finer at the low end, as on the web
        return { mn, mx,
                 [] (float start, float end, float t) { return start + t * t * (end - start); },
                 [] (float start, float end, float v) { return std::sqrt (juce::jmax (0.0f, (v - start) / (end - start))); },
                 nullptr };
    return { mn, mx, (float) k.step };
}

const KnobDef* LoamCharmorProcessor::findKnob (const juce::String& id)
{
    for (const auto& k : knobDefs())
        if (id == k.id)
            return &k;
    return nullptr;
}

juce::AudioProcessorValueTreeState::ParameterLayout LoamCharmorProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (const auto& k : knobDefs())
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { k.id, 1 }, k.label, rangeFor (k), (float) k.def));
    for (const auto& c : choiceDefs())
    {
        juce::StringArray labels;
        for (auto* l : c.labels) labels.add (juce::String (juce::CharPointer_UTF8 (l)));
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { c.id, 1 }, c.name, labels, c.def));
    }
    for (const auto& t : toggleDefs())
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { t.id, 1 }, t.name, t.def));
    layout.add (std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "bendRange", 1 }, "Bend Range", 1, 12, 2));
    return layout;
}

LoamCharmorProcessor::LoamCharmorProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "LoamElfCharmor", createLayout())
{
    for (const auto& k : knobDefs())
    {
        auto knob = std::make_unique<Knob>();
        knob->def = &k;
        knob->param = apvts.getParameter (k.id);
        knob->value = apvts.getRawParameterValue (k.id);
        knob->apply = applierFor (k.id);
        knobById[k.id] = knob.get();
        knobs.push_back (std::move (knob));
    }
    for (const auto& c : choiceDefs()) choiceValues[c.id] = apvts.getRawParameterValue (c.id);
    for (const auto& t : toggleDefs()) toggleValues[t.id] = apvts.getRawParameterValue (t.id);
    bendRange = apvts.getRawParameterValue ("bendRange");
    waveA = choiceValues["waveA"]; waveB = choiceValues["waveB"]; ftype = choiceValues["ftype"];
    l1Shape = choiceValues["l1Shape"]; l2Shape = choiceValues["l2Shape"]; modMode = choiceValues["modMode"];
    for (int i = 0; i < modSlots; ++i)
    {
        modSrc[(size_t) i] = choiceValues["modSrc" + juce::String (i)];
        modDst[(size_t) i] = choiceValues["modDst" + juce::String (i)];
    }
    l1Retrig = toggleValues["l1Retrig"]; l2Retrig = toggleValues["l2Retrig"]; imgLoop = toggleValues["imgLoop"];
    fxDist = toggleValues["fxDist"]; fxCho = toggleValues["fxCho"]; fxDly = toggleValues["fxDly"];
    fxRev = toggleValues["fxRev"]; fxComp = toggleValues["fxComp"];
    rememberPreset ("Init Saw");
}

bool LoamCharmorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void LoamCharmorProcessor::prepareToPlay (double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    engine.prepare (sampleRate);
    fx.prepare (sampleRate, samplesPerBlock);
    monoScratch.setSize (1, juce::jmax (1, samplesPerBlock));
}

//==============================================================================
void LoamCharmorProcessor::buildParams (Params& p, FxChain::Settings& s) const
{
    const auto custom = isCustomActive();
    const auto wheel = modWheel.load();
    for (const auto& k : knobs)
    {
        auto v = (double) k->value->load();
        if (custom && k->bound.load())   // CUST: the wheel moves the knob between its dots
        {
            const auto red = k->red.load(), green = k->green.load();
            v = k->param->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, red + (green - red) * wheel));
        }
        k->apply (p, s, v);
    }
    auto choice = [] (const std::atomic<float>* v) { return (int) v->load(); };
    p.waveA = choice (waveA); p.waveB = choice (waveB); p.ftype = choice (ftype);
    p.l1Shape = choice (l1Shape); p.l2Shape = choice (l2Shape); p.modMode = choice (modMode);
    for (size_t i = 0; i < (size_t) modSlots; ++i)
    {
        p.modSrc[i] = choice (modSrc[i]);
        p.modDst[i] = choice (modDst[i]);
    }
    auto toggle = [] (const std::atomic<float>* v) { return v->load() >= 0.5f; };
    p.l1Retrig = toggle (l1Retrig); p.l2Retrig = toggle (l2Retrig); p.imgLoop = toggle (imgLoop);
    s.dist = toggle (fxDist); s.cho = toggle (fxCho); s.dly = toggle (fxDly); s.rev = toggle (fxRev); s.comp = toggle (fxComp);
    p.bendRange = bendRange->load();
}

charmor::Params LoamCharmorProcessor::heardParams() const
{
    Params p;
    FxChain::Settings s;
    buildParams (p, s);
    return p;
}

void LoamCharmorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const auto n = buffer.getNumSamples();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);   // the on-screen keys join the MIDI

    Params p;
    FxChain::Settings s;
    buildParams (p, s);
    engine.setParams (p);
    fx.setSettings (s);
    if (fx.reverbNeedsSize (s.revSize) && wantedReverbSize.exchange (s.revSize) != s.revSize)
        triggerAsyncUpdate();

    auto applySustain = [this] { engine.setSustain (midiSustain || uiSustain.load()); };
    applySustain();

    // Two channels to render into, even for a mono output.
    float* left = buffer.getWritePointer (0);
    float* right = nullptr;
    if (buffer.getNumChannels() >= 2)
        right = buffer.getWritePointer (1);
    else
    {
        if (monoScratch.getNumSamples() < n)
            monoScratch.setSize (1, n, false, false, true);   // only if the host sends more than it promised
        right = monoScratch.getWritePointer (0);
    }

    int at = 0;
    for (const auto metadata : midi)
    {
        const auto pos = juce::jlimit (0, n, metadata.samplePosition);
        if (pos > at) { engine.render (left + at, right + at, pos - at); at = pos; }
        const auto msg = metadata.getMessage();
        if (msg.isNoteOn()) { engine.noteOn (msg.getNoteNumber(), msg.getFloatVelocity()); lastNote = msg.getNoteNumber(); }
        else if (msg.isNoteOff()) engine.noteOff (msg.getNoteNumber());
        else if (msg.isSustainPedalOn() || msg.isSustainPedalOff()) { midiSustain = msg.isSustainPedalOn(); applySustain(); }
        else if (msg.isPitchWheel()) bendWheel = juce::jlimit (-1.0f, 1.0f, (float) (msg.getPitchWheelValue() - 8192) / 8191.0f);
        else if (msg.isControllerOfType (1)) modWheel = (float) msg.getControllerValue() / 127.0f;
        else if (msg.isAllNotesOff() || msg.isAllSoundOff()) engine.allNotesOff();
        engine.setBend (bendWheel.load());
        engine.setWheel (modWheel.load());
    }
    engine.setBend (bendWheel.load());
    engine.setWheel (modWheel.load());
    if (at < n)
        engine.render (left + at, right + at, n - at);
    sustainActive = midiSustain || uiSustain.load();

    fx.process (left, right, n);

    // The egret: two short rising sines (2600 -> 3800 Hz, 3100 -> 4200 Hz), past the effects, as on the web.
    if (chirpPending.exchange (false)) { chirpT = 0; chirpPhase = {}; }
    for (int i = 0; i < n && chirpT >= 0; ++i)
    {
        double bird = 0;
        for (int c = 0; c < 2; ++c)
        {
            const auto local = chirpT - 0.11 * c;
            if (local < 0) continue;
            const auto f0 = 2600.0 + 500 * c, f1 = 3800.0 + 400 * c;
            const auto f = local >= 0.06 ? f1 : f0 * std::pow (f1 / f0, local / 0.06);
            const auto env = local < 0.002 ? 0.0001 + 0.1199 * local / 0.002 : local < 0.08 ? 0.12 * std::pow (0.0001 / 0.12, (local - 0.002) / 0.078) : 0.0;
            bird += std::sin (2 * pi * chirpPhase[(size_t) c]) * env;
            chirpPhase[(size_t) c] = std::fmod (chirpPhase[(size_t) c] + f / sampleRate, 1.0);
        }
        left[i] += (float) bird; right[i] += (float) bird;
        chirpT += 1 / sampleRate;
        if (chirpT > 0.25) chirpT = -1;
    }
    if (buffer.getNumChannels() == 1)
        for (int i = 0; i < n; ++i) left[i] = 0.5f * (left[i] + right[i]);
}

void LoamCharmorProcessor::handleAsyncUpdate()
{
    fx.buildReverb (wantedReverbSize.load());
}

//==============================================================================
LoamCharmorProcessor::Binding LoamCharmorProcessor::getBinding (const juce::String& id) const
{
    const auto it = knobById.find (id);
    if (it == knobById.end() || ! it->second->bound.load())
        return {};
    return { true, it->second->reversed.load(), it->second->red.load(), it->second->green.load() };
}

void LoamCharmorProcessor::setBinding (const juce::String& id, Binding b)
{
    if (const auto it = knobById.find (id); it != knobById.end())
    {
        it->second->red = b.red;
        it->second->green = b.green;
        it->second->reversed = b.reversed;
        it->second->bound = b.bound;
    }
}

bool LoamCharmorProcessor::isCustomActive() const { return (int) modMode->load() == modCustom; }

float LoamCharmorProcessor::getHeardNormalised (const juce::String& id) const
{
    const auto it = knobById.find (id);
    if (it == knobById.end())
        return 0.0f;
    const auto& k = *it->second;
    if (isCustomActive() && k.bound.load())
        return juce::jlimit (0.0f, 1.0f, k.red.load() + (k.green.load() - k.red.load()) * modWheel.load());
    return k.param->getValue();
}

//==============================================================================
void LoamCharmorProcessor::setImage (std::vector<unsigned char> bytes, int cols, const juce::String& name)
{
    if (bytes.empty() || cols <= 0 || bytes.size() != (size_t) cols * imageRows)
    {
        std::atomic_store (&image, std::shared_ptr<const Image>());
        imageName = {};
    }
    else
    {
        auto img = std::make_shared<Image>();
        img->cols = cols;
        img->bytes = std::move (bytes);
        img->amps.resize (img->bytes.size());
        for (size_t i = 0; i < img->bytes.size(); ++i)
            img->amps[i] = (float) imageByteToAmp (img->bytes[i]);
        std::atomic_store (&image, std::shared_ptr<const Image> (img));
        imageName = name;
    }
    engine.setImage (std::atomic_load (&image));
}

std::unique_ptr<juce::XmlElement> LoamCharmorProcessor::extrasToXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("EXTRAS");
    auto* cust = xml->createNewChildElement (custTag.toString());
    for (const auto& k : knobs)
        if (k->bound.load())
        {
            auto* e = cust->createNewChildElement ("BIND");
            e->setAttribute ("id", k->def->id);
            e->setAttribute ("red", k->red.load());
            e->setAttribute ("green", k->green.load());
            e->setAttribute ("reversed", k->reversed.load());
        }
    if (auto img = getImage())
    {
        auto* e = xml->createNewChildElement (imageTag.toString());
        e->setAttribute ("cols", img->cols);
        e->setAttribute ("name", imageName);
        juce::MemoryBlock data (img->bytes.data(), img->bytes.size());
        e->setAttribute ("data", data.toBase64Encoding());
    }
    return xml;
}

void LoamCharmorProcessor::extrasFromXml (const juce::XmlElement* xml)
{
    for (const auto& k : knobs)
        k->bound = false;
    if (xml == nullptr)
    {
        clearImage();
        return;
    }
    if (auto* cust = xml->getChildByName (custTag.toString()))
        for (auto* e : cust->getChildIterator())
            setBinding (e->getStringAttribute ("id"),
                        { true, e->getBoolAttribute ("reversed"), (float) e->getDoubleAttribute ("red"), (float) e->getDoubleAttribute ("green", 1.0) });
    if (auto* e = xml->getChildByName (imageTag.toString()))
    {
        juce::MemoryBlock data;
        data.fromBase64Encoding (e->getStringAttribute ("data"));
        std::vector<unsigned char> bytes ((const unsigned char*) data.getData(), (const unsigned char*) data.getData() + data.getSize());
        setImage (std::move (bytes), e->getIntAttribute ("cols"), e->getStringAttribute ("name"));
    }
    else
        clearImage();
}

//==============================================================================
void LoamCharmorProcessor::setParam (const juce::String& id, float value)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (value));
        p->endChangeGesture();
    }
}

void LoamCharmorProcessor::resetToDefaults()
{
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            ranged->beginChangeGesture();
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
            ranged->endChangeGesture();
        }
    for (const auto& k : knobs)
        k->bound = false;
    clearImage();
}

// Applies a factory preset's "key:value" changes, the web version's notation.
void LoamCharmorProcessor::applyChanges (const juce::String& changes)
{
    for (const auto& token : juce::StringArray::fromTokens (changes, " ", ""))
    {
        const auto key = token.upToFirstOccurrenceOf (":", false, false);
        const auto value = token.fromFirstOccurrenceOf (":", false, false);
        if (key.isEmpty())
            continue;
        if (key == "image" && value == "stairs")
        {
            int cols = 0;
            auto bytes = stairImage (cols);
            setImage (std::move (bytes), cols, "Overtone stairs");
        }
        else if (key.startsWith ("cust."))   // a knob on the mod wheel, from/to in knob units
        {
            const auto id = key.fromFirstOccurrenceOf (".", false, false);
            if (auto* p = apvts.getParameter (id))
                setBinding (id, { true, false, p->convertTo0to1 (value.upToFirstOccurrenceOf ("/", false, false).getFloatValue()),
                                  p->convertTo0to1 (value.fromFirstOccurrenceOf ("/", false, false).getFloatValue()) });
        }
        else if (auto it = choiceValues.find (key); it != choiceValues.end())
        {
            for (const auto& c : choiceDefs())
                if (key == c.id)
                    for (int i = 0; i < (int) c.keys.size(); ++i)
                        if (value == c.keys[(size_t) i])
                            setParam (key, (float) i);
        }
        else if (toggleValues.count (key) != 0)
            setParam (key, value == "true" ? 1.0f : 0.0f);
        else
            setParam (key, value.getFloatValue());
    }
}

void LoamCharmorProcessor::rememberPreset (const juce::String& name)
{
    presetName = name;
    presetSnapshot.clear();
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            presetSnapshot[ranged->getParameterID()] = ranged->getValue();
}

bool LoamCharmorProcessor::isPresetEdited() const
{
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (const auto it = presetSnapshot.find (ranged->getParameterID()); it == presetSnapshot.end() || std::abs (it->second - ranged->getValue()) > 1e-4f)
                return true;
    return false;
}

void LoamCharmorProcessor::loadFactoryPreset (int index)
{
    const auto& list = factoryPresets();
    const auto& preset = list[(size_t) juce::jlimit (0, (int) list.size() - 1, index)];
    resetToDefaults();
    applyChanges (preset.changes);
    rememberPreset (preset.name);
}

juce::File LoamCharmorProcessor::getUserPresetFolder()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("Loam").getChildFile ("Loam Elf-cHarmor").getChildFile ("Presets");
}

namespace
{
    juce::File findPreset (const juce::String& name)
    {
        for (const auto& file : LoamCharmorProcessor::getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
            if (auto xml = juce::XmlDocument::parse (file); xml != nullptr && xml->hasTagName (presetTag)
                    && xml->getStringAttribute ("name").equalsIgnoreCase (name))
                return file;
        return {};
    }
}

juce::StringArray LoamCharmorProcessor::getUserPresetNames() const
{
    juce::StringArray names;
    for (const auto& file : getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.xml"))
        if (auto xml = juce::XmlDocument::parse (file); xml != nullptr && xml->hasTagName (presetTag))
            names.addIfNotAlreadyThere (xml->getStringAttribute ("name"), true);
    names.sortNatural();
    return names;
}

bool LoamCharmorProcessor::saveUserPreset (const juce::String& name)
{
    juce::XmlElement xml (presetTag);
    xml.setAttribute ("name", name);
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            auto* e = xml.createNewChildElement ("PARAM");
            e->setAttribute ("id", ranged->getParameterID());
            e->setAttribute ("value", ranged->convertFrom0to1 (ranged->getValue()));
        }
    xml.addChildElement (extrasToXml().release());

    const auto folder = getUserPresetFolder();
    if (! folder.createDirectory())
        return false;
    auto file = findPreset (name);
    if (file == juce::File())
        file = folder.getNonexistentChildFile (juce::File::createLegalFileName (name), ".xml");
    if (! xml.writeTo (file))
        return false;
    rememberPreset (name);
    return true;
}

bool LoamCharmorProcessor::loadUserPreset (const juce::String& name)
{
    auto xml = juce::XmlDocument::parse (findPreset (name));
    if (xml == nullptr)
        return false;
    resetToDefaults();
    for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
        setParam (e->getStringAttribute ("id"), (float) e->getDoubleAttribute ("value"));
    extrasFromXml (xml->getChildByName ("EXTRAS"));
    rememberPreset (xml->getStringAttribute ("name"));
    return true;
}

bool LoamCharmorProcessor::deleteUserPreset (const juce::String& name)
{
    const auto file = findPreset (name);
    return file != juce::File() && file.deleteFile();
}

//==============================================================================
void LoamCharmorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty (presetProp, presetName, nullptr);
    if (auto xml = state.createXml())
    {
        xml->addChildElement (extrasToXml().release());
        copyXmlToBinary (*xml, destData);
    }
}

void LoamCharmorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;
    extrasFromXml (xml->getChildByName ("EXTRAS"));
    if (auto* extras = xml->getChildByName ("EXTRAS"))
        xml->removeChildElement (extras, true);
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    rememberPreset (apvts.state.getProperty (presetProp, "Init Saw").toString());
}

juce::AudioProcessorEditor* LoamCharmorProcessor::createEditor()
{
    return new LoamCharmorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LoamCharmorProcessor();
}
