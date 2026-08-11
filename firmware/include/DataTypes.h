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

    float airspeed = 0.0f;        // m/s, com sinal (Pitot -- ver lib/Pitot)
    float pitotRawVoltage = 0.0f; // tensao ADS crua no canal do pitot (SEM zero/filtro) -- so pra diagnostico/tara na pagina web
};


/*==========================================================
                    IMU Offsets
==========================================================*/

struct ImuOffsets
{
    float pitch = 0.0f;
    float roll = 0.0f;
    float yaw = 0.0f;

    // Nao e' da IMU, e' o zero do Pitot (MPXV7002DP) -- mora aqui
    // mesmo assim pra reusar o mesmo fluxo de tara/salvar/Preferences
    // que pitch/roll/yaw ja tem (mesmo padrao de SystemConfig.lora
    // ja fazer algo parecido). 1.8385V e' o zero de fabrica (sem
    // media estatica no boot -- ver comentario em Pitot::update()).
    float pitotZeroVoltage = 1.8385f;
};

/*==========================================================
                    PID Gains
==========================================================*/

struct PidGains
{
    float kp = 0.0f;
    float ki = 0.0f;
    float kd = 0.0f;

    // Teto do anti-windup (PID::PID_variables::integral fica preso em
    // [-integralLimit, +integralLimit]). Default 0.0f == integral sempre
    // travado em 0 (Ki sem efeito nenhum) ate' ser configurado pela
    // pagina PID -- mesmo comportamento "inerte" de antes desse campo
    // existir, so' que agora configuravel em vez de permanentemente
    // zerado (ver PID::updatePidVariables em lib/PID/PID.cpp).
    float integralLimit = 0.0f;
};

struct PidConfig
{
    PidGains pitch;
    PidGains roll;
    PidGains yaw; // Ainda nao usado no controle, mas ja fica pronto.

    // Trim simetrico por cima do neutro FISICO de cada servo (ver
    // ServoConfig::BaseNeutral) -- e' o que a pagina Servos/PID
    // realmente edita, nunca os graus base direto. flaperonTrim>0
    // soma no neutro do aileron esquerdo e SUBTRAI do direito (ver
    // comentario em main.cpp sobre qual direcao fisica isso vira --
    // confirme no teste com os servos de verdade, e' so trocar o sinal
    // aqui se estiver invertido).
    float flaperonTrim = 0.0f;
    float elevatorTrim = 0.0f;

    // Inverte o sinal do trim antes de aplicar (LEFT_AILERON + trim /
    // RIGHT_AILERON - trim, ver main.cpp::syncPidNeutralAngles()).
    // Existe porque o input numerico do navegador no celular nao tem
    // tecla de "-" -- sem isso, corrigir a direcao do trim exigiria
    // digitar um valor negativo, o que nao da' pra fazer no celular.
    bool invertFlaperonTrim = false;
    bool invertElevatorTrim = false;
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

// Resultado da aplicacao da config UBX no boot (ApplyGpsConfig, dentro
// de GPS::begin()) -- diferente de GpsStatus acima, que e' o fix ao
// vivo (muda o tempo todo). Isso aqui e' fixo depois do boot, so serve
// pra diagnostico: a config realmente foi aplicada no modulo, ou o GPS
// nem respondeu?
struct GpsConfigStatus
{
    bool ok = false; // true so se TODOS os comandos de gpsSettings.h foram confirmados por ACK

    uint32_t detectedBaud = 0; // baud em que o GPS respondeu ANTES de aplicar a config (0 = nenhum candidato respondeu)
    uint32_t finalBaud = 0;    // baud em que a UART fica depois de aplicar (pode ter mudado por um comando de troca de baudrate)

    uint8_t confirmedCount = 0;
    uint8_t totalCount = 0;

    // Leitura de volta (CFG-PRT/RATE/NAV5/GNSS) -- ja vem como um
    // objeto JSON valido (ex.: {"baud":115200,"rateMs":140,...}), pra
    // poder ser embutido direto (sem escapar aspas) dentro do JSON que
    // o WifiAP manda pro navegador.
    char configJson[320] = "{}";
};

enum class SystemEvent : uint8_t{
    NONE,

    OFFSET_CHANGED,
    PID_CHANGED,
    SYSTEM_CHANGED,
    LORA_CHANGED,

    CHECK_SD,
    CHECK_LORA,
    CHECK_GPS,

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

    // Decidido na hora de "Iniciar Voo" (checkbox na pagina Voo, ver
    // handleStartFlight() em WifiAP.cpp) -- default desligado (a
    // configuracao so' pode ser editada em CONFIG, como sempre foi).
    // Ligado, libera PID/trim pra edicao tambem durante o FLIGHT (ver
    // handlePid() e o bloco "newPidAvailable" em taskControl()).
    // Proposital NAO ter chave nas Preferences: cada voo exige marcar
    // de novo, nunca herda do voo anterior.
    bool liveTuningEnabled = false;

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

    // double, nao float -- float so' tem ~7 digitos decimais de
    // precisao no total, e' pouco pra um numero no formato
    // "-23.xxxxxxxxx" (2 digitos antes do ponto ja' comem quase
    // metade disso, sobrando so' uns 4-5 decimais REAIS -- os demais
    // seriam ruido, nao precisao de verdade). GPS::getLatitude()/
    // getLongitude() ja' retornam double (TinyGPSPlus), guardar aqui
    // como float jogava fora essa precisao antes mesmo do CSV/LoRa
    // formatarem o numero.
    double latitude;
    double longitude;

    float gpsAltitude;
    float baroAltitude;

    float course;
    float temperature;
    float battery;

    float airspeed;  // pitot (MPXV7002DP)
    float gpsSpeed;  // GPS (TinyGPSPlus speed.mps())

    uint8_t satellites;

    uint8_t day;
    uint8_t month;
    uint16_t year;

    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};