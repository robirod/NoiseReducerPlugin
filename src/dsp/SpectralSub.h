#ifndef SPECTRAL_SUB_H
#define SPECTRAL_SUB_H

#include <vector>
#include <complex>
#include <cstddef>

namespace NoiseReducer {

class SpectralSub {
public:
    explicit SpectralSub(size_t fftSize = 1024);
    ~SpectralSub() = default;

    void prepare(size_t fftSize);
    void reset();

    void setAlpha(float alpha); // Factor de sobre-sustracción (1.0 a 4.0)
    void setBeta(float beta);   // Piso espectral de ganancia (0.01 a 0.2)
    void setLearning(bool isLearning) { m_isLearning = isLearning; }

    void processSpectrum(std::vector<std::complex<float>>& spectrum);
    void learnFrame(const std::vector<std::complex<float>>& spectrum);

private:
    size_t m_fftSize;
    float m_alpha;
    float m_beta;
    bool m_isLearning;

    std::vector<float> m_noisePSD;
    size_t m_learnedFrames;
};

} // namespace NoiseReducer

#endif // SPECTRAL_SUB_H
