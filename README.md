# Threads-IS

**1. Main thread**
- carrega arquivos mp3 na memória
- carrega vetores de PCM de cada arquivo usando biblioteca minimp3
- determina quantidade de frames dos arquivos (todos tem a mesma duração e sample rate)
- recebe e repassa comandos da thread de interface
- aloca memória e inicializa variáveis para thread de áudio

**2. GUI/CLI thread**
- Comandos para 
    - parar/tocar global
    - encerrar processo
    - parar/tocar faixa
- Cria/deleta threads para renderizar gráficos para cada faixa
- Cada faixa é uma struct num vetor
   - Ao parar/tocar uma faixa, código muda boolean da struct no índice correspondente no vetor de faixas

**2. Audio callback thread**
- ajusta volume global ao tirar/adicionar faixas
- dentro de um loop de buffers de áudio,
   - checa quais faixas estão ativas (se nenhuma, preenche com 0)
   - monta frame para enviar para API RtAudio
   - envia frame para API

### Threads e Áudio
A thread de áudio concentra o envio dos buffers para a API, que envia aos drivers do SO. 
Não usamos mutex nem alocamos memória nela para que o envio dos buffers não atrase.

Para cumprir com o requisito de criar/encerrar threads com o comando de usuários, 
- usamos threads separadas para renderizar a interface de cada faixa.

**Dependências**
- RtAudio: https://caml.music.mcgill.ca/~gary/rtaudio/
- minimp3: https://github.com/lieff/minimp3?tab=readme-ov-file

---
### Como incluir bibliotecas no projeto

1. RtAudio
- Baixe o código fonte do projeto (é open source)
- Vá até o diretório do projeto
- Crie e navegue para um diretório 'build'
- Rode o arquivo CMake que está no diretório acima: `cmake ../`
- Rode o arquivo makefile criado: `make`
- Copie os arquivos binários nos diretórios adequados dos seu so: `sudo make install``

2. minimp3
- ...