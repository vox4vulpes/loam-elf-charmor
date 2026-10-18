# Loam Elf-cHarmor

**[Play it in your browser](https://vox4vulpes.github.io/loam-elf-charmor/)** ·
VST3 plugin coming next ·
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
- **Output** — volume and reverb
- **Presets** — factory sounds, and your own (kept in this browser)

## Playing it

Flip the power switch, then play two octaves from your computer keyboard,
tracker-style: `Z`–`M` is the lower octave (sharps on `S D G H J`), `Q`–`P`
the upper one (sharps on `2 3 5 6 7 9 0`), matched by position so any
keyboard layout works. Hold `Space` as a sustain pedal. A MIDI keyboard works
too in Chrome and Edge: notes, the sustain pedal and the pitch wheel (±2).

And the little egret next to the name has something to say.

## Coming next

The VST3 plugin.

## Releases

What changed in each version is in [CHANGELOG.md](CHANGELOG.md). To cut a
release, add a `## [x.y.z]` section there and push a version tag; the
[Release workflow](.github/workflows/release.yml) builds the VST3 plugin on
Windows, macOS and Linux and publishes the zips and the installer.

## License

[GNU AGPLv3](LICENSE). Copyright (c) 2026 vox4vulpes
