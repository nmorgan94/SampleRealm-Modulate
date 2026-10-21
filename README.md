# SampleRealm: Modulate

A 3-oscillator wavetable synthesizer with deep modulation

## Features

### Oscillators
- 3 independent wavetable oscillators, each with its own enable, gain, and pitch
- 21 waveforms — Sine, Saw, Square, Triangle, Pulse 25%, Pulse 10%, Formant Sine, Formant Vowel, Bitcrushed Ramp, FM Operator, Feedback FM, Wavefolded Sine, Fractal Pulse, Double Saw, Resonant Saw, Prime Cluster, Odd Harmonic Cluster, Log Square, Asymmetric Rectified Sine, Interleaved Saw Pulse, and Trapezoid
- A/B morphing — pick any two waveforms per oscillator and sweep continuously between them
- Coarse (±36 semitones) and fine (±99 cents) pitch per oscillator, plus a global coarse pitch
- Glide / portamento up to 2 seconds, with legato detection
- Unison up to 7 voices with adjustable detune and stereo spread
- Selectable polyphony, 1 to 8 voices
- Real-time waveform visualization

### Noise
- Switchable white or pink noise generator
- Independent level control

### Filter & Distortion
- Multi-mode filter — Lowpass, Highpass, Bandpass, Notch
- Selectable 12 dB/oct or 24 dB/oct slope, Butterworth-aligned across a two-stage cascade
- Pre-filter drive with an optional HQ mode (2× oversampled to suppress aliasing)
- Live filter-response display

### Modulation
- 4 LFOs with freely drawable curves and adjustable tension
- 5 starting shapes per LFO — Sine, Triangle, Ramp Up, Ramp Down, Square
- Per-note trigger or free-running mode
- Tempo sync across 15 divisions, from 1/16 to 4 bars, including triplets and dotted values
- Free-run rate from 0.01 to 20 Hz
- 4 assignable ADSR envelopes
- Modulation routing manager to assign sources to destinations
- Envelope and LFO playhead visualization

### Presets
- Save, load, and delete user presets

### Output
- Master gain, −24 to +6 dB
- Stereo peak metering with clip indication

## System Requirements

- **macOS** 12 or later, Apple silicon or Intel: VST3, AU and Standalone
- **Windows** 10 or later, 64-bit: VST3 and Standalone

## Build Requirements

- CMake 3.25+
- Git
- A C++23 compiler:
  - **macOS**: Xcode or its Command Line Tools (the `xcode` preset needs the full Xcode app)
  - **Windows**: Visual Studio 2022 or later with the "Desktop development with C++" workload

## Commands

Every command is the same on macOS and Windows, except the rows marked **macOS** or **Windows**.

| Task | Command |
|---|---|
| Build (Debug) | `cmake --workflow --preset debug` |
| Build (Release) | `cmake --workflow --preset release` |
| Run the unit tests | `cmake --workflow --preset unit` |
| Run the unit tests and pluginval | `cmake --workflow --preset test` |
| Run all tests against a Release build | `cmake --workflow --preset test-release` |
| Launch the Standalone app (Debug) | `cmake --build --preset run` |
| Install the plugin: **macOS** | Automatic on every build |
| Install the plugin: **Windows** | `cmake --build --preset install-windows`, from an administrator prompt |
| Open in an IDE: **macOS** | `cmake --preset xcode`, then open `build-xcode/Modulate.xcodeproj` |
| Open in an IDE: **Windows** | Open `build-debug\Modulate.sln` in Visual Studio after a Debug build |

`run` and `install-windows` use the Debug build, so run `cmake --workflow --preset debug` first.
`cmake --list-presets=all` lists every preset for your platform.

## Debugging in Xcode

Generate and open the project with the `xcode` preset (see [Commands](#commands)), then:

1. Select your plugin target from the scheme dropdown
2. Go to **Product → Scheme → Edit Scheme**
3. Click **Run** on the left sidebar
4. Under **Executable**, choose **Other** and pick a host such as AudioPluginHost
5. Press **Cmd+R** to build and run, then load Modulate in the host

## Unit Tests

Catch2 tests live in `Tests/`. They drive the processor directly to check parameter ranges and
defaults, bus layouts, state save/restore, and that the output stays finite across block sizes and
settings. Run them with the `unit` workflow, or `test` to include pluginval (see
[Commands](#commands)). Set `-DENABLE_UNIT_TESTS=OFF` to skip them.

## Validating the Plugin

[pluginval](https://github.com/Tracktion/pluginval) loads the built plugin as a host would and
tests it for stability. It is built from source on demand, so there is nothing to install.

```bash
cmake --build --preset debug --target validate     # Debug
cmake --build --preset release --target validate   # Release: validate this before shipping
```

Logs land in `build-debug/pluginval-logs/` or `build-release/pluginval-logs/`. Strictness defaults
to 10; use `-DPLUGINVAL_STRICTNESS=5` (range 1–10) for a faster run, or `-DENABLE_PLUGINVAL=OFF` to
skip pluginval entirely.

The AU test validates the installed component in `~/Library/Audio/Plug-Ins/Components`, since macOS
resolves Audio Units through its registry rather than by path, so it needs `PLUGIN_COPY_AFTER_BUILD`
left on. Whichever configuration was built most recently is the one installed. Steinberg's VST3
conformance validator is off by default, as it pulls ~300MB of SDK for one extra test; enable it
with `-DPLUGINVAL_VST3_VALIDATOR=ON`.

## CI

[`.github/workflows/ci.yml`](.github/workflows/ci.yml) builds the Release plugin on macOS and Windows
and runs the unit tests. Start it by hand from **Actions → CI → Run workflow**, or with
`gh workflow run ci.yml`. Tick **Also run pluginval** (`-f pluginval=true`) to validate too.

Each run keeps the unsigned plugins under **Artifacts** for 30 days: `Modulate-macOS.tar.gz` (VST3
and AU) and `Modulate-Windows.tar.gz` (VST3).