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
// Pergunta ao usuário qual música escolher
    std::cout << "\n=== Seleção de Música ===\n";
    std::cout << "1 - Billie Jean\n";
    std::cout << "2 - Dance No More\n";
    std::cout << "3 - Instrumental\n";
    std::cout << "4 - Back in Black\n";
    std::cout << "Escolha (1, 2, 3 ou 4): ";
    
    int choice = 1;
    std::cin >> choice;
    
    std::string songFolder;
    std::string songName;
    
    switch(choice) {
        case 2:
            songFolder = "songs/dance no more/";
            songName = "Dance No More";
            break;
        case 3:
            songFolder = "songs/instrumental/";
            songName = "Instrumental";
            break;
        case 4:
            songFolder = "songs/back in black/";
            songName = "Back in Black";
            break;
        default:
            songFolder = "songs/billie jean/";
            songName = "Billie Jean";
    }
    
    state->assetName = songName;
    std::cout << "Carregando: " << state->assetName << "\n\n";

// Lista de arquivos para carregar
    std::vector<std::string> fileNames;
    
    if (choice == 2) {
        // Dance No More
        fileNames = {
            songFolder + "bass.mp3",
            songFolder + "vocals.mp3",
            songFolder + "kick.mp3",
            songFolder + "otherdrums.mp3",
            songFolder + "hihat.mp3",
            songFolder + "snare.mp3",
            songFolder + "guitar.mp3",
            songFolder + "other.mp3",
        };
    } else if (choice == 3) {
        // Instrumental
        fileNames = {
            songFolder + "bass.mp3",
            songFolder + "synth.mp3",
            songFolder + "kick.mp3",
            songFolder + "snare.mp3",
            songFolder + "hihat.mp3",
            songFolder + "organ lead.mp3",
            songFolder + "organ.mp3",
            songFolder + "synth 2.mp3",
        };
    } else if (choice == 4) {
        // Back in Black
        fileNames = {
            songFolder + "bass.mp3",
            songFolder + "vocals.mp3",
            songFolder + "kick.mp3",
            songFolder + "hihat e cymbal.mp3",
            songFolder + "guitar.mp3",
            songFolder + "other.mp3",
            songFolder + "snare.mp3",
        };
    } else {
        // Billie Jean (default)
        fileNames = {
            songFolder + "bass.mp3",
            songFolder + "vocals.mp3",
            songFolder + "kick.mp3",
            songFolder + "other drums.mp3",
            songFolder + "guitar.mp3",
            songFolder + "other.mp3",
            songFolder + "snare.mp3",
        };
    }

    size_t maxFrames = 0;

    for (const auto& filename : fileNames) {
        Track track;
        track.isPlaying.store(false, std::memory_order_relaxed);

        int fileChannels = 0;
        int fileSampleRate = 0;

        // Extrai o nome do arquivo sem path e sem extensão
        size_t lastSlash = filename.find_last_of("/");
        size_t lastDot = filename.find_last_of(".");
        std::string trackNameOnly = filename.substr(lastSlash + 1, lastDot - lastSlash - 1);
        track.trackName = capitalize(trackNameOnly);

        std::cout << "Carregando [" << track.trackName << "]... ";
        
        if (decode_mp3_to_pcm(filename, track.pcmData, fileChannels, fileSampleRate)) {
            track.originalSampleRate = fileSampleRate;
            size_t frames = track.pcmData.size() / fileChannels;
            track.totalFrames = frames;
            track.currentFrame.store(0, std::memory_order_relaxed);
            double duration = static_cast<double>(frames) / fileSampleRate;
            std::cout << "Sucesso! (" << frames << " frames, " << fileSampleRate << " Hz, " 
                      << duration << "s)\n";
            
            // Por segurança, passa quantidade de canais (todas devem ser stereo)
            if (state->tracks.empty()) {
                state->channels = fileChannels;
                state->sampleRate = fileSampleRate;
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
        track.totalFrames = state->totalFrames;
        track.currentFrame.store(0, std::memory_order_relaxed);
    }

    std::cout << "Total de faixas carregadas: " << state->tracks.size() << "\n";
    std::cout << "Total de frames globais: " << state->totalFrames << "\n";
    std::cout << "Sample Rate: " << state->sampleRate << " Hz\n";
    double totalDuration = static_cast<double>(state->totalFrames) / state->sampleRate;
    std::cout << "Duração total: " << totalDuration << " segundos\n\n";
    
    // Avisa se tiver arquivos com sample rate diferente
    bool hasDifferentSampleRates = false;
    for (const auto& track : state->tracks) {
        if (track.originalSampleRate != state->sampleRate) {
            hasDifferentSampleRates = true;
            std::cout << "  ⚠️  " << track.trackName << " tem sample rate diferente: " 
                      << track.originalSampleRate << " Hz\n";
        }
    }
    if (hasDifferentSampleRates) {
        std::cout << "\nATENÇÃO: Arquivos com sample rates diferentes podem tocar acelerados/desacelerados!\n";
    }
}