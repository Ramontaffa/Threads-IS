# Build and Run Guide (Branch sem ImGui)

Este guia descreve como o repositório funciona na branch sem interface gráfica (ImGui), ou seja, com controle pelo terminal.

## 1. Visão geral da arquitetura

O projeto é um mixer de áudio em C++ com execução multi-thread:

- Thread principal: inicializa estado, carrega áudio, inicia stream e sobe a UI de terminal.
- Thread de áudio (callback RtAudio): mistura as faixas ativas em tempo real.
- Thread de UI terminal: lê comandos do teclado e altera o estado compartilhado.
- Threads de visualização: exibem status das faixas no terminal.

Estado compartilhado:

- Estrutura central em include/AudioState.h.
- Flags atômicas para sincronização entre threads:
  - globalPlay
  - programRunning
  - currentFrame
  - isPlaying por faixa

## 2. Dependências

Dependências de build:

- CMake >= 3.15
- Compilador C++ com suporte a C++17
- Biblioteca de threads do sistema

Dependências de áudio:

- RtAudio (baixada automaticamente via CMake FetchContent)
- minimp3 (arquivo de header já incluído no projeto)
- ALSA/PulseAudio no Linux/WSL

## 3. Estrutura principal de código

- src/main.cpp: fluxo de inicialização e encerramento do programa.
- src/AudioSetup.cpp: decode de MP3 e carga das faixas para memória.
- src/AudioEngine.cpp: setup de stream e callback de mixagem.
- src/UIManager.cpp: interface e controles via terminal.
- src/Visualizador.cpp: visualização de estado das faixas no terminal.
- CMakelists.txt: configuração do build e dependências.

## 4. Fluxo de execução (runtime)

1. audioSetup(&state) carrega as faixas de assets/.
2. rtAudioSetup(&state) prepara dispositivo e parâmetros de áudio.
3. startAudioStream(&state) abre e inicia a stream.
4. main cria a thread da UI de terminal (uiLoop).
5. UI altera estado atômico e callback lê esse estado para mixar.
6. Ao encerrar, stream é parada e recursos são liberados.

## 5. Build e run no WSL (recomendado no Windows)

### 5.1 Instalar dependências no Ubuntu

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake pkg-config \
  libasound2-dev libpulse-dev
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

## 6. Build e run via PowerShell (chamando WSL)

```powershell
wsl -d Ubuntu -- bash -lc "cd '/mnt/c/Users/rtaff/Desktop/CESAR/Período 26.1/IS/Threads-IS' && cmake -S . -B build && cmake --build build -j && ./build/AUDIO-THREADS"
```

## 7. Controles de terminal

Os comandos são lidos no loop de UI. Nesta branch sem ImGui, a interação principal é por teclado no terminal.

Exemplos de ações disponíveis:

- Ativar/desativar faixas
- Tocar/pausar reprodução global
- Reiniciar timeline
- Encerrar programa

Observação: confirme os atalhos exatos em src/UIManager.cpp, pois podem variar conforme evolução da branch.

## 8. Problemas comuns e diagnóstico rápido

### 8.1 App abre e fecha rápido

- Se executado com timeout/pipes em teste automático, pode encerrar por causa do comando de teste.
- Em execução interativa normal, o loop de UI deve manter o processo ativo.

### 8.2 Erro de áudio no WSL

- Mensagens do ALSA podem aparecer dependendo do dispositivo disponível no ambiente.
- Verifique backend de áudio no WSL/host e permissões de dispositivo.

### 8.3 Build com cache antigo de outro ambiente

- Se houver conflito de CMakeCache entre máquinas/OS:

```bash
rm -rf build
cmake -S . -B build
cmake --build build -j
```

## 9. Boas práticas para evoluir a branch terminal

- Não alterar o tamanho de tracks após iniciar stream.
- Evitar custo alto no callback de áudio.
- Manter comunicação entre threads via atomics.
- Evoluir comandos de UI de forma incremental para não afetar o motor de áudio.
