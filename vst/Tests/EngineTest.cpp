// Assert-style smoke test for the Elf-cHarmor engine. No framework: build and
// run it, a non-zero exit means something broke. Levels are the web version's
// (measured in Chrome with the same patches).
//   cmake --build build --target EngineTest && ./build/EngineTest

#include <chrono>
#include <cstdio>
#include <cmath>
#include <vector>
#include "../Source/CharmorEngine.h"

using namespace charmor;

static int failures = 0;

static void check (bool ok, const char* what)
{
    std::printf ("%s  %s\n", ok ? "ok  " : "FAIL", what);
    if (! ok)
        ++failures;
}

struct Rendered { std::vector<float> left, right; double seconds = 0; };

static Rendered render (const Params& p, std::vector<int> notes, double seconds, double sr = 48000.0)
{
    CharmorEngine e;
    e.prepare (sr);
    e.setParams (p);
    for (auto n : notes) e.noteOn (n, 0.9f);
    Rendered r;
    const auto len = (int) (seconds * sr);
    r.left.resize ((size_t) len); r.right.resize ((size_t) len);
    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < len; i += 512)
        e.render (r.left.data() + i, r.right.data() + i, std::min (512, len - i));
    r.seconds = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count();
    return r;
}

static double peak (const std::vector<float>& x) { double m = 0; for (auto v : x) m = std::fmax (m, std::abs ((double) v)); return m; }
static bool finite (const std::vector<float>& x) { for (auto v : x) if (! std::isfinite (v)) return false; return true; }

int main()
{
    const double sr = 48000.0;
    Params sine;
    sine.waveA = waveSin; sine.cutoff = 20000; sine.res = 0; sine.envAmt = 0; sine.keytrack = 0; sine.aA = 1; sine.aS = 100;

    // ---- pitch: a sine at A4 crosses zero upward 440 times a second
    {
        const auto r = render (sine, { 69 }, 1.0);
        int ups = 0;
        for (size_t i = 24001; i < 48000; ++i) if (r.left[i - 1] < 0 && r.left[i] >= 0) ++ups;
        check (std::abs (ups * 2 - 440) <= 2, "sine at A4 is 440 Hz");
    }

    // ---- the saw's harmonics: 1, 1/2, 1/3, 1/4, 1/5
    {
        auto p = sine; p.waveA = waveSaw;
        const auto r = render (p, { 57 }, 1.0);
        double lv[6] {};
        for (int k = 1; k <= 5; ++k)
        {
            double re = 0, im = 0;
            for (size_t i = 24000; i < 48000; ++i)
            {
                re += r.left[i] * std::cos (2 * pi * 220 * k * (double) i / sr);
                im += r.left[i] * std::sin (2 * pi * 220 * k * (double) i / sr);
            }
            lv[k] = std::hypot (re, im);
        }
        bool ok = true;
        for (int k = 2; k <= 5; ++k) ok = ok && std::abs (lv[k] / lv[1] - 1.0 / k) < 0.005;
        check (ok, "saw harmonics fall as 1/k");
    }

    // ---- levels as in the browser: a saw C4 peaks at 0.153, an 8-note open saw chord at 0.916
    {
        Params init;
        const auto one = render (init, { 60 }, 2.0);
        char what[96];
        std::snprintf (what, sizeof (what), "Init Saw C4 peaks like the web version (%.3f, web 0.153)", peak (one.left));
        check (std::abs (peak (one.left) - 0.153) < 0.01, what);

        auto open = init; open.cutoff = 20000; open.envAmt = 0;
        const auto chord = render (open, { 36, 48, 52, 55, 60, 64, 67, 72 }, 2.0);
        std::snprintf (what, sizeof (what), "8-note saw chord peaks like the web version (%.3f, web 0.916)", peak (chord.left));
        check (std::abs (peak (chord.left) - 0.916) < 0.03, what);
    }

    // ---- every effect sounds, without NaN
    {
        struct Case { const char* name; Params p; };
        std::vector<Case> cases;
        Params base;
        auto add = [&] (const char* name, auto tweak) { auto p = base; tweak (p); cases.push_back ({ name, p }); };
        add ("prism", [] (Params& p) { p.prism = 45; });
        add ("prism env", [] (Params& p) { p.prism = 20; p.prismEnv = 60; });
        add ("phaser", [] (Params& p) { p.phDepth = 80; p.phSpeed = 0.5; });
        add ("harmonizer", [] (Params& p) { p.harmVoices = 2; p.harmInterval = 7; });
        add ("blur", [] (Params& p) { p.blurTime = 1500; p.blurHarm = 50; });
        add ("unison", [] (Params& p) { p.uniVoices = 8; p.uniDetune = 25; p.uniSpread = 90; });
        add ("lfo to pitch", [] (Params& p) { p.modSrc[0] = srcLfo1; p.modDst[0] = dstPitch; p.modAmt[0] = 30; });
        add ("env3 to morph", [] (Params& p) { p.modSrc[0] = srcEnv3; p.modDst[0] = dstMorph; p.modAmt[0] = 100; });
        add ("rnd to pan", [] (Params& p) { p.modSrc[0] = srcLfo2; p.l2Shape = lfoRnd; p.l2Rate = 8; p.modDst[0] = dstPan; p.modAmt[0] = 100; });
        bool ok = true;
        for (const auto& c : cases)
        {
            const auto r = render (c.p, { 60 }, 1.0);
            const auto good = finite (r.left) && finite (r.right) && peak (r.left) > 0.01 && peak (r.left) < 1.0;
            if (! good) std::printf ("      %s: peak %.3f\n", c.name, peak (r.left));
            ok = ok && good;
        }
        check (ok, "prism, phaser, harmonizer, blur, unison and the mod matrix all sound, no NaN");
    }

    // ---- release: the voice stops
    {
        CharmorEngine e;
        e.prepare (sr);
        Params p; p.aR = 100;
        e.setParams (p);
        e.noteOn (60, 0.9f);
        std::vector<float> l (4800), r (4800);
        for (int i = 0; i < 10; ++i) e.render (l.data(), r.data(), 4800);
        e.noteOff (60);
        for (int i = 0; i < 10; ++i) e.render (l.data(), r.data(), 4800);
        check (e.getActiveVoices() == 0 && peak (l) < 1e-4, "a released note dies away and frees its voice");
    }

    // ---- the image: a single bright row plays that harmonic alone
    {
        auto img = std::make_shared<Image>();
        img->cols = 4;
        img->bytes.assign ((size_t) imageRows * 4, 0);
        for (int c = 0; c < 4; ++c) img->bytes[(size_t) (2 * 4 + c)] = 255;   // harmonic 3
        img->amps.resize (img->bytes.size());
        for (size_t i = 0; i < img->bytes.size(); ++i) img->amps[i] = (float) imageByteToAmp (img->bytes[i]);

        CharmorEngine e;
        e.prepare (sr);
        auto p = sine; p.imgAmount = 100;
        e.setParams (p);
        e.setImage (img);
        e.noteOn (57, 0.9f);   // 220 Hz: the image plays 660 Hz
        std::vector<float> l (48000), r (48000);
        e.render (l.data(), r.data(), 48000);
        int ups = 0;
        for (size_t i = 24001; i < 48000; ++i) if (l[i - 1] < 0 && l[i] >= 0) ++ups;
        check (std::abs (ups * 2 - 660) <= 2, "an image row plays its harmonic (harmonic 3 of 220 Hz = 660 Hz)");
    }

    // ---- speed: the heaviest case, 8 notes x 3 harmonizer layers x 8 unison copies
    {
        Params heavy;
        heavy.harmVoices = 2; heavy.uniVoices = 8; heavy.uniDetune = 25;
        const auto r = render (heavy, { 36, 48, 52, 55, 60, 64, 67, 72 }, 2.0);
        char what[96];
        std::snprintf (what, sizeof (what), "heaviest case renders faster than real time (%.1fx)", 2.0 / r.seconds);
        check (2.0 / r.seconds > 1.5, what);
    }

    std::printf ("%s\n", failures == 0 ? "all passed" : "FAILED");
    return failures == 0 ? 0 : 1;
}
