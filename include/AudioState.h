#pragma once
#include <vector>
#include <atomic>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cctype>

// Função helper para capitalizar string
inline std::string capitalize(const std::string& str) {
    if (str.empty()) return str;
    std::string result = str;
    result[0] = std::toupper(result[0]);
    return result;
}

// Representa faixas (árquivo) de áudio
struct Track {
    // Representa dados PCM do áudio (usando floats)
    std::vector<float> pcmData;
    // Representa se a faixa está tocando
    std::atomic<bool> isPlaying{false};
    // Nome amigável da faixa
    std::string trackName{"Unknown"};
    // Sample rate original do arquivo MP3
    int originalSampleRate{0};
    // Quantidade total de frames da faixa
    size_t totalFrames{0};
    // Cursor de reprodução da faixa (atualizado pela thread do instrumento)
    std::atomic<size_t> currentFrame{0};

    // 1. Construtor padrão necessário porque vamos criar construtores customizados
    Track() = default;

    // 2. Construtor de Movimento (Move Constructor)
    // Isso ensina o std::vector como mover uma Track na memória
    Track(Track&& other) noexcept 
        : pcmData(std::move(other.pcmData)), 
          isPlaying(other.isPlaying.load(std::memory_order_relaxed)),
          trackName(std::move(other.trackName)),
            originalSampleRate(other.originalSampleRate),
            totalFrames(other.totalFrames),
            currentFrame(other.currentFrame.load(std::memory_order_relaxed)) {
    }

    // 3. Operador de Atribuição de Movimento (Move Assignment Operator)
    Track& operator=(Track&& other) noexcept {
        if (this != &other) {
            pcmData = std::move(other.pcmData);
            isPlaying.store(other.isPlaying.load(std::memory_order_relaxed), std::memory_order_relaxed);
            trackName = std::move(other.trackName);
            originalSampleRate = other.originalSampleRate;
            totalFrames = other.totalFrames;
            currentFrame.store(other.currentFrame.load(std::memory_order_relaxed), std::memory_order_relaxed);
        }
        return *this;
    }

    // 4. Deletar explicitamente as cópias para evitar acidentes de performance
    Track(const Track&) = delete;
    Track& operator=(const Track&) = delete;
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
    // Sample rate global do sistema de áudio (48000 Hz típico)
    int sampleRate{48000};
    // Nome do asset sendo carregado (ex: "Assets" ou "Assets2")
    std::string assetName{"Assets"};
    // Threads de instrumento (uma por faixa)
    std::vector<std::thread> instrumentThreads;
    // Sincronização de play/pause/reset/stop das threads de instrumento
    std::mutex controlMutex;
    std::condition_variable controlCv;
};