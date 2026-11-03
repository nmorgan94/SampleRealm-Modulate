#pragma once

#include "ParameterSpecs.h"
#include "../dsp/ModulateSynth.h"

//==============================================================================
template <typename ValueOf>
ModulateSynth::Settings makeSynthSettings (ValueOf&& valueOf,
                                           const ModulateSynth::Assignments& modulation,
                                           const ModulateSynth::Curves& curves,
                                           const ModulateSynth::Tempo& tempo)
{
    ModulateSynth::Settings settings;

    for (size_t i = 0; i < ParameterSpecs::all.size(); ++i)
        settings.values[i] = valueOf (*ParameterSpecs::all[i]);

    settings.modulation = modulation;
    settings.curves = curves;
    settings.tempo = tempo;
    return settings;
}
