# Build and Run Guide (Global Context)

Este documento resume como o repositório funciona hoje para compilar e executar, com foco no fluxo atual com interface visual (Dear ImGui + GLFW) e fallback para terminal.

## 1. Visao geral da arquitetura

O projeto e um mixer de audio em C++ com multiplas threads:

- Thread principal: inicializa estado, carrega audios, sobe stream de audio e decide qual UI usar.
- Thread de audio (RtAudio callback): mistura as faixas ativas em tempo real.
- Thread/UI visual: janela ImGui (modo principal) para controles do mixer.
- Fallback UI terminal: usado quando GUI falha para inicializar.

Estado compartilhado:

- `AudioState` em `include/AudioState.h`.
- Flags atomicas para controle: `globalPlay`, `programRunning`, `currentFrame`, `isPlaying` por faixa.

## 2. Dependencias

### Build base

- CMake >= 3.15
- Compilador C++17
- Threads nativas

### Audio

- RtAudio (baixado automaticamente via CMake FetchContent)
- ALSA/PulseAudio no Linux/WSL

### GUI (branch com ImGui)

- GLFW (FetchContent)
- Dear ImGui (FetchContent)
- OpenGL

## 3. Estrutura principal de codigo

- `src/main.cpp`: fluxo de inicializacao, stream e escolha de UI.
- `src/AudioSetup.cpp`: decode MP3 e carga das faixas em memoria.
- `src/AudioEngine.cpp`: setup RtAudio + callback de mixagem.
- `src/GuiManager.cpp`: UI visual ImGui (play/pause, toggles, reset, progresso).
- `src/UIManager.cpp`: UI terminal (fallback).
- `CMakelists.txt`: configuracao de build e dependencias.

## 4. Fluxo de execucao (runtime)

1. `audioSetup(&state)` carrega os audios da pasta `assets/`.
2. `rtAudioSetup(&state)` prepara dispositivo e parametros de stream.
3. `startAudioStream(&state)` abre e inicia callback de mixagem.
4. Se GUI estiver habilitada, tenta abrir janela ImGui.
5. Se GUI falhar, usa loop de terminal automaticamente.
6. Ao encerrar UI, `programRunning` vira `false` e stream e finalizada.

## 5. Build e run no WSL (recomendado no Windows)

### 5.1 Dependencias de sistema (Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake pkg-config \
  libasound2-dev libpulse-dev \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxxf86vm-dev \
  libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev
```

### 5.2 Compilar

```bash
cmake -S . -B build
cmake --build build -j
```

### 5.3 Executar

```bash
./build/AUDIO-THREADS
```

## 6. Build e run direto pelo PowerShell (chamando WSL)

```powershell
wsl -d Ubuntu -- bash -lc "cd '/mnt/c/Users/rtaff/Desktop/CESAR/Período 26.1/IS/Threads-IS' && cmake -S . -B build && cmake --build build -j && ./build/AUDIO-THREADS"
```

## 7. Controles de interface

### GUI (ImGui)

- Play/Pause master
- Reset timeline
- 8 botoes de tracks (ON/OFF)
- Barra de progresso global
- Indicador de tracks ativas

### Terminal (fallback)

- Teclas numericas para toggle de faixa
- Tecla de play/pause global
- Tecla de reset
- Tecla para encerrar

## 8. Problemas comuns e diagnostico rapido

### Erro de build com GLFW/Wayland

- Solucao adotada no CMake: forcar backend X11 no Linux.

### Erro de OpenGL nao encontrado

- Instalar pacotes Mesa/OpenGL de desenvolvimento (sessao 5.1).

### Erro de X11 nao encontrado

- Instalar `libx11-dev` e pacotes relacionados (sessao 5.1).

### App inicia e encerra rapido

- Em testes automatizados com `timeout` isso e esperado.
- Em execucao interativa, a UI deve manter o app aberto.

## 9. Boas praticas para evoluir a interface

- Nao alterar a estrutura do vetor `tracks` depois da stream iniciar.
- Manter comunicacao UI -> audio via atomicos ja existentes.
- Evitar operacoes pesadas dentro do callback de audio.
- Evoluir UI por etapas: volume por faixa, mute/solo, layout por grupos, atalhos.
