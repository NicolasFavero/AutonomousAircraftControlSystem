#pragma once
#include <Arduino.h>
#include <RadioLib.h>
#include "DataTypes.h" //only to use Struct LoraConfig

class RF96W {
public:

    RF96W(
    uint8_t cs,
    uint8_t dio0,
    uint8_t rst,
    const LoraConfig& config
    );

    bool begin();

    void setConfig(const LoraConfig& newConfig);

    const LoraConfig&getConfig() const;

    bool reconfigure(const LoraConfig& newConfig);

    bool send(const char* msg);

    bool available();
    bool receive();

    const char* getPacket() const;

    float getRSSI() const;
    float getSNR() const;

    void print();

private:

    LoraConfig config;

    static void setFlag();

    inline static volatile bool packetReceived = false;

    Module module;
    SX1276 radio;

    float rssi = 0.0f;
    float snr  = 0.0f;

    char packetBuffer[256];
};