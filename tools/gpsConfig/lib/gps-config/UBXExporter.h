#pragma once

#include <Arduino.h>
#include "UBXConfig.h"

// Guarda, em RAM, os comandos UBX que foram efetivamente APLICADOS
// COM SUCESSO durante a sessao interativa (baudrate, update rate,
// dynamic model, fix mode, GNSS) e sabe imprimir esse historico como
// um bloco de codigo C++ pronto pra colar em gpsSettings.h do modulo
// oficial (ver ferramentas/gps-config/modulo-firmware/).
//
// Existe porque o receptor GPS tem limitacao de memoria persistente
// (bateria de backup / NVRAM) -- em vez de confiar que o proprio
// receptor guarda a configuracao entre boots, o firmware oficial
// reenvia esses comandos toda vez que liga. Esta classe e so o lado
// "gerar o codigo" disso; quem realmente reenvia no boot e o
// gpsInit.cpp do modulo oficial.
class UBXExporter
{
public:

    static constexpr uint8_t MAX_ENTRIES = 16;

    struct Entry
    {
        String description;

        uint8_t packet[UBX_CONFIG_MAX_PACKET];
        size_t length;

        // true so para o comando de troca de baudrate -- o unico
        // caso em que o modulo oficial precisa reconfigurar a UART
        // local (gpsSerial.begin/updateBaudRate) depois de enviar.
        bool isBaudrateChange;
        uint32_t newBaud;
    };

    void clear();

    // Registra um comando ja aplicado com sucesso. Retorna false se
    // MAX_ENTRIES ja foi atingido (nesse caso nada e registrado) ou
    // se length excede UBX_CONFIG_MAX_PACKET.
    bool record(const char* description,
                const uint8_t* packet,
                size_t length,
                bool isBaudrateChange = false,
                uint32_t newBaud = 0);

    uint8_t count() const { return entryCount; }

    // Imprime no Serial o bloco de codigo C++ pronto pra colar em
    // gpsSettings.h (arrays de bytes + tabela GpsSettings::Commands).
    // Se count()==0, imprime um aviso em vez do codigo.
    void printExport() const;

private:

    Entry entries[MAX_ENTRIES];
    uint8_t entryCount = 0;

    // Deriva um identificador C++ valido (ex.: "CFG-RATE (10 Hz)" ->
    // "CMD_00_CFG_RATE__10_HZ_") a partir da descricao e do indice,
    // usado como nome do array de bytes de cada entrada exportada.
    static String makeIdentifier(uint8_t index, const String& description);
};
