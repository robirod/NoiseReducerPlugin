#ifndef WIENER_FILTER_H
#define WIENER_FILTER_H

#include <vector>
#include <complex>
#include <cstddef>

namespace NoiseReducer {

class WienerFilter {
public:
    explicit WienerFilter(size_t fftSize = 1024);
    ~WienerFilter() = default;

    void prepare(size_t fftSize);
    void reset();

    void setSmoothingFactor(float alpha); // 0.8 a 0.99
    void setMinGainFloor(float minGainDb); // -40.0 a 0.0 dB
    void setLearning(bool isLearning) { m_isLearning = isLearning; }

    void processSpectrum(std::vector<std::complex<float>>& spectrum);
    void learnFrame(const std::vector<std::complex<float>>& spectrum);

private:
    size_t m_fftSize;
    float m_smoothingFactor;
    float m_minGain;
    bool m_isLearning;

    std::vector<float> m_noisePSD;
    std::vector<float> m_prevPriorPSD;
    size_t m_learnedFrames;
};

} // namespace NoiseReducer

#endif // WIENER_FILTER_H
