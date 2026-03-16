#include <iostream>
#include <thread>
#include "AudioState.h"
#include "AudioSetup.h"
#include "AudioEngine.h"
#include "UIManager.h"

int main() {
    AudioState* state;

    std::cout << "Iniciando..." << std::endl;

    /*
    Carrega arquivos mp3 na memória. Popula vetor de Tracks com dados PCM
    Usar biblioteca minimp3 
    */
    audioSetup(&state);

    /*
    Define configuracoes da API
    */
    rtAudioSetup(&state);
    
    // inicia stream
    if (!startAudioStream(&state)) {
        return 1;
    }

    // inicializa thread de ui
    std::thread uiThread(uiLoop, &state);

    // espera a thread de ui encerrar
    if (uiThread.joinable()) {
        uiThread.join();
    }

    //limpeza
    std::cout << "Encerrando..." << std::endl;
    stopAudioStream(); 

    return 0;
}