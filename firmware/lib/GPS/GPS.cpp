#include "GPS.h"
#include <Arduino.h>

GPS::GPS(HardwareSerial& serial) : serialGPS(serial){}

bool GPS::update(){

    bool receivedNewData = false;

    while (serialGPS.available())
    {
        gps.encode(serialGPS.read());
        receivedNewData = true;
    }

    if (!receivedNewData)
        return false;

    valid = gps.location.isValid();

    if (gps.location.isValid())
    {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
    }

    if (gps.altitude.isValid())
    {
        altitude = gps.altitude.meters();
    }

    if (gps.course.isValid())
    {
        course = gps.course.deg();
    }

    if (gps.speed.isValid())
    {
        speed = gps.speed.mps();
    }

    if (gps.satellites.isValid())
    {
        satellites = gps.satellites.value();
    }

    if (gps.time.isValid())
    {
        hour = gps.time.hour();
        minute = gps.time.minute();
        second = gps.time.second();
    }

    if (gps.date.isValid())
    {
        day = gps.date.day();
        month = gps.date.month();
        year = gps.date.year();
    }

    return true;
}
bool GPS::isValid() const {return valid;}
bool GPS::hasCommunication() const {return gps.passedChecksum() > 0;}
double GPS::getLatitude() const {return latitude;}
double GPS::getLongitude() const {return longitude;}
double GPS::getAltitude() const {return altitude;}
double GPS::getCourse() const {return course;}
double GPS::getSpeed() const {return speed;}
uint8_t GPS::getSatellites() const {return satellites;}
uint8_t GPS::getHour() const {return hour;}
uint8_t GPS::getMinute() const {return minute;}
uint8_t GPS::getSecond() const {return second;}
uint8_t GPS::getDay() const {return day;}
uint8_t GPS::getMonth() const{return month;}
uint16_t GPS::getYear() const {return year;}
void GPS::print(){
    Serial.print("VALID=");
    Serial.print(valid);

    Serial.print(" LAT=");
    Serial.print(latitude, 6);

    Serial.print(" LON=");
    Serial.print(longitude, 6);

    Serial.print(" ALT=");
    Serial.print(altitude, 1);

    Serial.print(" COURSE=");
    Serial.print(course, 1);

    Serial.print(" SAT=");
    Serial.print(satellites);

    Serial.print(" DATE=");
    Serial.print(day);
    Serial.print('/');
    Serial.print(month);
    Serial.print('/');
    Serial.print(year);

    Serial.print(" TIME=");
    Serial.print(hour);
    Serial.print(':');
    Serial.print(minute);
    Serial.print(':');
    Serial.println(second);
}
