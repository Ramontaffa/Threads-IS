#include "RtAudio.h"
#include "AudioState.h"
#include <iostream>

RtAudio::StreamParameters parameters;
RtAudio *dac = nullptr;
unsigned int sampleRate;
unsigned int bufferSize;

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

    // Itera todos os frames
    for (unsigned int i = 0; i < nFrames; i++) {
        // Preenche com silêncio se mudo ou se a faixa terminou
        if (!state->globalPlay.load(std::memory_order_relaxed) || 
            state->currentFrame.load(std::memory_order_relaxed) >= state->totalFrames) {            
            for (unsigned int c = 0; c < state->channels; c++) {
                *out++ = 0.0f; 
            }
            continue;
        }

        // Frame atual
        size_t frameIndex = state->currentFrame.load(std::memory_order_relaxed);
        
        //  Preenche dados dos canais esquerdo e direito
        for (unsigned int c = 0; c < state->channels; c++) {
            // Inicializa com silêncio e sem faixas ativas
            float mixedSample = 0.0f;
            int activeTracks = 0;
            
            // Calcula índice do frame correspondente ao canal atual
            size_t sampleIndex = (frameIndex * state->channels) + c;

            // Adiciona frames das faixas que estão ativas
            for (auto& track : state->tracks) {
                if (track.isPlaying.load(std::memory_order_relaxed)) {
                    mixedSample += track.pcmData[sampleIndex];
                    activeTracks++;
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

        // Incrementa frame na struct 
        state->currentFrame.fetch_add(1, std::memory_order_relaxed);
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
    return erro;
}
