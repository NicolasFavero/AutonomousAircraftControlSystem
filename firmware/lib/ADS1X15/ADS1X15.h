#pragma once
#include <Adafruit_ADS1X15.h>

//Wire.begin(SDA, SCL)  --> main.cpp

class ADS{
    public:

        ADS();
    
        bool begin();
        float batteryLevel(int analogPort = 0);
        float adcVoltage(int analogPort = 3);
    private:
        static constexpr float R1 = 470000.0; //BATTERY
        static constexpr float R2 = 220000.0; //GND

        Adafruit_ADS1115 ads;
};