#pragma once
#include <vector>
#include <atomic>

// faixas de áudio
struct Track {
    std::vector<float> pcmData;
    std::atomic<bool> isPlaying{false};
};

struct AudioState {
    std::vector<Track> tracks;
    // Pausar
    std::atomic<bool> globalPlay{false};
    // Encerrar
    std::atomic<bool> programRunning{true}; 
    
    size_t totalFrames{0};
    std::atomic<size_t> currentFrame{0};
    unsigned int channels{2}; // esquerda/direita
};