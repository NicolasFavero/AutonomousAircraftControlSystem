#pragma once
#include <Arduino.h>

// TO USE THE COMMENTED-OUT PINS (43, 44), YOU NEED TO DEACTIVATE THE SERIAL UART.

namespace Pins{
    // I2C
    inline constexpr uint8_t SDA = 3;
    inline constexpr uint8_t SCL = 2;

    // SPI
    inline constexpr uint8_t SCK  = 5;
    inline constexpr uint8_t MOSI = 6;
    inline constexpr uint8_t MISO = 7;

    // SD
    inline constexpr uint8_t SD_CS = 1;

    // GPS
    inline constexpr uint8_t GPS_TX = 13;

    // LoRa
    inline constexpr uint8_t LORA_CS = 4;
    inline constexpr uint8_t LORA_DIO0 = 8;
    inline constexpr uint8_t LORA_RESET = 9;
    //inline constexpr uint8_t LORA_BUSY = 44;
    
    // Servos
    inline constexpr uint8_t ELEVATOR = 12;
    inline constexpr uint8_t LEFT_AILERON = 11;
    inline constexpr uint8_t RIGHT_AILERON = 10;
    //inline constexpr uint8_t RUDDER = 43;
}