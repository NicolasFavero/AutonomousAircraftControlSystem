#pragma once
#include <Arduino.h>

#include "DataTypes.h"

class Telemetry
{
public:

    void update(const TelemetryData&);

    const char* buildCsv();

    const char* getCsvHeader() const;

    const char* buildLoraPacket(bool reduced);

    const char* stateToString(FlightMode state) const;

private:

    TelemetryData data;

    char csv[1024];
    char json[512];
};