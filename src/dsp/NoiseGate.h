#ifndef NOISE_GATE_H
#define NOISE_GATE_H

#include <cmath>
#include <algorithm>

namespace NoiseReducer {

class NoiseGate {
public:
    NoiseGate();
    ~NoiseGate() = default;

    void prepare(double sampleRate);
    void reset();

    void setThreshold(float thresholdDb); // -60.0 a 0.0 dB
    void setRatio(float ratio);           // 1.0 a 20.0
    void setAttackMs(float attackMs);     // 0.1 a 100.0 ms
    void setReleaseMs(float releaseMs);   // 10.0 a 1000.0 ms

    float processSample(float sample);
    void processBuffer(const float* input, float* output, size_t numSamples);

private:
    void updateCoefficients();

    double m_sampleRate;
    float m_thresholdDb;
    float m_thresholdLin;
    float m_ratio;
    float m_attackMs;
    float m_releaseMs;

    float m_alphaAttack;
    float m_alphaRelease;
    float m_envelope;
};

} // namespace NoiseReducer

#endif // NOISE_GATE_H
