#pragma once
#include <Arduino.h>

struct AttitudeData{
    float pitch = 0.0f;
    float roll = 0.0f;
    float yaw = 0.0f;

    // Posicao (em graus) efetivamente mandada pro servo naquele ciclo --
    // ou seja, angle.elevator/leftAileron/rightAileron ja depois do PID +
    // slew-rate + limite de hardware (ver PID::computePID). Fica perto do
    // pitch/roll/yaw de proposito, pra facilitar comparar o angulo lido
    // vs o angulo comandado no mesmo instante.
    float servoElevator = 90.0f;
    float servoLeftAileron = 90.0f;
    float servoRightAileron = 90.0f;

    float accX = 0.0f;
    float accY = 0.0f;
    float accZ = 0.0f;

    float gyroX = 0.0f;
    float gyroY = 0.0f;
    float gyroZ = 0.0f;

    float magX = 0.0f;
    float magY = 0.0f;
    float magZ = 0.0f;

    bool isImuOk = false;
};

struct NavigationData{
    double latitude = 0.0f;
    double longitude = 0.0f;

    float gpsAltitude = 0.0f;
    float course = 0.0f;

    uint8_t satellites = 0;

    float baroAltitude = 0.0f;
    float temperature = 0.0f;
    float battery = 0.0f;
};


/*==========================================================
                    IMU Offsets
==========================================================*/

struct ImuOffsets
{
    float pitch = 0.0f;
    float roll = 0.0f;
    float yaw = 0.0f;
};

/*==========================================================
                    PID Gains
==========================================================*/

struct PidGains
{
    float kp = 0.0f;
    float ki = 0.0f;
    float kd = 0.0f;
};

struct PidConfig
{
    PidGains pitch;
    PidGains roll;
    PidGains yaw; // Ainda nao usado no controle, mas ja fica pronto.
};

struct LoraConfig
{
    float frequency = 915.0f;

    float bandwidth = 125.0f;

    uint8_t spreadingFactor = 10;

    uint8_t codingRate = 5;

    uint8_t power = 20;

    uint16_t preambleLength = 8;

    uint8_t syncWord = 0x12;
};

enum class FlightMode : uint8_t
{
    CONFIG,
    COUNTDOWN,
    FLIGHT,
    LANDED
};

/*==========================================================
                    GPS Status
==========================================================*/

enum class GpsStatus : uint8_t
{
    NO_COMMUNICATION = 0,   // Vermelho
    NO_FIX           = 1,   // Laranja
    POOR_FIX         = 2,   // Amarelo
    GOOD_FIX         = 3    // Verde
};

enum class SystemEvent : uint8_t{
    NONE,

    OFFSET_CHANGED,
    PID_CHANGED,
    SYSTEM_CHANGED,
    LORA_CHANGED,

    CHECK_SD,
    CHECK_LORA,

    START_FLIGHT,
    END_FLIGHT,
    DEACTIVATE_FLIGHT,
    RESET_FLIGHT,

    DELETE_ALL_LOGS,
    DELETE_LOG,
    RENAME_LOG,

    TELEMETRY_REQUEST,

    RESTART
};
struct SystemConfig
{
    float batteryLimit = 7.5f;

    uint16_t telemetryPeriodMs = 1000;

    uint16_t lowBatteryTelemetryPeriodMs = 10000;

    uint16_t estimatedFlightTimeMin = 30;

    bool wifiEnabled = true;

    bool preFlightTelemetryEnabled = false;

    FlightMode flightMode = FlightMode::CONFIG;

    LoraConfig lora;
};

struct SystemStatus
{
    bool imuOk = false;

    GpsStatus gpsStatus = GpsStatus::NO_COMMUNICATION;

    bool bmpOk = false;

    bool adsOk = false;

    bool loraOk = false;
    bool loraTested = false;

    bool sdOk = false;
    bool sdTested = false;

    bool wifiRunning = false;

    bool wifiClientConnected = false;

    uint8_t wifiClients = 0;
};

struct TelemetryData
{
    bool gpsOk;
    bool imuOk;

    FlightMode state;

    float pitch;
    float roll;
    float yaw;

    // Posicao (graus) mandada pro servo -- ver comentario em AttitudeData.
    float servoElevator;
    float servoLeftAileron;
    float servoRightAileron;

    float accX;
    float accY;
    float accZ;

    float gyroX;
    float gyroY;
    float gyroZ;

    float magX;
    float magY;
    float magZ;

    float latitude;
    float longitude;

    float gpsAltitude;
    float baroAltitude;

    float course;
    float temperature;
    float battery;

    uint8_t satellites;

    uint8_t day;
    uint8_t month;
    uint16_t year;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};