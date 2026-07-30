#pragma once

#include <Arduino.h>

#include "GPSInfo.h"
#include "UBX.h"

class UBXDatabase
{
public:

    explicit UBXDatabase(GPSInfo& gps);

    void update(const UBXInterface::Packet& pkt);

private:

    GPSInfo& info;

    static uint16_t U2(const uint8_t* p);
    static uint32_t U4(const uint8_t* p);

    void parseMONVER(const UBXInterface::Packet& pkt);
    void parseMONHW(const UBXInterface::Packet& pkt);
    void parseMONHW2(const UBXInterface::Packet& pkt);

    void parseCFGPRT(const UBXInterface::Packet& pkt);
    void parseCFGRATE(const UBXInterface::Packet& pkt);
    void parseCFGNAV5(const UBXInterface::Packet& pkt);
    void parseCFGGNSS(const UBXInterface::Packet& pkt);

    const char* dynamicModel(uint8_t model);
    const char* fixMode(uint8_t mode);
};