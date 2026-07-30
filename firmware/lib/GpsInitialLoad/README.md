# Módulo GPS — para colar no firmware oficial

Estes 3 arquivos são o módulo enxuto que você copia para o **seu
projeto principal** (o firmware do drone/aeromodelo). Ele não
depende de nenhuma classe da ferramenta de diagnóstico
(`ferramentas/gps-config`) — só reenvia, no boot, os comandos UBX que
você já testou e validou interativamente.

| Arquivo | Papel |
|---|---|
| `gpsSettings.h` | **Só dados.** Os arrays de bytes exportados pela opção `E` da ferramenta. |
| `gpsInit.h` | Declaração de `loadGpsConfig(HardwareSerial&)`. |
| `gpsInit.cpp` | Implementação: envia os comandos de `gpsSettings.h`, na ordem, tratando o caso especial de troca de baudrate. |

## Por que isso existe

O receptor GPS tem memória persistente limitada (bateria de backup /
NVRAM) e não é confiável guardar a configuração entre boots com o
comando `S` (salvar) da ferramenta de diagnóstico. Em vez disso, a
estratégia é:

1. Testar a configuração interativamente na ferramenta de
   diagnóstico (`ferramentas/gps-config`).
2. Exportar (opção `E`) o que funcionou como código C++.
3. Colar esse código em `gpsSettings.h` aqui.
4. O firmware oficial reaplica esses comandos toda vez que liga,
   chamando `loadGpsConfig()`.

## Passo a passo de migração

### 1. Copie a pasta para o seu projeto

Copie `gpsSettings.h`, `gpsInit.h` e `gpsInit.cpp` para dentro de
`lib/gps/` (ou o nome que preferir) no seu projeto principal
PlatformIO:

```
seu-projeto-principal/
├── platformio.ini
├── src/
│   └── main.cpp
└── lib/
    └── gps/
        ├── gpsSettings.h
        ├── gpsInit.h
        └── gpsInit.cpp
```

### 2. Cole o código exportado em `gpsSettings.h`

Rode a ferramenta de diagnóstico, teste a configuração (baudrate,
measurement rate, dynamic model, fix mode, GNSS) e aperte `E`. Copie
o bloco impresso entre `---- COLE O BLOCO ABAIXO ----` e
`---- FIM DO BLOCO ----` e cole em `gpsSettings.h`, substituindo o
exemplo placeholder (o placeholder é só um poll de MON-VER — não
configura nada, é só pra o arquivo compilar antes de você colar o
conteúdo real).

### 3. Chame `loadGpsConfig()` no `main.cpp` do projeto principal

```cpp
#include <HardwareSerial.h>
#include "gpsInit.h"

constexpr int GPS_RX = 21;
constexpr int GPS_TX = 22;

// Baudrate em que o receptor está HOJE -- normalmente o de fábrica
// (9600 em muitos u-blox) ou o último que você configurou
// manualmente antes de gerar o export. Se o primeiro comando
// exportado for uma troca de baudrate, loadGpsConfig() já
// reconfigura a porta sozinha depois de enviá-lo.
constexpr uint32_t GPS_BOOT_BAUD = 9600;

HardwareSerial gpsSerial(1);

void setup()
{
    Serial.begin(115200);

    gpsSerial.begin(GPS_BOOT_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

    loadGpsConfig(gpsSerial);

    // A partir daqui, gpsSerial ja esta na configuracao testada --
    // siga com o resto do setup() do seu firmware (ex.: comecar a
    // ler NAV-PVT periodicamente).
}

void loop()
{
    // ... resto do firmware ...
}
```

### 4. (Opcional) Ativar log de depuração

Por padrão `loadGpsConfig()` roda em silêncio. Para ver no Serial
qual comando está sendo enviado a cada passo do boot, adicione ao
`platformio.ini` do seu projeto principal:

```ini
build_flags =
    -D GPS_INIT_DEBUG
```

## Pontos de atenção

- **`GPS_BOOT_BAUD` precisa bater com o baudrate real do receptor no
  momento do boot.** Se você trocou o baudrate na ferramenta de
  diagnóstico e não sabe qual é o valor atual do receptor, rode a
  ferramenta de novo (ela detecta automaticamente) antes de decidir
  esse valor.
- `gpsSerial.updateBaudRate()`, usado dentro de `gpsInit.cpp` para o
  caso de troca de baudrate, é específico do core ESP32 do Arduino.
  Se for portar para outra placa, troque por
  `gpsSerial.end()` + `gpsSerial.begin(novoBaud, ...)`.
- Este módulo **não** valida ACK/checksum das respostas do
  receptor durante o boot (é deliberadamente enxuto). Se quiser essa
  validação também no firmware oficial, você pode reaproveitar
  `UBX.h`/`UBX.cpp` da ferramenta de diagnóstico como uma lib extra —
  não é necessário para o funcionamento básico.
