#pragma once

#include <Arduino.h>

struct GPSInfo
{
    // Identificação
    String firmware;
    String hardware;
    String model;

    // Comunicação
    uint32_t baudrate = 0;

    // Navegação
    float updateRateHz = 0;
    uint16_t measurementRate = 0;

    String dynamicModel;
    String fixMode;

    // GNSS
    bool gps = false;
    bool sbas = false;
    bool galileo = false;
    bool beidou = false;
    bool qzss = false;
    bool glonass = false;
    bool imes = false;

    // Hardware
    uint16_t agc = 0;
    uint32_t noise = 0;

    // Flags

    bool hasMONVER = false;
    bool hasMONHW = false;
    bool hasMONHW2 = false;
    bool hasCFGPRT = false;
    bool hasCFGRATE = false;
    bool hasCFGNAV5 = false;
    bool hasCFGGNSS = false;

    void clear();

    void printSummary() const;
};