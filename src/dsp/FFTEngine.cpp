#include "FFTEngine.h"
#include <algorithm>
#include <stdexcept>

namespace NoiseReducer {

FFTEngine::FFTEngine(size_t fftSize)
    : m_fftSize(fftSize)
{
    // Validar potencia de 2
    if (fftSize == 0 || (fftSize & (fftSize - 1)) != 0) {
        throw std::invalid_argument("FFT size must be a power of 2.");
    }

    m_numBits = 0;
    while ((1ULL << m_numBits) < m_fftSize) {
        m_numBits++;
    }

    // Precalcular factores Twiddle
    m_twiddles.resize(m_fftSize / 2);
    const float pi = std::acos(-1.0f);
    for (size_t i = 0; i < m_fftSize / 2; ++i) {
        float angle = -2.0f * pi * static_cast<float>(i) / static_cast<float>(m_fftSize);
        m_twiddles[i] = std::complex<float>(std::cos(angle), std::sin(angle));
    }
}

void FFTEngine::bitReversePermutation(std::vector<std::complex<float>>& x) {
    for (size_t i = 0; i < m_fftSize; ++i) {
        size_t rev = 0;
        size_t temp = i;
        for (size_t b = 0; b < m_numBits; ++b) {
            rev = (rev << 1) | (temp & 1);
            temp >>= 1;
        }
        if (i < rev) {
            std::swap(x[i], x[rev]);
        }
    }
}

void FFTEngine::computeFFT(std::vector<std::complex<float>>& x, bool invert) {
    bitReversePermutation(x);

    for (size_t len = 2; len <= m_fftSize; len <<= 1) {
        size_t halfLen = len >> 1;
        size_t step = m_fftSize / len;

        for (size_t i = 0; i < m_fftSize; i += len) {
            for (size_t j = 0; j < halfLen; ++j) {
                std::complex<float> w = m_twiddles[j * step];
                if (invert) {
                    w = std::conj(w);
                }
                std::complex<float> u = x[i + j];
                std::complex<float> v = x[i + j + halfLen] * w;
                x[i + j] = u + v;
                x[i + j + halfLen] = u - v;
            }
        }
    }

    if (invert) {
        const float norm = 1.0f / static_cast<float>(m_fftSize);
        for (size_t i = 0; i < m_fftSize; ++i) {
            x[i] *= norm;
        }
    }
}

void FFTEngine::forward(const float* inputReal, std::vector<std::complex<float>>& outputComplex) {
    outputComplex.resize(m_fftSize);
    for (size_t i = 0; i < m_fftSize; ++i) {
        outputComplex[i] = std::complex<float>(inputReal[i], 0.0f);
    }
    computeFFT(outputComplex, false);
}

void FFTEngine::inverse(const std::vector<std::complex<float>>& inputComplex, float* outputReal) {
    std::vector<std::complex<float>> buffer = inputComplex;
    buffer.resize(m_fftSize);
    computeFFT(buffer, true);
    for (size_t i = 0; i < m_fftSize; ++i) {
        outputReal[i] = buffer[i].real();
    }
}

} // namespace NoiseReducer
