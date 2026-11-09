#pragma once

// A control with a modulation badge, which the editor refreshes when the state is replaced
struct ModulationTarget
{
    virtual ~ModulationTarget() = default;

    // Re-reads the assignment from the manager, clearing the badge when there is none
    virtual void refreshAssignment() = 0;
};
