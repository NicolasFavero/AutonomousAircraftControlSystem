#pragma once

#include <Arduino.h>

#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>

#include "DataTypes.h"

class CurrentConfig
{
public:

    static void begin();

    static void copy(SystemConfig& config);

    static void update(const SystemConfig& config);

    static SystemStatus& getStatus();

private:

    static SystemConfig currentConfig;

    static SystemStatus currentStatus;

    static portMUX_TYPE configMux;
};