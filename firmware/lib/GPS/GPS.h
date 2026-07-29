#pragma once

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

class GPS {
public:

    GPS(uint8_t TX_GPS = 13, int8_t RX_GPS = -1);

    bool begin();
    bool update();

    bool isValid() const;
    bool hasCommunication() const;

    double getLatitude() const;
    double getLongitude() const;
    double getAltitude() const;

    double getCourse() const;

    uint8_t getSatellites() const;

    uint8_t getHour() const;
    uint8_t getMinute() const;
    uint8_t getSecond() const;

    uint8_t getDay() const;
    uint8_t getMonth() const;
    uint16_t getYear() const;

    void print();

private:

    TinyGPSPlus gps;
    HardwareSerial serialGPS;

    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;

    double course = 0.0f;

    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;

    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;
    uint8_t satellites = 0;

    uint8_t TX_GPS;
    int8_t RX_GPS;

    bool valid = false;

    static constexpr uint32_t BAUDRATE = 9600;
};