#pragma once
#include <cmath>
#include <JuceHeader.h>

// ITU-R BS.1770 K-weighting pre-filter: a high-frequency shelf followed by
// a high-pass (the "RLB" curve), realised as two direct-form-II biquads.
// Coefficient derivation follows the standard's analogue-prototype ->
// bilinear-transform approach (as used by libebur128), so it stays accurate
// at any sample rate rather than only the 48 kHz constants printed in the spec.
class KWeightingFilter
{
public:
    void prepare(double sampleRate)
    {
        constexpr double pi = juce::MathConstants<double>::pi;

        {
            const double f0 = 1681.9744509555319;
            const double G  = 3.99984385397;
            const double Q  = 0.7071752369554196;

            const double K  = std::tan(pi * f0 / sampleRate);
            const double Vh = std::pow(10.0, G / 20.0);
            const double Vb = std::pow(Vh, 0.4996667741545416);

            const double a0 = 1.0 + K / Q + K * K;
            stage1.b0 = (Vh + Vb * K / Q + K * K) / a0;
            stage1.b1 = 2.0 * (K * K - Vh) / a0;
            stage1.b2 = (Vh - Vb * K / Q + K * K) / a0;
            stage1.a1 = 2.0 * (K * K - 1.0) / a0;
            stage1.a2 = (1.0 - K / Q + K * K) / a0;
        }
        {
            const double f0 = 38.13547087602444;
            const double Q  = 0.5003270373238773;
            const double K  = std::tan(pi * f0 / sampleRate);
            const double a0 = 1.0 + K / Q + K * K;

            stage2.b0 = 1.0;
            stage2.b1 = -2.0;
            stage2.b2 = 1.0;
            stage2.a1 = 2.0 * (K * K - 1.0) / a0;
            stage2.a2 = (1.0 - K / Q + K * K) / a0;
        }

        stage1.reset();
        stage2.reset();
    }

    void reset()
    {
        stage1.reset();
        stage2.reset();
    }

    float processSample(float x)
    {
        return (float) stage2.process(stage1.process((double) x));
    }

private:
    struct Biquad
    {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;

        void reset() { z1 = z2 = 0.0; }

        double process(double x)
        {
            const double y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    Biquad stage1, stage2;
};
