# Dynamic Noise Reducer (C++ / JUCE & Python Audio DSP)

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Build System](https://img.shields.io/badge/CMake-3.22%2B-brightgreen.svg)](https://cmake.org/)
[![Framework](https://img.shields.io/badge/JUCE-7.0-orange.svg)](https://juce.com/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

Un plugin de audio profesional en tiempo real (**VST3 / AU / Standalone**) y suite de procesamiento DSP para **Reducción de Ruido de Fondo Dinámica**. Diseñado en C++17 modular con integración JUCE y respaldado por un prototipo interactivo en Python de cero dependencias.

---

## 🎯 Alcance del Proyecto & Características

Este proyecto implementa tres de las técnicas más reconocidas en el procesamiento digital de señales de audio (DSP) para eliminar hums de baja frecuencia, ruido rosa de ventiladores y ruido ambiental no deseado:

1. **Noise Gate (Dominio del Tiempo)**:
   - Seguidor de envolvente rápido con constantes de ataque ($t_{\text{attack}}$) y liberación ($t_{\text{release}}$) exponenciales.
   - Atenuación progresiva basada en ratio cuando la señal cae por debajo del umbral (`Threshold`).

2. **Resta Espectral / Spectral Subtraction (Dominio de la Frecuencia STFT)**:
   - Análisis y síntesis mediante **STFT** (Short-Time Fourier Transform) con ventana de **Hann** y reconstitución por **Overlap-Add** (solapamiento al 75%).
   - Estimación del espectro de ruido $P_n(f)$ durante tramos silenciosos.
   - Factor de sobre-sustracción ajustable ($\alpha$) y piso espectral ($\beta$) para mitigar artefactos del tipo *"ruido musical"* (*musical noise*).

3. **Filtro de Wiener / Wiener Filter (Estimación SNR Dinámica)**:
   - Algoritmo de minimización del error cuadrático medio (MMSE) en el dominio espectral.
   - Estimación de SNR *a priori* y *a posteriori* utilizando el método **Decision-Directed** (Ephraim-Malah).
   - Generación de curva de ganancia suave $G(f) = \frac{\xi(f)}{1 + \xi(f)}$ para reducir ruido manteniendo la naturalidad de la voz e instrumentos.

---

## 🏗️ Arquitectura del Repositorio

```text
NoiseReducerPlugin/
├── CMakeLists.txt                 # Sistema de construcción CMake (C++ DSP + JUCE Plugin)
├── README.md                      # Documentación principal del proyecto
├── .gitignore                     # Configuración Git para C++, CMake, JUCE y Python
├── python/                        # Prototipo rápido de validación DSP (Python puro)
│   ├── generate_test_audio.py     # Generador de audio sintético (tono + armónicos + ruido)
│   ├── noise_reduction_proto.py   # Algoritmos completos en Python (0 dependencias)
│   └── requirements.txt           # Requisitos opcionales
├── src/                           # Código Fuente C++
│   ├── dsp/                       # Núcleo DSP puro (Thread-safe, desacoplado de la GUI)
│   │   ├── FFTEngine.h / .cpp     # FFT Radix-2 / IFFT optimizado (Cooley-Tukey)
│   │   ├── STFTBuffer.h / .cpp    # Buffers circulares, ventaneo Hann y Overlap-Add
│   │   ├── NoiseGate.h / .cpp     # Procesador dinámico temporal
│   │   ├── SpectralSub.h / .cpp   # Motor de Resta Espectral
│   │   ├── WienerFilter.h / .cpp  # Motor de Filtro de Wiener
│   │   └── NoiseReducerCore.h/.cpp# Unificador de algoritmos y controles Dry/Wet
│   ├── cli/                       # Herramienta de consola para procesamiento offline de WAVs
│   │   └── main_cli.cpp
│   └── plugin/                    # Integración con JUCE Framework
│       ├── PluginProcessor.h/.cpp # AudioProcessor y gestión APVTS para DAW
│       └── PluginEditor.h/.cpp    # Interfaz gráfica (GUI) funcional con controles JUCE
└── tests/                         # Suite de Pruebas Unitarias C++
    └── dsp_tests.cpp
```

---

## 📐 Fundamento Matemático

### 1. Transformada de Fourier de Tiempo Reducido (STFT)
Dada una señal discreta $x[n]$, el análisis por bloques ventaneados se define como:

$$X(m, k) = \sum_{n=0}^{N-1} x[n + m H] \cdot w[n] \cdot e^{-j \frac{2\pi}{N} k n}$$

donde $N=1024$ es el tamaño de la FFT, $H=256$ es el salto (*hop size*) y $w[n]$ es la ventana de Hann.

### 2. Resta Espectral
$$|\hat{S}(m, k)|^2 = \max \left( |X(m, k)|^2 - \alpha \cdot P_n(k), \; \beta \cdot P_n(k) \right)$$

### 3. Filtro de Wiener
La ganancia óptima $G(m, k)$ para cada bin de frecuencia viene dada por:

$$G(m, k) = \frac{\xi(m, k)}{1 + \xi(m, k)}$$

donde $\xi(m, k)$ es la estimación de la SNR *a priori*.

---

## 🚀 Compilación e Instalación

### Requisitos Previos
- **Compilador C++17**: Apple Clang, GCC o MSVC.
- **CMake**: Versión 3.22 o superior.
- **Python 3.x** (para ejecutar el prototipo de prueba).

### 1. Compilar el Motor C++ DSP, CLI y Pruebas Unitarias (Rápido)
```bash
# Configurar y compilar en carpeta build/
cmake -B build -DBUILD_JUCE_PLUGIN=OFF
cmake --build build

# Ejecutar las pruebas unitarias
./build/noise_reducer_tests
```

### 2. Procesar un archivo `.wav` desde la Terminal (CLI C++)
```bash
# Sintaxis: ./noise_reducer_cli <entrada.wav> <salida.wav> [modo: 0=Gate, 1=SpectralSub, 2=Wiener] [intensidad: 0.0-1.0]
./build/noise_reducer_cli python/noisy_signal.wav output_restado.wav 1 1.0
```

### 3. Compilar el Plugin de Audio VST3 / AU con JUCE
```bash
# Descarga automáticamente JUCE vía CMake FetchContent y genera los plugins VST3/AU
cmake -B build_plugin -DBUILD_JUCE_PLUGIN=ON
cmake --build build_plugin --config Release
```

---

## 🐍 Ejecución del Prototipo en Python

El prototipo en Python es 100% autosuficiente (no requiere librerías adicionales como SciPy o NumPy) e incluye su propio generador de FFT y archivos WAV.

```bash
# Genera audios de prueba y ejecuta los 3 algoritmos en Python
python3 python/noise_reduction_proto.py
```

Archivos generados en `python/`:
- `clean_signal.wav`: Señal limpia original (armónicos modulados).
- `noise_only.wav`: Muestra del ruido ambiental.
- `noisy_signal.wav`: Señal contaminada con ruido.
- `output_noisegate.wav`: Resultado de Noise Gate.
- `output_spectral_sub.wav`: Resultado de Resta Espectral.
- `output_wiener.wav`: Resultado de Filtro de Wiener.

---

## 🧪 Resultados de Verificación

```text
===========================================
      Ejecutando Pruebas Unitarias DSP     
===========================================
[Test] Verificando FFTEngine (Transformada e Inversa)... PASÓ (Error Máximo = 2.38419e-07)
[Test] Verificando NoiseGate (Atenuación bajo umbral)... PASÓ
[Test] Verificando NoiseReducerCore (Modos y procesamiento sin crashes)... PASÓ
[✓] TODAS LAS PRUEBAS UNITARIAS PASARON EXITOSAMENTE.
```

---

## 📜 Licencia

Este proyecto está distribuido bajo la licencia MIT. Siéntete libre de utilizarlo como referencia o base para proyectos de portafolio y desarrollo de audio.
