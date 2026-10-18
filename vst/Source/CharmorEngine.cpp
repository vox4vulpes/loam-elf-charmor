#include "CharmorEngine.h"

using namespace charmor;

namespace
{
    // Linear attack, then exponential moves; each time is how long to get ~99% there.
    void stepEnv (double& level, int& stage, double a, double d, double s, double r, double dt)
    {
        if (stage == 0)
        {
            level += dt / std::fmax (0.001, a / 1000);
            if (level >= 1) { level = 1; stage = 1; }
        }
        else if (stage == 1)
            level = s + (level - s) * std::exp (-dt * 4.6 / std::fmax (0.001, d / 1000));
        else
            level *= std::exp (-dt * 4.6 / std::fmax (0.001, r / 1000));
    }

    // How far a full-depth mod slot moves each target, and the range it stays in.
    struct TargetRange { double range, min, max; bool clamp; };
    const TargetRange targetRanges[numTargets] = {
        { 12, 0, 0, false },       // pitch, semitones
        { 6, 0, 0, false },        // cutoff, octaves
        { 100, 0, 100, true },     // res
        { 100, 0, 100, true },     // morph
        { 100, -100, 100, true },  // odd/even
        { 100, -100, 100, true },  // prism
        { 100, 0, 0, false },      // phaser offset
        { 100, 0, 100, true },     // blur harm
        { 100, 0, 100, true },     // pluck
        { 100, 0, 100, true },     // detune
        { 1, 0, 0, false },        // volume
        { 1, 0, 0, false },        // pan
    };

    double& targetField (Params& p, int target)
    {
        switch (target)
        {
            case dstRes:      return p.res;
            case dstMorph:    return p.morph;
            case dstOddEven:  return p.oddEven;
            case dstPrism:    return p.prism;
            case dstPhOffset: return p.phOffset;
            case dstBlurHarm: return p.blurHarm;
            case dstPluck:    return p.pluck;
            default:          return p.uniDetune;
        }
    }
}

CharmorEngine::CharmorEngine()
{
    rev.resize (tableN);
    for (int i = 0; i < tableN; ++i)
    {
        int r = 0;
        for (int bit = 0; bit < tableBits; ++bit)
            r |= ((i >> bit) & 1) << (tableBits - 1 - bit);
        rev[(size_t) i] = r;
    }
    cosT.resize (tableN / 2);
    sinT.resize (tableN / 2);
    for (int i = 0; i < tableN / 2; ++i)
    {
        cosT[(size_t) i] = std::cos (2 * pi * i / tableN);
        sinT[(size_t) i] = std::sin (2 * pi * i / tableN);
    }
    fftRe.resize (tableN);
    fftIm.resize (tableN);
    sine.resize (tableN + 1);
    for (int i = 0; i <= tableN; ++i)
        sine[(size_t) i] = std::sin (2 * pi * i / tableN);

    for (auto& v : voices)
    {
        for (auto* t : { &v.prev, &v.next, &v.prevHigh, &v.nextHigh })
            t->assign (tableN + 1, 0.0);
        v.bankPhase.assign ((size_t) maxCopies * (maxPartials + 1), 0.0);
    }
}

void CharmorEngine::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
    allNotesOff();
    outPos = block;
}

void CharmorEngine::setImage (std::shared_ptr<const Image> img)
{
    std::atomic_store (&image, std::move (img));
}

//==============================================================================
// out[n] = sum of amps[k] sin(2 pi k n / N), k = 1..K: one cycle of the waveform.
void CharmorEngine::buildTable (const Amps& amps, int K, std::vector<double>& out)
{
    std::fill (fftRe.begin(), fftRe.end(), 0.0);
    std::fill (fftIm.begin(), fftIm.end(), 0.0);
    for (int k = 1; k <= K; ++k)
        fftRe[(size_t) rev[(size_t) k]] = amps[(size_t) k];
    for (int size = 2; size <= tableN; size *= 2)
    {
        const int half = size >> 1, step = tableN / size;
        for (int start = 0; start < tableN; start += size)
            for (int j = 0; j < half; ++j)
            {
                const auto wr = cosT[(size_t) (j * step)], wi = -sinT[(size_t) (j * step)];
                const auto a = (size_t) (start + j), b = a + (size_t) half;
                const auto tr = wr * fftRe[b] - wi * fftIm[b], ti = wr * fftIm[b] + wi * fftRe[b];
                fftRe[b] = fftRe[a] - tr; fftIm[b] = fftIm[a] - ti;
                fftRe[a] += tr; fftIm[a] += ti;
            }
    }
    for (int n = 0; n < tableN; ++n)
        out[(size_t) n] = -fftIm[(size_t) n];   // forward FFT of real amplitudes: -Im is the sine sum
    out[tableN] = out[0];
}

//==============================================================================
void CharmorEngine::noteOn (int note, float velocity)
{
    Voice* v = nullptr;
    for (auto& x : voices) if (x.active && x.note == note) { v = &x; break; }
    if (v == nullptr) for (auto& x : voices) if (! x.active) { v = &x; break; }
    if (v == nullptr)   // steal the oldest
    {
        v = &voices[0];
        for (auto& x : voices) if (x.order < v->order) v = &x;
    }
    if (! v->active)
    {
        v->amp = 0; v->fenv = 0; v->level = 0;
        v->amps.fill (0); v->bankAmps.fill (0);
    }
    v->fresh = v->note != note || ! v->active;   // a new pitch starts on its own table, no crossfade
    v->active = true; v->note = note; v->gate = true; v->sustained = false; v->vel = velocity;
    v->t = 0; v->order = ++counter; v->ampStage = 0; v->fStage = 0; v->e2Stage = 0; v->e3Stage = 0;
    if (v->fresh) { v->env2 = 0; v->env3 = 0; }
    for (int l = 0; l < 2; ++l) { v->lfoPh[(size_t) l] = 0; v->lfoHeld[(size_t) l] = random.nextDouble() * 2 - 1; }
    // Each copy starts at a random point of the cycle, so the copies don't cancel.
    for (int c = 0; c < maxCopies; ++c)
        v->phase[(size_t) c] = c == 0 ? 0.0 : random.nextDouble();
    if (v->bank)
        enterBank (*v);
}

void CharmorEngine::noteOff (int note)
{
    for (auto& v : voices)
        if (v.active && v.note == note && v.gate)
        {
            if (sustain) { v.sustained = true; v.gate = false; }
            else release (v);
        }
}

void CharmorEngine::setSustain (bool on)
{
    sustain = on;
    if (! on)
        for (auto& v : voices)
            if (v.sustained) { v.sustained = false; release (v); }
}

void CharmorEngine::allNotesOff()
{
    for (auto& v : voices)
        v.active = false;
}

void CharmorEngine::release (Voice& v)
{
    v.gate = false; v.ampStage = 2; v.fStage = 2; v.e2Stage = 2; v.e3Stage = 2;
}

//==============================================================================
// The voice's mod sources this block, and its parameters after the matrix.
void CharmorEngine::modulate (Voice& v, double dt)
{
    const auto& p = params;
    stepEnv (v.env2, v.e2Stage, p.e2A, p.e2D, p.e2S / 100, p.e2R, dt);
    stepEnv (v.env3, v.e3Stage, p.e3A, p.e3D, p.e3S / 100, p.e3R, dt);
    double src[numSources] = { 0, v.env2, v.env3, 0, 0, v.vel, modWheel };
    for (int l = 0; l < 2; ++l)
    {
        const auto rate = l == 0 ? p.l1Rate : p.l2Rate, delay = l == 0 ? p.l1Delay : p.l2Delay;
        const auto retrig = l == 0 ? p.l1Retrig : p.l2Retrig;
        double ph, held;
        if (retrig)   // each note runs its own LFO from the start of its cycle
        {
            ph = v.lfoPh[(size_t) l] + rate * dt;
            if (ph >= 1) { ph = std::fmod (ph, 1.0); v.lfoHeld[(size_t) l] = random.nextDouble() * 2 - 1; }
            v.lfoPh[(size_t) l] = ph; held = v.lfoHeld[(size_t) l];
        }
        else { ph = gLfoPh[(size_t) l]; held = gHeld[(size_t) l]; }
        const auto fade = delay > 0 ? std::fmin (1.0, v.t / (delay / 1000)) : 1.0;
        src[l == 0 ? srcLfo1 : srcLfo2] = lfoShape (l == 0 ? p.l1Shape : p.l2Shape, ph, held) * fade;
        v.lfoView[(size_t) l] = ph;
    }

    v.pv = p;
    double cutOct = 0;
    v.pitch = 0; v.volume = 1; v.pan = 0;
    // The mod wheel's own job: VIB (up to +-60 cents at 5.5 Hz) or FILT (up to 4 octaves).
    if (p.modMode == modVib) v.pitch += std::sin (2 * pi * vibPh) * modWheel * 0.6;
    else if (p.modMode == modFilter) cutOct += modWheel * 4;
    for (int i = 0; i < modSlots; ++i)
    {
        const auto from = p.modSrc[(size_t) i], to = p.modDst[(size_t) i];
        const auto amt = p.modAmt[(size_t) i];
        if (from <= srcNone || from >= numSources || to < 0 || to >= numTargets || amt == 0)
            continue;
        const auto x = src[from] * amt / 100 * targetRanges[to].range;
        if (to == dstPitch) v.pitch += x;
        else if (to == dstCutoff) cutOct += x;
        else if (to == dstVolume) v.volume *= std::fmax (0.0, 1 + x);
        else if (to == dstPan) v.pan += x;
        else targetField (v.pv, to) += x;
    }
    if (cutOct != 0) v.pv.cutoff = p.cutoff * std::pow (2.0, cutOct);
    for (int t = 0; t < numTargets; ++t)
        if (targetRanges[t].clamp)
        {
            auto& f = targetField (v.pv, t);
            f = std::fmax (targetRanges[t].min, std::fmin (targetRanges[t].max, f));
        }
    v.pan = std::fmax (-1.0, std::fmin (1.0, v.pan));
}

// Every copy of a note: the unison spread, repeated for each harmonizer voice.
int CharmorEngine::setupCopies (Voice& v)
{
    const auto& p = v.pv;
    const int U = std::max (1, std::min (maxUnison, (int) std::lround (p.uniVoices)));
    const int H = std::max (0, std::min (maxHarm, (int) std::lround (p.harmVoices)));
    const auto hLevel = p.harmLevel / 100, norm = 1 / std::sqrt (U * (1 + H * hLevel * hLevel));
    int n = 0;
    for (int hv = 0; hv <= H; ++hv)
    {
        const auto hRatio = std::pow (2.0, hv * p.harmInterval / 12), hGain = hv == 0 ? 1.0 : hLevel;
        for (int u = 0; u < U; ++u)
        {
            const auto det = U == 1 ? 0.0 : p.uniDetune * (2.0 * u / (U - 1) - 1);
            const auto pan = U == 1 ? 0.0 : p.uniSpread / 100 * (2.0 * u / (U - 1) - 1);
            // the voice's own PAN (from the mod matrix) as a balance on top of the spread
            const auto balL = v.pan > 0 ? 1 - v.pan : 1.0, balR = v.pan < 0 ? 1 + v.pan : 1.0;
            v.cRatio[(size_t) n] = hRatio * std::pow (2.0, det / 1200);
            v.cL[(size_t) n] = (U == 1 ? std::sqrt (0.5) : std::cos ((pan + 1) * pi / 4)) * norm * hGain * balL;
            v.cR[(size_t) n] = (U == 1 ? std::sqrt (0.5) : std::sin ((pan + 1) * pi / 4)) * norm * hGain * balR;
            ++n;
        }
    }
    return n;
}

// The voice's place in the IMAGE: SPEED seconds from first column to last, then loop or hold.
const Amps* CharmorEngine::imageColumn (Voice& v, const Params& p)
{
    const auto img = std::atomic_load (&image);
    if (img == nullptr || img->cols <= 0 || p.imgAmount <= 0)
        return nullptr;
    const int cols = img->cols;
    auto pos = v.t / std::fmax (0.05, p.imgSpeed) * cols;
    pos = p.imgLoop ? std::fmod (pos, (double) cols) : std::fmin (cols - 1.0, pos);
    const int c0 = (int) std::floor (pos), c1 = p.imgLoop ? (c0 + 1) % cols : std::min (cols - 1, c0 + 1);
    const auto fr = pos - c0;
    for (int k = 1; k <= maxPartials; ++k)
    {
        const auto r = (size_t) (k - 1) * (size_t) cols;
        imgCol[(size_t) k] = img->amps[r + (size_t) c0] * (1 - fr) + img->amps[r + (size_t) c1] * fr;
    }
    v.imgPos = pos / cols;
    return &imgCol;
}

// New harmonic levels: the voice's recipe, eased toward by BLUR TIME.
void CharmorEngine::updateAmps (Voice& v, const Params& p, double f0, int K, double s, double dtStep)
{
    const auto fc = effectiveCutoff (p, f0, v.fenv);
    voiceAmps (p, f0, K, fc, s, v.t, lfo, v.target, v.scratch, imageColumn (v, p));
    const auto keep = p.blurTime > 0 ? std::exp (-dtStep / (p.blurTime / 1000)) : 0.0;
    for (int k = 1; k <= maxPartials; ++k)
        v.amps[(size_t) k] = v.target[(size_t) k] + (v.amps[(size_t) k] - v.target[(size_t) k]) * keep;
    v.f0 = f0; v.cutoff = fc; v.count = K; v.stretch = s;
}

// PRISM starts: every partial picks up where the waveform's cycle is, so the switch is seamless.
void CharmorEngine::enterBank (Voice& v)
{
    for (int c = 0; c < maxCopies; ++c)
        for (int k = 1; k <= maxPartials; ++k)
            v.bankPhase[(size_t) (c * (maxPartials + 1) + k)] = std::fmod (k * v.phase[(size_t) c], 1.0);
    v.bankAmps = v.amps;
    v.bank = true;
}

void CharmorEngine::renderTable (Voice& v, double f0, int copies, float* L, float* R, double nyq)
{
    const auto& p = v.pv;
    // The base table holds every harmonic below Nyquist for the unison's highest copy;
    // harmonizer copies pitched above that read a second table with fewer.
    const auto baseMax = std::pow (2.0, p.uniDetune / 1200);
    auto upMax = baseMax;
    for (int c = 0; c < copies; ++c) upMax = std::fmax (upMax, v.cRatio[(size_t) c]);
    const int K = std::min (maxPartials, (int) std::floor (nyq / (f0 * baseMax)));
    const bool needHigh = upMax > baseMax * 1.0001;
    const int Kh = std::min (K, (int) std::floor (nyq / (f0 * upMax)));
    if (v.fresh || v.sinceUpdate >= updateBlocks)
    {
        updateAmps (v, p, f0, K, 0, updateBlocks * block / sampleRate);
        std::swap (v.prev, v.next);
        buildTable (v.amps, K, v.next);
        if (needHigh)
        {
            std::swap (v.prevHigh, v.nextHigh);
            buildTable (v.amps, Kh, v.nextHigh);
            if (! v.highReady || v.fresh) { v.prevHigh = v.nextHigh; v.highReady = true; }
        }
        else v.highReady = false;
        if (v.fresh) { v.prev = v.next; v.fresh = false; }
        v.sinceUpdate = 0;
    }
    const auto dw = 1.0 / updateBlocks / block, e0 = v.level, e1 = v.amp * v.vel * voiceGain * v.volume, de = (e1 - e0) / block;
    for (int c = 0; c < copies; ++c)
    {
        const bool high = needHigh && v.cRatio[(size_t) c] > baseMax * 1.0001;
        const auto& prev = high ? v.prevHigh : v.prev;
        const auto& next = high ? v.nextHigh : v.next;
        const auto inc = f0 * v.cRatio[(size_t) c] / sampleRate * tableN;
        auto ph = v.phase[(size_t) c] * tableN;
        const auto gl = v.cL[(size_t) c], gr = v.cR[(size_t) c];
        auto w = (double) v.sinceUpdate / updateBlocks, e = e0;
        for (int i = 0; i < block; ++i)
        {
            const auto i0 = (size_t) (int) ph;
            const auto fr = ph - (double) i0;
            const auto x0 = prev[i0] + (prev[i0 + 1] - prev[i0]) * fr;
            const auto x1 = next[i0] + (next[i0 + 1] - next[i0]) * fr;
            const auto s = (x0 + (x1 - x0) * w) * e;
            L[i] += (float) (s * gl); R[i] += (float) (s * gr);
            ph += inc; if (ph >= tableN) ph -= tableN;
            w += dw; e += de;
        }
        v.phase[(size_t) c] = ph / tableN;
    }
    v.level = e1;
    ++v.sinceUpdate;
}

void CharmorEngine::renderBank (Voice& v, double f0, double s, int copies, float* L, float* R, double nyq, int active)
{
    if (! v.bank)
        enterBank (v);
    int K = std::min (maxPartials, bankBudget / (copies * active));
    while (K > 1 && f0 * partialRatio (K, s) >= nyq) --K;   // the stretched series reaches Nyquist sooner
    K = std::max (1, K);
    v.oldAmps = v.bankAmps;
    updateAmps (v, v.pv, f0, K, s, block / sampleRate);
    const auto e0 = v.level, e1 = v.amp * v.vel * voiceGain * v.volume, de = (e1 - e0) / block;
    for (int c = 0; c < copies; ++c)
    {
        buf.fill (0);
        const auto base = (size_t) (c * (maxPartials + 1));
        const auto fc0 = f0 * v.cRatio[(size_t) c] / sampleRate;
        for (int k = 1; k <= K; ++k)
        {
            const auto inc = fc0 * partialRatio (k, s);
            if (inc >= 0.48) break;
            auto amp = v.oldAmps[(size_t) k];
            const auto dAmp = (v.amps[(size_t) k] - amp) / block;
            if (std::abs (amp) < 1e-7 && std::abs (v.amps[(size_t) k]) < 1e-7) continue;
            auto ph = v.bankPhase[base + (size_t) k];
            for (int i = 0; i < block; ++i)
            {
                const auto x = ph * tableN;
                const auto i0 = (size_t) (int) x;
                buf[(size_t) i] += amp * (sine[i0] + (sine[i0 + 1] - sine[i0]) * (x - (double) i0));
                amp += dAmp; ph += inc;
                if (ph >= 1) ph -= 1;
            }
            v.bankPhase[base + (size_t) k] = ph;
        }
        const auto gl = v.cL[(size_t) c], gr = v.cR[(size_t) c];
        auto e = e0;
        for (int i = 0; i < block; ++i) { L[i] += (float) (buf[(size_t) i] * e * gl); R[i] += (float) (buf[(size_t) i] * e * gr); e += de; }
        v.phase[(size_t) c] = v.bankPhase[base + 1];   // so leaving PRISM picks up the cycle where it is
    }
    v.bankAmps = v.amps;
    v.level = e1;
}

//==============================================================================
void CharmorEngine::render (float* left, float* right, int num)
{
    while (num > 0)
    {
        if (outPos == block)
        {
            outL.fill (0); outR.fill (0);
            processBlock (outL.data(), outR.data());
            outPos = 0;
        }
        const auto n = std::min (num, block - outPos);
        std::copy (outL.begin() + outPos, outL.begin() + outPos + n, left);
        std::copy (outR.begin() + outPos, outR.begin() + outPos + n, right);
        left += n; right += n; num -= n; outPos += n;
    }
}

void CharmorEngine::processBlock (float* L, float* R)
{
    const auto& p = params;
    const auto dt = block / sampleRate, nyq = sampleRate * 0.48;
    Voice* newest = nullptr;
    int active = 0;
    lfo = std::fmod (lfo + p.phSpeed * dt, 1.0);
    vibPh = std::fmod (vibPh + 5.5 * dt, 1.0);
    for (int l = 0; l < 2; ++l)   // free-running LFOs, shared by every voice
    {
        gLfoPh[(size_t) l] += (l == 0 ? p.l1Rate : p.l2Rate) * dt;
        if (gLfoPh[(size_t) l] >= 1) { gLfoPh[(size_t) l] = std::fmod (gLfoPh[(size_t) l], 1.0); gHeld[(size_t) l] = random.nextDouble() * 2 - 1; }
    }
    for (auto& x : voices) if (x.active) ++active;

    for (auto& v : voices)
    {
        if (! v.active)
            continue;
        stepEnv (v.amp, v.ampStage, p.aA, p.aD, p.aS / 100, p.aR, dt);
        stepEnv (v.fenv, v.fStage, p.fA, p.fD, p.fS / 100, p.fR, dt);
        if (v.ampStage == 2 && v.amp < 1e-4) { v.active = false; continue; }

        modulate (v, dt);
        const auto copies = setupCopies (v);
        const auto f0 = 440 * std::pow (2.0, (v.note - 69 + bend * p.bendRange + v.pitch) / 12);
        const auto s = prismStretch (v.pv, v.fenv);
        if (std::abs (s) > 1e-4)
            renderBank (v, f0, s, copies, L, R, nyq, active);
        else
        {
            if (v.bank) { v.bank = false; v.fresh = true; }   // back to tables, from the cycle's current place
            renderTable (v, f0, copies, L, R, nyq);
        }
        v.t += dt;
        if (newest == nullptr || v.order > newest->order)
            newest = &v;
    }

    int stillActive = 0;
    for (auto& x : voices) if (x.active) ++stillActive;
    activeVoices = stillActive;

    // The screen: the newest voice's harmonics, about 30 times a second.
    if (++blocks % 12 == 0)
    {
        const juce::SpinLock::ScopedTryLockType lock (snapshotLock);
        if (lock.isLocked())
        {
            snapshot.voices = active;
            if (newest != nullptr)
            {
                const auto& v = *newest;
                snapshot.note = v.note; snapshot.count = v.count; snapshot.f0 = v.f0; snapshot.cutoff = v.cutoff;
                snapshot.stretch = v.stretch; snapshot.lfo = lfo; snapshot.imgPos = v.imgPos; snapshot.t = v.t;
                snapshot.lfoPhase = v.lfoView;
                snapshot.envLevel = { v.amp, v.fenv, v.env2, v.env3 };
                snapshot.envStage = { v.ampStage, v.fStage, v.e2Stage, v.e3Stage };
                for (int k = 0; k <= maxPartials; ++k)
                    snapshot.amps[(size_t) k] = k <= v.count ? (float) (v.amps[(size_t) k] * v.level) : 0.0f;
            }
            else
                snapshot.note = -1;
        }
    }
}

CharmorEngine::Snapshot CharmorEngine::getSnapshot() const
{
    const juce::SpinLock::ScopedLockType lock (snapshotLock);
    return snapshot;
}
