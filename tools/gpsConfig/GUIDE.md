# U-BLOX Config Tool — Como Usar

## O que é

Projeto PlatformIO para ESP32 que conversa com um receptor GPS u-blox
(via protocolo binário UBX) pela UART. Serve para consultar e alterar
a configuração do receptor (baudrate, taxa de atualização, dynamic
model, fix mode, constelações GNSS) e para descobrir detalhes sobre o
próprio módulo (firmware, hardware, se é original ou clone, etc).

Desde a migração para PlatformIO, a ferramenta também **exporta**, em
formato C++ pronto para colar, a configuração exata que você testou e
aplicou durante a sessão — veja [Exportando a configuração](#exportando-a-configuração-para-o-firmware-oficial)
mais abaixo. Isso existe porque o próprio módulo GPS tem limitação de
memória persistente/bateria de backup, então em vez de depender do
receptor guardar a configuração sozinho, o firmware oficial reaplica
os comandos a cada boot.

## Ligação

Em `src/main.cpp`:

```cpp
constexpr int GPS_RX = 43; // TX do ESP <- RX do GPS
constexpr int GPS_TX = 13; // RX do ESP -> TX do GPS
```

Ajuste esses dois números se sua fiação for diferente.

O monitor serial do PC (USB) roda fixo em 115200 (já configurado em
`platformio.ini` como `monitor_speed`). A UART que fala com o GPS é a
`gpsSerial` (Serial1 do ESP32), e o baudrate dela é **dinâmico** — é
sobre isso que o próximo tópico explica.

## Detecção automática de baudrate

Ao ligar, o sketch testa sozinho os baudrates mais comuns (9600,
19200, 38400, 57600, 115200, 230400): manda um poll de MON-VER em
cada um e vê em qual deles o receptor responde. Assim que acha, usa
esse baudrate pro resto da sessão e mostra na tela "Baudrate
detectado: ...".

Isso existe porque, sem ele, se você troca o baudrate do receptor
pela opção **B** e depois o ESP32 reinicia (queda de energia, reset,
etc.), o sketch voltava a falar no baudrate padrão do código-fonte e
não conseguia mais conversar com o receptor. Com a detecção
automática isso não é mais necessário.

Se por algum motivo a detecção falhar (nenhum dos baudrates testados
respondeu — cabo desconectado, módulo desligado, baudrate fora da
lista testada), o sketch avisa e assume o último valor conhecido.
Confira a fiação e, se precisar, troque manualmente pela opção **B**.

## Menu (via Monitor Serial, 115200 baud, terminador de linha "Newline")

Consultas (envia o comando e mostra o pacote cru interpretado):

```
1-9  MON-VER, MON-HW, CFG-PRT, CFG-RATE, CFG-NAV5, CFG-GNSS,
     CFG-CFG, MON-HW2, MON-GNSS (a ordem exata aparece no menu
     impresso, gerado a partir de UBXCommands.h)
```

Configuração:

| Tecla | Ação |
|---|---|
| `B` | Alterar baudrate do receptor (troca no chip e acompanha localmente) |
| `F` | Alterar measurement rate / frequência de atualização |
| `M` | Alterar Dynamic Model (Portable, Automotive, Airborne 1g/2g/4g...) |
| `X` | Forçar Fix Mode (2D only / 3D only / Auto) |
| `G` | Habilitar/desabilitar uma constelação GNSS |
| `S` | Salvar a configuração atual na memória não volátil do receptor |
| `E` | **Exportar** para C++ tudo que foi aplicado com sucesso na sessão |

Diagnóstico:

| Tecla | Ação |
|---|---|
| `D` | Roda a sequência completa de consultas e imprime um relatório resumido |
| `I` | Reimprime o último relatório já coletado, sem consultar de novo |
| `L` | Monitor de fix ao vivo (NAV-PVT), sai ao apertar qualquer tecla |

## Dynamic Model x Fix Mode — pra que serve cada um

**Dynamic Model** (opção `M`) ajusta os filtros internos do receptor
pro tipo de movimento esperado (parado, carro, avião, etc.) — ele não
decide se o fix é 2D ou 3D.

**Fix Mode** (opção `X`) é quem decide isso: com "Auto" (padrão de
fábrica na maioria dos receptores), o receptor entrega uma posição
mesmo com poucos satélites visíveis, calculando uma altitude "de
mentirinha" (assumida, não medida) pra fechar a solução em 2D.
Forçando "3D only", o receptor só entrega fix quando tem satélites
suficientes pra calcular altitude de verdade — útil se seu código usa
a altitude do GPS pra algo crítico (ex.: parte do controle de um
aeromodelo/drone), já que evita usar um valor de altitude que não
reflete a realidade.

## Sobre o GLONASS não "pegar"

A lógica de habilitar/desabilitar constelação (opção `G`) lê o bloco
de configuração atual do receptor (CFG-GNSS), altera só o bit de
habilitado do bloco da constelação escolhida e reenvia — os outros
blocos ficam intocados. Isso está correto e é a forma recomendada
pela própria u-blox de alterar uma constelação sem mexer nas demais.

Se ao tentar habilitar GLONASS você receber "constelação não
suportada", é porque o bloco de GLONASS simplesmente não aparece na
resposta do CFG-GNSS — ou seja, esse hardware não suporta essa
constelação. Isso é comum em receptores u-blox 6/7: muitos desses só
suportam GPS+SBAS(+QZSS) de forma concorrente; suporte concorrente a
GLONASS só ficou comum a partir da geração u-blox M8. Nesse caso não
é bug do código: é limitação do chip. Rode a consulta `6` (CFG-GNSS)
pra ver quais blocos o seu receptor realmente lista.

## Descobrindo o modelo real do seu GPS

Rode `D` (diagnóstico completo) e olhe o campo "Firmware" no
relatório — ele mostra a linha PROTVER (versão do protocolo UBX)
extraída do MON-VER. É essa versão que identifica a geração do chip,
e não o nome impresso na etiqueta/anúncio do módulo (que pode estar
errado, especialmente em clones vendidos como "M8N"):

| PROTVER | Geração |
|---|---|
| 10-13 | u-blox 5 / 6 |
| 14 | u-blox 6 / 7 (o código já sinaliza esse caso) |
| 15-17 | u-blox 7 |
| 18-19 | u-blox M8 |
| 20-23 | u-blox M8 |
| 27-29 | u-blox M9 |
| 34+ | u-blox M10 |

O relatório (`printSummary`) já avisa quando o firmware contém "14" e
mostra um alerta extra se, além disso, o receptor implementar
CFG-GNSS e MON-HW2 (recursos que só apareceram em gerações mais
novas) — sinal de que pode ser um clone com firmware modificado,
misturando comportamento de gerações diferentes.

## Exportando a configuração para o firmware oficial

Como o módulo GPS não guarda a configuração de forma confiável
(bateria de backup / NVRAM limitada), a estratégia é: você usa esta
ferramenta pra **testar interativamente** (opções `B`, `F`, `M`, `X`,
`G`) até achar a configuração que quer, e então aperta `E`.

A opção `E` imprime, no Monitor Serial, um bloco de código C++ pronto
com todos os comandos UBX que foram **efetivamente aplicados com
sucesso** durante a sessão (na ordem em que você aplicou), prontos
para colar em `gpsSettings.h` do módulo oficial (veja
`modulo-firmware/README.md` para o passo a passo completo de
migração).

Pontos importantes:

- Só entram no export os comandos que o receptor de fato confirmou
  (ACK), ou, no caso do baudrate, o comando de troca (que não tem
  ACK por natureza — ver comentário em `UBXConfig::setBaudrate`).
- A ordem de aplicação é preservada, incluindo uma eventual troca de
  baudrate no meio — o módulo oficial (`gpsInit.cpp`) sabe que precisa
  reconfigurar a UART local depois de um comando desse tipo.
- Se você não aplicou nada ainda na sessão (só fez consultas com
  `1`-`9` ou `D`), a opção `E` avisa que não há nada pra exportar.
- Rodar `E` não altera nada no receptor — é uma leitura do histórico
  local da sessão, mantido em RAM pelo `UBXExporter`.
