# Loam Elf-cHarmor

**[Play it in your browser](https://vox4vulpes.github.io/loam-elf-charmor/)** ·
**[Download the VST3 plugin and standalone app](https://github.com/vox4vulpes/loam-elf-charmor/releases/latest)** ·
part of the [Loam Synth Bundle](https://github.com/vox4vulpes/loam-synth-vst)

小鷺 · Additive Synthesizer: every sound is a sum of up to 256 sine-wave
harmonics, shaped in the spectrum rather than by a filter circuit. It runs
entirely in the browser via the Web Audio API — no plugins, no install.

Open `index.html` in any modern browser to play it.

## How it works

Each voice works out the level of every harmonic — from the timbre, the
spectral filter and PLUCK — and every few milliseconds turns them into one
cycle of its waveform (an inverse FFT), which the voice and its unison copies
play at their own pitch. With PRISM on, the harmonics no longer fit one cycle,
so the voice plays them as a bank of sine partials instead. The screen shows the harmonics of the newest note and
the filter curve shaping them; with nothing playing, it previews C4.

## Controls

- **Timbre** — two harmonic recipes, **A** and **B** (SIN, SAW, SQR, TRI,
  PULSE, ORGAN), **MORPH** between them, **ODD/EVEN** (left: fewer even
  harmonics, hollow; right: fewer odd ones) and **PLUCK** (the higher a
  harmonic, the faster it fades — strings and harps)
- **Filter** — a spectral filter, LP / BP / HP with 24 dB/oct slopes:
  **CUTOFF**, **RES** (a peak at the cutoff), **ENV** (how far the filter
  envelope moves it, ±6 octaves) and **KEY** (key tracking)
- **Filter Env** and **Amp Env** — attack, decay, sustain, release
- **Unison** — up to 8 copies of each note: **VOICES**, **DETUNE**, **SPREAD**
- **Prism** — pushes the harmonics off the harmonic series: harmonic *k*
  moves to *k*^(1 + s), stretched upward (bells, metal) or squeezed. **ENV**
  lets the filter envelope move it
- **Phaser** — notches across the spectrum, **NOTCHES** per octave, at
  **DEPTH**, slid along by **OFFSET** and, at **SPEED**, by their own LFO
- **Harmonizer** — up to two extra copies of every note, **INTERVAL** apart
  (±12 semitones), at **LEVEL**: fifths, octaves, chords from one key
- **Blur** — **TIME** smears the harmonics in time (notes swell in, timbre
  changes melt), **HARM** smears them into their neighbours
- **Mod** page — **LFO 1** and **LFO 2** (SIN, TRI, SAW, SQR, RND; rate, fade-in,
  RETRIG per note), **ENV 2** and **ENV 3**, each drawn on its own little
  screen (the filter and amp envelopes too) with a dot showing where the
  newest note is on it, and a six-slot **mod matrix**:
  any of them, velocity or the mod wheel onto pitch, cutoff, resonance, morph,
  odd/even, prism, phaser offset, blur, pluck, detune, volume or pan
- **Wheels** — as on Loam Eight Sub: the pitch wheel's range is set with + / −
  (±1 to ±12, shown on the wheel); the button over the mod wheel steps
  **VIB** (vibrato), **FILT** (opens the filter up to 4 octaves), **CUST** and
  **MTRX** (only through the matrix). In CUST every knob gets a small button:
  click once to put it on the wheel (green), again to reverse it (red), a third
  time to free it; drag the red and green dots on its ring to set where the
  wheel takes it, from red (wheel down) to green (wheel up). Presets keep their wheel setup
- **Image** page — load a picture, or drop one on the screen: left to right
  is time (**TIME** seconds for the whole picture, **LOOP** or hold the last
  column), each row from the bottom is a harmonic (1 to 256), brighter is
  louder over 60 dB. Or load a **sound**: it is analysed into the harmonics
  of its pitch, frame by frame, and plays back at any note. **AMOUNT** mixes
  the image with the timbre. Pictures are kept with presets
- **FX** page — **DIST** (drive, tone, mix), **CHORUS** (rate, depth, mix),
  **DELAY** (time, feedback, mix), **REVERB** (size, mix) and **COMP**
  (threshold, ratio, gain), in that order, each with its own switch
- **Output** — volume
- **Presets** — factory sounds (wheels and effects included), and your own (kept in this browser)

Turn a knob by dragging it up or down, double-click it for its default, or
click its value and type one: `4.5k`, `250ms`, `1.5s`, `+7`, `30%`.

## Playing it

Flip the power switch, then play two octaves from your computer keyboard,
tracker-style: `Z`–`M` is the lower octave (sharps on `S D G H J`), `Q`–`P`
the upper one (sharps on `2 3 5 6 7 9 0`), matched by position so any
keyboard layout works. Hold `Space` as a sustain pedal. A MIDI keyboard works
too in Chrome and Edge: notes, the sustain pedal, and the pitch and mod wheels,
which move the on-screen ones.

And the little egret next to the name has something to say.

## VST3 plugin

[`vst/`](vst/) is the same synth as a VST3 plugin and standalone app, built with
JUCE: the engine is the web version's AudioWorklet ported line for line (same
128-sample blocks, tables, envelopes and LFOs; its levels match the browser's),
with the same four pages, presets, CUST wheel and image loading.

- Every knob, choice and switch is an automatable parameter; CUST assignments
  and the IMAGE picture are saved with the project and in user presets, which
  are files under your user app-data folder in `Loam/Loam Elf-cHarmor/Presets`.
- Pictures (PNG, JPEG, GIF) and sounds (WAV, AIFF, FLAC, Ogg, MP3) load from the
  IMAGE page's buttons or by dropping them on its screen.
- The window can be resized; the panel scales with it.

To build it yourself (C++17; on Linux a few dev packages, see
[`.github/workflows/build-vst.yml`](.github/workflows/build-vst.yml)):

```sh
cmake -S vst -B vst/build -DCMAKE_BUILD_TYPE=Release
cmake --build vst/build --config Release
```

`vst/Tests/EngineTest.cpp` checks the engine against the web version (pitch,
harmonics, levels) without any GUI dependencies (`-DLOAM_BUILD_PLUGIN=OFF`).

## Releases

What changed in each version is in [CHANGELOG.md](CHANGELOG.md). To cut a
release, add a `## [x.y.z]` section there and push a version tag; the
[Release workflow](.github/workflows/release.yml) builds the VST3 plugin on
Windows, macOS and Linux and publishes the zips and the installer.

## License

[GNU AGPLv3](LICENSE). Copyright (c) 2026 vox4vulpes
