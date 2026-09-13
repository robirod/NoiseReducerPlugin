#include "NoiseReducerCore.h"
#include <algorithm>

namespace NoiseReducer {

NoiseReducerCore::NoiseReducerCore(size_t fftSize, size_t hopSize)
    : m_sampleRate(44100.0),
      m_algorithm(ReductionAlgorithm::SpectralSubtraction),
      m_thresholdDb(-30.0f),
      m_intensity(1.0f),
      m_dryWet(1.0f),
      m_isLearning(false),
      m_spectralSub(fftSize),
      m_wienerFilter(fftSize),
      m_stftBuffer(fftSize, hopSize)
{
}

void NoiseReducerCore::prepare(double sampleRate, size_t samplesPerBlock) {
    m_sampleRate = sampleRate;
    m_noiseGate.prepare(sampleRate);
    m_spectralSub.prepare(m_stftBuffer.getFFTSize());
    m_wienerFilter.prepare(m_stftBuffer.getFFTSize());
    m_stftBuffer.reset();

    m_dryBuffer.resize(samplesPerBlock);
    m_wetBuffer.resize(samplesPerBlock);
}

void NoiseReducerCore::reset() {
    m_noiseGate.reset();
    m_spectralSub.reset();
    m_wienerFilter.reset();
    m_stftBuffer.reset();
}

void NoiseReducerCore::setAlgorithm(ReductionAlgorithm algo) {
    m_algorithm = algo;
}

void NoiseReducerCore::setThresholdDb(float thresholdDb) {
    m_thresholdDb = thresholdDb;
    m_noiseGate.setThreshold(thresholdDb);
}

void NoiseReducerCore::setIntensity(float intensity) {
    m_intensity = std::max(0.0f, std::min(1.0f, intensity));
    m_noiseGate.setRatio(1.0f + m_intensity * 19.0f); // 1.0 a 20.0
    m_spectralSub.setAlpha(1.0f + m_intensity * 3.0f); // 1.0 a 4.0
    m_wienerFilter.setMinGainFloor(-40.0f * m_intensity);
}

void NoiseReducerCore::setDryWet(float dryWet) {
    m_dryWet = std::max(0.0f, std::min(1.0f, dryWet));
}

void NoiseReducerCore::setNoiseLearn(bool isLearning) {
    m_isLearning = isLearning;
    m_spectralSub.setLearning(isLearning);
    m_wienerFilter.setLearning(isLearning);
}

void NoiseReducerCore::processBuffer(const float* input, float* output, size_t numSamples) {
    if (m_wetBuffer.size() < numSamples) {
        m_dryBuffer.resize(numSamples);
        m_wetBuffer.resize(numSamples);
    }

    // Guardar copia dry
    std::copy(input, input + numSamples, m_dryBuffer.begin());

    if (m_algorithm == ReductionAlgorithm::NoiseGate) {
        m_noiseGate.processBuffer(input, m_wetBuffer.data(), numSamples);
    } else if (m_algorithm == ReductionAlgorithm::SpectralSubtraction) {
        m_stftBuffer.process(input, m_wetBuffer.data(), numSamples, [this](std::vector<std::complex<float>>& spectrum) {
            m_spectralSub.processSpectrum(spectrum);
        });
    } else if (m_algorithm == ReductionAlgorithm::WienerFilter) {
        m_stftBuffer.process(input, m_wetBuffer.data(), numSamples, [this](std::vector<std::complex<float>>& spectrum) {
            m_wienerFilter.processSpectrum(spectrum);
        });
    }

    // Mezcla Dry / Wet
    for (size_t i = 0; i < numSamples; ++i) {
        output[i] = (1.0f - m_dryWet) * m_dryBuffer[i] + m_dryWet * m_wetBuffer[i];
    }
}

} // namespace NoiseReducer
