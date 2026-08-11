#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

#include "DataTypes.h"


constexpr char AP_SSID[] = "AsaAutonoma";

constexpr char AP_PASSWORD[] = "12345678";

constexpr uint8_t MAX_CLIENTS = 1;


class SdLogger;

class WifiAP
{
public:

    WifiAP();

    /*
    ==========================================================
                        AP Control
    ==========================================================
    */

    bool begin();

    void print();
    
    void stop();

    void update();

    bool isRunning() const;

    bool hasClient() const;

    uint8_t getConnectedClients() const;

    void attachSdLogger(SdLogger* logger); 
    /*
    ==========================================================
                        Incoming Data
    ==========================================================
    */

    void setAttitude(const AttitudeData& data);

    void setNavigation(const NavigationData& data);

    // Resultado da config UBX aplicada no boot (GPS::begin()) --
    // diferente de setNavigation()/setSystemStatus(), que sao
    // atualizados o tempo todo, isso e' chamado uma vez so, logo apos
    // gps.begin() (ver setupWifi() em main.cpp).
    void setGpsConfigStatus(const GpsConfigStatus& status);

    void setSystemStatus(const SystemStatus& data);

    void setTelemetryJson(const char* json);

    const char* getRequestedLog() const;
    const char* getRenameTarget() const;
    /*
    ==========================================================
                    Current Configuration
    ==========================================================
    */

    void setSystemConfig(const SystemConfig& config);

    void setOffsets(const ImuOffsets& offsets);

    void setPidConfig(const PidConfig& config);

    void setFlightMode(FlightMode mode);

    // Tempo restante (ms) da contagem regressiva atual, calculado no
    // main.cpp a partir do countdownStartMillis real -- assim o
    // front-end so exibe o numero que vem do firmware em vez de
    // rodar um timer local proprio (que desincroniza facil: ao
    // navegar pra outra pagina e voltar, reativar o voo, etc). Vale
    // 0 fora do modo COUNTDOWN.
    void setCountdownRemaining(unsigned long ms);

    FlightMode getFlightMode() const;
    /*
    ==========================================================
                        Event System
    ==========================================================
    */

    SystemEvent getPendingEvent() const;

    void clearPendingEvent();

    const SystemConfig& getSystemConfig() const;

    const ImuOffsets& getOffsets() const;

    const PidConfig& getPidConfig() const;

    private:

        SdLogger* sdLogger = nullptr;

        unsigned long countdownRemainingMs = 0;

        char requestedLog[32];
        char renameTarget[32];

        const char* telemetryJson = nullptr;
        /*
        ==========================================================
                            WiFi
        ==========================================================
        */

        WebServer server{80};

        bool running = false;

        /*
        ==========================================================
                            Cached Data
        ==========================================================
        */

        AttitudeData attitude;

        NavigationData navigation;

        GpsConfigStatus gpsConfig;

        SystemStatus status;

        SystemConfig systemConfig;

        ImuOffsets offsets;

        PidConfig pidConfig;

        /*
        ==========================================================
                            Events
        ==========================================================
        */

        volatile SystemEvent pendingEvent =
            SystemEvent::NONE;

        /*
        ==========================================================
                            Routes
        ==========================================================
        */

        void configureRoutes();

        /*
        ==========================================================
            Web estatica (tudo pre-comprimido em gzip na flash,
            nada e montado em RAM em tempo de execucao)
        ==========================================================
        */

        void sendGzip(
            const char* contentType,
            const uint8_t* data,
            size_t len,
            bool longCache
        );

        void handleConfig();

        void handleStatus();

        void handleGpsConfig();

        void handleOffsets();

        void handlePid();

        void handleSystem();

        void handleLora();

        void handleCheckSd();

        void handleCheckLora();

        void handleCheckGps();

        void handleStartFlight();

        void handleEndFlight();

        void handleDeactivateFlight();

        void handleResetFlight();

        void handleLogs();

        void handleDeleteAllLogs();

        void handleDeleteLog();

        void handleRenameLog();

        void handleDownloadLog();

        void handleTelemetry();

        void handleRestart();

        void handleNotFound();

        /*
        ==========================================================
                            Helpers
        ==========================================================
        */

        void sendJsonStatus();

        void sendJsonConfig();

        void sendJsonGpsConfig();

        void sendJsonSuccess(
            const char* message
        );

        void sendJsonError(
            const char* message
        );

        static constexpr size_t STATUS_JSON_SIZE = 1024;

        static constexpr size_t MESSAGE_JSON_SIZE = 128;

        char statusJson[STATUS_JSON_SIZE];

        char messageJson[MESSAGE_JSON_SIZE];
};
