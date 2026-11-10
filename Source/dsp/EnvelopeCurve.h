#pragma once

#include <algorithm>
#include <cmath>

//==============================================================================
// A segment's level for its time: positive curves move fast early, negative slowly
namespace EnvelopeCurve
{
    inline float shapeProgress (float progress, float curve)
    {
        constexpr float minimumCurve = 0.001f;
        constexpr float maximumSteepness = 20.0f;

        if (progress <= 0.0f || progress >= 1.0f || std::abs (curve) < minimumCurve)
            return std::clamp (progress, 0.0f, 1.0f);

        const auto steepness = 1.0f + std::abs (curve) * (maximumSteepness - 1.0f);
        return curve > 0.0f ? std::log (1.0f + progress * (steepness - 1.0f)) / std::log (steepness)
                            : (std::pow (steepness, progress) - 1.0f) / (steepness - 1.0f);
    }

    // The inverse of shapeProgress
    inline float progressAt (float shapedProgress, float curve)
    {
        return shapeProgress (shapedProgress, -curve);
    }
}
