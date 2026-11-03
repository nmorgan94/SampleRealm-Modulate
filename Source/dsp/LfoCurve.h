#pragma once

#include <juce_core/juce_core.h>
#include <algorithm>
#include <array>
#include <span>

//==============================================================================
// An LFO's drawn shape: points from x 0 to 1, joined by a Catmull-Rom spline whose
// tension runs from smooth (0) through the default (0.5) to straight lines (1)
namespace LfoCurve
{
    struct Point
    {
        constexpr Point(float xPos = 0.0f, float yPos = 0.5f) : x(xPos), y(yPos) {}

        float x;
        float y;
    };

    inline constexpr size_t maxPoints = 32;

    // A fixed-size copy of a curve, for the audio thread. No points means the default shape
    struct Points
    {
        std::array<Point, maxPoints> points {};
        size_t count = 0;

        std::span<const Point> view() const { return { points.data(), count }; }

        bool operator==(const Points& other) const
        {
            return std::ranges::equal(view(), other.view(), [](const Point& a, const Point& b)
            {
                return juce::exactlyEqual(a.x, b.x) && juce::exactlyEqual(a.y, b.y);
            });
        }
    };

    inline constexpr std::array defaultPoints { Point(0.0f, 0.5f), Point(0.25f, 1.0f), Point(0.5f, 0.5f),
                                                Point(0.75f, 0.0f), Point(1.0f, 0.5f) };

    inline float catmullRom(float y0, float y1, float y2, float y3, float t, float tension)
    {
        float cardinalParam = (tension - 0.5f) * 2.0f;
        float tensionScale = (1.0f - cardinalParam) * 0.5f;
        float t2 = t * t;
        float t3 = t2 * t;

        float result = tensionScale * (
            (2.0f * y1) +
            (-y0 + y2) * t +
            (2.0f * y0 - 5.0f * y1 + 4.0f * y2 - y3) * t2 +
            (-y0 + 3.0f * y1 - 3.0f * y2 + y3) * t3
        ) + cardinalParam * (y1 + (y2 - y1) * t);

        return juce::jlimit(0.0f, 1.0f, result);
    }

    // The curve's value at x in [0, 1]
    inline float evaluate(std::span<const Point> points, float tension, float x)
    {
        x = juce::jlimit(0.0f, 1.0f, x);
        if (points.size() < 2)
            return 0.5f;

        size_t segmentIndex = points.size() - 1;
        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            if (x <= points[i + 1].x)
            {
                segmentIndex = i;
                break;
            }
        }

        if (segmentIndex >= points.size() - 1)
            return points.back().y;

        const auto& startPoint  = points[segmentIndex];
        const auto& endPoint    = points[segmentIndex + 1];
        const auto& beforePoint = (segmentIndex > 0) ? points[segmentIndex - 1] : startPoint;
        const auto& afterPoint  = (segmentIndex + 2 < points.size()) ? points[segmentIndex + 2] : endPoint;

        // Points can share an x
        if (endPoint.x <= startPoint.x)
            return endPoint.y;

        const float t = (x - startPoint.x) / (endPoint.x - startPoint.x);
        return catmullRom(beforePoint.y, startPoint.y, endPoint.y, afterPoint.y, t, tension);
    }

    //==============================================================================
    struct Shape
    {
        const char* name;
        std::span<const Point> points;
        float tension;
    };

    inline constexpr std::array rampUpPoints { Point(0.0f, 0.0f), Point(1.0f, 1.0f) };
    inline constexpr std::array rampDownPoints { Point(0.0f, 1.0f), Point(1.0f, 0.0f) };
    inline constexpr std::array squarePoints { Point(0.0f, 1.0f), Point(0.49f, 1.0f), Point(0.51f, 0.0f),
                                               Point(0.99f, 0.0f), Point(1.0f, 1.0f) };

    inline constexpr std::array shapes {
        Shape { "Sine",      defaultPoints,  0.5f },
        Shape { "Triangle",  defaultPoints,  1.0f },
        Shape { "Ramp Up",   rampUpPoints,   1.0f },
        Shape { "Ramp Down", rampDownPoints, 1.0f },
        Shape { "Square",    squarePoints,   1.0f }
    };
}
