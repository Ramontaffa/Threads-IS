

// carrega arquivos mp3 em assets/
void audioSetup(AudioState* state) {

    int numTracks = 8;
    // Alocar memória para dados PCM
    for (int i = 0; i < numTracks; ++i) {
        Track track;
        // inicializar com dados de verdade (aqui inicializando com 0)
        track.pcmData.resize(state.totalFrames * state.channels, 0.0f);
        track.isPlaying.store(false, std::memory_order_relaxed); // Inicializa mudo
        state.tracks.push_back(std::move(track));
    }
}