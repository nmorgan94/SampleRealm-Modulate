#include <juce_audio_basics/juce_audio_basics.h>
#include "shared/ParameterSpecs.h"
#include "shared/UiInfoJson.h"
#include "dsp/ModulateSynth.h"
#include "dsp/PeakMeter.h"

// BuildCommit.cpp
extern const char* const buildCommit;

//==============================================================================
namespace
{
    constexpr int maxBlockSize = 128;
    constexpr int numChannels = PeakMeter::maxChannels;

    constexpr auto numParams = static_cast<int> (ParameterSpecs::all.size());
    constexpr auto numWaveforms = static_cast<int> (ParameterSpecs::waveformNames.size());

    auto values = ModulateSynth::defaultValues();
    ModulateSynth::Assignments modulation {};
    ModulateSynth::Curves curves {};
    double currentSampleRate = 48000.0;
    Transport transport;

    ModulateSynth synth;
    PeakMeter meter { numChannels };
    juce::AudioBuffer<float> buffer { numChannels, maxBlockSize };

    // Played at the start of the next block
    juce::MidiBuffer pendingMidi;

    // x, y pairs, written before setCurve or curveValue
    std::array<float, 2 * LfoCurve::maxPoints> points {};

    std::array<float, 4 * Modulation::numSources> liveValues {};

    // For the web UI's tests
    Filter responseFilter;
    juce::AudioBuffer<float> wavetable { 1, WavetableGenerator::getWavetableSize() };
    std::array<float, 2> range {};

    std::span<const LfoCurve::Point> pointsWritten (int count, std::array<LfoCurve::Point, LfoCurve::maxPoints>& into)
    {
        const auto size = static_cast<size_t> (std::clamp (count, 0, static_cast<int> (LfoCurve::maxPoints)));

        for (size_t i = 0; i < size; ++i)
            into[i] = { points[2 * i], points[2 * i + 1] };

        return { into.data(), size };
    }
}

#define EXPORT(name) extern "C" __attribute__ ((export_name (#name)))

EXPORT (uiInfo) const char* uiInfo()
{
    static const auto json = uiInfoJson (PLUGIN_VERSION, buildCommit);
    return json.c_str();
}

EXPORT (prepare) void prepare (double sampleRate)
{
    currentSampleRate = sampleRate;
    meter.reset();
    buffer.clear();
    synth.setSettings ({ values, modulation, curves });
    synth.prepare (sampleRate, maxBlockSize, numChannels);
}

// Stored as the plugin's parameter would store it, snapped to its step
EXPORT (setParam) void setParam (int index, float value)
{
    if (index < 0 || index >= numParams)
        return;

    const auto i = static_cast<size_t> (index);
    const auto& spec = *ParameterSpecs::all[i];
    values[i] = ParameterSpecs::snapToLegalValue (spec, ParameterSpecs::fromProportion (spec, ParameterSpecs::toProportion (spec, value)));
}

// Source 1 is an LFO, 2 an envelope, and 0 clears it. The range is in the parameter's 0..1 proportion
EXPORT (setModulation) void setModulation (int index, int source, int sourceIndex, float min, float max)
{
    if (index >= 0 && index < numParams)
        modulation[static_cast<size_t> (index)] = { Modulation::sourceFromNumber (source), sourceIndex, min, max };
}

EXPORT (points) float* pointsBuffer()
{
    return points.data();
}

EXPORT (maxCurvePoints) int maxCurvePoints()
{
    return LfoCurve::maxPoints;
}

// From the first count points written to points(). None gives the default shape
EXPORT (setCurve) void setCurve (int lfo, int count)
{
    if (lfo < 0 || lfo >= ModulateSynth::numLfos)
        return;

    auto& curve = curves[static_cast<size_t> (lfo)];
    curve.count = pointsWritten (count, curve.points).size();
}

// The page plays the host: render moves the position on while playing
EXPORT (setTransport) void setTransport (double bpm, bool playing, double ppq)
{
    transport = { bpm, playing, ppq };
}

EXPORT (midi) void midi (int status, int data1, int data2)
{
    if (status < 0x80 || status >= 0xf0)
        return;

    pendingMidi.addEvent (juce::MidiMessage (status, data1 & 0x7f, data2 & 0x7f), 0);
}

// Read the output back from channel(0) and channel(1)
EXPORT (render) void render (int numSamples)
{
    juce::AudioBuffer<float> block { buffer.getArrayOfWritePointers(), numChannels, std::clamp (numSamples, 0, maxBlockSize) };

    synth.setSettings ({ values, modulation, curves });
    synth.process (block, pendingMidi, transport);
    pendingMidi.clear();

    meter.process (block);
    transport.advance (block.getNumSamples(), currentSampleRate);
}

EXPORT (channel) float* channel (int index)
{
    return buffer.getWritePointer (std::clamp (index, 0, numChannels - 1));
}

// The loudest output sample since the last call, as linear gain
EXPORT (peak) float peak (int index)
{
    return index >= 0 && index < numChannels ? meter.readAndReset (index) : 0.0f;
}

// Per source: LFO phases, LFO values, envelope levels, envelope stage progress
EXPORT (live) float* live()
{
    const auto& state = synth.getLive();
    auto* out = liveValues.data();

    for (const auto* group : { &state.lfoPhase, &state.lfoValue, &state.envelopeLevel, &state.envelopeProgress })
        for (const auto& value : *group)
            *out++ = value.load();

    return liveValues.data();
}

// As ADSREnvelope::State: idle, attack, decay, sustain, release
EXPORT (envelopeStage) int envelopeStage (int index)
{
    if (index < 0 || index >= ModulateSynth::numEnvelopes)
        return 0;

    return std::to_underlying (synth.getLive().envelopeStage[static_cast<size_t> (index)].load());
}

//==============================================================================
// The DSP that webui/ ports, for its tests

EXPORT (curveValue) float curveValue (int count, float tension, float x)
{
    std::array<LfoCurve::Point, LfoCurve::maxPoints> into;
    return LfoCurve::evaluate (pointsWritten (count, into), tension, x);
}

EXPORT (envelopeCurve) float envelopeCurve (float progress, float curve)
{
    return EnvelopeCurve::shapeProgress (progress, curve);
}

EXPORT (waveform) float* waveform (int type)
{
    WavetableGenerator::generateWavetable (wavetable, static_cast<WaveformType> (std::clamp (type, 0, numWaveforms - 1)));
    return wavetable.getWritePointer (0);
}

EXPORT (filterMagnitude) float filterMagnitude (int mode, int slope, float cutoff, float resonance, float frequency, double sampleRate)
{
    responseFilter.prepare (sampleRate);
    responseFilter.setParameters (static_cast<FilterMode> (std::clamp (mode, 0, 3)), static_cast<FilterSlope> (std::clamp (slope, 0, 1)),
                                  cutoff, resonance);
    return responseFilter.getMagnitudeAt (frequency);
}

EXPORT (defaultRange) float* defaultRange (int source, float base)
{
    const auto assignment = Modulation::makeDefaultRange (Modulation::sourceFromNumber (source), 0, base);
    range = { assignment.minValue, assignment.maxValue };
    return range.data();
}

EXPORT (depthChange) float* depthChange (int source, float min, float max, float base, float delta)
{
    Modulation::Assignment assignment { Modulation::sourceFromNumber (source), 0, min, max };
    Modulation::applyDepthChange (assignment, base, delta);
    range = { assignment.minValue, assignment.maxValue };
    return range.data();
}

// A parameter's value under modulation, as the editor's dot shows it
EXPORT (modulatedValue) float modulatedValue (int index, float value, float min, float max, float sourceValue)
{
    if (index < 0 || index >= numParams)
        return 0.0f;

    return ModulateSynth::modulatedValue (*ParameterSpecs::all[static_cast<size_t> (index)], value,
                                          { Modulation::Source::LFO, 0, min, max }, sourceValue);
}
