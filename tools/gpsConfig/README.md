# U-BLOX GPS Config Tool (PlatformIO)

Ferramenta interativa para ESP32 que conversa com um receptor GPS
u-blox via protocolo binário UBX. Serve para diagnosticar o módulo
(firmware, hardware, se é original ou clone) e para testar/aplicar
configurações (baudrate, taxa de atualização, dynamic model, fix
mode, constelações GNSS) — e depois **exportar** essa configuração
como código C++ para colar no firmware oficial do drone/aeromodelo.

Para o guia completo do menu e do que cada opção faz, veja
[`COMO_USAR.md`](COMO_USAR.md). Para o passo a passo de migrar a
configuração testada aqui para o firmware principal, veja
[`modulo-firmware/README.md`](modulo-firmware/README.md).

## Estrutura da pasta

```
ferramentas/gps-config/
├── platformio.ini
├── README.md                  <- este arquivo
├── COMO_USAR.md                <- guia de uso do menu interativo
├── src/
│   └── main.cpp                <- ponto de entrada (setup/loop/menu)
├── lib/
│   └── gps-config/
│       ├── UBX.h / UBX.cpp             <- envio/recepcao de pacotes UBX crus
│       ├── UBXCommands.h               <- comandos de consulta pre-montados
│       ├── UBXConfig.h / .cpp          <- monta e envia comandos de SET
│       ├── UBXDatabase.h / .cpp        <- interpreta respostas -> GPSInfo
│       ├── UBXDecoder.h / .cpp         <- imprime pacotes decodificados
│       ├── UBXExporter.h / .cpp        <- registra e exporta config aplicada
│       └── GPSInfo.h / .cpp            <- estado agregado + relatorio resumo
└── modulo-firmware/            <- MODULO SEPARADO para o firmware oficial
    ├── README.md                <- como colar isso no seu projeto principal
    ├── gpsSettings.h            <- so dados: cole aqui a saida da opcao E
    ├── gpsInit.h
    └── gpsInit.cpp              <- envia os comandos de gpsSettings.h no boot
```

`modulo-firmware/` **não** faz parte da compilação deste projeto de
diagnóstico — é o conteúdo que você copia manualmente para dentro do
seu firmware principal. Veja o README dessa subpasta.

## Requisitos

- [PlatformIO](https://platformio.org/) (extensão do VS Code ou CLI).
- Placa ESP32 (o ambiente `platformio.ini` já vem configurado para
  `esp32dev` — ajuste o `board` se a sua for diferente, ex.
  `esp32-s3-devkitc-1`).
- Um receptor GPS u-blox conectado por UART.

## Ligação

Em `src/main.cpp`:

```cpp
constexpr int GPS_RX = 21; // RX do ESP32 <- TX do GPS
constexpr int GPS_TX = 22; // TX do ESP32 -> RX do GPS
```

Ajuste esses dois números conforme sua fiação.

## Compilar e carregar

Pela CLI do PlatformIO, dentro de `ferramentas/gps-config/`:

```bash
# Compilar
pio run

# Compilar e gravar no ESP32 (ajuste a porta se necessario)
pio run --target upload

# Abrir o monitor serial (115200 baud, terminador Newline)
pio device monitor
```

Ou, pelo VS Code com a extensão PlatformIO: abra esta pasta
(`ferramentas/gps-config/`) como projeto, use os ícones da barra
inferior (✓ Build, → Upload, 🔌 Monitor).

> **Importante:** o Monitor Serial do PlatformIO precisa estar
> configurado para enviar `\n` (Newline) como terminador de linha —
> já está definido em `platformio.ini` via `monitor_eol = LF`, então
> `pio device monitor` já funciona certo sem ajuste manual.

## Uso rápido

1. Ligue a placa com o Monitor Serial aberto. A ferramenta detecta o
   baudrate do GPS automaticamente.
2. Use as teclas `1`-`9` para consultar o receptor, ou `D` para rodar
   o diagnóstico completo com relatório resumido.
3. Use `B`, `F`, `M`, `X`, `G` para testar mudanças de configuração —
   cada uma já reconsulta e mostra o resultado.
4. Quando a configuração estiver do jeito que você quer, aperte `E`
   para imprimir o código C++ pronto para colar em
   `modulo-firmware/gpsSettings.h`.
5. Siga o passo a passo em
   [`modulo-firmware/README.md`](modulo-firmware/README.md) para
   colar esse código no firmware principal do drone/aeromodelo.

Detalhes de cada opção do menu, incluindo a diferença entre Dynamic
Model e Fix Mode e como identificar a geração real do chip (mesmo em
clones com etiqueta errada), estão em
[`COMO_USAR.md`](COMO_USAR.md).
