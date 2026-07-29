#include "Telemetry.h"
#include <Arduino.h>

void Telemetry::update(const TelemetryData& newData){
    data = newData;
}
const char* Telemetry::buildCsv(){

    snprintf(
        csv,
        sizeof(csv),

        "%lu,"
        "%s,"
        "%d,"
        "%d,"
        "%d,"
        "%02d/%02d/%02d,"
        "%02d:%02d:%02d,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"

        "%.2f,"
        "%.2f,"
        "%.2f,"

        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.2f,"
        "%.6f,"
        "%.6f,"
        "%.2f,"
        "%.2f",

        millis(),

        stateToString(data.state),

        data.gpsOk,
        data.imuOk,

        data.satellites,

        data.day,
        data.month,
        data.year % 100,

        data.hour,
        data.minute,
        data.second,

        data.pitch,
        data.roll,
        data.yaw,

        data.servoElevator,
        data.servoLeftAileron,
        data.servoRightAileron,

        data.accX,
        data.accY,
        data.accZ,

        data.gyroX,
        data.gyroY,
        data.gyroZ,

        data.magX,
        data.magY,
        data.magZ,

        data.baroAltitude,
        data.gpsAltitude,

        data.temperature,

        data.latitude,
        data.longitude,

        data.course,

        data.battery
    );

    return csv;
}
const char* Telemetry::getCsvHeader() const{

    return
        "millis,"
        "state,"
        "gpsValid,"
        "imuValid,"
        "sats,"
        "date,"
        "time,"
        "pitch,"
        "roll,"
        "yaw,"
        "servoElevator,"
        "servoLeftAileron,"
        "servoRightAileron,"
        "accX,"
        "accY,"
        "accZ,"
        "gyroX,"
        "gyroY,"
        "gyroZ,"
        "magX,"
        "magY,"
        "magZ,"
        "baroAlt,"
        "gpsAlt,"
        "temp,"
        "lat,"
        "lon,"
        "course,"
        "battery";
}
const char* Telemetry::buildLoraPacket(bool reduced){

    if(reduced)
    {
        snprintf(
            json,
            sizeof(json),

            "{\"s\":\"%s\","
            "\"g\":%d,"
            "\"alt\":%.2f,"
            "\"lat\":%.6f,"
            "\"lon\":%.6f,"
            "\"c\":%.2f,"
            "\"bat\":%.2f}",

            stateToString(data.state),

            data.gpsOk,

            data.gpsAltitude,

            data.latitude,
            data.longitude,

            data.course,

            data.battery
        );

        return json;
    }

    snprintf(
        json,
        sizeof(json),

        "{\"state\":\"%s\","
        "\"gpsOk\":%d,"
        "\"imuOk\":%d,"
        "\"pitch\":%.2f,"
        "\"roll\":%.2f,"
        "\"yaw\":%.2f,"
        "\"bmpAlt\":%.2f,"
        "\"gpsAlt\":%.2f,"
        "\"lat\":%.6f,"
        "\"lon\":%.6f,"
        "\"course\":%.2f,"
        "\"temp\":%.2f,"
        "\"date\":\"%02d/%02d/%02d\","
        "\"time\":\"%02d:%02d:%02d\","
        "\"sats\":%d,"
        "\"bat\":%.2f}",

        stateToString(data.state),

        data.gpsOk,
        data.imuOk,

        data.pitch,
        data.roll,
        data.yaw,

        data.baroAltitude,
        data.gpsAltitude,

        data.latitude,
        data.longitude,

        data.course,

        data.temperature,

        data.day,
        data.month,
        data.year % 100,

        data.hour,
        data.minute,
        data.second,

        data.satellites,

        data.battery
    );

    return json;
}
const char* Telemetry::stateToString(FlightMode state) const{
    
    switch(state)
    {
        case FlightMode::CONFIG:
            return "CONFIG";

        case FlightMode::COUNTDOWN:
            return "COUNTDOWN";

        case FlightMode::FLIGHT:
            return "FLIGHT";

        case FlightMode::LANDED:
            return "LANDED";

        default:
            return "UNKNOWN";
    }
}