#pragma once

#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// So le/parseia NMEA/UBX (TinyGPSPlus) de uma UART que ja chega aberta
// -- nao possui a HardwareSerial, nao sabe nada de ApplyGpsConfig nem
// de gpsSettings.h. A aplicacao da config UBX no boot (e o re-teste
// via SystemEvent::CHECK_GPS) e' feita direto no main.cpp, ANTES de
// 'serial' ser passada aqui -- ver comentario no main.cpp sobre
// ApplyGpsConfig. Isso evita essa lib (interna, generica) depender de
// uma lib externa que so' faz sentido nessa aeronave especifica.
class GPS {
public:

    // 'serial' precisa ja estar aberta (begin() chamado, no baud
    // final -- ver ApplyGpsConfig no main.cpp) antes do primeiro
    // update().
    explicit GPS(HardwareSerial& serial);

    bool update();

    bool isValid() const;
    bool hasCommunication() const;

    double getLatitude() const;
    double getLongitude() const;
    double getAltitude() const;

    double getCourse() const;
    double getSpeed() const; // m/s

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
    HardwareSerial& serialGPS;

    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;

    double course = 0.0f;
    double speed = 0.0f;

    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;

    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;
    uint8_t satellites = 0;

    bool valid = false;
};
