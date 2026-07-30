#include "gpsInit.h"
#include "gpsSettings.h"

// Defina GPS_INIT_DEBUG (antes de incluir este arquivo, ou como
// build_flag -D GPS_INIT_DEBUG no platformio.ini do projeto
// principal) para imprimir no Serial o progresso do envio. Sem essa
// definicao, loadGpsConfig() roda em silencio.
#ifdef GPS_INIT_DEBUG
    #define GPS_INIT_LOG(x) Serial.println(x)
#else
    #define GPS_INIT_LOG(x)
#endif

namespace
{
    // Espera por algum indicio de resposta do receptor (o inicio do
    // sync UBX, 0xB5 0x62) por ate 'timeoutMs'. Nao valida classe/ID
    // nem checksum -- este modulo e deliberadamente enxuto, e o
    // objetivo aqui e so dar um tempo pro receptor processar o
    // comando antes do proximo, nao auditar a resposta (isso quem
    // faz e a ferramenta de diagnostico, na hora de testar).
    bool waitForResponse(HardwareSerial& serial, uint32_t timeoutMs)
    {
        uint32_t deadline = millis() + timeoutMs;
        bool sawSync = false;

        while ((int32_t)(deadline - millis()) > 0)
        {
            if (!serial.available())
                continue;

            uint8_t b = serial.read();

            if (!sawSync)
            {
                if (b == 0xB5)
                    sawSync = true;

                continue;
            }

            return true; // viu 0xB5 seguido de qualquer byte -- ja basta
        }

        return false;
    }
}

void loadGpsConfig(HardwareSerial& gpsSerial)
{
    using namespace GpsSettings;

    GPS_INIT_LOG("loadGpsConfig: iniciando envio da configuracao exportada...");

    for (size_t i = 0; i < COMMAND_COUNT; i++)
    {
        const Command& cmd = Commands[i];

#ifdef GPS_INIT_DEBUG
        Serial.print("  [");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.print(COMMAND_COUNT);
        Serial.print("] ");
        Serial.println(cmd.description);
#endif

        while (gpsSerial.available())
            gpsSerial.read();

        gpsSerial.write(cmd.packet, cmd.length);
        gpsSerial.flush();

        if (cmd.isBaudrateChange)
        {
            // O receptor pode passar a falar no baud novo antes de
            // qualquer resposta chegar -- nao ha resposta sincrona
            // pra esperar aqui (mesma logica de
            // UBXInterface::send()/UBXConfig::setBaudrate() na
            // ferramenta de diagnostico). So reconfigura a UART
            // local e segue.
            delay(200);

            // updateBaudRate() e especifico do core ESP32 do
            // Arduino. Se estiver usando outra placa, troque por
            // gpsSerial.end() seguido de
            // gpsSerial.begin(cmd.newBaud, ...).
            gpsSerial.updateBaudRate(cmd.newBaud);

            delay(200);
        }
        else
        {
            waitForResponse(gpsSerial, 300);
        }

        delay(50);
    }

    GPS_INIT_LOG("loadGpsConfig: concluido.");
}
