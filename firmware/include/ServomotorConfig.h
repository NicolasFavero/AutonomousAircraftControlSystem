#pragma once
#include <Arduino.h>
namespace ServoConfig{

    inline constexpr uint8_t FREQUENCY = 50;

    inline constexpr uint8_t RESOLUTION = 14;

    namespace Channel{
        inline constexpr uint8_t ELEVATOR = 0;

        inline constexpr uint8_t LEFT_AILERON = 1;

        inline constexpr uint8_t RIGHT_AILERON = 2;

        //inline constexpr uint8_t RUDDER = 3;
    }
}
