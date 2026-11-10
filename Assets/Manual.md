# Modulate

A wavetable synth with three oscillators, noise, a filter with drive, four LFOs and four envelopes.
The LFOs and envelopes can move most knobs.

## Oscillators

- The switch turns an oscillator on.
- Each oscillator morphs between the wavetables in its two menus. One knob sets the morph, the other its level.
- The semitone display moves it up to three octaves either way. FIN moves it in cents.

## Noise

White or pink noise, mixed in at its LEVEL when switched on.

## Filter

Lowpass, highpass, bandpass or notch, at 12 or 24 dB per octave. CUTOFF and RESONANCE shape it, and the
display shows the response. DRIVE saturates the sound before the filter. HQ oversamples the drive for less
aliasing, at more CPU.

## Voice

- VOICES is how many notes can play at once.
- UNISON stacks copies of each note. DETUNE spreads their pitch and SPREAD their stereo position.
- PITCH moves every oscillator in semitones. GLIDE slides to a note played while another is held.
- With RATE on, the glide time is the time for an octave, so smaller steps slide faster. Off, every slide
  takes the same time.
- With one voice, notes played legato slide without restarting, and letting go returns to the key still held.

## LFOs

- Each LFO draws its own shape. Drag a point to move it, and double-click to add or remove one.
- TENSION bends the lines between the points.
- In Trigger mode an LFO restarts with each note. In Sync mode it runs freely.
- RATE is in Hz. With SYNC on, the rate is a note length at the host's tempo instead.

## Envelopes

Each envelope has attack, decay, sustain and release. Its display shows the shape and where the playing
note has reached.

## Modulation

- Drag the cross on an LFO or ENV tab onto a knob to modulate it. The knob shows which source moves it.
- Drag the source's badge on the knob up or down to set how far it moves.
- Right-click the knob to clear it.

## Presets

- The arrows step through the presets, and the name opens the full list.
- A * after the name means the sound has changed since the preset was loaded.
- Save overwrites the current user preset. For a factory preset it asks for a name.
- The ... menu has Init Preset, Save As, Rename and Delete. In the plugin it also has Show Preset Folder.
- Factory presets can't be renamed, deleted or overwritten.

## Master

The knob at the top right sets the output level. The meter beside it shows the output peak, and the light
on top turns on when it goes over 0 dB.
