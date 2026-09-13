#include "../dsp/NoiseReducerCore.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdint>

#pragma pack(push, 1)
struct WavHeader {
    char riff[4];          // "RIFF"
    uint32_t chunkSize;
    char wave[4];          // "WAVE"
    char fmt[4];           // "fmt "
    uint32_t subchunk1Size; // 16 para PCM
    uint16_t audioFormat;   // 1 para PCM
    uint16_t numChannels;   // 1 para mono, 2 para stereo
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample; // 16
    char data[4];          // "data"
    uint32_t subchunk2Size;
};
#pragma pack(pop)

bool readWavMono16(const std::string& filename, std::vector<float>& samples, uint32_t& sampleRate) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[!] Error al abrir archivo de entrada: " << filename << std::endl;
        return false;
    }

    WavHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(WavHeader));
    if (std::strncmp(header.riff, "RIFF", 4) != 0 || std::strncmp(header.wave, "WAVE", 4) != 0) {
        std::cerr << "[!] Archivo no es un WAV válido." << std::endl;
        return false;
    }

    sampleRate = header.sampleRate;
    size_t numSamples = header.subchunk2Size / (header.bitsPerSample / 8);
    std::vector<int16_t> rawBuffer(numSamples);
    file.read(reinterpret_cast<char*>(rawBuffer.data()), header.subchunk2Size);

    samples.resize(numSamples / header.numChannels);
    for (size_t i = 0; i < samples.size(); ++i) {
        samples[i] = static_cast<float>(rawBuffer[i * header.numChannels]) / 32768.0f;
    }

    return true;
}

bool writeWavMono16(const std::string& filename, const std::vector<float>& samples, uint32_t sampleRate) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "[!] Error al abrir archivo de salida: " << filename << std::endl;
        return false;
    }

    WavHeader header;
    std::memcpy(header.riff, "RIFF", 4);
    std::memcpy(header.wave, "WAVE", 4);
    std::memcpy(header.fmt, "fmt ", 4);
    header.subchunk1Size = 16;
    header.audioFormat = 1; // PCM
    header.numChannels = 1;
    header.sampleRate = sampleRate;
    header.bitsPerSample = 16;
    header.byteRate = sampleRate * 1 * 16 / 8;
    header.blockAlign = 1 * 16 / 8;
    std::memcpy(header.data, "data", 4);
    header.subchunk2Size = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
    header.chunkSize = 36 + header.subchunk2Size;

    file.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));

    std::vector<int16_t> rawBuffer(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        float clamped = std::max(-1.0f, std::min(1.0f, samples[i]));
        rawBuffer[i] = static_cast<int16_t>(clamped * 32767.0f);
    }

    file.write(reinterpret_cast<const char*>(rawBuffer.data()), header.subchunk2Size);
    return true;
}

int main(int argc, char* argv[]) {
    std::cout << "===========================================" << std::endl;
    std::cout << "   NoiseReducer CLI (C++ Audio DSP Core)  " << std::endl;
    std::cout << "===========================================" << std::endl;

    if (argc < 3) {
        std::cout << "Uso: " << argv[0] << " <input.wav> <output.wav> [modo: 0=Gate, 1=SpectralSub, 2=Wiener] [intensity: 0.0-1.0]" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    int mode = (argc > 3) ? std::stoi(argv[3]) : 1; // Por defecto Spectral Subtraction
    float intensity = (argc > 4) ? std::stof(argv[4]) : 1.0f;

    std::vector<float> inputSamples;
    uint32_t sampleRate = 44100;
    if (!readWavMono16(inputFile, inputSamples, sampleRate)) {
        return 1;
    }

    std::cout << "[*] Archivo leído: " << inputFile << " (" << inputSamples.size() << " muestras, " << sampleRate << " Hz)" << std::endl;

    NoiseReducer::NoiseReducerCore dspCore(1024, 256);
    dspCore.prepare(sampleRate, 512);
    dspCore.setAlgorithm(static_cast<NoiseReducer::ReductionAlgorithm>(mode));
    dspCore.setIntensity(intensity);
    dspCore.setDryWet(1.0f);

    // Entrenar ruido en los primeros 0.4s (silencio inicial)
    dspCore.setNoiseLearn(true);

    std::vector<float> outputSamples(inputSamples.size());
    const size_t blockSize = 256;
    for (size_t i = 0; i < inputSamples.size(); i += blockSize) {
        size_t currentBlock = std::min(blockSize, inputSamples.size() - i);
        if (i > static_cast<size_t>(0.4 * sampleRate)) {
            dspCore.setNoiseLearn(false);
        }
        dspCore.processBuffer(&inputSamples[i], &outputSamples[i], currentBlock);
    }

    if (writeWavMono16(outputFile, outputSamples, sampleRate)) {
        std::cout << "[+] Archivo procesado guardado en: " << outputFile << std::endl;
    }

    return 0;
}
