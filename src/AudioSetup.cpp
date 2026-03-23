#include "AudioSetup.h"
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cstring>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include "minimp3.h"

bool decode_mp3_to_pcm(const std::string& filename, std::vector<float>& pcm_buffer, 
                       int& outChannels, int& outSampleRate) {
    mp3dec_t mp3d;
    mp3dec_init(&mp3d);

    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        std::cerr << "Falha ao ler arquivo: " << filename << "\n";
        return false;
    }

    std::vector<uint8_t> input_mp3_buffer(16 * 1024); 
    size_t bytes_read = 0;
    
    while (true) {
        ifs.read(reinterpret_cast<char*>(input_mp3_buffer.data() + bytes_read), input_mp3_buffer.size() - bytes_read);
        size_t new_bytes = ifs.gcount();
        bytes_read += new_bytes;

        if (new_bytes == 0 && bytes_read == 0) break; // Fim do arquivo

        mp3dec_frame_info_t info;
        float pcm_output[MINIMP3_MAX_SAMPLES_PER_FRAME]; 
        
        int samples_decoded = mp3dec_decode_frame(&mp3d, input_mp3_buffer.data(), bytes_read, pcm_output, &info);

        if (samples_decoded > 0) {
            pcm_buffer.insert(pcm_buffer.end(), pcm_output, pcm_output + samples_decoded * info.channels);
            outChannels = info.channels;
            outSampleRate = info.hz;
        }

        if (info.frame_bytes > 0) {
            bytes_read -= info.frame_bytes;
            std::memmove(input_mp3_buffer.data(), input_mp3_buffer.data() + info.frame_bytes, bytes_read);
        } else {
            if (new_bytes == 0) break; 
        }
    }
    return true;
}

/*
TO DO:
    Esta função deve inicializar as Tracks definidas em no struct AudioState.
    Para tanto, ela deve 
        - calcular a quantidade de frames dos arquivos
        - calcular a quantidade de samples dos arquivos
        - alocar memória
        - converter dados do mp3 em dados PCM (em float)
        - inicializar estado das faixas
*/
void audioSetup(AudioState* state) {
// Lista de arquivos para carregar
    std::vector<std::string> fileNames = {
        "assets/1_organ_lead_trimmed.mp3",
        "assets/2_bass_trimmed.mp3",
        "assets/4_kick_trimmed.mp3"
    };

    size_t maxFrames = 0;

    for (const auto& filename : fileNames) {
        Track track;
        track.isPlaying.store(false, std::memory_order_relaxed);

        int fileChannels = 0;
        int fileSampleRate = 0;

        std::cout << "Carregando " << filename << "... ";
        
        if (decode_mp3_to_pcm(filename, track.pcmData, fileChannels, fileSampleRate)) {
            std::cout << "Sucesso! (" << track.pcmData.size() / fileChannels << " frames)\n";
            
            // Por segurança, passa quantidade de canais (todas devem ser stereo)
            if (state->tracks.empty()) {
                state->channels = fileChannels;
            }

            // Armazena arquivo com maior quantidade de frames
            size_t currentFrames = track.pcmData.size() / state->channels;
            if (currentFrames > maxFrames) {
                maxFrames = currentFrames;
            }

            state->tracks.push_back(std::move(track));
        }
    }

    state->totalFrames = maxFrames;

    // Se as faixas não tiverem o mesmo tamanho, adiciona silêncio
    for (auto& track : state->tracks) {
        size_t requiredSize = state->totalFrames * state->channels;
        if (track.pcmData.size() < requiredSize) {
            track.pcmData.resize(requiredSize, 0.0f);
        }
    }

    std::cout << "Total de faixas carregadas: " << state->tracks.size() << "\n";
    std::cout << "Total de frames globais: " << state->totalFrames << "\n";
}