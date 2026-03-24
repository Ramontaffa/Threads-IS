#include "RtAudio.h"
#include "AudioState.h"
#include <iostream>
#include <chrono>

RtAudio::StreamParameters parameters;
RtAudio *dac = nullptr;
unsigned int sampleRate;
unsigned int bufferSize;

namespace {

void instrumentLoop(AudioState* state, size_t trackIndex) {
    constexpr auto tick = std::chrono::milliseconds(5);

    while (state->programRunning.load(std::memory_order_relaxed)) {
        std::unique_lock<std::mutex> lock(state->controlMutex);
        state->controlCv.wait_for(lock, tick, [&]() {
            return !state->programRunning.load(std::memory_order_relaxed) ||
                   (state->globalPlay.load(std::memory_order_relaxed) &&
                    state->tracks[trackIndex].isPlaying.load(std::memory_order_relaxed));
        });

        if (!state->programRunning.load(std::memory_order_relaxed)) {
            break;
        }

        const bool shouldAdvance = state->globalPlay.load(std::memory_order_relaxed) &&
                                   state->tracks[trackIndex].isPlaying.load(std::memory_order_relaxed);
        lock.unlock();

        if (!shouldAdvance) {
            continue;
        }

        // A thread do instrumento permanece ativa e sincronizada por eventos.
        // O avanço de frame fica centralizado na callback de áudio para evitar saltos.
        (void)trackIndex;
    }
}

} // namespace

void rtAudioSetup(AudioState* state) {
    if (state->channels != 2) {
        std::cout << "Erro ao fazer decoding. Faixas devem ser stereo\n";
    }
    state->channels = 2;
    sampleRate = state->sampleRate; // Usa sample rate dos arquivos carregados
    bufferSize = 256;   // Cada buffer terá 256 samples

    std::cout << "Inicializando RtAudio com sample rate: " << sampleRate << " Hz\n";

    try {
        dac = new RtAudio();
    } catch (RtAudioErrorType &error) {
        exit(EXIT_FAILURE);
    }

    parameters.deviceId = dac->getDefaultOutputDevice();
    parameters.nChannels = state->channels;
    parameters.firstChannel = 0;
    
}

// =======================================
// FUNÇÃO CALLBACK
// =======================================
int mixingCallback(void *outputBuffer, void *inputBuffer, unsigned int nFrames,
                   double streamTime, RtAudioStreamStatus status, void *userData) {
    
    // Casting
    float *out = static_cast<float *>(outputBuffer);
    AudioState *state = static_cast<AudioState *>(userData);

    if (status) {
        std::cerr << "Erro!" << std::endl;
        return 1;
    }

    // Itera todos os frames do buffer de áudio
    for (unsigned int i = 0; i < nFrames; i++) {
        // Preenche com silêncio se master estiver pausado.
        if (!state->globalPlay.load(std::memory_order_relaxed)) {
            for (unsigned int c = 0; c < state->channels; c++) {
                *out++ = 0.0f; 
            }
            continue;
        }
        
        //  Preenche dados dos canais esquerdo e direito
        for (unsigned int c = 0; c < state->channels; c++) {
            // Inicializa com silêncio e sem faixas ativas
            float mixedSample = 0.0f;
            int activeTracks = 0;

            // Adiciona frames das faixas que estão ativas
            for (auto& track : state->tracks) {
                if (track.isPlaying.load(std::memory_order_relaxed)) {
                    const size_t trackTotalFrames = track.totalFrames;
                    if (trackTotalFrames == 0) {
                        continue;
                    }

                    const size_t frameIndex = track.currentFrame.load(std::memory_order_relaxed) % trackTotalFrames;
                    const size_t sampleIndex = (frameIndex * state->channels) + c;
                    if (sampleIndex < track.pcmData.size()) {
                        mixedSample += track.pcmData[sampleIndex];
                        activeTracks++;
                    }
                }
            }

            // MUITO BAIXO, AJUSTAR !
            // Divide total pela quantidade de faixas ativas (diminui volume para áudio nao clippar)
            if (activeTracks > 1) {
                mixedSample /= static_cast<float>(activeTracks);
            }

            // Escreve no buffer da API e avança posição do buffer
            *out++ = mixedSample;
        }

        // Avança os cursores das faixas no mesmo ritmo do callback de áudio.
        size_t monitorFrame = state->currentFrame.load(std::memory_order_relaxed);
        bool foundMonitor = false;
        for (auto& track : state->tracks) {
            if (!track.isPlaying.load(std::memory_order_relaxed) || track.totalFrames == 0) {
                continue;
            }

            const size_t current = track.currentFrame.load(std::memory_order_relaxed);
            const size_t next = (current + 1) % track.totalFrames;
            track.currentFrame.store(next, std::memory_order_relaxed);

            if (!foundMonitor) {
                monitorFrame = next;
                foundMonitor = true;
            }
        }

        if (foundMonitor) {
            state->currentFrame.store(monitorFrame, std::memory_order_relaxed);
        }
    }

    return 0; // faz RtAudio continuar a rodar a stream
}

int startAudioStream(AudioState* state) {

    if (dac->getDeviceCount() < 1) {
        std::cout << "Sem dispositivos disponíveis!\n";
        return 1;
    }

    RtAudioErrorType err = dac->openStream(&parameters, nullptr, RTAUDIO_FLOAT32,
                                           sampleRate, &bufferSize, &mixingCallback, (void *)state);
    
 
    if (err != RTAUDIO_NO_ERROR) {
        std::cerr << "\n[ERRO CRÍTICO] Falha no openStream: " << dac->getErrorText() << std::endl;
        return 1;
    }

    std::cout << "Stream aberta com sucesso.\n";

  
    err = dac->startStream();
    
    if (err != RTAUDIO_NO_ERROR) {
        std::cerr << "\n[ERRO CRÍTICO] Falha no startStream: " << dac->getErrorText() << std::endl;
        return 1;
    }

    return 0; 
}

int stopAudioStream() {
    int erro = 0;
    
    // Captura o erro em vez de usar try/catch
    RtAudioErrorType err = dac->stopStream();
    if (err != RTAUDIO_NO_ERROR) {
        std::cerr << "Erro ao parar stream: " << dac->getErrorText() << "\n";
        erro = 1;
    }
    
    if (dac->isStreamOpen()) {
        dac->closeStream();
    }

    delete dac;
    dac = nullptr;
    return erro;
}

int startInstrumentThreads(AudioState* state) {
    if (state == nullptr) {
        return 1;
    }

    state->instrumentThreads.clear();
    state->instrumentThreads.reserve(state->tracks.size());

    for (size_t i = 0; i < state->tracks.size(); ++i) {
        state->instrumentThreads.emplace_back(instrumentLoop, state, i);
    }

    return 0;
}

void wakeInstrumentThreads(AudioState* state) {
    if (state == nullptr) {
        return;
    }
    state->controlCv.notify_all();
}

void stopInstrumentThreads(AudioState* state) {
    if (state == nullptr) {
        return;
    }

    state->programRunning.store(false, std::memory_order_relaxed);
    state->controlCv.notify_all();

    for (auto& thread : state->instrumentThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    state->instrumentThreads.clear();
}
