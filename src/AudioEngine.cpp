#include "RtAudio.h"


RtAudio::StreamParameters parameters;
RtAudio dac;

// configuracoes iniciais
void rtAudioSetup(AudioState* state) {
    state.channels = 2;
    unsigned int sampleRate = 44100; // hz
    unsigned int bufferFrames = 256; // samples em cada buffer
    // calcula quantidade total
    state.totalFrames = sampleRate * audiolength;

    parameters.deviceId = dac.getDefaultOutputDevice();
    parameters.nChannels = state.channels;
    parameters.firstChannel = 0;    
}

// inicia stream passando funcao callback
int startAudioStream(AudioState* state) {
    try {
        dac.openStream(&parameters, nullptr, RTAUDIO_FLOAT32,
                       sampleRate, &bufferFrames, &mixingCallback, (void *)&state);
        dac.startStream();
    }
    catch (RtAudioError& e) {
        e.printMessage();
        return 1;
    }
}

// encerra stream
int stopAudioStream() {

    try {
        dac.stopStream();
    }
    catch (RtAudioError& e) {
        e.printMessage();
    }
    
    if (dac.isStreamOpen()) {
        dac.closeStream();
    }
    return 0;
}

// callback para alimentar API
// ver documentacao de RtAudio se dúvidas
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