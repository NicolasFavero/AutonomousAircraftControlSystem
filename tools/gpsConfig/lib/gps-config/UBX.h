#pragma once

#include <Arduino.h>

class UBXInterface
{
public:

    struct Packet
    {
        uint8_t cls;
        uint8_t id;

        uint16_t length;

        uint8_t payload[512];

        bool validChecksum;

        bool received;
    };

    explicit UBXInterface(HardwareSerial& serial);

    void begin(uint32_t baud);

    // Envia um pacote e nao espera resposta. Existe separado de
    // request() para o caso de troca de baudrate: o receptor pode
    // passar a falar no baud novo antes de qualquer resposta chegar,
    // entao nao ha como ler uma resposta sincrona na porta ainda
    // configurada pro baud antigo.
    void send(const uint8_t* packet, size_t length);

    bool request(const uint8_t* packet,
                 size_t length,
                 Packet& response,
                 uint32_t timeout = 2000);

    // Publico e estatico: UBXConfig usa o mesmo calculo pra montar
    // pacotes de SET (nao ha motivo pra ter duas implementacoes do
    // checksum UBX no projeto).
    static uint16_t ubxChecksum(const uint8_t* data,
                         size_t len,
                         uint8_t& ckA,
                         uint8_t& ckB);

private:

    HardwareSerial& gps;

    bool readPacket(Packet& packet,
                    uint32_t timeout);

    bool readByte(uint8_t& b,
                  uint32_t deadline);
};