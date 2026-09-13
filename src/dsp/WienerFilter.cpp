#include "WienerFilter.h"
#include <cmath>
#include <algorithm>

namespace NoiseReducer {

WienerFilter::WienerFilter(size_t fftSize)
    : m_fftSize(fftSize),
      m_smoothingFactor(0.96f),
      m_minGain(0.05f),
      m_isLearning(false),
      m_learnedFrames(0)
{
    size_t half = m_fftSize / 2 + 1;
    m_noisePSD.assign(half, 1e-6f);
    m_prevPriorPSD.assign(half, 0.0f);
}

void WienerFilter::prepare(size_t fftSize) {
    m_fftSize = fftSize;
    size_t half = m_fftSize / 2 + 1;
    m_noisePSD.assign(half, 1e-6f);
    m_prevPriorPSD.assign(half, 0.0f);
    reset();
}

void WienerFilter::reset() {
    m_learnedFrames = 0;
    std::fill(m_prevPriorPSD.begin(), m_prevPriorPSD.end(), 0.0f);
}

void WienerFilter::setSmoothingFactor(float alpha) {
    m_smoothingFactor = std::max(0.5f, std::min(0.99f, alpha));
}

void WienerFilter::setMinGainFloor(float minGainDb) {
    m_minGain = std::pow(10.0f, minGainDb / 20.0f);
}

void WienerFilter::learnFrame(const std::vector<std::complex<float>>& spectrum) {
    size_t half = m_fftSize / 2 + 1;
    for (size_t k = 0; k < half; ++k) {
        float mag2 = std::norm(spectrum[k]);
        m_noisePSD[k] = (m_noisePSD[k] * static_cast<float>(m_learnedFrames) + mag2) / static_cast<float>(m_learnedFrames + 1);
    }
    m_learnedFrames++;
}

void WienerFilter::processSpectrum(std::vector<std::complex<float>>& spectrum) {
    if (m_isLearning) {
        learnFrame(spectrum);
    }

    size_t half = m_fftSize / 2 + 1;
    for (size_t k = 0; k < half; ++k) {
        float mag2 = std::norm(spectrum[k]);
        float noisePsd = std::max(m_noisePSD[k], 1e-9f);

        // SNR a posteriori
        float postSnr = mag2 / noisePsd;

        // SNR a priori (Decision-Directed Approach)
        float priorSnr = m_smoothingFactor * (m_prevPriorPSD[k] / noisePsd) +
                         (1.0f - m_smoothingFactor) * std::max(postSnr - 1.0f, 0.0f);

        // Ganancia de Wiener G = priorSnr / (1 + priorSnr)
        float gain = priorSnr / (1.0f + priorSnr);
        gain = std::max(m_minGain, std::min(1.0f, gain));

        // Actualizar PSD estimada limpia para el siguiente frame
        m_prevPriorPSD[k] = mag2 * (gain * gain);

        spectrum[k] *= gain;
        if (k > 0 && k < m_fftSize / 2) {
            spectrum[m_fftSize - k] *= gain;
        }
    }
}

} // namespace NoiseReducer
