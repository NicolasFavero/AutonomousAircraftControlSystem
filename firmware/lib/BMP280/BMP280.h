#pragma once
#include <Adafruit_BMP280.h>

//Wire.begin(SDA, SCL)  --> main.cpp

class BMP280{
    public:
        BMP280();

        bool begin();
        bool update();

        float getRawAltitude() const ;
        float getRelativeAltitude() const ;
        float getPressure() const;
        float getTemperature() const ;

        void print();

    private:
        Adafruit_BMP280 bmp; 

        static constexpr uint8_t ADDRESS = 0x76;
        static constexpr float SEA_LEVEL_PRESSURE = 1013.25f;
        
        float altitude = 0.0f;
        float firstAltitude = 0.0f;
        float pressure = 0.0f;
        float temperature = 0.0f;
};