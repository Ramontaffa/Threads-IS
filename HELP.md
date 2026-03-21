## Conceitos de Áudio

**Áudio digital**
- O áudio digital funciona reproduzindo um fluxo constante de amostras de áudio (audio samples) para o conversor digital-analógico (DAC) da sua placa de som ou interface de áudio. 
- As amostras são reproduzidas a uma taxa constante conhecida como taxa de amostragem (sample rate).

**Taxa de amostragem/Sample rate**
- A taxa de amostragem é o número de vezes por segundo que um sinal analógico é medido (amostrado) para convertê-lo em áudio digital, medida em Hertz (Hz) ou quilohertz (kHz).

**Bit-depth/Profundidae**
- O tamanho/profundidade da amostra é a resolução ou precisão de cada medição, (por exemplo, 16 bits ou 24 bits), determinando a faixa dinâmica.
- Maior profundidade indica arquivos mais pesados, mas maior variação possível de volume.

**Audio drivers**
- Em sistemas operacionais de uso geral, programas normalmente não enviam amostras individuais para o DAC
- Programas enviam buffers de amostras para o driver ou uma camada intermediária do sistema operacional.

**Bibliotecas de áudio**
- Bibliotecas podem expor APIs que abstraem o tratamento de drivers de áudio específicos do sistema operacional.
- `Seu código -> API da biblioteca -> Driver de áudio específico do sistema operacional -> DAC`

**Audio frame**
- em programação de áudio, é um conjunto de amostras (samples) contíguas processadas de uma só vez

**Buffer de áudio**
- conjunto de frames processados ​​em conjunto
- possui um número predeterminado de frames (por exemplo, 64, 128, 512, 1024).
- permite que a CPU do computador processe o áudio em lotes gerenciáveis, em vez de frame a frame
    - Buffers menores resultam em menor latência (ideal para gravação ao vivo), mas maior carga da CPU. 
    - Buffers maiores resultam em maior latência, mas melhor estabilidade para mixagem e masterização.

**Amostra de áudio/Audio sample**
- menor unidade de tempo utilizável do áudio digital
- contém informações de amplitude (volume) naquele ponto específico no tempo
- Cada amostra representa a altura da forma de onda em um determinado ponto no tempo. 

**Bit-rate/Taxa de bits**
- medida de dados por segundo dado por taxa de amostragem (44.100) * profundidade de bits (16) * canais (2)

**Stereo**
- formato de áudio em que há dois canais independentes de sinais (esquerdo e direito)

**PCM/Modulação por Código de Pulso**
- um método para converter sinais analógicos (voz, música, vídeo) em dados digitais binários (0s e 1s)
- O processo envolve amostragem (captura da amplitude), quantização (arredondamento de valores) e codificação (conversão para binário)

**Sinal entrelaçado/Interweaved data**
- Modo de armazenar amostras de múltiplos canais
- Armazena amostras de canais diferentes contiguamente.
- Os dados PCM intercalados são armazenados uma amostra por canal, na ordem dos canais, antes de passar para a próxima amostra. 
- Um frame PCM é composto por um grupo de amostras para cada canal. 
    - Se você tiver áudio estéreo com canais esquerdo e direito, uma amostra de cada um formará um frame.

• Frame 0: `[amostra_esquerda][amostra_direita]`
• Frame 1: `[amostra_esquerda][amostra_direita]`
• Frame 2: `[amostra_esquerda][amostra_direita]`
...

**Volume**
- Ao somar sinais de áudio, o volume aumenta. 
- Se você somar muitas faixas, o sinal excederá o valor máximo permitido (geralmente 1,0 ou -1,0 para áudio de ponto flutuante) e causará distorção digital.