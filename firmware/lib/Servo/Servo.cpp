#include "Servo.h"
//#include <esp32-hal-ledc.h> if some compilation bugs happen

Servo::Servo(
    uint8_t pin_,
    uint8_t channel_,
    uint16_t frequency_,
    uint8_t resolution_,
    uint16_t minPulseUs_,
    uint16_t maxPulseUs_,
    float minAngle_,
    float maxAngle_
)
:
pin(pin_),
channel(channel_),
frequency(frequency_),
resolution(resolution_),
minPulseUs(minPulseUs_),
maxPulseUs(maxPulseUs_),
minAngle(minAngle_),
maxAngle(maxAngle_)
{}

bool Servo::begin(){

    ledcAttachChannel(
        pin,
        frequency,
        resolution,
        channel
    );

    writeMicroseconds(
        (minPulseUs + maxPulseUs) / 2
    );

    return true;
}
void Servo::write(float angle){

    angle = constrain(
        angle,
        minAngle,
        maxAngle
    );

    currentAngle = angle;

    writeMicroseconds(
        (uint16_t)angleToPulse(angle)
    );
}
void Servo::writeMicroseconds(uint16_t pulseUs){

    pulseUs = constrain(
        pulseUs,
        minPulseUs,
        maxPulseUs
    );

    currentPulseUs = pulseUs;

    ledcWrite(
        pin,
        pulseToDuty(pulseUs)
    );
}
float Servo::getAngle() const {return currentAngle;}
uint16_t Servo::getPulseUs() const {return currentPulseUs;}
float Servo::angleToPulse(float angle) const{

    return minPulseUs +
           ((angle - minAngle) *
           (maxPulseUs - minPulseUs)) /
           (maxAngle - minAngle);
}
uint32_t Servo::pulseToDuty(uint16_t pulseUs) const{
    
    uint32_t maxDuty =
        (1UL << resolution) - 1;

    uint32_t periodUs =
        1000000UL / frequency;

    return ((uint64_t)pulseUs * maxDuty)
           / periodUs;
}