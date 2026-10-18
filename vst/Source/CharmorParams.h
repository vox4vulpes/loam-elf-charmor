#pragma once

#include <vector>
#include "CharmorDsp.h"

// The web version's parameters (index.html: DEFAULTS, KNOBS, FACTORY), shared
// by the processor (as plugin parameters) and the editor (labels, units).
namespace charmor
{
    enum class Unit { pct, plusMinus, hz, hzFine, ms, cents, semitones, perOct, seconds, dB, ratio, none };
    enum class Scale { linear, log, curve2 };   // curve2: finer at the low end (value ~ t^2)

    struct KnobDef
    {
        const char* id;
        const char* label;
        double min, max, def;
        Unit unit;
        Scale scale = Scale::linear;
        double step = 0;
    };

    inline const std::vector<KnobDef>& knobDefs()
    {
        static const std::vector<KnobDef> list {
            { "morph", "Morph", 0, 100, 0, Unit::pct },
            { "oddEven", "Odd/Even", -100, 100, 0, Unit::plusMinus },
            { "pluck", "Pluck", 0, 100, 0, Unit::pct },
            { "cutoff", "Cutoff", 40, 20000, 4000, Unit::hz, Scale::log },
            { "res", "Res", 0, 100, 20, Unit::pct },
            { "envAmt", "Env", -100, 100, 30, Unit::plusMinus },
            { "keytrack", "Key", 0, 100, 50, Unit::pct },
            { "fA", "Attack", 1, 4000, 5, Unit::ms, Scale::log },
            { "fD", "Decay", 10, 4000, 400, Unit::ms, Scale::log },
            { "fS", "Sustain", 0, 100, 30, Unit::pct },
            { "fR", "Release", 10, 6000, 400, Unit::ms, Scale::log },
            { "aA", "Attack", 1, 4000, 5, Unit::ms, Scale::log },
            { "aD", "Decay", 10, 4000, 600, Unit::ms, Scale::log },
            { "aS", "Sustain", 0, 100, 80, Unit::pct },
            { "aR", "Release", 10, 6000, 300, Unit::ms, Scale::log },
            { "uniVoices", "Voices", 1, 8, 1, Unit::none, Scale::linear, 1 },
            { "uniDetune", "Detune", 0, 100, 15, Unit::cents },
            { "uniSpread", "Spread", 0, 100, 50, Unit::pct },
            { "prism", "Amount", -100, 100, 0, Unit::plusMinus },
            { "prismEnv", "Env", -100, 100, 0, Unit::plusMinus },
            { "phDepth", "Depth", 0, 100, 0, Unit::pct },
            { "phNotches", "Notches", 0.25, 8, 2, Unit::perOct, Scale::log },
            { "phOffset", "Offset", 0, 100, 0, Unit::pct },
            { "phSpeed", "Speed", 0, 4, 0, Unit::hzFine, Scale::curve2 },
            { "harmVoices", "Voices", 0, 2, 0, Unit::none, Scale::linear, 1 },
            { "harmInterval", "Interval", -12, 12, 7, Unit::semitones, Scale::linear, 1 },
            { "harmLevel", "Level", 0, 100, 70, Unit::pct },
            { "blurTime", "Time", 0, 3000, 0, Unit::ms, Scale::curve2 },
            { "blurHarm", "Harm", 0, 100, 0, Unit::pct },
            { "l1Rate", "Rate", 0.05, 20, 4, Unit::hzFine, Scale::log },
            { "l1Delay", "Fade in", 0, 2000, 0, Unit::ms, Scale::curve2 },
            { "l2Rate", "Rate", 0.05, 20, 0.5, Unit::hzFine, Scale::log },
            { "l2Delay", "Fade in", 0, 2000, 0, Unit::ms, Scale::curve2 },
            { "e2A", "Attack", 1, 4000, 5, Unit::ms, Scale::log },
            { "e2D", "Decay", 10, 4000, 500, Unit::ms, Scale::log },
            { "e2S", "Sustain", 0, 100, 0, Unit::pct },
            { "e2R", "Release", 10, 6000, 300, Unit::ms, Scale::log },
            { "e3A", "Attack", 1, 4000, 300, Unit::ms, Scale::log },
            { "e3D", "Decay", 10, 4000, 1000, Unit::ms, Scale::log },
            { "e3S", "Sustain", 0, 100, 50, Unit::pct },
            { "e3R", "Release", 10, 6000, 800, Unit::ms, Scale::log },
            { "modAmt0", "Amount", -100, 100, 0, Unit::plusMinus },
            { "modAmt1", "Amount", -100, 100, 0, Unit::plusMinus },
            { "modAmt2", "Amount", -100, 100, 0, Unit::plusMinus },
            { "modAmt3", "Amount", -100, 100, 0, Unit::plusMinus },
            { "modAmt4", "Amount", -100, 100, 0, Unit::plusMinus },
            { "modAmt5", "Amount", -100, 100, 0, Unit::plusMinus },
            { "imgAmount", "Amount", 0, 100, 100, Unit::pct },
            { "imgSpeed", "Time", 0.1, 30, 4, Unit::seconds, Scale::log },
            { "distDrive", "Drive", 0, 100, 40, Unit::pct },
            { "distTone", "Tone", 500, 16000, 6000, Unit::hz, Scale::log },
            { "distMix", "Mix", 0, 100, 50, Unit::pct },
            { "choRate", "Rate", 0.05, 5, 0.6, Unit::hzFine, Scale::log },
            { "choDepth", "Depth", 0, 100, 40, Unit::pct },
            { "choMix", "Mix", 0, 100, 50, Unit::pct },
            { "dlyTime", "Time", 20, 1500, 350, Unit::ms, Scale::log },
            { "dlyFb", "Feedbk", 0, 95, 35, Unit::pct },
            { "dlyMix", "Mix", 0, 100, 25, Unit::pct },
            { "revSize", "Size", 0.3, 8, 2.4, Unit::seconds, Scale::log },
            { "reverb", "Mix", 0, 100, 20, Unit::pct },
            { "compThresh", "Thresh", -60, 0, -18, Unit::dB },
            { "compRatio", "Ratio", 1, 20, 4, Unit::ratio, Scale::log },
            { "compGain", "Gain", 0, 24, 0, Unit::dB },
            { "volume", "Volume", 0, 100, 70, Unit::pct },
        };
        return list;
    }

    struct ChoiceDef
    {
        const char* id;
        const char* name;
        std::vector<const char*> keys;     // the web version's values
        std::vector<const char*> labels;
        int def;
    };

    inline const std::vector<ChoiceDef>& choiceDefs()
    {
        static const std::vector<const char*> waves { "sin", "saw", "sqr", "tri", "pulse", "organ" };
        static const std::vector<const char*> waveLabels { "SIN", "SAW", "SQR", "TRI", "PULSE", "ORGAN" };
        static const std::vector<const char*> shapes { "sin", "tri", "saw", "sqr", "rnd" };
        static const std::vector<const char*> shapeLabels { "SIN", "TRI", "SAW", "SQR", "RND" };
        static const std::vector<const char*> sources { "none", "env2", "env3", "lfo1", "lfo2", "vel", "wheel" };
        static const std::vector<const char*> sourceLabels { "\xe2\x80\x94", "ENV 2", "ENV 3", "LFO 1", "LFO 2", "VELOCITY", "MOD WHEEL" };
        static const std::vector<const char*> targets { "pitch", "cutoff", "res", "morph", "oddEven", "prism", "phOffset", "blurHarm", "pluck", "uniDetune", "volume", "pan" };
        static const std::vector<const char*> targetLabels { "Pitch", "Cutoff", "Res", "Morph", "Odd/Even", "Prism", "Phaser offset", "Blur harm", "Pluck", "Detune", "Volume", "Pan" };
        static const std::vector<ChoiceDef> list {
            { "waveA", "Timbre A", waves, waveLabels, 1 },
            { "waveB", "Timbre B", waves, waveLabels, 2 },
            { "ftype", "Filter Type", { "lp", "bp", "hp" }, { "LP", "BP", "HP" }, 0 },
            { "l1Shape", "LFO 1 Shape", shapes, shapeLabels, 0 },
            { "l2Shape", "LFO 2 Shape", shapes, shapeLabels, 1 },
            { "modMode", "Mod Wheel Mode", { "vib", "filter", "custom", "matrix" }, { "VIB", "FILT", "CUST", "MTRX" }, 0 },
            { "modSrc0", "Slot 1 Source", sources, sourceLabels, 0 }, { "modDst0", "Slot 1 Target", targets, targetLabels, 1 },
            { "modSrc1", "Slot 2 Source", sources, sourceLabels, 0 }, { "modDst1", "Slot 2 Target", targets, targetLabels, 0 },
            { "modSrc2", "Slot 3 Source", sources, sourceLabels, 0 }, { "modDst2", "Slot 3 Target", targets, targetLabels, 3 },
            { "modSrc3", "Slot 4 Source", sources, sourceLabels, 0 }, { "modDst3", "Slot 4 Target", targets, targetLabels, 5 },
            { "modSrc4", "Slot 5 Source", sources, sourceLabels, 0 }, { "modDst4", "Slot 5 Target", targets, targetLabels, 10 },
            { "modSrc5", "Slot 6 Source", sources, sourceLabels, 0 }, { "modDst5", "Slot 6 Target", targets, targetLabels, 11 },
        };
        return list;
    }

    struct ToggleDef { const char* id; const char* name; bool def; };
    inline const std::vector<ToggleDef>& toggleDefs()
    {
        static const std::vector<ToggleDef> list {
            { "l1Retrig", "LFO 1 Retrig", true }, { "l2Retrig", "LFO 2 Retrig", false }, { "imgLoop", "Image Loop", true },
            { "fxDist", "Dist On", false }, { "fxCho", "Chorus On", false }, { "fxDly", "Delay On", false },
            { "fxRev", "Reverb On", true }, { "fxComp", "Comp On", false },
        };
        return list;
    }

    // The factory presets, written as on the web: changes from the defaults.
    // "cust.<knob>:<from>/<to>" puts a knob on the mod wheel (in knob units),
    // "image:stairs" is Spectral Arp's overtone staircase.
    struct FactoryPreset { const char* name; const char* changes; };
    inline const std::vector<FactoryPreset>& factoryPresets()
    {
        static const std::vector<FactoryPreset> list {
            { "Init Saw", "" },
            { "Glass Pad", "waveA:sin waveB:organ morph:45 cutoff:2600 res:30 envAmt:20 fA:800 fD:2000 fS:60 aA:600 aD:1500 aS:80 aR:1800 uniVoices:4 uniDetune:12 uniSpread:80 reverb:45" },
            { "Harp Pluck", "waveA:saw pluck:70 cutoff:9000 res:10 envAmt:0 aA:2 aD:1800 aS:0 aR:900 reverb:30" },
            { "Hollow Lead", "waveA:sqr waveB:pulse morph:30 oddEven:-40 cutoff:3000 res:45 envAmt:25 fD:600 aA:5 aD:400 aS:85 aR:200 uniVoices:2 uniDetune:8" },
            { "Supersaw", "waveA:saw cutoff:7000 res:10 envAmt:10 uniVoices:8 uniDetune:25 uniSpread:90 aR:500 reverb:25 fxDly:true dlyTime:375 dlyFb:30 dlyMix:18" },
            { "Drawbar Organ", "waveA:organ cutoff:12000 res:0 envAmt:0 keytrack:0 aA:5 aD:100 aS:100 aR:80 reverb:25" },
            { "Vowel Sweep", "waveA:saw ftype:bp cutoff:700 res:70 envAmt:45 keytrack:30 fA:400 fD:1400 fS:40 fR:900 aA:20 aD:800 aS:80 aR:600 uniVoices:2 uniDetune:6 reverb:30" },
            { "Sub Bass", "waveA:sin waveB:tri morph:35 cutoff:800 res:0 envAmt:15 keytrack:0 aA:2 aD:300 aS:90 aR:120 reverb:0" },
            { "Bell Partials", "waveA:organ waveB:pulse morph:40 oddEven:30 pluck:35 cutoff:6000 res:15 envAmt:0 aA:2 aD:2500 aS:0 aR:2000 uniVoices:3 uniDetune:5 reverb:40" },
            { "Prism Bells", "waveA:saw prism:45 pluck:20 cutoff:9000 res:10 envAmt:0 aA:2 aD:3000 aS:0 aR:2500 reverb:45" },
            { "Metal Sweep", "waveA:sqr prism:30 prismEnv:40 cutoff:6000 envAmt:20 fA:5 fD:1500 fS:0 fR:800 aD:2000 aS:40 aR:900 reverb:30" },
            { "Phaser Pad", "waveA:saw waveB:organ morph:30 phDepth:80 phNotches:1.5 phSpeed:0.15 cutoff:6000 envAmt:10 aA:500 aD:1500 aS:80 aR:1500 uniVoices:3 uniDetune:10 uniSpread:70 reverb:45 fxCho:true choRate:0.4 choMix:40" },
            { "Fifth Lead", "waveA:saw harmVoices:1 harmInterval:7 harmLevel:70 cutoff:3500 res:35 envAmt:30 uniVoices:2 uniDetune:6 aR:250 reverb:20 fxDly:true dlyTime:300 dlyFb:30 dlyMix:22" },
            { "Vibrato Lead", "waveA:saw waveB:sqr morph:25 cutoff:4000 res:25 l1Shape:sin l1Rate:5.5 l1Delay:400 l1Retrig:true modSrc0:lfo1 modDst0:pitch modAmt0:3 aR:250 reverb:25" },
            { "Wobble Bass", "waveA:saw cutoff:400 res:50 envAmt:0 keytrack:30 l1Shape:sin l1Rate:3 l1Retrig:true modSrc0:lfo1 modDst0:cutoff modAmt0:45 aA:2 aS:90 aR:120 uniVoices:2 uniDetune:10 reverb:5 fxDist:true distDrive:35 distTone:5000 distMix:40 fxComp:true compThresh:-20 compRatio:4 compGain:6" },
            { "Morph Pad", "waveA:sin waveB:saw e3A:2500 e3D:3000 e3S:100 e3R:2000 modSrc0:env3 modDst0:morph modAmt0:100 l2Shape:tri l2Rate:0.2 l2Retrig:false modSrc1:lfo2 modDst1:prism modAmt1:12 cutoff:6000 uniVoices:4 uniDetune:12 uniSpread:80 aA:800 aR:2000 reverb:50" },
            { "Spectral Arp", "image:stairs imgAmount:100 imgSpeed:2 imgLoop:true cutoff:12000 res:0 envAmt:0 aA:5 aD:600 aS:100 aR:600 reverb:30 fxDly:true dlyTime:250 dlyFb:40 dlyMix:25" },
            { "Wheel Morph", "waveA:saw waveB:sqr cutoff:1500 res:30 envAmt:0 aA:20 aS:90 aR:600 uniVoices:2 uniDetune:8 reverb:30 modMode:custom cust.morph:0/100 cust.cutoff:1500/9000 cust.prism:0/40" },
            { "Blur Swell", "waveA:saw waveB:sqr morph:50 blurTime:1500 blurHarm:40 cutoff:5000 envAmt:25 fA:600 fD:2000 fS:50 aA:50 aS:90 aR:1500 reverb:50" },
        };
        return list;
    }

    // Spectral Arp's picture: the overtone series as a staircase, one harmonic a column.
    inline std::vector<unsigned char> stairImage (int& cols)
    {
        const int steps[] = { 1, 2, 3, 4, 5, 6, 8, 10, 12, 10, 8, 6, 5, 4, 3, 2 };
        cols = 16;
        std::vector<unsigned char> b ((size_t) cols * imageRows, 0);
        for (int c = 0; c < cols; ++c)
        {
            b[(size_t) ((steps[c] - 1) * cols + c)] = 255;
            if (b[(size_t) c] < 150) b[(size_t) c] = 150;
        }
        return b;
    }
}
