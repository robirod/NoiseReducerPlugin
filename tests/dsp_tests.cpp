#include "../src/dsp/FFTEngine.h"
#include "../src/dsp/STFTBuffer.h"
#include "../src/dsp/NoiseGate.h"
#include "../src/dsp/NoiseReducerCore.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <cassert>

void testFFT() {
    std::cout << "[Test] Verificando FFTEngine (Transformada e Inversa)... ";
    const size_t N = 512;
    NoiseReducer::FFTEngine fft(N);

    std::vector<float> input(N);
    for (size_t i = 0; i < N; ++i) {
        input[i] = std::sin(2.0f * 3.14159265f * 10.0f * i / static_cast<float>(N));
    }

    std::vector<std::complex<float>> spec;
    fft.forward(input.data(), spec);

    std::vector<float> reconstructed(N);
    fft.inverse(spec, reconstructed.data());

    float maxError = 0.0f;
    for (size_t i = 0; i < N; ++i) {
        float err = std::abs(input[i] - reconstructed[i]);
        if (err > maxError) maxError = err;
    }

    assert(maxError < 1e-4f);
    std::cout << "PASÓ (Error Máximo = " << maxError << ")" << std::endl;
}

void testNoiseGate() {
    std::cout << "[Test] Verificando NoiseGate (Atenuación bajo umbral)... ";
    NoiseReducer::NoiseGate gate;
    gate.prepare(44100.0);
    gate.setThreshold(-20.0f); // ~0.1 lineal
    gate.setRatio(10.0f);

    // Señal silenciosa (debe ser atenuada)
    std::vector<float> quietInput(1000, 0.01f);
    std::vector<float> quietOutput(1000, 0.0f);
    gate.processBuffer(quietInput.data(), quietOutput.data(), 1000);

    assert(std::abs(quietOutput.back()) < std::abs(quietInput.back()));
    std::cout << "PASÓ (Salida en silencio = " << quietOutput.back() << " vs Entrada = " << quietInput.back() << ")" << std::endl;
}

void testNoiseReducerCore() {
    std::cout << "[Test] Verificando NoiseReducerCore (Modos y procesamiento sin crashes)... ";
    NoiseReducer::NoiseReducerCore core(512, 128);
    core.prepare(44100.0, 256);

    std::vector<float> buffer(512);
    for (size_t i = 0; i < 512; ++i) {
        buffer[i] = static_cast<float>(i % 100) / 100.0f - 0.5f;
    }

    std::vector<float> outBuffer(512);

    // Modo Spectral Subtraction
    core.setAlgorithm(NoiseReducer::ReductionAlgorithm::SpectralSubtraction);
    core.processBuffer(buffer.data(), outBuffer.data(), 512);

    // Modo Wiener
    core.setAlgorithm(NoiseReducer::ReductionAlgorithm::WienerFilter);
    core.processBuffer(buffer.data(), outBuffer.data(), 512);

    // Modo Noise Gate
    core.setAlgorithm(NoiseReducer::ReductionAlgorithm::NoiseGate);
    core.processBuffer(buffer.data(), outBuffer.data(), 512);

    std::cout << "PASÓ" << std::endl;
}

int main() {
    std::cout << "===========================================" << std::endl;
    std::cout << "      Ejecutando Pruebas Unitarias DSP     " << std::endl;
    std::cout << "===========================================" << std::endl;

    testFFT();
    testNoiseGate();
    testNoiseReducerCore();

    std::cout << "[✓] TODAS LAS PRUEBAS UNITARIAS PASARON EXITOSAMENTE." << std::endl;
    return 0;
}
