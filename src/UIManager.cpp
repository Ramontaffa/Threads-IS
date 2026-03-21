#include "UIManager.h"
#include "Visualizador.h"
#include <iostream>
#include <string>
#include <thread>
#include <vector>

void uiLoop(AudioState* state) {
    std::vector<std::thread> threadsVisualizacao;

    // Cria thread independente para cada faixa
    for (size_t i = 0; i < state->tracks.size(); ++i) {
        threadsVisualizacao.emplace_back(visualizerLoop, state, i);
    }

    std::cout << "\n--- Controles --- \n";
    std::cout << "Comandos: '0', '1', '2' '3, '4', '5', '6', '7', '8', '9', '10' (Ativar/Desativar faixa) \n";
    std::cout << "\t 'p' (Tocar/Pausar todas as faixas) \n";
    std::cout << "\t 'q' (Encerrar programa) \n";

    // Espera e executa comando do usuário em loop
    std::string input;
    while (state->programRunning.load(std::memory_order_relaxed)) {
        std::cin >> input;

        if (input == "q") {
            state->programRunning.store(false, std::memory_order_relaxed);
        } 
        else if (input == "p") {
            bool currentPlay = state->globalPlay.load(std::memory_order_relaxed);
            state->globalPlay.store(!currentPlay, std::memory_order_relaxed);
            std::cout << (!currentPlay ? "TOCANDO" : "PAUSADO") << "\n";
        }
        else if (input == "0" || input == "1" || input == "2" || 
                 input == "3" || input == "4" || input == "5" ||
                 input == "6" || input == "7" || input == "8" ||
                 input == "9" || input == "10") {
            int trackIdx = std::stoi(input);
            bool currentStatus = state->tracks[trackIdx].isPlaying.load(std::memory_order_relaxed);
            state->tracks[trackIdx].isPlaying.store(!currentStatus, std::memory_order_relaxed);
        }
    }

    // Usuário encerrou programa
    // Mata todas as threads de visualização
    for (auto& t : threadsVisualizacao) {
        if (t.joinable()) {
            t.join();
        }
    }
}