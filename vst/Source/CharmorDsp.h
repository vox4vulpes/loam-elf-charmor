#pragma once

#include <array>
#include <cmath>
#include <vector>

// The shared part of the web version's DSP (index.html, <script id="dsp-shared">),
// line for line: harmonic recipes, the spectral filter, prism, phaser, blur
// and the image. The engine plays with it; the editor draws with it.
namespace charmor
{
    constexpr int maxPartials = 256, modSlots = 6;
    constexpr double pi = 3.14159265358979323846;

    enum Wave { waveSin, waveSaw, waveSqr, waveTri, wavePulse, waveOrgan, numWaves };
    enum FilterType { filterLp, filterBp, filterHp };
    enum LfoShape { lfoSin, lfoTri, lfoSaw, lfoSqr, lfoRnd };
    enum ModMode { modVib, modFilter, modCustom, modMatrix };
    enum ModSource { srcNone, srcEnv2, srcEnv3, srcLfo1, srcLfo2, srcVel, srcWheel, numSources };
    enum ModTarget { dstPitch, dstCutoff, dstRes, dstMorph, dstOddEven, dstPrism, dstPhOffset,
                     dstBlurHarm, dstPluck, dstDetune, dstVolume, dstPan, numTargets };

    // Everything the voices need, in the web version's units.
    struct Params
    {
        int waveA = waveSaw, waveB = waveSqr;
        double morph = 0, oddEven = 0, pluck = 0;
        int ftype = filterLp;
        double cutoff = 4000, res = 20, envAmt = 30, keytrack = 50;
        double fA = 5, fD = 400, fS = 30, fR = 400;
        double aA = 5, aD = 600, aS = 80, aR = 300;
        double uniVoices = 1, uniDetune = 15, uniSpread = 50;
        double prism = 0, prismEnv = 0;
        double phDepth = 0, phNotches = 2, phOffset = 0, phSpeed = 0;
        double harmVoices = 0, harmInterval = 7, harmLevel = 70;
        double blurTime = 0, blurHarm = 0;
        int l1Shape = lfoSin; double l1Rate = 4, l1Delay = 0; bool l1Retrig = true;
        int l2Shape = lfoTri; double l2Rate = 0.5, l2Delay = 0; bool l2Retrig = false;
        double e2A = 5, e2D = 500, e2S = 0, e2R = 300;
        double e3A = 300, e3D = 1000, e3S = 50, e3R = 800;
        std::array<int, modSlots> modSrc { srcNone, srcNone, srcNone, srcNone, srcNone, srcNone };
        std::array<int, modSlots> modDst { dstCutoff, dstPitch, dstMorph, dstPrism, dstVolume, dstPan };
        std::array<double, modSlots> modAmt {};
        double imgAmount = 100, imgSpeed = 4; bool imgLoop = true;
        int modMode = modVib; double bendRange = 2;
    };

    // Harmonic amplitudes of each timbre, harmonic 1 at full level; index k = harmonic number.
    using Timbres = std::array<std::array<double, maxPartials + 1>, numWaves>;
    inline const Timbres& timbres()
    {
        static const Timbres t = []
        {
            Timbres x {};
            const double organ[17] = { 0, 1, 0.7, 0.5, 0.5, 0, 0.3, 0, 0.35, 0, 0.15, 0, 0.12, 0, 0, 0, 0.1 };
            for (int k = 1; k <= maxPartials; ++k)
            {
                x[waveSin][(size_t) k] = k == 1 ? 1.0 : 0.0;
                x[waveSaw][(size_t) k] = 1.0 / k;
                x[waveSqr][(size_t) k] = k % 2 ? 1.0 / k : 0.0;
                x[waveTri][(size_t) k] = k % 2 ? (((k - 1) / 2) % 2 ? -1.0 : 1.0) / (k * k) : 0.0;
                x[wavePulse][(size_t) k] = std::sin (pi * k * 0.25) / (k * std::sin (pi * 0.25));   // 25% pulse
                x[waveOrgan][(size_t) k] = k <= 16 ? organ[k] : 0.0;
            }
            return x;
        }();
        return t;
    }

    // Amplitude of harmonic k: timbre A morphed into B, then the odd/even balance.
    inline double timbreAt (const Params& p, int k)
    {
        const auto m = p.morph / 100;
        auto a = timbres()[(size_t) p.waveA][(size_t) k] * (1 - m) + timbres()[(size_t) p.waveB][(size_t) k] * m;
        if (p.oddEven < 0 && k % 2 == 0) a *= 1 + p.oddEven / 100;                // fewer evens: hollow
        else if (p.oddEven > 0 && k % 2 == 1 && k > 1) a *= 1 - p.oddEven / 100;   // fewer odds
        return a;
    }

    // The spectral filter: 24 dB/oct slopes; RES adds a peak a fifth of an octave wide.
    inline double filterGain (int type, double f, double fc, double res)
    {
        const auto x = std::log2 (f / fc);
        double g;
        if (type == filterLp)      g = 1 / std::sqrt (1 + std::pow (2.0, 8 * x));
        else if (type == filterHp) g = 1 / std::sqrt (1 + std::pow (2.0, -8 * x));
        else                       g = 1 / std::sqrt (1 + std::pow (2.0, 8 * (std::abs (x) - 0.5)));   // bp: an octave wide
        return g * (1 + res / 100 * 12 * std::exp (-x * x / (2 * 0.15 * 0.15)));
    }

    // Cutoff after the filter envelope (ENV: up to +/- 6 octaves) and key tracking.
    inline double effectiveCutoff (const Params& p, double f0, double fenv)
    {
        const auto fc = p.cutoff * std::pow (2.0, p.envAmt / 100 * 6 * fenv) * std::pow (f0 / 261.63, p.keytrack / 100);
        return std::fmin (20000.0, std::fmax (20.0, fc));
    }

    // PLUCK: harmonic k fades at 3 x (k - 1) x PLUCK per second, so the top dies first.
    inline double pluckGain (const Params& p, int k, double t)
    {
        return p.pluck > 0 ? std::exp (-p.pluck / 100 * 3 * (k - 1) * t) : 1.0;
    }

    // LFO shapes, one cycle over ph 0..1; RND holds a random value per cycle.
    inline double lfoShape (int shape, double ph, double held)
    {
        if (shape == lfoTri) return ph < 0.5 ? 4 * ph - 1 : 3 - 4 * ph;
        if (shape == lfoSaw) return 2 * ph - 1;
        if (shape == lfoSqr) return ph < 0.5 ? 1 : -1;
        if (shape == lfoRnd) return held;
        return std::sin (2 * pi * ph);
    }

    // PRISM: harmonic k moves to k^(1 + s); ENV lets the filter envelope push it.
    inline double prismStretch (const Params& p, double fenv)
    {
        return std::fmax (-0.5, std::fmin (0.5, (p.prism + p.prismEnv * fenv) / 100 * 0.5));
    }
    inline double partialRatio (int k, double s) { return s == 0 ? (double) k : std::pow ((double) k, 1 + s); }

    // PHASER: notches evenly spread in log frequency, NOTCHES per octave, slid by OFFSET and its LFO.
    inline double phaserGain (const Params& p, double f, double lfo)
    {
        if (p.phDepth <= 0)
            return 1;
        const auto x = std::log2 (f / 20) * p.phNotches + p.phOffset / 100 + lfo;
        return 1 - p.phDepth / 100 * (0.5 - 0.5 * std::cos (2 * pi * x));
    }

    // BLUR HARM: three passes mixing every harmonic's level with its neighbours'; signs kept.
    template <typename Array>
    void blurHarmonics (Array& a, int K, double amount, Array& scratch)
    {
        if (amount <= 0)
            return;
        const auto h = amount / 100 * 0.5;
        for (int k = 1; k <= K; ++k) scratch[(size_t) k] = std::abs (a[(size_t) k]);
        for (int n = 0; n < 3; ++n)
        {
            auto prev = scratch[1];
            for (int k = 1; k <= K; ++k)
            {
                const auto cur = scratch[(size_t) k], next = k < K ? scratch[(size_t) k + 1] : 0.0;
                scratch[(size_t) k] = cur * (1 - h) + (prev + next) * 0.5 * h;
                prev = cur;
            }
        }
        for (int k = 1; k <= K; ++k) a[(size_t) k] = (a[(size_t) k] < 0 ? -1 : 1) * scratch[(size_t) k];
    }

    // IMAGE: rows are harmonics 1..256 from the bottom, columns time; brightness spans 60 dB.
    constexpr int imageRows = maxPartials;
    inline double imageByteToAmp (int b) { return b == 0 ? 0.0 : std::pow (10.0, (b / 255.0 - 1) * 3); }

    struct Image
    {
        int cols = 0;
        std::vector<unsigned char> bytes;   // imageRows x cols, row r = harmonic r + 1
        std::vector<float> amps;            // the same as levels
    };

    // Every harmonic's level for a voice: timbre (mixed with its IMAGE column), BLUR HARM,
    // then filter, phaser and PLUCK at the harmonic's actual frequency (after PRISM).
    using Amps = std::array<double, maxPartials + 1>;
    inline void voiceAmps (const Params& p, double f0, int K, double fc, double s, double t, double lfo,
                           Amps& out, Amps& scratch, const Amps* img)
    {
        const auto im = img != nullptr ? p.imgAmount / 100 : 0.0;
        for (int k = 1; k <= maxPartials; ++k)
            out[(size_t) k] = k > K ? 0.0 : timbreAt (p, k) * (1 - im) + (im != 0 ? (*img)[(size_t) k] * im : 0.0);
        blurHarmonics (out, K, p.blurHarm, scratch);
        for (int k = 1; k <= K; ++k)
        {
            const auto f = f0 * partialRatio (k, s);
            out[(size_t) k] *= filterGain (p.ftype, f, fc, p.res) * phaserGain (p, f, lfo) * pluckGain (p, k, t);
        }
    }
}
