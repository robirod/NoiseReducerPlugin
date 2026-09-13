#!/usr/bin/env python3
"""
generate_test_audio.py
Genera archivos .wav sintéticos para pruebas de reducción de ruido:
1. clean_signal.wav: Tonos armónicos modulados (simulando voz/instrumento limpio)
2. noise_only.wav: Hum de baja frecuencia + ruido rosa/blanco (simulando ventilador)
3. noisy_signal.wav: Señal limpia mezclada con ruido de fondo
"""

import os
import math
import wave
import struct

def generate_audio_files(output_dir="."):
    os.makedirs(output_dir, exist_ok=True)
    
    sample_rate = 44100
    duration_sec = 5.0
    num_samples = int(sample_rate * duration_sec)
    
    clean_samples = []
    noise_samples = []
    noisy_samples = []
    
    # Parámetros de la señal limpia (armónicos modulados tipo voz/flauta)
    f0 = 220.0  # La3 (220 Hz)
    
    # Generar muestras
    import random
    random.seed(42)  # Reproducible
    
    for i in range(num_samples):
        t = i / sample_rate
        
        # Envolvente de volumen (simula silabas/notas con pausas)
        # Activo durante [0.5, 1.8]s y [2.5, 4.2]s
        env = 0.0
        if 0.5 <= t <= 1.8:
            env = 0.5 * (1.0 - math.cos(2 * math.pi * (t - 0.5) / 1.3))
        elif 2.5 <= t <= 4.2:
            env = 0.5 * (1.0 - math.cos(2 * math.pi * (t - 2.5) / 1.7))
            
        # Señal limpia (fundamental + 2º y 3º armónico)
        clean = env * (
            0.6 * math.sin(2 * math.pi * f0 * t) +
            0.3 * math.sin(2 * math.pi * (2 * f0) * t) +
            0.15 * math.sin(2 * math.pi * (3 * f0) * t)
        )
        
        # Ruido de fondo: Hum de 60 Hz + 120 Hz + Ruido Blanco
        hum_60 = 0.08 * math.sin(2 * math.pi * 60.0 * t)
        hum_120 = 0.04 * math.sin(2 * math.pi * 120.0 * t)
        white_noise = 0.05 * (random.random() * 2.0 - 1.0)
        noise = hum_60 + hum_120 + white_noise
        
        # Mezcla
        noisy = clean + noise
        
        clean_samples.append(clean)
        noise_samples.append(noise)
        noisy_samples.append(noisy)
        
    def save_wav(filename, samples):
        filepath = os.path.join(output_dir, filename)
        with wave.open(filepath, 'w') as wf:
            wf.setnchannels(1)        # Mono
            wf.setsampwidth(2)        # 16-bit PCM
            wf.setframerate(sample_rate)
            
            packed_data = bytearray()
            for s in samples:
                # Clip entre -1.0 y 1.0
                clamped = max(-1.0, min(1.0, s))
                int_val = int(clamped * 32767.0)
                packed_data.extend(struct.pack('<h', int_val))
            wf.writeframes(packed_data)
        print(f"[+] Archivo creado: {filepath}")

    save_wav("clean_signal.wav", clean_samples)
    save_wav("noise_only.wav", noise_samples)
    save_wav("noisy_signal.wav", noisy_samples)

if __name__ == "__main__":
    script_dir = os.path.dirname(os.path.abspath(__file__))
    generate_audio_files(script_dir)
