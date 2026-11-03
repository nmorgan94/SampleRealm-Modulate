#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../shared/ParameterSpecs.h"

// Item ids start at 1, as ComboBoxAttachment expects
inline void addChoices (juce::ComboBox& box, const ParameterSpecs::Spec& spec)
{
    int id = 1;

    for (const auto* choice : spec.choices)
        box.addItem (choice, id++);
}
