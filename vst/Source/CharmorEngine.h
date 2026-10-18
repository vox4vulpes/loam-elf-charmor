#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <juce_core/juce_core.h>
#include "CharmorDsp.h"

// The web version's AudioWorklet (index.html, <script id="dsp-engine">) in
// C++: eight voices, each turning its harmonic levels into one cycle of its
// waveform by inverse FFT every 256 samples (crossfaded), read by every unison
// and harmonizer copy at its own pitch -- or, with PRISM on, played as a bank
// of sine partials. It runs in the worklet's 128-sample blocks, so envelopes,
// LFOs and tables move exactly as they do in the browser.
class CharmorEngine
{
public:
    static constexpr int block = 128, numVoices = 8, maxUnison = 8, maxHarm = 2;
    static constexpr int maxCopies = maxUnison * (maxHarm + 1);
    static constexpr int tableN = 4096, tableBits = 12, updateBlocks = 2, bankBudget = 1024;
    static constexpr double voiceGain = 0.12;

    CharmorEngine();
    void prepare (double sampleRate);
    void setParams (const charmor::Params& p) { params = p; }
    void noteOn (int note, float velocity);
    void noteOff (int note);
    void setSustain (bool);
    void allNotesOff();
    void setBend (float wheel) { bend = wheel; }    // -1..1; BEND RANGE scales it
    void setWheel (float wheel) { modWheel = wheel; }

    // The picture or analysed sound IMAGE plays; null for none. Any thread.
    void setImage (std::shared_ptr<const charmor::Image>);

    // Fills (not adds to) num samples of left and right.
    void render (float* left, float* right, int num);

    // What the screen and the graphs show: the newest voice, refreshed ~30 times a second.
    struct Snapshot
    {
        int voices = 0, note = -1, count = 0;
        double f0 = 0, cutoff = 1000, stretch = 0, lfo = 0, imgPos = 0, t = 0;
        std::array<double, 2> lfoPhase {};
        std::array<double, 4> envLevel {};   // amp, filter, env 2, env 3
        std::array<int, 4> envStage {};
        std::array<float, charmor::maxPartials + 1> amps {};
    };
    Snapshot getSnapshot() const;
    int getActiveVoices() const { return activeVoices.load(); }

private:
    struct Voice
    {
        bool active = false, gate = false, sustained = false, fresh = true, bank = false, highReady = false;
        int note = -1, sinceUpdate = 0, count = 0;
        double vel = 1, t = 0, level = 0;
        std::uint64_t order = 0;
        double amp = 0, fenv = 0, env2 = 0, env3 = 0;
        int ampStage = 0, fStage = 0, e2Stage = 0, e3Stage = 0;
        charmor::Amps amps {}, target {}, scratch {}, bankAmps {}, oldAmps {};
        std::vector<double> prev, next, prevHigh, nextHigh;   // tableN + 1 each
        std::array<double, maxCopies> phase {}, cRatio {}, cL {}, cR {};
        std::vector<double> bankPhase;                         // maxCopies x (maxPartials + 1)
        double f0 = 440, cutoff = 1000, stretch = 0, imgPos = 0;
        std::array<double, 2> lfoPh {}, lfoHeld {}, lfoView {};
        charmor::Params pv;
        double pitch = 0, volume = 1, pan = 0;
    };

    void processBlock (float* left, float* right);
    void release (Voice&);
    void modulate (Voice&, double dt);
    int setupCopies (Voice&);
    const charmor::Amps* imageColumn (Voice&, const charmor::Params&);
    void updateAmps (Voice&, const charmor::Params&, double f0, int K, double s, double dtStep);
    void enterBank (Voice&);
    void renderTable (Voice&, double f0, int copies, float* L, float* R, double nyq);
    void renderBank (Voice&, double f0, double s, int copies, float* L, float* R, double nyq, int active);
    void buildTable (const charmor::Amps& amps, int K, std::vector<double>& out);

    double sampleRate = 44100.0;
    charmor::Params params;
    std::array<Voice, numVoices> voices;
    std::uint64_t counter = 0, blocks = 0;
    bool sustain = false;
    double bend = 0, modWheel = 0, lfo = 0, vibPh = 0;
    std::array<double, 2> gLfoPh {}, gHeld {};
    juce::Random random;

    std::shared_ptr<const charmor::Image> image;
    charmor::Amps imgCol {};

    // FFT and sine tables
    std::vector<int> rev;
    std::vector<double> cosT, sinT, fftRe, fftIm, sine;
    std::array<double, block> buf {};

    // the 128-sample block being handed out
    std::array<float, block> outL {}, outR {};
    int outPos = block;

    mutable juce::SpinLock snapshotLock;
    Snapshot snapshot;
    std::atomic<int> activeVoices { 0 };
};
