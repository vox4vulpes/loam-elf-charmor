#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

// The web version's FX page as a chain: DIST -> CHORUS -> DELAY -> REVERB ->
// COMP -> volume. Each unit is dry + wet; off means dry only.
class FxChain
{
public:
    struct Settings
    {
        bool dist = false; double distDrive = 40, distTone = 6000, distMix = 50;
        bool cho = false;  double choRate = 0.6, choDepth = 40, choMix = 50;
        bool dly = false;  double dlyTime = 350, dlyFb = 35, dlyMix = 25;
        bool rev = true;   double revSize = 2.4, reverb = 20;
        bool comp = false; double compThresh = -18, compRatio = 4, compGain = 0;
        double volume = 70;
    };

    void prepare (double sampleRate, int maximumBlockSize);
    void setSettings (const Settings&);
    void process (float* left, float* right, int numSamples);
    bool isReverbReady() const { return reverb.getCurrentIRSize() > 0; }

private:
    struct Smooth   // the web's setTargetAtTime(value, now, 0.01)
    {
        double value = 0, target = 0, coeff = 0;
        void reset (double sr, double v) { coeff = std::exp (-1.0 / (0.01 * sr)); value = target = v; }
        double next() { value = target + (value - target) * coeff; return value; }
    };
    struct Lowpass   // Web Audio's lowpass biquad with Q = -3.0103 dB: Butterworth
    {
        double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        void set (double freq, double sr);
        float process (float x);
    };

    void buildReverb (double seconds);
    void setUnit (Smooth& dry, Smooth& wet, bool on, double dryOn, double wetOn);

    double sr = 44100.0;
    Settings s;
    bool first = true;

    // DIST
    double driveG = 1, driveComp = 1;
    std::array<Lowpass, 2> distTone;
    Smooth distDry, distWet;

    // CHORUS: a 12 ms delay per side swept +-depth in opposite directions
    juce::AudioBuffer<float> choBuf;
    int choWrite = 0;
    double choPhase = 0;
    Smooth choDry, choWet, choDepth, choRate;

    // DELAY: echoes darkening as they repeat
    juce::AudioBuffer<float> dlyBuf;
    int dlyWrite = 0;
    std::array<Lowpass, 2> dlyTone;
    Smooth dlyDry, dlyWet, dlyTime, dlyFb;

    // REVERB
    juce::dsp::Convolution reverb;
    juce::AudioBuffer<float> revBuf;
    double builtSize = -1;
    Smooth revDry, revWet;

    // COMP
    double envDb = -120;
    Smooth compDry, compWet, makeup;

    Smooth volume;
};
