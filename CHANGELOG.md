# Changelog

Each release's notes on GitHub start with its section from this file — the
release workflow refuses to publish a version that has no section here.

## [1.0.0]

### Added
- **Loam Elf-cHarmor** (web): 小鷺 · Additive Synthesizer. Eight voices of up
  to 256 harmonics each, built from two timbres (SIN, SAW, SQR, TRI, PULSE,
  ORGAN) with MORPH, ODD/EVEN and PLUCK; a spectral LP / BP / HP filter with
  resonance, envelope and key tracking; filter and amp envelopes; unison up to
  8 copies with detune and stereo spread; volume.
- Spectral effects: **Prism** (harmonics stretched or squeezed off the
  harmonic series, also by the filter envelope), **Phaser** (moving notches
  across the spectrum), **Harmonizer** (up to two extra copies of each note an
  interval apart) and **Blur** (harmonics smeared in time and into each other).
- **Mod page**: two LFOs, two more envelopes and a six-slot mod matrix
  (envelopes, LFOs, velocity, mod wheel → pitch, cutoff, resonance, morph,
  odd/even, prism, phaser offset, blur, pluck, detune, volume, pan). Every
  LFO and envelope (filter and amp included) has a graph of its shape, with
  fade-in and segment marks and a dot following the newest note.
- **Image page**: play a picture as harmonics over time, or resynthesise a
  sound (its pitch found, each frame analysed into harmonics) to play at any
  note; amount, time and loop; drop files on the screen. A new factory preset,
  Spectral Arp, plays the overtone series as a staircase picture.
- **FX page**: distortion, chorus, delay, reverb and compressor in a chain,
  each with its own on/off switch; the reverb moved here from Output.
- **Pitch and mod wheels** as on Loam Eight Sub: bend range ± on the pitch
  wheel, VIB / FILT / CUST / MTRX on the mod wheel, kept with presets. In
  CUST every knob gets a button to put it on the wheel, and red and green dots
  on its ring for where the wheel takes it.
- A spectrum screen showing the harmonics of the newest note and the filter
  curve, or a preview of C4 while nothing plays.
- Nineteen factory presets and your own, saved in the browser.
- Click a knob's value to type one in (`4.5k`, `250ms`, `1.5s`, `+7`...).
- Computer-keyboard and MIDI play (notes, sustain pedal, pitch wheel).
- **VST3 plugin and standalone app** for Windows, macOS and Linux: the same
  engine, pages, presets, CUST wheel and image loading as the web version,
  every control automatable, CUST and the image saved with the project, user
  presets as files, a resizable window. Windows installer included.
- A pixel-art elf girl as the browser-tab and app icon, and the little egret
  beside the name.
