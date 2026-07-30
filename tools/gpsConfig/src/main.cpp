#include <HardwareSerial.h>

#include "UBX.h"
#include "UBXCommands.h"
#include "GPSInfo.h"
#include "UBXDatabase.h"
#include "UBXDecoder.h"
#include "UBXConfig.h"
#include "UBXExporter.h"

// Ajuste os pinos conforme sua ligação
constexpr int GPS_RX = 43; // TX do ESP <- RX do GPS
constexpr int GPS_TX = 13; // RX do ESP -> TX do GPS

HardwareSerial gpsSerial(1);

UBXInterface ubx(gpsSerial);
GPSInfo info;
UBXDatabase database(info);
UBXConfig config(ubx);
UBXExporter exporter;

uint32_t currentBaud = 115200;
void printMenu();
void runQuery(uint8_t index);
void runDiagnostic();

void menuChangeBaudrate();
void menuChangeUpdateRate();
void menuChangeDynamicModel();
void menuChangeFixMode();
void menuChangeGnss();
void menuSave();
void menuMonitorFix();
void menuExport();

long readNumber(const char* prompt);
void flushInput();

const char* dynamicModelName(uint8_t model);
const char* fixModeName(uint8_t mode);
const char* gnssName(uint8_t gnssId);

// Lista de baudrates mais comuns em receptores u-blox. Usada so no
// boot pra descobrir em que velocidade o receptor esta falando de
// fato, sem precisar recompilar toda vez que o baud e trocado e o
// ESP32 reinicia.
constexpr uint32_t BAUD_CANDIDATES[] = {9600, 19200, 38400, 57600, 115200, 230400};
constexpr uint8_t BAUD_CANDIDATES_COUNT = sizeof(BAUD_CANDIDATES) / sizeof(BAUD_CANDIDATES[0]);

uint32_t detectBaudrate();

void setup()
{
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("Detectando baudrate do GPS...");

    uint32_t detected = detectBaudrate();

    if (detected)
    {
        currentBaud = detected;

        Serial.print("Baudrate detectado: ");
        Serial.println(currentBaud);
    }
    else
    {
        Serial.print("Nao respondeu em nenhum baud testado -- assumindo ");
        Serial.print(currentBaud);
        Serial.println(" (confira/troque pela opcao B se estiver errado).");

        gpsSerial.begin(currentBaud, SERIAL_8N1, GPS_TX, GPS_RX);
    }

    Serial.println("Se o baudrate acima nao bater com o do receptor, use a opcao B do menu.");

    delay(200);

    printMenu();
}

// Testa, em sequencia, os baudrates mais comuns: reabre a UART em
// cada um e manda um poll de MON-VER com timeout curto. O primeiro
// que responder com checksum valido e o baud real do receptor.
// Existe porque depois de usar a opcao B (trocar baudrate), o
// receptor guarda o baud novo -- mas essa variavel local currentBaud
// volta pro valor padrao no proximo boot do ESP32, entao sem isso a
// porta local ficaria falando num baud que o receptor nao usa mais.
uint32_t detectBaudrate()
{
    for (uint8_t i = 0; i < BAUD_CANDIDATES_COUNT; i++)
    {
        uint32_t baud = BAUD_CANDIDATES[i];

        Serial.print("  Tentando ");
        Serial.print(baud);
        Serial.println("...");

        gpsSerial.begin(baud, SERIAL_8N1, GPS_TX, GPS_RX);
        delay(100);

        UBXInterface::Packet resp;

        if (ubx.request(UBX::MON_VER, sizeof(UBX::MON_VER), resp, 700) &&
            resp.validChecksum)
        {
            return baud;
        }

        gpsSerial.end();
        delay(50);
    }

    return 0;
}

void loop()
{
    if (!Serial.available())
        return;

    char c = Serial.read();

    if (c >= '1' && c <= '9')
    {
        uint8_t index = c - '1';

        if (index < UBX::COMMAND_COUNT)
            runQuery(index);
    }
    else
    {
        switch (c)
        {
            case 'b': case 'B': menuChangeBaudrate();     break;
            case 'f': case 'F': menuChangeUpdateRate();   break;
            case 'm': case 'M': menuChangeDynamicModel(); break;
            case 'x': case 'X': menuChangeFixMode();      break;
            case 'g': case 'G': menuChangeGnss();         break;
            case 's': case 'S': menuSave();               break;
            case 'e': case 'E': menuExport();             break;
            case 'd': case 'D': runDiagnostic();          break;
            case 'i': case 'I': info.printSummary();      break;
            case 'l': case 'L': menuMonitorFix();         break;

            default:
                flushInput();
                return;
        }
    }

    flushInput();
    printMenu();
}

void printMenu()
{
    Serial.println();
    Serial.println("================ U-BLOX ================");
    Serial.println(" Consultas (envia o comando e mostra o pacote)");

    for (uint8_t i = 0; i < UBX::COMMAND_COUNT; i++)
    {
        Serial.print("  ");
        Serial.print(i + 1);
        Serial.print(" - ");
        Serial.println(UBX::Commands[i].name);
    }

    Serial.println();
    Serial.println(" Configuracao");
    Serial.println("  B - Alterar baudrate");
    Serial.println("  F - Alterar frequencia de atualizacao (measurement rate)");
    Serial.println("  M - Alterar Dynamic Model");
    Serial.println("  X - Forcar Fix Mode (2D only / 3D only / Auto)");
    Serial.println("  G - Habilitar/desabilitar constelacao GNSS");
    Serial.println("  S - Salvar configuracao na memoria do receptor");
    Serial.print  ("  E - Exportar para C++ o que foi aplicado nesta sessao");
    Serial.print(" (");
    Serial.print(exporter.count());
    Serial.println(" registrado(s))");
    Serial.println();
    Serial.println(" Diagnostico");
    Serial.println("  D - Diagnostico completo / detectar GPS");
    Serial.println("  I - Reimprimir relatorio (dados ja coletados)");
    Serial.println("  L - Monitor de fix ao vivo (aperta qualquer tecla pra sair)");
    Serial.println("=========================================");
    Serial.print("Baud atual da porta local: ");
    Serial.println(currentBaud);
    Serial.println();
}

// Envia uma das consultas fixas (poll), atualiza o GPSInfo com a
// resposta e mostra a interpretacao detalhada do pacote.
void runQuery(uint8_t index)
{
    const UBX::Command& cmd = UBX::Commands[index];

    Serial.print("Consultando ");
    Serial.print(cmd.name);
    Serial.println("...");

    UBXInterface::Packet resp;

    if (!ubx.request(cmd.packet, cmd.length, resp))
    {
        Serial.println("Sem resposta (timeout).");
        return;
    }

    database.update(resp);
    UBXDecoder::decode(resp);
}

// Sequencia automatica de consultas: MON-VER, MON-HW, MON-HW2,
// CFG-PRT, CFG-RATE, CFG-NAV5 e CFG-GNSS. Cada resposta atualiza o
// GPSInfo; no final, imprime o relatorio completo (mesma logica de
// deteccao de familia do modulo que "Detectar GPS" usaria).
void runDiagnostic()
{
    static const uint8_t* const packets[] = {
        UBX::MON_VER, UBX::MON_HW, UBX::MON_HW2,
        UBX::CFG_PRT, UBX::CFG_RATE, UBX::CFG_NAV5, UBX::CFG_GNSS
    };

    static const size_t lengths[] = {
        sizeof(UBX::MON_VER), sizeof(UBX::MON_HW), sizeof(UBX::MON_HW2),
        sizeof(UBX::CFG_PRT), sizeof(UBX::CFG_RATE), sizeof(UBX::CFG_NAV5), sizeof(UBX::CFG_GNSS)
    };

    static const char* const names[] = {
        "MON-VER", "MON-HW", "MON-HW2", "CFG-PRT", "CFG-RATE", "CFG-NAV5", "CFG-GNSS"
    };

    Serial.println();
    Serial.println("Executando diagnostico completo...");

    info.clear();

    for (uint8_t i = 0; i < sizeof(packets) / sizeof(packets[0]); i++)
    {
        Serial.print("  ");
        Serial.print(names[i]);
        Serial.println("...");

        UBXInterface::Packet resp;

        if (ubx.request(packets[i], lengths[i], resp))
            database.update(resp);
        else
            Serial.println("    sem resposta.");

        delay(100);
    }

    info.printSummary();
}

void menuChangeBaudrate()
{
    long newBaud = readNumber("Digite o novo baudrate (ex: 9600, 38400, 115200):");

    if (newBaud <= 0)
    {
        Serial.println("Valor invalido.");
        return;
    }

    Serial.println("Enviando novo baudrate para o receptor...");

    uint8_t packet[UBX_CONFIG_MAX_PACKET];
    size_t len = 0;

    config.setBaudrate((uint32_t)newBaud, packet, &len);

    // O receptor pode passar a falar no baud novo a qualquer momento
    // depois do envio -- a porta local precisa acompanhar antes de
    // qualquer nova consulta.
    delay(200);

    gpsSerial.begin((uint32_t)newBaud, SERIAL_8N1, GPS_TX, GPS_RX);
    currentBaud = (uint32_t)newBaud;

    delay(200);

    Serial.println("Confirmando (CFG-PRT)...");
    runQuery(2); // indice de "CFG-PRT" em UBX::Commands

    if (len > 0)
    {
        String desc = "Baudrate -> " + String(newBaud);
        exporter.record(desc.c_str(), packet, len, true, (uint32_t)newBaud);
    }
}

void menuChangeUpdateRate()
{
    long ms = readNumber("Digite o novo measurement rate em ms (ex: 100 = 10Hz, 1000 = 1Hz):");

    if (ms <= 0 || ms > 65535)
    {
        Serial.println("Valor invalido.");
        return;
    }

    uint8_t packet[UBX_CONFIG_MAX_PACKET];
    size_t len = 0;

    if (config.setUpdateRate((uint16_t)ms, packet, &len))
    {
        Serial.println("ACK recebido.");

        String desc = "CFG-RATE measurementRate=" + String(ms) + "ms";
        exporter.record(desc.c_str(), packet, len);
    }
    else
    {
        Serial.println("Sem ACK -- confira manualmente.");
    }

    runQuery(3); // indice de "CFG-RATE"
}

void menuChangeDynamicModel()
{
    Serial.println();
    Serial.println("Modelos disponiveis:");
    Serial.println("  0 - Portable");
    Serial.println("  2 - Stationary");
    Serial.println("  3 - Pedestrian");
    Serial.println("  4 - Automotive");
    Serial.println("  5 - Sea");
    Serial.println("  6 - Airborne 1g");
    Serial.println("  7 - Airborne 2g");
    Serial.println("  8 - Airborne 4g");

    long model = readNumber("Digite o numero do modelo:");

    if (model < 0 || model > 8)
    {
        Serial.println("Valor invalido.");
        return;
    }

    uint8_t packet[UBX_CONFIG_MAX_PACKET];
    size_t len = 0;

    if (config.setDynamicModel((uint8_t)model, packet, &len))
    {
        Serial.println("ACK recebido.");

        String desc = "CFG-NAV5 dynamicModel=" + String(dynamicModelName((uint8_t)model));
        exporter.record(desc.c_str(), packet, len);
    }
    else
    {
        Serial.println("Sem ACK -- confira manualmente.");
    }

    runQuery(4); // indice de "CFG-NAV5"
}

// Fix Mode e diferente de Dynamic Model: dynamic model ajusta os
// filtros de movimento (esperado pra carro, aviao, etc); fix mode
// decide se o receptor aceita entregar uma solucao so-2D quando nao
// tem satelites suficientes pra 3D. Pra aeromodelo/drone, forcar
// "3D only" evita receber altitude calculada artificialmente (a
// tal "altitude de mentirinha") quando o fix real e so 2D.
void menuChangeFixMode()
{
    Serial.println();
    Serial.println("Fix Mode:");
    Serial.println("  1 - 2D only");
    Serial.println("  2 - 3D only");
    Serial.println("  3 - Auto 2D/3D");

    long mode = readNumber("Digite o numero do fix mode:");

    if (mode < 1 || mode > 3)
    {
        Serial.println("Valor invalido.");
        return;
    }

    uint8_t packet[UBX_CONFIG_MAX_PACKET];
    size_t len = 0;

    if (config.setFixMode((uint8_t)mode, packet, &len))
    {
        Serial.println("ACK recebido.");

        String desc = "CFG-NAV5 fixMode=" + String(fixModeName((uint8_t)mode));
        exporter.record(desc.c_str(), packet, len);
    }
    else
    {
        Serial.println("Sem ACK -- confira manualmente.");
    }

    runQuery(4); // indice de "CFG-NAV5"
}

void menuChangeGnss()
{
    Serial.println();
    Serial.println("Constelacoes:");
    Serial.println("  0 - GPS");
    Serial.println("  1 - SBAS");
    Serial.println("  2 - Galileo");
    Serial.println("  3 - BeiDou");
    Serial.println("  4 - IMES");
    Serial.println("  5 - QZSS");
    Serial.println("  6 - GLONASS");

    long gnssId = readNumber("Digite o numero da constelacao:");

    if (gnssId < 0 || gnssId > 6)
    {
        Serial.println("Valor invalido.");
        return;
    }

    long enableValue = readNumber("Habilitar (1) ou desabilitar (0)?");
    bool enable = enableValue != 0;

    uint8_t packet[UBX_CONFIG_MAX_PACKET];
    size_t len = 0;

    UBXConfig::GnssSetResult result = config.setGnss((uint8_t)gnssId, enable, packet, &len);

    switch (result)
    {
        case UBXConfig::GnssSetResult::Ack:
        {
            Serial.println("ACK recebido.");

            String desc = String("CFG-GNSS ") + gnssName((uint8_t)gnssId) + (enable ? "=ON" : "=OFF");
            exporter.record(desc.c_str(), packet, len);
            break;
        }

        case UBXConfig::GnssSetResult::Nack:
            Serial.println("Receptor recusou (NACK): essa constelacao existe nesse hardware,");
            Serial.println("mas nao pode ser ligada nessa combinacao -- comum quando duas");
            Serial.println("constelacoes disputam os mesmos canais de rastreio (ex.: GLONASS");
            Serial.println("concorrente com GPS em alguns u-blox 7).");
            break;

        case UBXConfig::GnssSetResult::NotSupported:
            Serial.println("Essa constelacao nao existe nesse receptor -- nao ha bloco");
            Serial.println("correspondente na resposta do CFG-GNSS (hardware nao implementa).");
            break;

        case UBXConfig::GnssSetResult::NoResponse:
            Serial.println("Sem resposta do receptor -- confira a conexao/baudrate.");
            break;
    }

    runQuery(5); // indice de "CFG-GNSS"
}

void menuSave()
{
    Serial.println("Salvando configuracao na memoria do receptor...");

    if (config.saveConfig())
        Serial.println("Configuracao salva (ACK recebido).");
    else
        Serial.println("Sem ACK -- confira manualmente.");
}

// Imprime, no Monitor Serial, o codigo C++ pronto pra colar em
// gpsSettings.h com tudo que foi aplicado com sucesso nesta sessao
// (ver UBXExporter e modulo-firmware/README.md).
void menuExport()
{
    exporter.printExport();
}

// Consulta NAV-PVT repetidamente (poll, sem habilitar mensagem
// automatica no receptor) e imprime uma linha compacta por fix --
// sem bloco de decode() inteiro, sem buffer acumulando historico
// (cada linha e processada e descartada na hora). Sai assim que
// qualquer tecla for digitada no monitor serial.
void menuMonitorFix()
{
    Serial.println();
    Serial.println("Monitor de fix ao vivo -- aperte qualquer tecla pra sair.");
    Serial.println();

    flushInput();

    while (!Serial.available())
    {
        UBXInterface::Packet resp;

        if (ubx.request(UBX::NAV_PVT, sizeof(UBX::NAV_PVT), resp, 1200))
            UBXDecoder::printPVTLine(resp);
        else
            Serial.println("(sem resposta)");

        delay(100); // poll simples -- nao tenta bater a taxa de atualizacao configurada
    }
}

// Bloqueia ate chegar pelo menos um caractere, le um numero (aceita
// negativo, mas os chamadores rejeitam valores negativos) e descarta
// o resto da linha digitada.
long readNumber(const char* prompt)
{
    Serial.println();
    Serial.println(prompt);

    while (!Serial.available())
        delay(10);

    long value = Serial.parseInt();

    flushInput();

    return value;
}

void flushInput()
{
    while (Serial.available())
        Serial.read();
}

const char* dynamicModelName(uint8_t model)
{
    switch (model)
    {
        case 0: return "Portable";
        case 2: return "Stationary";
        case 3: return "Pedestrian";
        case 4: return "Automotive";
        case 5: return "Sea";
        case 6: return "Airborne1g";
        case 7: return "Airborne2g";
        case 8: return "Airborne4g";
        default: return "Unknown";
    }
}

const char* fixModeName(uint8_t mode)
{
    switch (mode)
    {
        case 1: return "2Donly";
        case 2: return "3Donly";
        case 3: return "Auto";
        default: return "Unknown";
    }
}

const char* gnssName(uint8_t gnssId)
{
    switch (gnssId)
    {
        case 0: return "GPS";
        case 1: return "SBAS";
        case 2: return "Galileo";
        case 3: return "BeiDou";
        case 4: return "IMES";
        case 5: return "QZSS";
        case 6: return "GLONASS";
        default: return "Unknown";
    }
}
