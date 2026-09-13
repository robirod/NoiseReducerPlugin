#include "STFTBuffer.h"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace NoiseReducer {

STFTBuffer::STFTBuffer(size_t fftSize, size_t hopSize)
    : m_fftSize(fftSize),
      m_hopSize(hopSize),
      m_fftEngine(fftSize),
      m_inputFifoWriteIdx(0),
      m_samplesInFifo(0)
{
    // Ventana de Hann precalculada
    m_window.resize(m_fftSize);
    const float pi = std::acos(-1.0f);
    for (size_t i = 0; i < m_fftSize; ++i) {
        m_window[i] = 0.5f * (1.0f - std::cos(2.0f * pi * static_cast<float>(i) / static_cast<float>(m_fftSize - 1)));
    }

    m_inputFifo.assign(m_fftSize, 0.0f);
    m_outputFifo.assign(m_fftSize * 2, 0.0f);
    
    m_fftInputFrame.resize(m_fftSize);
    m_fftOutputFrame.resize(m_fftSize);
    m_spectrum.resize(m_fftSize);
}

void STFTBuffer::reset() {
    std::fill(m_inputFifo.begin(), m_inputFifo.end(), 0.0f);
    std::fill(m_outputFifo.begin(), m_outputFifo.end(), 0.0f);
    m_inputFifoWriteIdx = 0;
    m_samplesInFifo = 0;
}

void STFTBuffer::processFrame(ProcessCallback callback) {
    // 1. Extraer frame con ventaneo
    for (size_t i = 0; i < m_fftSize; ++i) {
        size_t readIdx = (m_inputFifoWriteIdx + i) % m_fftSize;
        m_fftInputFrame[i] = m_inputFifo[readIdx] * m_window[i];
    }

    // 2. FFT directa
    m_fftEngine.forward(m_fftInputFrame.data(), m_spectrum);

    // 3. Modificación del espectro
    if (callback) {
        callback(m_spectrum);
    }

    // 4. IFFT inversa
    m_fftEngine.inverse(m_spectrum, m_fftOutputFrame.data());

    // 5. Overlap-Add en buffer de salida con ventaneo de síntesis y normalización (4x overlap para 75% o 2x para 50%)
    float normFactor = 1.0f / (static_cast<float>(m_fftSize) / static_cast<float>(m_hopSize) * 0.375f);
    for (size_t i = 0; i < m_fftSize; ++i) {
        m_outputFifo[i] += m_fftOutputFrame[i] * m_window[i] * normFactor;
    }
}

void STFTBuffer::process(const float* input, float* output, size_t numSamples, ProcessCallback callback) {
    for (size_t s = 0; s < numSamples; ++s) {
        // Copiar entrada al FIFO de entrada
        m_inputFifo[m_inputFifoWriteIdx] = input[s];
        m_inputFifoWriteIdx = (m_inputFifoWriteIdx + 1) % m_fftSize;
        m_samplesInFifo++;

        // Si alcanzamos un salto (hopSize), procesamos un frame completo
        if (m_samplesInFifo >= m_hopSize) {
            m_samplesInFifo -= m_hopSize;
            processFrame(callback);
        }

        // Obtener muestra de salida del FIFO de salida
        output[s] = m_outputFifo[0];
        
        // Shift a la izquierda el FIFO de salida
        std::memmove(m_outputFifo.data(), m_outputFifo.data() + 1, (m_outputFifo.size() - 1) * sizeof(float));
        m_outputFifo.back() = 0.0f;
    }
}

} // namespace NoiseReducer
