#include <Adafruit_BMP280.h>
#include "BMP280.h"
#include <Arduino.h>
//Wire.begin(SDA, SCL)  --> main.cpp

BMP280::BMP280():bmp(){}
bool BMP280::begin(){
    if (!bmp.begin(0x76)) {
        return false;
    }

    bmp.setSampling(
        Adafruit_BMP280::MODE_NORMAL,
        Adafruit_BMP280::SAMPLING_X1,
        Adafruit_BMP280::SAMPLING_X16,
        Adafruit_BMP280::FILTER_X4,
        Adafruit_BMP280::STANDBY_MS_1
    );

    float sum = 0.0f;

    for(int i = 0; i < 10; i++){
        sum += bmp.readAltitude(SEA_LEVEL_PRESSURE);
        delay(20);
    }
    firstAltitude = sum / 10.0f;
    
    return true;
}
bool BMP280::update(){  //its bool to padronize, but BMP280 in normal Mode doesnt need verification

        temperature = bmp.readTemperature();
        pressure = bmp.readPressure();
        altitude = bmp.readAltitude(1013.25); 

        return true;


}
float BMP280::getRawAltitude() const{return altitude;}
float BMP280::getRelativeAltitude() const{return altitude - firstAltitude;}
float BMP280::getPressure() const {return pressure;}
float BMP280::getTemperature() const {return temperature;}
void BMP280::print(){  

    Serial.print(F("Temperature = "));
    Serial.print(temperature);
    Serial.println(" *C");

    Serial.print(F("Pressure = "));
    Serial.print(pressure);
    Serial.println(" Pa");

    Serial.print(F("Approx altitude = "));
    Serial.print(altitude); /* Adjusted to local forecast! */
    Serial.print(" m  Rel = ");
    Serial.print(altitude - firstAltitude);
    Serial.println(" m");

    Serial.println();
}