#ifndef FFT_ENGINE_H
#define FFT_ENGINE_H

#include <vector>
#include <complex>
#include <cmath>
#include <cstddef>

namespace NoiseReducer {

class FFTEngine {
public:
    explicit FFTEngine(size_t fftSize = 1024);
    ~FFTEngine() = default;

    size_t getFFTSize() const { return m_fftSize; }
    
    // Transformada directa e inversa (in-place o out-of-place)
    void forward(const float* inputReal, std::vector<std::complex<float>>& outputComplex);
    void inverse(const std::vector<std::complex<float>>& inputComplex, float* outputReal);

private:
    void bitReversePermutation(std::vector<std::complex<float>>& x);
    void computeFFT(std::vector<std::complex<float>>& x, bool invert);

    size_t m_fftSize;
    size_t m_numBits;
    std::vector<std::complex<float>> m_twiddles;
};

} // namespace NoiseReducer

#endif // FFT_ENGINE_H
