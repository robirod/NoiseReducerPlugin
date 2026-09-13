#ifndef STFT_BUFFER_H
#define STFT_BUFFER_H

#include "FFTEngine.h"
#include <vector>
#include <complex>
#include <functional>

namespace NoiseReducer {

class STFTBuffer {
public:
    STFTBuffer(size_t fftSize = 1024, size_t hopSize = 256);
    ~STFTBuffer() = default;

    void reset();
    
    // Procesa un buffer de audio de entrada y genera las muestras procesadas por overlap-add
    // callback de procesamiento espectral: espectro -> espectro procesado
    using ProcessCallback = std::function<void(std::vector<std::complex<float>>& spectrum)>;

    void process(const float* input, float* output, size_t numSamples, ProcessCallback callback);

    size_t getFFTSize() const { return m_fftSize; }
    size_t getHopSize() const { return m_hopSize; }

private:
    void processFrame(ProcessCallback callback);

    size_t m_fftSize;
    size_t m_hopSize;

    FFTEngine m_fftEngine;

    std::vector<float> m_window;
    std::vector<float> m_inputFifo;
    std::vector<float> m_outputFifo;
    
    size_t m_inputFifoWriteIdx;
    size_t m_samplesInFifo;

    std::vector<float> m_fftInputFrame;
    std::vector<float> m_fftOutputFrame;
    std::vector<std::complex<float>> m_spectrum;
};

} // namespace NoiseReducer

#endif // STFT_BUFFER_H
