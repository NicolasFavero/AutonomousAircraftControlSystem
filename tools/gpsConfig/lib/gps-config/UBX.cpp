#include "UBX.h"

UBXInterface::UBXInterface(HardwareSerial& serial)
    : gps(serial)
{
}

void UBXInterface::begin(uint32_t baud)
{
    gps.begin(baud);
}

void UBXInterface::send(const uint8_t* packet, size_t length)
{
    while (gps.available())
        gps.read();

    gps.write(packet, length);
    gps.flush();
}

bool UBXInterface::request(const uint8_t *packet,
                           size_t length,
                           Packet &response,
                           uint32_t timeout)
{
    send(packet, length);

    return readPacket(response, timeout);
}

bool UBXInterface::readByte(uint8_t &b,
                            uint32_t deadline)
{
    while ((int32_t)(deadline - millis()) > 0)
    {
        if (gps.available())
        {
            b = gps.read();
            return true;
        }
    }

    return false;
}

bool UBXInterface::readPacket(Packet &packet,
                              uint32_t timeout)
{
    packet.received = false;

    // Prazo absoluto (nao "por byte"): sem isso, se a porta estiver
    // no baud errado e continuar recebendo bytes (mesmo que so
    // ruido/lixo), o laco de procura do sincronismo abaixo nunca
    // fica sem bytes pra ler e o timeout por-byte nunca dispara --
    // trava buscando 0xB5 0x62 pra sempre. Com um prazo absoluto,
    // a funcao inteira desiste no maximo 'timeout' ms depois de
    // comecar, não importa quantos bytes (validos ou nao) cheguem.
    uint32_t deadline = millis() + timeout;

    uint8_t b;

    while (true)
    {
        if (!readByte(b, deadline))
            return false;

        if (b != 0xB5)
            continue;

        if (!readByte(b, deadline))
            return false;

        if (b != 0x62)
            continue;

        break;
    }

    if (!readByte(packet.cls, deadline))
        return false;

    if (!readByte(packet.id, deadline))
        return false;

    uint8_t l1, l2;

    if (!readByte(l1, deadline))
        return false;

    if (!readByte(l2, deadline))
        return false;

    packet.length = l1 | (l2 << 8);

    if (packet.length > sizeof(packet.payload))
        return false;

    for (uint16_t i = 0; i < packet.length; i++)
    {
        if (!readByte(packet.payload[i], deadline))
            return false;
    }

    uint8_t ckA;
    uint8_t ckB;

    if (!readByte(ckA, deadline))
        return false;

    if (!readByte(ckB, deadline))
        return false;

    uint8_t calcA;
    uint8_t calcB;

    uint8_t temp[4 + packet.length];

    temp[0] = packet.cls;
    temp[1] = packet.id;
    temp[2] = l1;
    temp[3] = l2;

    memcpy(temp + 4,
           packet.payload,
           packet.length);

    ubxChecksum(temp,
                sizeof(temp),
                calcA,
                calcB);

    packet.validChecksum =
        (ckA == calcA) &&
        (ckB == calcB);

    packet.received = true;

    return true;
}

uint16_t UBXInterface::ubxChecksum(const uint8_t *data,
                                   size_t len,
                                   uint8_t &ckA,
                                   uint8_t &ckB)
{
    ckA = 0;
    ckB = 0;

    for (size_t i = 0; i < len; i++)
    {
        ckA += data[i];
        ckB += ckA;
    }

    return (ckA << 8) | ckB;
}