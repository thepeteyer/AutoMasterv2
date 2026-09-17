#pragma once
#include <cmath>
#include <algorithm>

// Stateless per-sample waveshapers for the Saturation tab. Each takes an
// already drive-scaled sample and returns a shaped sample in roughly [-1, 1].
namespace Saturator
{
    enum Character { Tape = 0, Tube = 1, Transistor = 2, Clip = 3 };

    inline float tape(float x)
    {
        return x / std::sqrt(1.0f + x * x);
    }

    // Different curves either side of zero -> even-harmonic "valve" colour,
    // with no DC bias to remove since both branches pass through the origin.
    inline float tube(float x)
    {
        constexpr float a = 1.6f;
        constexpr float s = 1.6f; // firmer on the negative half
        if (x >= 0.0f)
            return (1.0f - std::exp(-a * x)) / a;
        return -(1.0f - std::exp(a * x)) / (a * s);
    }

    inline float transistor(float x)
    {
        constexpr float k = 3.0f;
        return std::tanh(k * x) / std::tanh(k);
    }

    inline float clip(float x)
    {
        return std::clamp(x, -1.0f, 1.0f);
    }

    inline float shape(Character character, float x)
    {
        switch (character)
        {
            case Tube:       return tube(x);
            case Transistor: return transistor(x);
            case Clip:       return clip(x);
            case Tape:
            default:         return tape(x);
        }
    }
}
