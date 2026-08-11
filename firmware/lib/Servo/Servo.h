#pragma once

#include <Arduino.h>

class Servo {
public:

    Servo(
        uint8_t pin,
        uint8_t channel = 0,
        uint16_t frequency = 50,
        uint8_t resolution = 16,
        uint16_t minPulseUs = 500,
        uint16_t maxPulseUs = 2500,
        float minAngle = 0.0f,
        float maxAngle = 180.0f
    );

    bool begin();

    void write(float angle);
    void writeMicroseconds(uint16_t pulseUs);

    float getAngle() const;
    uint16_t getPulseUs() const;

private:

    float angleToPulse(float angle) const;
    uint32_t pulseToDuty(uint16_t pulseUs) const;

    uint8_t pin;
    uint8_t channel;

    uint32_t frequency;
    uint8_t resolution;

    uint16_t minPulseUs;
    uint16_t maxPulseUs;

    float minAngle;
    float maxAngle;

    float currentAngle = 0.0f;
    uint16_t currentPulseUs = 1500;
};