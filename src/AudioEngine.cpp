//#include "RtAudio.h"
#include "AudioState.h"
#include <iostream>

RtAudio::StreamParameters parameters;
RtAudio *dac = 0;
unsigned int sampleRate;
unsigned int bufferSize;

void rtAudioSetup(AudioState* state) {
    state->channels = 2;
    sampleRate = 44100; // Taxa padrão (em Hz)
    bufferSize = 256;   // Cada buffer terá 256 samples

    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = state->channels;
    parameters.firstChannel = 0;    

    try {
        dac = new RtAudio();
    } catch (RtError &error) {
        error.printMessage();
        exit(EXIT_FAILURE);
    }
}

// inicia stream passando funcao callback
int startAudioStream(AudioState* state) {
    if (dac.getDeviceCount() < 1) {
        std::cout << "Sem dispositivos disponíveis!\n";
        return 1;
    }

    try {
        dac->openStream(&parameters, nullptr, RTAUDIO_FLOAT32,
                        sampleRate, &bufferSize, &mixingCallback, (void *)&state);
        std::cout << "Stream aberta com sucesso.\n";

        dac->startStream();
    }
    catch (RtAudioError &e) {
        e.printMessage();
        delete dac;
        return 1;
    }
}

int stopAudioStream() {

    int erro = 0;
    try {
        dac->stopStream();
    }
    catch (RtAudioError& e) {
        e.printMessage();
        erro = 1;
    }
    
    if (dac->isStreamOpen()) {
        dac->closeStream();
    }

    delete dac;

    return erro;
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

    // itera todos os frames
    for (unsigned int i = 0; i < nFrames; i++) {
        // Preenche com silencio se mudo ou se a faixa terminou
        if (!state->globalPlay.load(std::memory_order_relaxed) || 
            state->currentFrame.load(std::memory_order_relaxed) >= state->totalFrames) {            
            for (unsigned int c = 0; c < state->channels; c++) {
                *out++ = 0.0f; 
            }
            continue;
        }

        // frame atual
        size_t frameIndex = state->currentFrame.load(std::memory_order_relaxed);
        
        // popula faixas da esquerda e direita
        for (unsigned int c = 0; c < state->channels; c++) {
            // inicializa com silencio e sem faixas ativas
            float mixedSample = 0.0f;
            int activeTracks = 0;
            
            // calcula indice do frame correspondente (esquerda ou direita)
            size_t sampleIndex = (frameIndex * state->channels) + c;

            // adiciona frames de faixas ativas ao total
            for (auto& track : state->tracks) {
                if (track.isPlaying.load(std::memory_order_relaxed)) {
                    mixedSample += track.pcmData[sampleIndex];
                    activeTracks++;
                }
            }

            // divide buffer por quantidade de faixas ativas (diminui volume para audio nao clippar)
            if (activeTracks > 1) {
                mixedSample /= static_cast<float>(activeTracks);
            }

            // escreve no buffer da API e avança posicao do buffer
            *out++ = mixedSample;
        }

        // avanca cursor para proximo buffer
        state->currentFrame.fetch_add(1, std::memory_order_relaxed);
    }

    return 0; // faz RtAudio continuar a rodar a stream
}