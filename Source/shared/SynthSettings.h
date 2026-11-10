#pragma once

#include "ParameterSpecs.h"
#include "../dsp/ModulateSynth.h"

//==============================================================================
template <typename ValueOf>
ModulateSynth::Settings makeSynthSettings (ValueOf&& valueOf,
                                           const ModulateSynth::Assignments& modulation,
                                           const ModulateSynth::Curves& curves)
{
    ModulateSynth::Settings settings;

    for (size_t i = 0; i < ParameterSpecs::all.size(); ++i)
        settings.values[i] = valueOf (*ParameterSpecs::all[i]);

    settings.modulation = modulation;
    settings.curves = curves;
    return settings;
}
