#include "AudioState.h"
//#include "minimp3.h"

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

    int numTracks = 10;
    for (int i = 0; i < numTracks; ++i) {
        Track track;

        // Inicializar com dados de verdade (aqui inicializando com 0)
        track.pcmData.resize(state->totalFrames * state->channels, 0.0f);

        // Inicializa faixa como mudo
        track.isPlaying.store(false, std::memory_order_relaxed);

        state->tracks.push_back(std::move(track));
    }
}

/*
#define MINIMP3_IMPLEMENTATION
#include "minimp3.h""
#include <cstdio>
#include <vector>
#include <fstream>
#include <iostream>
#define MINIMP3_FLOAT_OUPUT

std::vector<float> pcm_data = decode_mp3_to_pcm("mymp3File.mp3");

stf::vector<float> decode_mp3_to_pcm(const* filename) {
    mp3_dec_t mp3d;
    mp3_dec_init(&mp3d);;

    std::ifstream ifs(filename, std::ios::binary);

    if (!ifs) {
        std::cout << "Falha ao ler arquivo " << filename << "\n";
        return {};
    }
    std::vector<float> pcm_buffer;

    // An MP3 frame size is typically around 16KB for reliable sync
    std::vector<uint8_t> input_mp3_buffer(16 * 1024); 
    size_t bytes_read = 0;
    
    // Decoding loop
    while (true) {
        // Read a chunk of MP3 data from file
        ifs.read(reinterpret_cast<char*>(input_mp3_buffer.data() + bytes_read), input_mp3_buffer.size() - bytes_read);
        size_t new_bytes = ifs.gcount();
        bytes_read += new_bytes;

        if (new_bytes == 0 && bytes_read == 0) break; // End of file

        mp3dec_frame_info_t info;
        // Output buffer for PCM samples (max 1152 samples per frame * 2 channels)
        short pcm_output[1152 * 2]; 
        
        // Decode the frame
        int samples_decoded = mp3dec_decode_frame(&mp3d, input_mp3_buffer.data(), bytes_read, pcm_output, &info);

        if (samples_decoded > 0) {
            // Append the decoded PCM data to the output buffer
            pcm_buffer.insert(pcm_buffer.end(), pcm_output, pcm_output + samples_decoded * info.channels);
        }

        if (info.frame_bytes > 0) {
            // Remove the consumed bytes from the input buffer
            bytes_read -= info.frame_bytes;
            std::memmove(input_mp3_buffer.data(), input_mp3_buffer.data() + info.frame_bytes, bytes_read);
        } else {
            // Handle error or end of stream condition (e.g. need more data)
            if (new_bytes == 0) break; // Reached end of file but couldn't decode
        }
    }

    // After the loop, the audio properties (sample rate, channels) are available in info or via mp3d.info
    std::cout << "Decoded PCM: Sample Rate=" << mp3d.info.hz << ", Channels=" << mp3d.info.channels << ", Samples=" << pcm_buffer.size() << std::endl;

    return pcm_buffer;
}
*/