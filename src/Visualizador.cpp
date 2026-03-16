#include "Visualizador.h"
#include <iostream>
#include <thread>
#include <chrono>

/*
Visualizador para faixa 
    imprime renderiza feed back visual para cada faixa
*/
void visualizerLoop(AudioState* state, int trackIndex) {

    // Em loop até thread de UI encerrar esta instancia
    while (state->programRunning.load(std::memory_order_relaxed)) {
        
        // Se faixa estiver tocando e programa estiver tocando,
        if (state->tracks[trackIndex].isPlaying.load(std::memory_order_relaxed) &&
            state->globalPlay.load(std::memory_order_relaxed)) {
            
            // fazer um VU ou qualquer animacao ou só imprimir msm
            // para ler volume, precisa ler PCM do frame atual
            std::cout << "[Track " << trackIndex << "] tocando... \n";
        }
        
        // ~20 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}