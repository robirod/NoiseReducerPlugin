#include "SpectralSub.h"
#include <cmath>
#include <algorithm>

namespace NoiseReducer {

SpectralSub::SpectralSub(size_t fftSize)
    : m_fftSize(fftSize),
      m_alpha(2.0f),
      m_beta(0.03f),
      m_isLearning(false),
      m_learnedFrames(0)
{
    m_noisePSD.assign(m_fftSize / 2 + 1, 1e-6f);
}

void SpectralSub::prepare(size_t fftSize) {
    m_fftSize = fftSize;
    m_noisePSD.assign(m_fftSize / 2 + 1, 1e-6f);
    reset();
}

void SpectralSub::reset() {
    m_learnedFrames = 0;
}

void SpectralSub::setAlpha(float alpha) {
    m_alpha = std::max(0.5f, std::min(6.0f, alpha));
}

void SpectralSub::setBeta(float beta) {
    m_beta = std::max(0.001f, std::min(0.3f, beta));
}

void SpectralSub::learnFrame(const std::vector<std::complex<float>>& spectrum) {
    size_t half = m_fftSize / 2 + 1;
    for (size_t k = 0; k < half; ++k) {
        float mag2 = std::norm(spectrum[k]);
        m_noisePSD[k] = (m_noisePSD[k] * static_cast<float>(m_learnedFrames) + mag2) / static_cast<float>(m_learnedFrames + 1);
    }
    m_learnedFrames++;
}

void SpectralSub::processSpectrum(std::vector<std::complex<float>>& spectrum) {
    if (m_isLearning) {
        learnFrame(spectrum);
    }

    size_t half = m_fftSize / 2 + 1;
    for (size_t k = 0; k < half; ++k) {
        float mag2 = std::norm(spectrum[k]);
        float mag = std::sqrt(mag2);

        float subMag2 = mag2 - m_alpha * m_noisePSD[k];
        float floorMag2 = m_beta * m_noisePSD[k];
        float cleanMag2 = std::max(subMag2, floorMag2);
        float cleanMag = std::sqrt(cleanMag2);

        float gain = (mag > 1e-9f) ? (cleanMag / mag) : 0.0f;
        gain = std::max(m_beta, std::min(1.0f, gain));

        spectrum[k] *= gain;
        if (k > 0 && k < m_fftSize / 2) {
            spectrum[m_fftSize - k] *= gain;
        }
    }
}

} // namespace NoiseReducer
