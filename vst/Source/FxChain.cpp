#include "FxChain.h"

namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    // Linear-interpolated read, `delay` samples behind the write position.
    float readDelay (const float* data, int size, int write, double delay)
    {
        auto pos = (double) write - delay;
        while (pos < 0) pos += size;
        const auto i0 = (int) pos, i1 = (i0 + 1) % size;
        const auto fr = (float) (pos - i0);
        return data[i0] + (data[i1] - data[i0]) * fr;
    }
}

void FxChain::Lowpass::set (double freq, double sampleRate)
{
    const auto w0 = twoPi * juce::jmin (freq, sampleRate * 0.499) / sampleRate;
    const auto alpha = std::sin (w0) / (2.0 * std::sqrt (0.5));   // Q = 0.7071
    const auto c = std::cos (w0), a0 = 1.0 + alpha;
    b0 = (1.0 - c) / 2.0 / a0; b1 = (1.0 - c) / a0; b2 = b0;
    a1 = -2.0 * c / a0; a2 = (1.0 - alpha) / a0;
}

float FxChain::Lowpass::process (float x)
{
    const auto y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = x; y2 = y1; y1 = y;
    return (float) y;
}

void FxChain::prepare (double sampleRate, int maximumBlockSize)
{
    sr = sampleRate;
    choBuf.setSize (2, (int) (sr * 0.05) + 2);
    choBuf.clear();
    dlyBuf.setSize (2, (int) (sr * 2.0) + 2);
    dlyBuf.clear();
    revBuf.setSize (2, juce::jmax (1, maximumBlockSize));
    reverb.prepare ({ sr, (juce::uint32) juce::jmax (1, maximumBlockSize), 2 });
    first = true;
    setSettings (s);
    buildReverb (s.revSize);
}

void FxChain::buildReverb (double seconds)
{
    // SIZE seconds of stereo noise fading as (1 - x)^3, the web's room, normalised
    // the way a ConvolverNode does by default.
    const auto len = juce::jmax (1, (int) std::floor (sr * seconds));
    juce::AudioBuffer<float> ir (2, len);
    juce::Random random (0x5eed);
    double power = 0;
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* d = ir.getWritePointer (ch);
        for (int i = 0; i < len; ++i)
        {
            d[i] = (random.nextFloat() * 2.0f - 1.0f) * (float) std::pow (1.0 - (double) i / len, 3.0);
            power += (double) d[i] * d[i];
        }
    }
    power = juce::jmax (0.000125, std::sqrt (power / (2.0 * len)));
    ir.applyGain ((float) (1.0 / power * 0.00125 * 44100.0 / sr));
    reverb.loadImpulseResponse (std::move (ir), sr, juce::dsp::Convolution::Stereo::yes,
                                juce::dsp::Convolution::Trim::no, juce::dsp::Convolution::Normalise::no);
    builtSize = seconds;
}

void FxChain::setUnit (Smooth& dry, Smooth& wet, bool on, double dryOn, double wetOn)
{
    if (first) { dry.reset (sr, on ? dryOn : 1); wet.reset (sr, on ? wetOn : 0); }
    else { dry.target = on ? dryOn : 1; wet.target = on ? wetOn : 0; }
}

void FxChain::setSettings (const Settings& settings)
{
    s = settings;
    auto set = [this] (Smooth& sm, double v) { if (first) sm.reset (sr, v); else sm.target = v; };

    // DIST: tanh(g x) / tanh(g), g from 1 to 20, scaled by 1/sqrt(g)
    driveG = 1 + s.distDrive / 100 * 19;
    driveComp = 1 / std::sqrt (driveG);
    for (auto& f : distTone) f.set (s.distTone, sr);
    setUnit (distDry, distWet, s.dist, 1 - s.distMix / 100, s.distMix / 100);

    set (choRate, s.choRate);
    set (choDepth, s.choDepth / 100 * 0.004);
    setUnit (choDry, choWet, s.cho, 1 - s.choMix / 200, s.choMix / 100);

    set (dlyTime, s.dlyTime / 1000);
    set (dlyFb, s.dlyFb / 100 * 0.9);
    for (auto& f : dlyTone) f.set (6000, sr);
    setUnit (dlyDry, dlyWet, s.dly, 1, s.dlyMix / 100);

    setUnit (revDry, revWet, s.rev, 1, s.reverb / 100 * 0.6);

    setUnit (compDry, compWet, s.comp, 0, 1);
    set (makeup, std::pow (10.0, s.compGain / 20));
    set (volume, s.volume / 100);
    first = false;
}

void FxChain::process (float* L, float* R, int n)
{
    const auto tanhG = std::tanh (driveG);
    auto* cbL = choBuf.getWritePointer (0); auto* cbR = choBuf.getWritePointer (1);
    auto* dbL = dlyBuf.getWritePointer (0); auto* dbR = dlyBuf.getWritePointer (1);
    const auto choSize = choBuf.getNumSamples(), dlySize = dlyBuf.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        float l = L[i], r = R[i];

        // DIST
        {
            auto shape = [&] (float x) { return (float) (std::tanh (driveG * juce::jlimit (-1.0f, 1.0f, x)) / tanhG * driveComp); };
            const auto wl = distTone[0].process (shape (l)), wr = distTone[1].process (shape (r));
            const auto d = (float) distDry.next(), w = (float) distWet.next();
            l = l * d + wl * w; r = r * d + wr * w;
        }
        // CHORUS
        {
            cbL[choWrite] = l; cbR[choWrite] = r;
            const auto depth = choDepth.next() * sr, mod = std::sin (twoPi * choPhase);
            choPhase += choRate.next() / sr;
            if (choPhase >= 1) choPhase -= 1;
            const auto base = 0.012 * sr;
            const auto wl = readDelay (cbL, choSize, choWrite, base + depth * mod);
            const auto wr = readDelay (cbR, choSize, choWrite, base - depth * mod);
            choWrite = (choWrite + 1) % choSize;
            const auto d = (float) choDry.next(), w = (float) choWet.next();
            l = l * d + wl * w; r = r * d + wr * w;
        }
        // DELAY
        {
            const auto t = juce::jlimit (1.0, dlySize - 2.0, dlyTime.next() * sr), fb = dlyFb.next();
            const auto el = dlyTone[0].process (readDelay (dbL, dlySize, dlyWrite, t));
            const auto er = dlyTone[1].process (readDelay (dbR, dlySize, dlyWrite, t));
            dbL[dlyWrite] = l + (float) (el * fb); dbR[dlyWrite] = r + (float) (er * fb);
            dlyWrite = (dlyWrite + 1) % dlySize;
            const auto d = (float) dlyDry.next(), w = (float) dlyWet.next();
            l = l * d + el * w; r = r * d + er * w;
        }
        L[i] = l; R[i] = r;
    }

    // REVERB, a block at a time
    for (int start = 0; start < n; start += revBuf.getNumSamples())
    {
        const auto num = juce::jmin (revBuf.getNumSamples(), n - start);
        revBuf.copyFrom (0, 0, L + start, num);
        revBuf.copyFrom (1, 0, R + start, num);
        auto blockView = juce::dsp::AudioBlock<float> (revBuf).getSubBlock (0, (size_t) num);
        reverb.process (juce::dsp::ProcessContextReplacing<float> (blockView));
        for (int i = 0; i < num; ++i)
        {
            const auto d = (float) revDry.next(), w = (float) revWet.next();
            L[start + i] = L[start + i] * d + revBuf.getSample (0, i) * w;
            R[start + i] = R[start + i] * d + revBuf.getSample (1, i) * w;
        }
    }

    // COMP: a soft-knee compressor on the louder side, then make-up and volume
    const auto attack = std::exp (-1.0 / (0.005 * sr)), release = std::exp (-1.0 / (0.2 * sr));
    const auto thr = s.compThresh, ratio = juce::jmax (1.0, s.compRatio), knee = 6.0;
    for (int i = 0; i < n; ++i)
    {
        const auto level = juce::jmax (std::abs (L[i]), std::abs (R[i]));
        const auto db = level > 1e-6f ? 20 * std::log10 ((double) level) : -120.0;
        envDb = db > envDb ? db + (envDb - db) * attack : db + (envDb - db) * release;
        const auto over = envDb - thr;
        double reduce = 0;
        if (over > knee / 2) reduce = over * (1 - 1 / ratio);
        else if (over > -knee / 2) reduce = (1 - 1 / ratio) * (over + knee / 2) * (over + knee / 2) / (2 * knee);
        const auto g = (float) (std::pow (10.0, -reduce / 20) * makeup.next());
        const auto d = (float) compDry.next(), w = (float) compWet.next(), v = (float) volume.next();
        L[i] = (L[i] * d + L[i] * g * w) * v;
        R[i] = (R[i] * d + R[i] * g * w) * v;
    }
}
