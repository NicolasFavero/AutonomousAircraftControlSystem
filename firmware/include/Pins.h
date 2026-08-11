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
    inline constexpr uint8_t GPS_TX = 13; // TX do GPS -> RX do ESP32 (leitura NMEA/UBX)
    inline constexpr uint8_t GPS_RX = 43; // RX do GPS <- TX do ESP32 (necessario pra ApplyGpsConfig mandar comandos UBX -- antes so a leitura existia). Livre pra uso: ARDUINO_USB_CDC_ON_BOOT=1 tira o Serial da UART0 fixa (pinos 43/44), sobra pra GPIO.

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