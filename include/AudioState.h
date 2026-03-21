#pragma once
#include <vector>
#include <atomic>

// Representa faixas (árquivo) de áudio
struct Track {
    // Representa dados PCM do áudio (usando floats)
    std::vector<float> pcmData;
    // Representa se a faixa está tocando
    std::atomic<bool> isPlaying{false};
};

struct AudioState {
    // Vetor com todas as faixas de áudio
    std::vector<Track> tracks;
    // Flag para pausar/tocar todas as faixas simultâneamente
    std::atomic<bool> globalPlay{false};
    // Flag para encerrar programa
    std::atomic<bool> programRunning{true}; 
    // Quantidade total de frames que as faixas têm
    size_t totalFrames{0};
    // Frame sendo processado no momento
    std::atomic<size_t> currentFrame{0};
    // Quantital total de samples
    size_t numSamples{0};
    // Quantidade de canais (stereo = 2, esquerda e direita)
    unsigned int channels{2};
};