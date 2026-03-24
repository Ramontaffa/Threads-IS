# Threads-IS

Um mixer de áudio em tempo real utilizando threads para paralelizar operações: thread principal para carregamento de dados, thread de áudio (callback) para mixagem em tempo real com buffer de áudio, e thread de interface (GUI ou CLI) para controle do aplicativo.

## Como Rodar

### Pré-requisitos
- **CMake** >= 3.15
- **Compilador C++17** (gcc, clang, MSVC)
- **Threads nativas do sistema operacional**

### Instalação de Dependências

#### macOS
```bash
brew install cmake
```

#### Linux/Ubuntu/WSL
```bash
sudo apt-get update
sudo apt-get install cmake build-essential
```

#### Windows (via WSL - recomendado)
```bash
wsl -d Ubuntu -- bash -lc "sudo apt-get update && sudo apt-get install cmake build-essential"
```

### Compilação e Execução

#### 🎨 Modo 1: Com ImGui (GUI Gráfica) - **PADRÃO**

**IMPORTANTE:** Para ativar a GUI gráfica, use a flag `-DAUDIO_THREADS_ENABLE_GUI=ON`:

**Em uma linha (compile + execute):**
```bash
cmake -S . -B build -DAUDIO_THREADS_ENABLE_GUI=ON && cmake --build build -j && ./build/AUDIO-THREADS
```

**Ou em etapas:**
```bash
cmake -S . -B build -DAUDIO_THREADS_ENABLE_GUI=ON   # 1. Configurar projeto COM GUI
cmake --build build -j                               # 2. Compilar com ImGui
./build/AUDIO-THREADS                                # 3. Executa com janela gráfica
```

**Se já compilou sem GUI e quer ativar agora:**
```bash
rm -rf build                                          # Limpar cache
cmake -S . -B build -DAUDIO_THREADS_ENABLE_GUI=ON   # Reconfigurar COM GUI
cmake --build build -j
./build/AUDIO-THREADS
```

**Resultado esperado:** Janela gráfica com:
- **Master Control Panel** no topo com botões Play/Pause e Reset
- **Abas (Tabs):**
  - `Mixer`: 8 botões de tracks com cores (cinza=OFF, verde=ON)
  - `Status Details`: Tabela com status detalhado de cada track
- **Barras de progresso** em tempo real com percentual
- **Asset name** e **Sample rate** exibidos

**⚠️ Se abrir em terminal mesmo com GUI ativado:**
Significa que a inicialização do ImGui falhou (OpenGL/GLFW). Veja a seção **Troubleshooting** abaixo.

---

#### 💻 Modo 2: Sem ImGui (Interface em Terminal)

Se você quer **apenas modo terminal** (sem dependências gráficas como OpenGL/GLFW):

```bash
cmake -S . -B build -DAUDIO_THREADS_ENABLE_GUI=OFF
cmake --build build -j
./build/AUDIO-THREADS
```

**Resultado:** Interface textual no terminal:
```
=====================================
      DJ Mixer (Teclado)         
=====================================

Master: TOCANDO
Frame atual: 234567 / 3745152

Faixas:
  [1] Track 1 : ON
  [2] Track 2 : OFF
  ...

Controles:
  1-8 : Ativa/desativa faixa
  ESPACO: Play/Pause global
  r : Reinicia música (frame 0)
  q : Encerrar
```

---

### 📋 Comparação Rápida

| Aspecto | Com ImGui | Sem ImGui |
|---------|-----------|-----------|
| **Comando CMake** | `cmake -S . -B build` | `cmake -S . -B build -DAUDIO_THREADS_ENABLE_GUI=OFF` |
| **Interface** | Janela gráfica bonita com botões | Terminal com teclado |
| **Dependências** | GLFW + ImGui + OpenGL | Apenas RtAudio |
| **Caso de uso** | Apresentação, uso visual | Terminal remoto, sem gráficos |

---

### 📝 Notas Importantes
- O CMake automaticamente baixa todas as dependências (RtAudio, GLFW, Dear ImGui) via FetchContent
- **Se ImGui falhar**: o programa automaticamente cai para modo terminal como fallback
- **Áudio roda em tempo real em paralelo** em ambos os modos (3 threads sincronizadas)
- O projeto foi testado em **macOS, Linux, e WSL (Windows)**

---

## Arquitetura do Projeto

### Threads Principais

**1. Main Thread (Principal)**
- Carrega arquivos MP3 da pasta `assets/` na memória
- Converte dados MP3 para PCM usando biblioteca `minimp3`
- Determina quantidade de frames (todas faixas têm mesma duração e sample rate)
- Aloca e inicializa estruturas compartilhadas (`AudioState`)
- Prepara dispositivo de áudio via RtAudio
- Inicia thread de interface (GUI ou CLI)
- Aguarda término das outras threads

**2. Audio Callback Thread (Gerada por RtAudio)**
- Thread dedicada criada pelo SO quando inicia a stream de áudio
- Executada periodicamente quando o buffer de áudio precisa ser preenchido
- **Função crítica**: mistura (mixagem) as faixas ativas em tempo real
- Algoritmo:
  1. Itera cada frame do buffer (ex: 256 samples)
  2. Para cada canal (L/R), soma amostras de todas as faixas ativas
  3. Divide pelo número de faixas ativas (gain normalization)
  4. Envia buffer ao DAC (Digital-to-Analog Converter)
- **Otimização**: Usa apenas `std::atomic` (sem mutex) para manter baixa latência
- Incrementa `currentFrame` de forma thread-safe com memory ordering relaxed

**3. UI Thread (GUI ou CLI)**
- Roda loop de interface gráfica (ImGui) ou terminal (CLI)
- Recebe comandos do usuário:
  - Play/Pause global
  - Play/Pause por faixa individual
  - Reset/Exit
- Modifica flags atômicas em `AudioState` de forma thread-safe
- Renderiza visualizadores para cada faixa ativa

### Thread Safety & Sincronização

O projeto **não usa mutex**, apenas `std::atomic` para máxima performance. Detalhes:

- **`globalPlay`** (atomic bool): Flag compartilhada para pausar/tocar tudo
- **`currentFrame`** (atomic size_t): Frame atual, incrementado com `memory_order_relaxed`
- **`isPlaying`** (atomic bool por track): Se aquela faixa está tocando
- **`programRunning`** (atomic bool): Flag para encerrar programa

Memory ordering `relaxed` é seguro aqui porque não há dependências entre operações.

---

## Dependências

- **RtAudio** 6.0.1: Abstração cross-platform para entrada/saída de áudio
- **GLFW** 3.4: Janela e input (usada na GUI)
- **Dear ImGui** 1.91.9b: UI gráfica em tempo real
- **OpenGL**: Renderização (Mac: `<OpenGL/gl.h>`, Linux/Windows: `<GL/gl.h>`)
- **minimp3** (incluído): Decodificação de MP3

---

## Estrutura de Código

```
src/main.cpp           → Iniciação, threads e fluxo principal
src/AudioSetup.cpp     → Carregamento de arquivos MP3
src/AudioEngine.cpp    → RtAudio setup e callback de mixagem
src/GuiManager.cpp     → UI gráfica (ImGui + GLFW)
src/UIManager.cpp      → UI terminal (fallback)
src/StatusMonitor.cpp  → Funções helper (formatTime, renderProgressBar)
src/Visualizador.cpp   → (A ser preenchido)

include/AudioState.h   → Estruturas compartilhadas e atomics
include/AudioEngine.h  → Funções de áudio
include/AudioSetup.h   → Funções de setup
include/GuiManager.h   → Interface gráfica
include/UIManager.h    → Interface terminal
include/StatusMonitor.h → Funções helper de formatação
include/minimp3.h      → Decodificador MP3
```

---

## 🎨 Interface Gráfica (ImGui)

### Arquitetura da UI

A interface gráfica é composta por 3 camadas principais:

#### 1️⃣ Master Control Panel (Topo)
- **Altura**: ~100px
- **Componentes**:
  - Botão **PLAY/PAUSE** (variável, verde quando tocando): ativa/desativa playback global
  - Botão **RESET**: volta ao frame 0
  - **Status em tempo real**: mostra "PLAYING" ou "PAUSED"
  - **Frame counter**: exibe frame atual vs total
  - **Barra de progresso**: visualização percentual (atualizada via `currentFrame` atômico)
- **Responsabilidade**: controle global do mixer
- **Threading**: Modifica `state->globalPlay` e `state->currentFrame` (atomics, thread-safe)

#### 2️⃣ Aba "Mixer"
- **Layout**: Grid 4x2 (4 botões por linha, 2 linhas = 8 tracks)
- **Cada botão de faixa**:
  - **Tamanho**: 270px width × 60px height
  - **Cores dinâmicas**:
    - ✅ **Verde [+]** quando `isPlaying == true` (tonalidade: RGB 50,150,80)
    - ⬜ **Cinza [ ]** quando `isPlaying == false` (tonalidade: RGB 60,60,70)
  - **Label**: `TrackName + " [ON]"` ou `" [OFF]"`
  - **Clique**: Alterna `state->tracks[i].isPlaying` (atomic toggle)
- **Responsabilidade**: controle individual de tracks de áudio
- **Atualização**: Buttons lêem estado atômico, renderizam cor diferente
- **Threading**: thread de UI (ImGui) lê atomics, audio callback lê atomics (lock-free)

#### 3️⃣ Aba "Status Details"
- **Layout**: Tabela ImGui com 4 colunas:
  1. **Track Name** (200px): nome da faixa
  2. **Status** (150px): mostra "[+] ACTIVE" (verde) ou "[ ] MUTED" (cinza)
  3. **Progress** (stretch): barra visual em escala 0-100%
  4. **Time** (100px): tempo formatado via `formatTime()`
- **Dados dinâmicos** (lidos a cada frame):
  - `trackName`: string estática (carregada em setup)
  - `isPlaying`: boolean atômico (colorido em tempo real)
  - `currentFrame / totalFrames`: progress ratio (atômica, atualizada por audio thread)
  - `formatTime(currentFrame, sampleRate)`: retorna "mm:ss" (ex: "01:23")
- **Responsabilidade**: monitorar status detalhado de cada track

### Helper Functions (StatusMonitor.h/.cpp)

Para manter a UI limpa, usamos funções utilitárias:

**`std::string formatTime(size_t frames, uint32_t sampleRate)`**
- **Entrada**: número de frames (uint) e taxa de amostragem (ex: 44100 Hz)
- **Saída**: string formatada "mm:ss" (ex: "03:45")
- **Exemplo**: 176400 frames @ 44100 Hz = 4 segundos = "00:04"
- **Uso**: Renderizar tempo em Status Details

**`std::string renderProgressBar(size_t current, size_t total, int width)`**
- **Entrada**: frame atual, total de frames, largura desejada (ex: 40 chars)
- **Saída**: barra ASCII (ex: "================================----" para 87% progress)
- **Uso**: Renderizar progresso em modo terminal (fallback CLI)

### Atualização em Tempo Real

A UI se atualiza **automaticamente** a cada frame do loop ImGui:

```cpp
while (!glfwWindowShouldClose(window) && state->programRunning) {
    glfwPollEvents();
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    // Lê atomics de forma thread-safe
    bool isMasterPlaying = state->globalPlay.load(std::memory_order_relaxed);
    size_t currentFrame = state->currentFrame.load(std::memory_order_relaxed);
    
    // Renderiza com valores atuais
    ImGui::Text("Time: %zu / %zu", currentFrame, state->totalFrames);
    
    ImGui::Render();
    // ... swap buffers
}
```

**Características**:
- ⚡ **Lock-free**: Usa `std::atomic` sem mutex (máxima performance)
- 🔄 **Não-bloqueante**: Audio thread nunca espera por UI
- 📊 **Sincronização implícita**: Memory ordering garante consistência
- 🎯 **60 FPS**: ImGui roda ~60 vezes/segundo (sincronizado com glfwSwapInterval)

### Como a Audio Thread Atualiza a UI

**Sem comunicação direta!** A estratégia é:

1. **Audio callback** (callback thread) incrementa `state->currentFrame` em cada bloco de áudio
2. **UI loop** (ImGui thread) lê `state->currentFrame` a cada frame
3. **Não há locks**: `std::atomic` com `memory_order_relaxed` é suficiente

Exemplo:
```cpp
// Em GuiManager.cpp
size_t currentFrame = state->currentFrame.load(std::memory_order_relaxed);
ImGui::ProgressBar(currentFrame / (float)state->totalFrames);

// Em AudioEngine.cpp (callback, outra thread)
for (size_t i = 0; i < frameCount; i++) {
    // ... mixar áudio ...
    state->currentFrame.store(state->currentFrame.load() + 1, std::memory_order_relaxed);
}
```

---

## ⚠️ Troubleshooting & Debug

### ImGui não está abrindo (ou fica aberto em modo terminal)

Se você rodou `./build/AUDIO-THREADS` e a janela gráfica não apareceu, o programa automaticamente caiu para **modo terminal fallback**. Isso é normal em alguns ambientes.

#### 🔍 Modo Debug com Mensagens Verbosas

Para descobrir **exatamente onde** a inicialização do ImGui está falhando:

**1. Limpe build anterior:**
```bash
rm -rf build
```

**2. Recompile com debug:**
```bash
cmake -S . -B build
cmake --build build -j
```

**3. Rode capturando stderr (onde logs de debug vão):**
```bash
./build/AUDIO-THREADS 2>&1 | grep -E "DEBUG|ERRO" | head -20
```

**Ou veja tudo:**
```bash
./build/AUDIO-THREADS 2>&1 | head -50
```

#### Mensagens de Debug esperadas:

✅ Se funcionar:
```
[DEBUG] Iniciando GLFW...
[DEBUG] Criando janela GLFW...
[DEBUG] Janela criada com sucesso. Inicializando contexto OpenGL...
[DEBUG] Inicializando ImGui para GLFW+OpenGL...
[DEBUG] Inicializando ImGui OpenGL3 backend com GLSL versão: #version 410
[DEBUG] ImGui inicializado com sucesso!
```

❌ Se falhar, procure por `ERRO:` para identificar:
- **`glfwInit() falhou`** → Problema com GLFW/display
- **`glfwCreateWindow() falhou`** → Sem acesso a janelas gráficas
- **`ImGui_ImplGlfw_InitForOpenGL() falhou`** → Problema com ImGui setup
- **`ImGui_ImplOpenGL3_Init() falhou`** → Problema com contexto OpenGL

#### Soluções comuns por SO:

**macOS:**
- Certifique-se que GLFW e OpenGL estão disponíveis (ambos nativos)
- Se em **headless mode** (sem tela), use `-DAUDIO_THREADS_ENABLE_GUI=OFF`

**Linux/WSL:**
- Instale dependências gráficas:
  ```bash
  sudo apt-get install libgl1-mesa-dev libxrandr-dev libxinerama-dev libxcursor-dev
  ```
- Se em **X11 forwarding remoto**, use `-DAUDIO_THREADS_ENABLE_GUI=OFF`

**Windows/WSL:**
- WSL1 não tem suporte direto a gráficos; use WSL2 com **X Server (vcxsrv)** instalado
- Ou simplesmente use modo CLI com `-DAUDIO_THREADS_ENABLE_GUI=OFF`

#### Fallback automático:

Se ImGui falhar mesmo assim, o programa automaticamente usa **interface em terminal** (UIManager.cpp). Funciona perfeitamente!

Para forçar modo terminal sem tentar ImGui:
```bash
cmake -S . -B build -DAUDIO_THREADS_ENABLE_GUI=OFF
cmake --build build -j
./build/AUDIO-THREADS
```

---

## Conceitos-Chave: Threads, Concorrência e Paralelismo

Para entender este projeto profundamente, consulte o documento [`THREADS_EXPLICACAO.md`](THREADS_EXPLICACAO.md) que detalha:

- **Paralelismo real** vs **Concorrência lógica**: Como 3 threads rodam simultaneamente
- **Mutex vs Atomic**: Por que não usamos lock (e como atomics evitam glitches)
- **Memory Ordering**: `memory_order_relaxed` e quando usar
- **Real-time audio constraints**: Latência (<100ms) e jitter (zero para áudio perfeito)
- **Race conditions**: Como as evitamos com atomics e read-only separation
- **Producer-Consumer pattern**: UI thread (producer) e audio callback (consumer)
- **Lock-free programming**: Técnicas usadas para máxima performance

**📖 Recomendação**: Leia este documento para entender a **arquitetura completa** do projeto!