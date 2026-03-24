#include <iostream>
#include <thread>
#include "AudioState.h"
#include "AudioSetup.h"
#include "AudioEngine.h"
#include "UIManager.h"
#ifdef AUDIO_THREADS_ENABLE_GUI
#include "GuiManager.h"
#endif

int main() {
    // Struct para guardar informações relevantes sobre o estado do áudio (acessada por múltiplas threads)
    AudioState state;

    std::cout << "Iniciando programa..." << std::endl;

    /*
    TO DO:
        Esta função deve carregar os arquivos mp3 na memória. 
        Para tanto, ela deve popular o vetor de Tracks do struct AudioState 
        com os dados PCM dos árquivos, usando a biblioteca minimp3. 
    */
    audioSetup(&state);

    /*
        Inicializa instância da struct RtAudio, que abstrai a interface de áudio para o programa
    */
    rtAudioSetup(&state);
    
    /* 
        Tenta inicializar stream. Se falhar, encerra programa.
        Ao iniciar a stream, RtAudio instrui o SO a criar um thread de áudio dedicada.
        Quando o DAC precisar de dados, o SO acorda a thread dedicada, que executa nossa função callback
        para alimentar o buffer que é enviado ao DAC
    */ 
    if (startAudioStream(&state) != 0) {
        std::cout << "Falha ao iniciar o motor de áudio. Encerrando...\n";
        return 1;
    }

#ifdef AUDIO_THREADS_ENABLE_GUI
    // GUI em janela como interface principal. Se falhar, usa fallback terminal.
    if (!runGui(&state)) {
        std::cout << "Falha ao inicializar GUI. Voltando para modo terminal...\n";
        std::thread uiThread(uiLoop, &state);
        if (uiThread.joinable()) {
            uiThread.join();
        }
    }
#else
    std::thread uiThread(uiLoop, &state);
    if (uiThread.joinable()) {
        uiThread.join();
    }
#endif

    // Destrói instância de RtAudio e libera memória
    std::cout << "Encerrando programa..." << std::endl;
    if (stopAudioStream()) {
        std::cout << "Erro ao fechar stream\n";
        return 1;
    } 

    return 0;
}