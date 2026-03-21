#include "Visualizador.h"
#include <iostream>
#include <thread>
#include <chrono>

/*
    Função executada por uma thread responsável por um faixa específica.
    Imprime no terminal informações da faixa. 
    TO DO:
        - Adaptar para não espamar console e imprimir quando faixa for mutada.
*/
void visualizerLoop(AudioState* state, int trackIndex) {

    // Em loop até thread de UI atualizar a flag de encerrar o programa
    while (state->programRunning.load(std::memory_order_relaxed)) {
        
        // Se faixa estiver tocando e programa estiver tocando,
        if (state->tracks[trackIndex].isPlaying.load(std::memory_order_relaxed) &&
            state->globalPlay.load(std::memory_order_relaxed)) {

            std::cout << "[Faixa n. " << trackIndex << "]  está tocando... \n";
        }
        
        // Atualiza status a cada 20 FPS
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}