#ifndef NOISE_REDUCER_CORE_H
#define NOISE_REDUCER_CORE_H

#include "NoiseGate.h"
#include "SpectralSub.h"
#include "WienerFilter.h"
#include "STFTBuffer.h"
#include <vector>

namespace NoiseReducer {

enum class ReductionAlgorithm {
    NoiseGate = 0,
    SpectralSubtraction = 1,
    WienerFilter = 2
};

class NoiseReducerCore {
public:
    NoiseReducerCore(size_t fftSize = 1024, size_t hopSize = 256);
    ~NoiseReducerCore() = default;

    void prepare(double sampleRate, size_t samplesPerBlock);
    void reset();

    // Controles de parámetros en tiempo real
    void setAlgorithm(ReductionAlgorithm algo);
    void setThresholdDb(float thresholdDb); // Para NoiseGate o umbral espectral
    void setIntensity(float intensity);     // 0.0 (0%) a 1.0 (100%)
    void setDryWet(float dryWet);           // 0.0 (100% dry) a 1.0 (100% wet)
    void setNoiseLearn(bool isLearning);

    void processBuffer(const float* input, float* output, size_t numSamples);

private:
    double m_sampleRate;
    ReductionAlgorithm m_algorithm;

    float m_thresholdDb;
    float m_intensity;
    float m_dryWet;
    bool m_isLearning;

    NoiseGate m_noiseGate;
    SpectralSub m_spectralSub;
    WienerFilter m_wienerFilter;
    STFTBuffer m_stftBuffer;

    std::vector<float> m_dryBuffer;
    std::vector<float> m_wetBuffer;
};

} // namespace NoiseReducer

#endif // NOISE_REDUCER_CORE_H
