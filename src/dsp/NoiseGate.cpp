#include "NoiseGate.h"

namespace NoiseReducer {

NoiseGate::NoiseGate()
    : m_sampleRate(44100.0),
      m_thresholdDb(-30.0f),
      m_thresholdLin(0.03162277f),
      m_ratio(10.0f),
      m_attackMs(10.0f),
      m_releaseMs(100.0f),
      m_alphaAttack(0.0f),
      m_alphaRelease(0.0f),
      m_envelope(0.0f)
{
    updateCoefficients();
}

void NoiseGate::prepare(double sampleRate) {
    m_sampleRate = sampleRate;
    updateCoefficients();
    reset();
}

void NoiseGate::reset() {
    m_envelope = 0.0f;
}

void NoiseGate::setThreshold(float thresholdDb) {
    m_thresholdDb = thresholdDb;
    m_thresholdLin = std::pow(10.0f, thresholdDb / 20.0f);
}

void NoiseGate::setRatio(float ratio) {
    m_ratio = std::max(1.0f, ratio);
}

void NoiseGate::setAttackMs(float attackMs) {
    m_attackMs = std::max(0.1f, attackMs);
    updateCoefficients();
}

void NoiseGate::setReleaseMs(float releaseMs) {
    m_releaseMs = std::max(1.0f, releaseMs);
    updateCoefficients();
}

void NoiseGate::updateCoefficients() {
    m_alphaAttack = std::exp(-1.0f / (static_cast<float>(m_sampleRate) * (m_attackMs / 1000.0f)));
    m_alphaRelease = std::exp(-1.0f / (static_cast<float>(m_sampleRate) * (m_releaseMs / 1000.0f)));
}

float NoiseGate::processSample(float sample) {
    float inputAbs = std::abs(sample);

    if (inputAbs > m_envelope) {
        m_envelope = m_alphaAttack * m_envelope + (1.0f - m_alphaAttack) * inputAbs;
    } else {
        m_envelope = m_alphaRelease * m_envelope + (1.0f - m_alphaRelease) * inputAbs;
    }

    float gain = 1.0f;
    if (m_envelope < m_thresholdLin && m_envelope > 1e-9f) {
        float dbBelow = 20.0f * std::log10(m_envelope / m_thresholdLin);
        float gainDb = dbBelow * (1.0f - 1.0f / m_ratio);
        gain = std::pow(10.0f, gainDb / 20.0f);
    } else if (m_envelope <= 1e-9f) {
        gain = 0.0f;
    }

    return sample * gain;
}

void NoiseGate::processBuffer(const float* input, float* output, size_t numSamples) {
    for (size_t i = 0; i < numSamples; ++i) {
        output[i] = processSample(input[i]);
    }
}

} // namespace NoiseReducer
