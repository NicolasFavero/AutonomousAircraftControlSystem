#pragma once

#include <Arduino.h>
#include "UBX.h"

class UBXDecoder
{
public:

    static void decode(const UBXInterface::Packet& pkt);

private:

    static void decodeMONVER(const UBXInterface::Packet& pkt);
    static void decodeMONHW(const UBXInterface::Packet& pkt);
    static void decodeMONHW2(const UBXInterface::Packet& pkt);

    static void decodeCFGPRT(const UBXInterface::Packet& pkt);
    static void decodeCFGRATE(const UBXInterface::Packet& pkt);
    static void decodeCFGNAV5(const UBXInterface::Packet& pkt);
    static void decodeCFGGNSS(const UBXInterface::Packet& pkt);
    static void decodeCFGCFG(const UBXInterface::Packet& pkt);
    static void decodeNAVPVT(const UBXInterface::Packet& pkt);

    static const char* dynamicModel(uint8_t model);

public:

    // Linha unica e compacta com o essencial do fix (usada pelo modo
    // de monitoramento continuo, pra nao poluir o serial imprimindo
    // o bloco inteiro de decode() a cada consulta).
    static void printPVTLine(const UBXInterface::Packet& pkt);
};