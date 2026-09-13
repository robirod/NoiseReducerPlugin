#!/usr/bin/env python3
"""
noise_reduction_proto.py
Prototipo DSP autosuficiente en Python para Reducción de Ruido (sin dependencias externas):
- Noise Gate (Dinámica en tiempo)
- Resta Espectral / Spectral Subtraction (Dominio Frecuencia STFT con FFT pura)
- Filtro de Wiener / Wiener Filter (Estimación SNR a priori/posteriori)
"""

import os
import math
import wave
import struct

# --- FFT Radix-2 Cooley-Tukey en Python puro ---
def fft(x):
    N = len(x)
    if N <= 1:
        return list(x)
    even = fft(x[0::2])
    odd = fft(x[1::2])
    T = [math.e ** (-2j * math.pi * k / N) * odd[k] for k in range(N // 2)]
    return [even[k] + T[k] for k in range(N // 2)] + [even[k] - T[k] for k in range(N // 2)]

def ifft(x):
    N = len(x)
    x_conj = [c.conjugate() for c in x]
    X_fft = fft(x_conj)
    return [c.conjugate() / N for c in X_fft]

# --- Lectura y Escritura de Archivos WAV 16-bit Mono ---
def read_wav(filename):
    with wave.open(filename, 'rb') as wf:
        n_channels = wf.getnchannels()
        sampwidth = wf.getsampwidth()
        framerate = wf.getframerate()
        n_frames = wf.getnframes()
        data = wf.readframes(n_frames)
        
    samples = []
    if sampwidth == 2:
        fmt = f"<{n_frames * n_channels}h"
        raw_samples = struct.unpack(fmt, data)
        # Convertir mono (si es estéreo, tomar el canal izquierdo)
        for i in range(0, len(raw_samples), n_channels):
            samples.append(raw_samples[i] / 32768.0)
    else:
        raise ValueError("Solo soporta archivos WAV PCM de 16 bits.")
        
    return framerate, samples

def write_wav(filename, samples, sample_rate=44100):
    with wave.open(filename, 'wb') as wf:
        wf.setnchannels(1)
        wf.setsampwidth(2)
        wf.setframerate(sample_rate)
        packed = bytearray()
        for s in samples:
            clamped = max(-1.0, min(1.0, float(s)))
            val = int(clamped * 32767.0)
            packed.extend(struct.pack('<h', val))
        wf.writeframes(packed)
    print(f"[+] Archivo generado: {filename}")

# --- 1. Algoritmo Noise Gate ---
def noise_gate(audio_data, threshold_db=-24.0, ratio=8.0, attack_ms=10.0, release_ms=100.0, sample_rate=44100):
    threshold_lin = 10.0 ** (threshold_db / 20.0)
    alpha_attack = math.exp(-1.0 / (sample_rate * (attack_ms / 1000.0)))
    alpha_release = math.exp(-1.0 / (sample_rate * (release_ms / 1000.0)))
    
    envelope = 0.0
    processed = [0.0] * len(audio_data)
    
    for i, sample in enumerate(audio_data):
        input_abs = abs(sample)
        if input_abs > envelope:
            envelope = alpha_attack * envelope + (1.0 - alpha_attack) * input_abs
        else:
            envelope = alpha_release * envelope + (1.0 - alpha_release) * input_abs
            
        if envelope < threshold_lin and envelope > 1e-9:
            db_below = 20.0 * math.log10(envelope / threshold_lin)
            gain_db = db_below * (1.0 - 1.0 / ratio)
            gain = 10.0 ** (gain_db / 20.0)
        elif envelope <= 1e-9:
            gain = 0.0
        else:
            gain = 1.0
            
        processed[i] = sample * gain
        
    return processed

# --- 2. Algoritmo Resta Espectral ---
def spectral_subtraction(audio_data, sample_rate=44100, frame_size=512, hop_size=256, alpha=2.0, beta=0.03):
    # Ventana de Hann
    window = [0.5 * (1.0 - math.cos(2.0 * math.pi * n / (frame_size - 1))) for n in range(frame_size)]
    num_frames = (len(audio_data) - frame_size) // hop_size + 1
    
    # Estimación de perfil de ruido (primeros 0.4s)
    noise_frames_count = max(1, int(0.4 * sample_rate / hop_size))
    noise_mag2 = [0.0] * (frame_size // 2 + 1)
    
    for i in range(min(noise_frames_count, num_frames)):
        frame = [audio_data[i * hop_size + n] * window[n] for n in range(frame_size)]
        spec = fft(frame)[:frame_size // 2 + 1]
        for k in range(len(spec)):
            noise_mag2[k] += (spec[k].real**2 + spec[k].imag**2)
            
    for k in range(len(noise_mag2)):
        noise_mag2[k] /= noise_frames_count
        
    output_signal = [0.0] * len(audio_data)
    window_sum = [0.0] * len(audio_data)
    
    for i in range(num_frames):
        start = i * hop_size
        frame = [audio_data[start + n] * window[n] for n in range(frame_size)]
        spec = fft(frame)
        
        half = frame_size // 2 + 1
        spec_half = spec[:half]
        
        cleaned_spec = [0j] * frame_size
        for k in range(half):
            mag2 = spec_half[k].real**2 + spec_half[k].imag**2
            sub_mag2 = mag2 - alpha * noise_mag2[k]
            floor_mag2 = beta * noise_mag2[k]
            res_mag2 = max(sub_mag2, floor_mag2)
            
            mag = math.sqrt(mag2)
            if mag > 1e-9:
                gain = math.sqrt(res_mag2) / mag
            else:
                gain = 0.0
                
            cleaned_spec[k] = spec_half[k] * gain
            if 0 < k < frame_size // 2:
                cleaned_spec[frame_size - k] = cleaned_spec[k].conjugate()
                
        cleaned_frame = [c.real for c in ifft(cleaned_spec)]
        
        for n in range(frame_size):
            idx = start + n
            if idx < len(audio_data):
                output_signal[idx] += cleaned_frame[n] * window[n]
                window_sum[idx] += window[n] ** 2
                
    for idx in range(len(audio_data)):
        if window_sum[idx] > 1e-6:
            output_signal[idx] /= window_sum[idx]
            
    return output_signal

# --- 3. Algoritmo Filtro de Wiener ---
def wiener_filter(audio_data, sample_rate=44100, frame_size=512, hop_size=256, smoothing_factor=0.95):
    window = [0.5 * (1.0 - math.cos(2.0 * math.pi * n / (frame_size - 1))) for n in range(frame_size)]
    num_frames = (len(audio_data) - frame_size) // hop_size + 1
    
    noise_frames_count = max(1, int(0.4 * sample_rate / hop_size))
    noise_psd = [0.0] * (frame_size // 2 + 1)
    
    for i in range(min(noise_frames_count, num_frames)):
        frame = [audio_data[i * hop_size + n] * window[n] for n in range(frame_size)]
        spec = fft(frame)[:frame_size // 2 + 1]
        for k in range(len(spec)):
            noise_psd[k] += (spec[k].real**2 + spec[k].imag**2)
            
    for k in range(len(noise_psd)):
        noise_psd[k] = max(noise_psd[k] / noise_frames_count, 1e-9)
        
    output_signal = [0.0] * len(audio_data)
    window_sum = [0.0] * len(audio_data)
    prev_prior_psd = [0.0] * (frame_size // 2 + 1)
    
    for i in range(num_frames):
        start = i * hop_size
        frame = [audio_data[start + n] * window[n] for n in range(frame_size)]
        spec = fft(frame)
        half = frame_size // 2 + 1
        spec_half = spec[:half]
        
        cleaned_spec = [0j] * frame_size
        for k in range(half):
            mag2 = spec_half[k].real**2 + spec_half[k].imag**2
            post_snr = mag2 / noise_psd[k]
            
            prior_snr = smoothing_factor * (prev_prior_psd[k] / noise_psd[k]) + (1.0 - smoothing_factor) * max(post_snr - 1.0, 0.0)
            gain = prior_snr / (prior_snr + 1.0)
            gain = max(0.05, min(1.0, gain))
            
            prev_prior_psd[k] = mag2 * (gain ** 2)
            
            cleaned_spec[k] = spec_half[k] * gain
            if 0 < k < frame_size // 2:
                cleaned_spec[frame_size - k] = cleaned_spec[k].conjugate()
                
        cleaned_frame = [c.real for c in ifft(cleaned_spec)]
        
        for n in range(frame_size):
            idx = start + n
            if idx < len(audio_data):
                output_signal[idx] += cleaned_frame[n] * window[n]
                window_sum[idx] += window[n] ** 2
                
    for idx in range(len(audio_data)):
        if window_sum[idx] > 1e-6:
            output_signal[idx] /= window_sum[idx]
            
    return output_signal

def main():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    input_wav = os.path.join(script_dir, "noisy_signal.wav")
    
    if not os.path.exists(input_wav):
        print("[*] Generando archivos de prueba de audio...")
        from generate_test_audio import generate_audio_files
        generate_audio_files(script_dir)
        
    sr, audio = read_wav(input_wav)
    print(f"[*] Procesando '{input_wav}' (Tasa: {sr} Hz, Muestras: {len(audio)})")
    
    print("[1/3] Ejecutando Noise Gate...")
    out_gate = noise_gate(audio, sample_rate=sr)
    write_wav(os.path.join(script_dir, "output_noisegate.wav"), out_gate, sr)
    
    print("[2/3] Ejecutando Resta Espectral...")
    out_spec = spectral_subtraction(audio, sample_rate=sr)
    write_wav(os.path.join(script_dir, "output_spectral_sub.wav"), out_spec, sr)
    
    print("[3/3] Ejecutando Filtro de Wiener...")
    out_wiener = wiener_filter(audio, sample_rate=sr)
    write_wav(os.path.join(script_dir, "output_wiener.wav"), out_wiener, sr)
    
    print("[✓] Prototipo ejecutado correctamente.")

if __name__ == "__main__":
    main()
