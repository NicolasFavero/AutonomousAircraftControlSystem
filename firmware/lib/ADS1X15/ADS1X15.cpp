#include <Adafruit_ADS1X15.h>
#include <Arduino.h>
#include "ADS1X15.h"
//Wire.begin(SDA, SCL)  --> main.cpp

ADS::ADS():ads() {}

bool ADS::begin(){

    if (!ads.begin()) {
        return false;
    }

    ads.setGain(GAIN_ONE); // +/-4.096V
    return true;
}
float ADS::batteryLevel(int analogPort){

    int16_t raw = ads.readADC_SingleEnded(analogPort);

    float vADS = raw * 0.125f / 1000.0f; // mV -> V

    float vBat = vADS * ((R1 + R2) / R2);

    vBat *= 1.0157; //descobri o valor através de testes/medições

    return vBat;
}
float ADS::adcVoltage(int analogPort){
    
    int16_t raw = ads.readADC_SingleEnded(analogPort);
    return raw * 0.125f / 1000.0f;
}