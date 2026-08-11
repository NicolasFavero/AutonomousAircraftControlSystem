#include "WifiAP.h"
#include "Web/Web.h"
#include "ServomotorConfig.h"

#include "SdLogger.h"
/*
Leitura direta do SD apenas para requisições HTTP síncronas
(listagem e download de logs).

Operações que modificam o cartão SD continuam utilizando
SystemEvent e são executadas pelo main.cpp.
*/

WifiAP::WifiAP(): server(80){}
bool WifiAP::begin(){
    if(running)
        return true;

    WiFi.mode(WIFI_AP);

    if(!WiFi.softAP(
        AP_SSID,
        AP_PASSWORD,
        1,
        false,
        MAX_CLIENTS))
    {
        return false;
    }

    configureRoutes();

    server.begin();

    running = true;

    return true;
}
void WifiAP::print(){
    Serial.println();
    Serial.println("========== WiFi AP ==========");

    Serial.print("SSID: ");
    Serial.println(AP_SSID);

    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);

    Serial.print("IP: ");
    Serial.println(WiFi.softAPIP());

    Serial.print("Channel: ");
    Serial.println(WiFi.channel());

    Serial.print("Max Clients: ");
    Serial.println(MAX_CLIENTS);

    Serial.print("Connected: ");
    Serial.println(WiFi.softAPgetStationNum());

    Serial.println("=============================");
}
void WifiAP::stop(){
    if(!running)
        return;

    server.stop();

    WiFi.softAPdisconnect(true);

    WiFi.mode(WIFI_OFF);

    running = false;
}
void WifiAP::update(){
    if(!running)
        return;

    server.handleClient();
}
bool WifiAP::isRunning() const{
    return running;
}
bool WifiAP::hasClient() const{
    return WiFi.softAPgetStationNum() > 0;
}
uint8_t WifiAP::getConnectedClients() const{
    return WiFi.softAPgetStationNum();
}
void WifiAP::attachSdLogger(SdLogger* logger){
    sdLogger = logger;
}
void WifiAP::setAttitude(const AttitudeData& data){
    attitude = data;
}
void WifiAP::setNavigation(const NavigationData& data){
    navigation = data;
}
void WifiAP::setGpsConfigStatus(const GpsConfigStatus& status){
    gpsConfig = status;
}
void WifiAP::setSystemStatus(const SystemStatus& data){
    status = data;
}
void WifiAP::setTelemetryJson(const char* json){
    telemetryJson = json;
}
const char* WifiAP::getRequestedLog() const{
    return requestedLog;
}
const char* WifiAP::getRenameTarget() const{
    return renameTarget;
}
void WifiAP::setSystemConfig(const SystemConfig& config){
    systemConfig = config;
}
void WifiAP::setOffsets(const ImuOffsets& newOffsets){
    offsets = newOffsets;
}
void WifiAP::setPidConfig(const PidConfig& config){
    pidConfig = config;
}
void WifiAP::setFlightMode(FlightMode mode){
    systemConfig.flightMode = mode;
}
void WifiAP::setCountdownRemaining(unsigned long ms){
    countdownRemainingMs = ms;
}
FlightMode WifiAP::getFlightMode() const{
    return systemConfig.flightMode;
}
SystemEvent WifiAP::getPendingEvent() const{
    return pendingEvent;
}
void WifiAP::clearPendingEvent(){
    pendingEvent = SystemEvent::NONE;
}
const SystemConfig&
WifiAP::getSystemConfig() const{
    return systemConfig;
}
const ImuOffsets&
WifiAP::getOffsets() const{
    return offsets;
}
const PidConfig&
WifiAP::getPidConfig() const{
    return pidConfig;
}
void WifiAP::configureRoutes(){
    /*
    ==========================================================
        SHELL / ESTATICOS
        Tudo aqui e servido direto da flash ja comprimido em
        gzip, sem nenhuma montagem em RAM (sem malloc/snprintf).
        style.css e script.js tem URL versionada (?v=hash),
        entao podem ficar em cache "para sempre" no navegador:
        se o conteudo mudar, a versao muda e o cache e ignorado.
    ==========================================================
    */

    server.on("/", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Shell::PAGE_GZ,
            Web::Shell::PAGE_GZ_LEN,
            false
        );
    });
    server.on("/style.css", HTTP_GET, [this]()
    {
        sendGzip(
            "text/css",
            Web::Style::CSS_GZ,
            Web::Style::CSS_GZ_LEN,
            true
        );
    });
    server.on("/script.js", HTTP_GET, [this]()
    {
        sendGzip(
            "application/javascript",
            Web::Script::JS_GZ,
            Web::Script::JS_GZ_LEN,
            true
        );
    });
    /*
    ==========================================================
                        HTML PAGES (fragmentos AJAX)
    ==========================================================
    */
    server.on("/page/status", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::STATUS_PAGE_GZ,
            Web::Pages::STATUS_PAGE_GZ_LEN,
            false
        );
    });
    server.on("/page/offsets", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::OFFSET_PAGE_GZ,
            Web::Pages::OFFSET_PAGE_GZ_LEN,
            false
        );
    });
    server.on("/page/pid", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::PID_PAGE_GZ,
            Web::Pages::PID_PAGE_GZ_LEN,
            false
        );
    });
    server.on("/page/servos", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::SERVOS_PAGE_GZ,
            Web::Pages::SERVOS_PAGE_GZ_LEN,
            false
        );
    });
    server.on("/page/system", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::SYSTEM_PAGE_GZ,
            Web::Pages::SYSTEM_PAGE_GZ_LEN,
            false
        );
    });
    server.on("/page/lora", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::LORA_PAGE_GZ,
            Web::Pages::LORA_PAGE_GZ_LEN,
            false
        );
    });
    server.on("/page/gps", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::GPS_PAGE_GZ,
            Web::Pages::GPS_PAGE_GZ_LEN,
            false
        );
    });
    server.on("/page/flight", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::FLIGHT_PAGE_GZ,
            Web::Pages::FLIGHT_PAGE_GZ_LEN,
            false
        );
    });
    server.on("/page/console", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::CONSOLE_PAGE_GZ,
            Web::Pages::CONSOLE_PAGE_GZ_LEN,
            false
        );
    });
    server.on("/page/logs", HTTP_GET, [this]()
    {
        sendGzip(
            "text/html",
            Web::Pages::LOG_PAGE_GZ,
            Web::Pages::LOG_PAGE_GZ_LEN,
            false
        );
    });
    /*
    ==========================================================
                        API
    ==========================================================
    */
    server.on("/api/config", HTTP_GET, [this]()
    {
        handleConfig();
    });
    server.on("/api/status", HTTP_GET, [this]()
    {
        handleStatus();
    });
    server.on("/api/gpsConfig", HTTP_GET, [this]()
    {
        handleGpsConfig();
    });
    server.on("/api/offsets", HTTP_POST, [this]()
    {
        handleOffsets();
    });
    server.on("/api/pid", HTTP_POST, [this]()
    {
        handlePid();
    });
    server.on("/api/system", HTTP_POST, [this]()
    {
        handleSystem();
    });
    server.on("/api/lora", HTTP_POST, [this]()
    {
        handleLora();
    });
    server.on("/api/checksd", HTTP_POST, [this]()
    {
        handleCheckSd();
    });
    server.on("/api/checklora", HTTP_POST, [this]()
    {
        handleCheckLora();
    });
    server.on("/api/checkgps", HTTP_POST, [this]()
    {
        handleCheckGps();
    });
    server.on("/api/start", HTTP_POST, [this]()
    {
        handleStartFlight();
    });
    server.on("/api/end", HTTP_POST, [this]()
    {
        handleEndFlight();
    });
    server.on("/api/deactivate", HTTP_POST, [this]()
    {
        handleDeactivateFlight();
    });
    server.on("/api/reset", HTTP_POST, [this]()
    {
        handleResetFlight();
    });
    server.on("/api/logs", HTTP_GET, [this]()
    {
        handleLogs();
    });
    server.on("/api/logs/delete", HTTP_POST, [this]()
    {
        handleDeleteLog();
    });
    server.on("/api/logs/rename", HTTP_POST, [this]()
    {
        handleRenameLog();
    });
    server.on("/api/logs/deleteAll", HTTP_POST, [this]()
    {
        handleDeleteAllLogs();
    });
    server.on("/api/logs/download", HTTP_GET, [this]()
    {
        handleDownloadLog();
    });
    server.on("/api/telemetry", HTTP_GET, [this]()
    {
        handleTelemetry();
    });
    server.on("/api/restart", HTTP_POST, [this]()
    {
        handleRestart();
    });
    /*
    ==========================================================
                        404
    ==========================================================
    */
    server.onNotFound([this]()
    {
        handleNotFound();
    });
}
void WifiAP::sendGzip(const char* contentType, const uint8_t* data, size_t len, bool longCache){

    server.sendHeader("Content-Encoding", "gzip");

    server.sendHeader(
        "Cache-Control",
        longCache
            ? "public, max-age=31536000, immutable"
            : "no-cache"
    );

    server.send_P(
        200,
        contentType,
        reinterpret_cast<PGM_P>(data),
        len
    );
}
void WifiAP::handleConfig(){
    sendJsonConfig();
}
void WifiAP::handleStatus(){
    sendJsonStatus();
}
void WifiAP::handleGpsConfig(){
    sendJsonGpsConfig();
}
void WifiAP::handleOffsets(){
    if(systemConfig.flightMode != FlightMode::CONFIG)
    {
        sendJsonError(
            "Configuration Locked"
        );
        return;
    }

    if(server.hasArg("pitch"))
        offsets.pitch =
            server.arg("pitch").toFloat();

    if(server.hasArg("roll"))
        offsets.roll =
            server.arg("roll").toFloat();

    if(server.hasArg("yaw"))
        offsets.yaw =
            server.arg("yaw").toFloat();

    if(server.hasArg("pitotZero"))
        offsets.pitotZeroVoltage =
            server.arg("pitotZero").toFloat();

    pendingEvent =
        SystemEvent::OFFSET_CHANGED;

    sendJsonSuccess(
        "Offsets Updated"
    );
}
void WifiAP::handlePid(){
    // Fora do CONFIG, so' libera se "Permitir alterar PID/trim durante o
    // voo" foi marcado ao iniciar o voo (liveTuningEnabled, ver
    // handleStartFlight() abaixo) E o modo for FLIGHT de verdade --
    // continua travado em COUNTDOWN/LANDED mesmo com a opcao marcada.
    bool liveTuningActive =
        systemConfig.liveTuningEnabled &&
        systemConfig.flightMode == FlightMode::FLIGHT;

    if(systemConfig.flightMode != FlightMode::CONFIG && !liveTuningActive)
    {
        sendJsonError(
            "Configuration Locked"
        );
        return;
    }

    if(server.hasArg("pitchKp"))
        pidConfig.pitch.kp =
            server.arg("pitchKp").toFloat();

    if(server.hasArg("pitchKi"))
        pidConfig.pitch.ki =
            server.arg("pitchKi").toFloat();

    if(server.hasArg("pitchKd"))
        pidConfig.pitch.kd =
            server.arg("pitchKd").toFloat();

    if(server.hasArg("pitchIntegralLimit"))
        pidConfig.pitch.integralLimit =
            server.arg("pitchIntegralLimit").toFloat();

    if(server.hasArg("rollKp"))
        pidConfig.roll.kp =
            server.arg("rollKp").toFloat();

    if(server.hasArg("rollKi"))
        pidConfig.roll.ki =
            server.arg("rollKi").toFloat();

    if(server.hasArg("rollKd"))
        pidConfig.roll.kd =
            server.arg("rollKd").toFloat();

    if(server.hasArg("rollIntegralLimit"))
        pidConfig.roll.integralLimit =
            server.arg("rollIntegralLimit").toFloat();

    if(server.hasArg("yawKp"))
        pidConfig.yaw.kp =
            server.arg("yawKp").toFloat();

    if(server.hasArg("yawKi"))
        pidConfig.yaw.ki =
            server.arg("yawKi").toFloat();

    if(server.hasArg("yawKd"))
        pidConfig.yaw.kd =
            server.arg("yawKd").toFloat();

    if(server.hasArg("yawIntegralLimit"))
        pidConfig.yaw.integralLimit =
            server.arg("yawIntegralLimit").toFloat();

    if(server.hasArg("flaperonTrim"))
        pidConfig.flaperonTrim =
            server.arg("flaperonTrim").toFloat();

    if(server.hasArg("elevatorTrim"))
        pidConfig.elevatorTrim =
            server.arg("elevatorTrim").toFloat();

    if(server.hasArg("invertFlaperonTrim"))
        pidConfig.invertFlaperonTrim =
            server.arg("invertFlaperonTrim") == "1";

    if(server.hasArg("invertElevatorTrim"))
        pidConfig.invertElevatorTrim =
            server.arg("invertElevatorTrim") == "1";

    pendingEvent =
        SystemEvent::PID_CHANGED;

    sendJsonSuccess(
        "PID Updated"
    );
}
void WifiAP::handleSystem(){
    if(systemConfig.flightMode != FlightMode::CONFIG)
    {
        sendJsonError(
            "Configuration Locked"
        );
        return;
    }

    if(server.hasArg("battery"))
        systemConfig.batteryLimit =
            server.arg("battery").toFloat();

    if(server.hasArg("telemetry"))
        systemConfig.telemetryPeriodMs =
            server.arg("telemetry").toInt();

    if(server.hasArg("flightTime"))
        systemConfig.estimatedFlightTimeMin =
            server.arg("flightTime").toInt();

    if(server.hasArg("preFlightTelemetry"))
        systemConfig.preFlightTelemetryEnabled =
            server.arg("preFlightTelemetry") == "1";

    if(server.hasArg("wifi"))
        systemConfig.wifiEnabled =
            server.arg("wifi") == "1";

    pendingEvent = SystemEvent::SYSTEM_CHANGED;

    sendJsonSuccess("System Updated");
}
void WifiAP::handleLora(){
    if(systemConfig.flightMode != FlightMode::CONFIG){
        sendJsonError(
            "Configuration Locked"
        );
        return;
    }

    if(server.hasArg("freq"))
        systemConfig.lora.frequency =
            server.arg("freq").toFloat();

    if(server.hasArg("bw"))
        systemConfig.lora.bandwidth =
            server.arg("bw").toFloat();

    if(server.hasArg("sf"))
        systemConfig.lora.spreadingFactor =
            server.arg("sf").toInt();

    if(server.hasArg("cr"))
        systemConfig.lora.codingRate =
            server.arg("cr").toInt();

    if(server.hasArg("power"))
        systemConfig.lora.power =
            server.arg("power").toInt();

    if(server.hasArg("sync"))
    {
        systemConfig.lora.syncWord =
            strtoul(
                server.arg("sync").c_str(),
                nullptr,
                16
            );
    }

    if(server.hasArg("pre"))
        systemConfig.lora.preambleLength =
            server.arg("pre").toInt();

    pendingEvent = SystemEvent::LORA_CHANGED;

    sendJsonSuccess("LoRa Updated");
}
void WifiAP::handleCheckSd(){
    if(systemConfig.flightMode != FlightMode::CONFIG)
    {
        sendJsonError(
            "Configuration Locked"
        );
        return;
    }

    pendingEvent = SystemEvent::CHECK_SD;

    sendJsonSuccess(
        "SD Test Requested"
    );
}
void WifiAP::handleCheckLora(){
    if(systemConfig.flightMode != FlightMode::CONFIG)
    {
        sendJsonError(
            "Configuration Locked"
        );
        return;
    }

    pendingEvent = SystemEvent::CHECK_LORA;

    sendJsonSuccess(
        "LoRa Test Requested"
    );
}
void WifiAP::handleCheckGps(){
    if(systemConfig.flightMode != FlightMode::CONFIG)
    {
        sendJsonError(
            "Configuration Locked"
        );
        return;
    }

    pendingEvent = SystemEvent::CHECK_GPS;

    sendJsonSuccess(
        "GPS Diagnostic Requested"
    );
}
void WifiAP::handleStartFlight(){
    if(systemConfig.flightMode != FlightMode::CONFIG)
    {
        sendJsonError(
            "Flight Already Started"
        );
        return;
    }

    // Sempre mandado pela pagina Voo (0 ou 1, nunca ausente) -- sem
    // hasArg de proposito, decide de novo a cada voo (ver comentario em
    // SystemConfig::liveTuningEnabled, DataTypes.h).
    systemConfig.liveTuningEnabled =
        server.arg("liveTuning") == "1";

    pendingEvent =
        SystemEvent::START_FLIGHT;

    sendJsonSuccess(
        "Countdown Started"
    );
}
void WifiAP::handleEndFlight(){
    if(systemConfig.flightMode != FlightMode::COUNTDOWN &&
       systemConfig.flightMode != FlightMode::FLIGHT)
    {
        sendJsonError(
            "No Active Flight"
        );
        return;
    }

    pendingEvent =
        SystemEvent::END_FLIGHT;

    sendJsonSuccess(
        "Flight Ended"
    );
}
void WifiAP::handleDeactivateFlight(){
    // Botao de emergencia: forca o modo para LANDED em qualquer
    // situacao (CONFIG, COUNTDOWN ou FLIGHT), sem as restricoes do
    // "Finalizar Voo" normal. Serve para travar o sistema em modo
    // seguro mesmo que o fluxo normal de voo nao tenha sido usado.
    if(systemConfig.flightMode == FlightMode::LANDED)
    {
        sendJsonError(
            "Already Landed"
        );
        return;
    }

    pendingEvent =
        SystemEvent::DEACTIVATE_FLIGHT;

    sendJsonSuccess(
        "Flight Deactivated"
    );
}
void WifiAP::handleResetFlight(){
    // Reativa o voo sem precisar reiniciar o ESP32: só permitido a
    // partir de LANDED (depois de um voo finalizado/cancelado).
    // Volta para CONFIG, o que automaticamente libera offsets/PID/
    // sistema/LoRa pra edicao de novo (todos os handlers ja travam
    // com base em flightMode == CONFIG).
    if(systemConfig.flightMode != FlightMode::LANDED)
    {
        sendJsonError(
            "Not Landed"
        );
        return;
    }

    pendingEvent =
        SystemEvent::RESET_FLIGHT;

    sendJsonSuccess(
        "Flight Reset"
    );
}
void WifiAP::handleLogs(){
    if(sdLogger == nullptr)
    {
        sendJsonError("SD Logger Not Attached");
        return;
    }

    String json;

    if(!sdLogger->listFiles(json))
    {
        sendJsonError("Failed To Read SD");
        return;
    }

    server.send(
        200,
        "application/json",
        json
    );
}
void WifiAP::handleDeleteAllLogs(){
    pendingEvent = SystemEvent::DELETE_ALL_LOGS;

    sendJsonSuccess("Delete Requested");
}
void WifiAP::handleDeleteLog(){
    if(!server.hasArg("file"))
    {
        sendJsonError("Missing file");

        return;
    }

    strncpy(
        requestedLog,
        server.arg("file").c_str(),
        sizeof(requestedLog) - 1
    );

    requestedLog[sizeof(requestedLog) - 1] = '\0';

    pendingEvent = SystemEvent::DELETE_LOG;

    sendJsonSuccess("Delete Requested");
}
void WifiAP::handleRenameLog(){
    if(!server.hasArg("file") || !server.hasArg("newName"))
    {
        sendJsonError("Missing file or newName");
        return;
    }

    strncpy(
        requestedLog,
        server.arg("file").c_str(),
        sizeof(requestedLog) - 1
    );
    requestedLog[sizeof(requestedLog) - 1] = '\0';

    strncpy(
        renameTarget,
        server.arg("newName").c_str(),
        sizeof(renameTarget) - 1
    );
    renameTarget[sizeof(renameTarget) - 1] = '\0';

    // Mesmas regras de seguranca do download: sem path traversal,
    // sem subdiretorio, sempre terminando em .csv.
    const char* names[2] = { requestedLog, renameTarget };

    for(const char* name : names)
    {
        if(strstr(name, "..") != nullptr ||
           strchr(name, '/')  != nullptr ||
           strchr(name, '\\') != nullptr)
        {
            sendJsonError("Invalid File");
            return;
        }

        const char* ext = strrchr(name, '.');

        if(ext == nullptr || strcmp(ext, ".csv") != 0)
        {
            sendJsonError("Invalid File");
            return;
        }
    }

    pendingEvent = SystemEvent::RENAME_LOG;

    sendJsonSuccess("Rename Requested");
}
void WifiAP::handleDownloadLog(){
    if(!server.hasArg("file"))
    {
        sendJsonError("Missing File");
        return;
    }

    if(sdLogger == nullptr)
    {
        sendJsonError("SD Logger Not Attached");
        return;
    }

    strncpy(
        requestedLog,
        server.arg("file").c_str(),
        sizeof(requestedLog) - 1
    );

    requestedLog[sizeof(requestedLog) - 1] = '\0';

    // Não permite caminhos relativos
    if(strstr(requestedLog, "..") != nullptr)
    {
        sendJsonError("Invalid File");
        return;
    }

    // Não permite subdiretórios
    if(strchr(requestedLog, '/') != nullptr ||
       strchr(requestedLog, '\\') != nullptr)
    {
        sendJsonError("Invalid File");
        return;
    }

    // Apenas arquivos .csv (a checagem abaixo ja cobre isso, mas
    // deixa explicito: nao existe mais exigencia de prefixo "voo",
    // pois /api/logs/rename permite renomear o arquivo livremente).
    // Deve terminar em ".csv"
    const char* ext = strrchr(requestedLog, '.');

    if(ext == nullptr || strcmp(ext, ".csv") != 0)
    {
        sendJsonError("Invalid File");
        return;
    }

    File32 file;

    if(!sdLogger->openRead(requestedLog, file))
    {
        sendJsonError("File Not Found");
        return;
    }

    server.sendHeader(
        "Content-Disposition",
        String("attachment; filename=\"") +
        requestedLog +
        "\""
    );

    server.setContentLength(file.size());

    server.send(
        200,
        "text/csv",
        ""
    );

    WiFiClient client = server.client();

    uint8_t buffer[512];

    while(file.available())
    {
        size_t len = file.read(
            buffer,
            sizeof(buffer)
        );

        if(len == 0)
            break;

        client.write(buffer, len);
    }

    file.close();
}
void WifiAP::handleTelemetry(){

    server.send(
        200,
        "application/json",
        telemetryJson ? telemetryJson : "{}"
    );

    // Nada disparava SystemEvent::TELEMETRY_REQUEST antes -- por isso
    // telemetryJson ficava sempre nullptr e a pagina Console so via "{}".
    // So pede um pacote novo se nao houver outro evento (offset/sistema/
    // lora/log) ja esperando para nao atropela-lo -- pendingEvent guarda
    // só 1 evento por vez.
    if(pendingEvent == SystemEvent::NONE)
    {
        pendingEvent = SystemEvent::TELEMETRY_REQUEST;
    }
}
void WifiAP::handleRestart(){
    pendingEvent =
        SystemEvent::RESTART;

    sendJsonSuccess(
        "Restart Requested"
    );
}
void WifiAP::handleNotFound(){
    server.send(
        404,
        "text/plain",
        "404"
    );
}

void WifiAP::sendJsonStatus(){
    const char* mode;

    // Nome do log que esta sendo escrito agora (sem a barra inicial,
    // para bater com os nomes que /api/logs devolve) -- usado pelo
    // front-end pra marcar esse arquivo como "Em andamento" na pagina
    // de logs. Vazio quando nao ha log aberto no momento.
    const char* currentLog =
        (sdLogger != nullptr && sdLogger->isOpen())
            ? sdLogger->getFileName()
            : "";

    if(currentLog[0] == '/')
        currentLog++;

    switch(systemConfig.flightMode)
    {
        case FlightMode::CONFIG:
            mode = "CONFIG";
            break;

        case FlightMode::COUNTDOWN:
            mode = "COUNTDOWN";
            break;

        case FlightMode::FLIGHT:
            mode = "FLIGHT";
            break;

        case FlightMode::LANDED:
            mode = "LANDED";
            break;

        default:
            mode = "UNKNOWN";
            break;
    }

    // Leitura "facil" do flaperon pra' pagina Servos -- os angulos crus
    // (servoLeftAileron/servoRightAileron) sao dificeis de ler de
    // cabeca porque os neutros sao assimetricos (85/103, ver
    // ServoConfig::BaseNeutral) e um sobe quando o outro numero desce.
    // Essa conta desfaz essa assimetria: mostra o quanto os flaperons
    // estao deslocados do neutro fisico, numa escala unica (0 =
    // neutro, mesmo sinal/direcao fisica que PidConfig::flaperonTrim
    // ja usa). Calculada dos 2 lados e tirada a media pra continuar
    // fazendo sentido mesmo durante o FLIGHT, quando o PID mexe nos
    // ailerons em direcoes opostas (banking) -- em CONFIG (so' trim,
    // sem PID rodando) os 2 lados sempre batem exatamente.
    float easyFlaperon =
        (
            (attitude.servoLeftAileron - ServoConfig::BaseNeutral::LEFT_AILERON) +
            (ServoConfig::BaseNeutral::RIGHT_AILERON - attitude.servoRightAileron)
        ) / 2.0f;

    snprintf(
        statusJson,
        sizeof(statusJson),

        "{"

        "\"flightMode\":\"%s\","

        "\"pitch\":%.2f,"
        "\"roll\":%.2f,"
        "\"yaw\":%.2f,"

        "\"servoElevator\":%.2f,"
        "\"servoLeftAileron\":%.2f,"
        "\"servoRightAileron\":%.2f,"
        "\"easyFlaperon\":%.2f,"

        "\"pitchOffset\":%.2f,"
        "\"rollOffset\":%.2f,"
        "\"yawOffset\":%.2f,"

        "\"lat\":%.9f,"
        "\"lon\":%.9f,"

        "\"gpsAlt\":%.2f,"
        "\"baroAlt\":%.2f,"

        "\"battery\":%.2f,"

        "\"airspeed\":%.2f,"
        "\"pitotRawVoltage\":%.4f,"

        "\"wifiEnabled\":%s,"

        "\"gpsSat\":%u,"
        "\"gpsStatus\":%u,"

        "\"imuOk\":%s,"
        "\"bmpOk\":%s,"
        "\"loraOk\":%s,"
        "\"loraTested\":%s,"
        "\"sdOk\":%s,"
        "\"sdTested\":%s,"

        "\"telemetryMs\":%u,"

        "\"batteryLimit\":%.2f,"

        "\"wifiClients\":%u,"

        "\"currentLog\":\"%s\","

        "\"countdownRemainingMs\":%lu"

        "}",

        mode,

        attitude.pitch,
        attitude.roll,
        attitude.yaw,

        attitude.servoElevator,
        attitude.servoLeftAileron,
        attitude.servoRightAileron,
        easyFlaperon,

        offsets.pitch,
        offsets.roll,
        offsets.yaw,

        navigation.latitude,
        navigation.longitude,

        navigation.gpsAltitude,
        navigation.baroAltitude,

        navigation.battery,

        navigation.airspeed,
        navigation.pitotRawVoltage,

        systemConfig.wifiEnabled ? "true" : "false",

        navigation.satellites,

        static_cast<uint8_t>(status.gpsStatus),

        status.imuOk ? "true" : "false",

        status.bmpOk ? "true" : "false",

        status.loraOk ? "true" : "false",

        status.loraTested ? "true" : "false",

        status.sdOk ? "true" : "false",

        status.sdTested ? "true" : "false",

        systemConfig.telemetryPeriodMs,

        systemConfig.batteryLimit,

        WiFi.softAPgetStationNum(),

        currentLog,

        countdownRemainingMs
    );

    server.send(
        200,
        "application/json",
        statusJson
    );
}
void WifiAP::sendJsonConfig(){
    snprintf(
        statusJson,
        sizeof(statusJson),

        "{"

        "\"pitchOffset\":%.2f,"
        "\"rollOffset\":%.2f,"
        "\"yawOffset\":%.2f,"
        "\"pitotOffset\":%.4f,"

        "\"pitchKp\":%.4f,"
        "\"pitchKi\":%.4f,"
        "\"pitchKd\":%.4f,"
        "\"pitchIntegralLimit\":%.2f,"

        "\"rollKp\":%.4f,"
        "\"rollKi\":%.4f,"
        "\"rollKd\":%.4f,"
        "\"rollIntegralLimit\":%.2f,"

        "\"yawKp\":%.4f,"
        "\"yawKi\":%.4f,"
        "\"yawKd\":%.4f,"
        "\"yawIntegralLimit\":%.2f,"

        "\"flaperonTrim\":%.2f,"
        "\"elevatorTrim\":%.2f,"
        "\"invertFlaperonTrim\":%s,"
        "\"invertElevatorTrim\":%s,"

        "\"batteryLimit\":%.2f,"
        "\"telemetryMs\":%u,"
        "\"lowBatteryTelemetryMs\":%u,"

        "\"preFlightTelemetryEnabled\":%s,"

        "\"frequency\":%.3f,"
        "\"bandwidth\":%.1f,"
        "\"spreadingFactor\":%u,"
        "\"codingRate\":%u,"
        "\"power\":%u,"
        "\"syncWord\":%u,"
        "\"preambleLength\":%u"

        "}",

        offsets.pitch,
        offsets.roll,
        offsets.yaw,
        offsets.pitotZeroVoltage,

        pidConfig.pitch.kp,
        pidConfig.pitch.ki,
        pidConfig.pitch.kd,
        pidConfig.pitch.integralLimit,

        pidConfig.roll.kp,
        pidConfig.roll.ki,
        pidConfig.roll.kd,
        pidConfig.roll.integralLimit,

        pidConfig.yaw.kp,
        pidConfig.yaw.ki,
        pidConfig.yaw.kd,
        pidConfig.yaw.integralLimit,

        pidConfig.flaperonTrim,
        pidConfig.elevatorTrim,
        pidConfig.invertFlaperonTrim ? "true" : "false",
        pidConfig.invertElevatorTrim ? "true" : "false",

        systemConfig.batteryLimit,
        systemConfig.telemetryPeriodMs,
        systemConfig.lowBatteryTelemetryPeriodMs,

        systemConfig.preFlightTelemetryEnabled ? "true" : "false",

        systemConfig.lora.frequency,
        systemConfig.lora.bandwidth,
        systemConfig.lora.spreadingFactor,
        systemConfig.lora.codingRate,
        systemConfig.lora.power,
        systemConfig.lora.syncWord,
        systemConfig.lora.preambleLength
    );

    server.send(
        200,
        "application/json",
        statusJson
    );
}
void WifiAP::sendJsonGpsConfig(){
    // gpsConfig.configJson ja e' um objeto JSON valido (montado pelo
    // ApplyGpsConfig dentro de GPS::begin()) -- entra direto via %s,
    // SEM aspas ao redor, virando um valor aninhado de verdade em vez
    // de uma string escapada.
    snprintf(
        statusJson,
        sizeof(statusJson),

        "{"
        "\"ok\":%s,"
        "\"detectedBaud\":%lu,"
        "\"finalBaud\":%lu,"
        "\"confirmed\":%u,"
        "\"total\":%u,"
        "\"config\":%s"
        "}",

        gpsConfig.ok ? "true" : "false",
        (unsigned long)gpsConfig.detectedBaud,
        (unsigned long)gpsConfig.finalBaud,
        gpsConfig.confirmedCount,
        gpsConfig.totalCount,
        gpsConfig.configJson
    );

    server.send(
        200,
        "application/json",
        statusJson
    );
}
void WifiAP::sendJsonSuccess(const char* message){

    snprintf(
        messageJson,
        sizeof(messageJson),

        "{\"success\":true,\"message\":\"%s\"}",

        message
    );

    server.send(
        200,
        "application/json",
        messageJson
    );
}
void WifiAP::sendJsonError(const char* message){

    snprintf(
        messageJson,
        sizeof(messageJson),

        "{\"success\":false,\"message\":\"%s\"}",

        message
    );

    server.send(
        400,
        "application/json",
        messageJson
    );
}