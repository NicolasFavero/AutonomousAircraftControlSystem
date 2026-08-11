// Arduino
#include "Arduino.h"

// Config
#include "Pins.h"
#include "ServomotorConfig.h"
#include "DataTypes.h"
#include "CurrentConfig.h"

// Drivers
#include "IMU.h"
#include "GPS.h"
#include "ApplyGpsConfig.h"
#include "gpsSettings.h"
#include "BMP280.h"
#include "ADS1X15.h"
#include "Pitot.h"

// Communication
#include "RF96W.h"
#include "WiFiAP.h"

// Storage
#include "SdLogger.h"
#include "PreferencesManager.h"

// Control
#include "PID.h"
#include "Servo.h"

// Telemetry
#include "Telemetry.h" //ALTEREI PRA DEBUG     float pitch = imuValid ? attitude.pitch : 0.0f; LEMBRAR DE VOLTAR ATRAS DEPOIS!!!!!!!!!!!!!!!

// Debug
#include "Led.h"
//=======================================================================================================================================================
//OTHERS
unsigned long lastTelemetryMillis = 0;
unsigned long countdownStartMillis = 0;
constexpr unsigned long COUNTDOWN_DURATION_MS = 10000; // mesmo valor do COUNTDOWN_DURATION_S no script.js

// Timer PROPRIO do registro no SD (ver handleOutputs()) -- antes
// reusava a mesma lastTelemetryMillis do envio LoRa, o que fazia o
// registro no SD "resetar o relogio" que o LoRa checava logo em
// seguida, e o LoRa quase nunca dava tempo de vencer o periodo
// configurado. Agora sao independentes: SD grava a cada
// SD_LOG_INTERVAL_MS, LoRa manda a cada systemConfig.telemetryPeriodMs,
// sem um atrapalhar o outro.
unsigned long lastSdMillis = 0;
constexpr unsigned long SD_LOG_INTERVAL_MS = 10;
volatile bool newOffsetsAvailable = false;
volatile bool newPidAvailable = false;

bool wifiEnabled = false;
bool wifiTemporary = false;

unsigned long wifiStartMillis = 0;
//==================================================================STRUCTS===============================================================================
ImuOffsets currentOffsets;
PidConfig currentPidConfig;
SystemConfig systemConfig;
SystemStatus systemStatus;
GpsConfigStatus gpsConfigStatus;
//========================================================================================================================================================
//==================================================================CLASSES==============================================================================
// Drivers
IMU imu;
ADS ads;
Pitot pitot(ads);
BMP280 bmp;

// GPS: ApplyGpsConfig (aplicada uma vez em beginGps(), setup()) e GPS
// (parser continuo, TinyGPSPlus) compartilham a MESMA UART -- nenhuma
// das duas possui/cria a HardwareSerial, so' recebem por referencia.
HardwareSerial gpsSerial(1);
GPS gps(gpsSerial);

// Communication
RF96W lora(Pins::LORA_CS, Pins::LORA_DIO0, Pins::LORA_RESET, systemConfig.lora);
WifiAP wifi;

// Storage
SdLogger sd(Pins::SD_CS);
PreferencesManager nvs;

// Control
PID pid;

// Actuators
//
// minAngle/maxAngle (ultimos 2 args) NAO sao mais o default generico
// 0-180 do construtor -- sao o fim de curso mecanico real de cada servo
// (ServoConfig::SafeRange), pra' write() (lib/Servo) recusar qualquer
// angulo fora disso, venha de onde vier a chamada (PID ou o trim
// direto em main.cpp). Antes disso um trim grande demais no "Aplicar e
// Travar Servos" conseguia mandar o servo direto pro batente fisico
// sem nenhum clamp no meio do caminho.
Servo elevator    (Pins::ELEVATOR,      ServoConfig::Channel::ELEVATOR,      ServoConfig::FREQUENCY, ServoConfig::RESOLUTION, 500, 2500, ServoConfig::SafeRange::ELEVATOR_MIN,      ServoConfig::SafeRange::ELEVATOR_MAX);
Servo leftAlieron (Pins::LEFT_AILERON,  ServoConfig::Channel::LEFT_AILERON,  ServoConfig::FREQUENCY, ServoConfig::RESOLUTION, 500, 2500, ServoConfig::SafeRange::LEFT_AILERON_MIN,  ServoConfig::SafeRange::LEFT_AILERON_MAX);
Servo rightAlieron(Pins::RIGHT_AILERON, ServoConfig::Channel::RIGHT_AILERON, ServoConfig::FREQUENCY, ServoConfig::RESOLUTION, 500, 2500, ServoConfig::SafeRange::RIGHT_AILERON_MIN, ServoConfig::SafeRange::RIGHT_AILERON_MAX);
//Servo rudder    (Pins::RUDDER,        ServoConfig::Channel::RUDDER,        ServoConfig::FREQUENCY, ServoConfig::RESOLUTION); 

// Telemetry
Telemetry telemetry;

//Visual Debug
Led led;

//========================================================================================================================================================
//==============================================================FREEERTOS QUEUES===============================================================================
QueueHandle_t attitudeQueue;
QueueHandle_t navigationQueue;
portMUX_TYPE offsetMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE pidMux = portMUX_INITIALIZER_UNLOCKED;
//========================================================================================================================================================
//=================================================================PROTOTYPES=============================================================================
// TASKS
void taskControl(void *pv);
void taskData(void *pv);

// TASK CONTROL HELPERS
bool readAttitudeData(AttitudeData& attitude);
void syncPidNeutralAngles();

// TASK DATA HELPERS
void sendNavigationData(NavigationData& navigationData);
void handleWifiTimeout();
void handleOutputs(FlightMode mode);
void handleWifi(const AttitudeData& attitude, const NavigationData& navigationData);
void updateGpsStatus();
void setupWifi();
bool beginGps();
TelemetryData buildTelemetryData(AttitudeData& attitude);
// GENERAL HELPERS
bool isLowPowerModeEnabled();

// Recalcula o neutro (BaseNeutral + trim) dos 3 servos a partir de
// currentPidConfig -- inclui o sinal do trim invertido quando
// invertFlaperonTrim/invertElevatorTrim esta' ligado (ver comentario
// em PidConfig, DataTypes.h: existe pq o teclado numerico do celular
// nao tem "-"). Chamada tanto no boot quanto sempre que a config muda
// (bloco "newPidAvailable" em taskControl()), pra nao duplicar essa
// conta nos dois lugares.
void syncPidNeutralAngles(){
  float flapTrim = currentPidConfig.invertFlaperonTrim
      ? -currentPidConfig.flaperonTrim
      : currentPidConfig.flaperonTrim;

  float elevTrim = currentPidConfig.invertElevatorTrim
      ? -currentPidConfig.elevatorTrim
      : currentPidConfig.elevatorTrim;

  // Clampado no fim de curso real (ServoConfig::SafeRange) -- sem isso
  // um trim grande demais produzia um neutralAngle fora do range
  // seguro, e o "Aplicar e Travar Servos" (setAngle = neutralAngle,
  // logo abaixo) escrevia esse valor DIRETO no servo sem passar por
  // nenhum constrain (diferente do caminho do PID, que sempre clampou
  // via PID::applyServoOutput). O objeto Servo (lib/Servo) tambem
  // clampa no mesmo range como ultima linha de defesa, mas isso aqui
  // evita que a UI mostre um "neutro" mentiroso, fora do que o servo
  // fisicamente consegue atingir.
  pid.elevator.neutralAngle = constrain(
      ServoConfig::BaseNeutral::ELEVATOR + elevTrim,
      ServoConfig::SafeRange::ELEVATOR_MIN,
      ServoConfig::SafeRange::ELEVATOR_MAX
  );

  pid.leftAileron.neutralAngle = constrain(
      ServoConfig::BaseNeutral::LEFT_AILERON + flapTrim,
      ServoConfig::SafeRange::LEFT_AILERON_MIN,
      ServoConfig::SafeRange::LEFT_AILERON_MAX
  );

  pid.rightAileron.neutralAngle = constrain(
      ServoConfig::BaseNeutral::RIGHT_AILERON - flapTrim,
      ServoConfig::SafeRange::RIGHT_AILERON_MIN,
      ServoConfig::SafeRange::RIGHT_AILERON_MAX
  );
}

//=========================================================================================================================================================
//===================================================================SETUP=================================================================================
void setup(){
  Serial.begin(115200);
  Serial.println("Beginning...");
  Wire.begin(Pins::SDA, Pins::SCL);
  Wire.setClock(400000); 

  SPI.begin(Pins::SCK, Pins::MISO, Pins::MOSI);

  led.white();
  //led.white();

  nvs.loadSystem(systemConfig);
  nvs.loadOffsets(currentOffsets);
  nvs.loadPid(currentPidConfig);

  pitot.setZeroVoltage(currentOffsets.pitotZeroVoltage);

  // Sem isso, o offset salvo nas Preferences so era realmente
  // aplicado no calculo do angulo (localOffsets, dentro da task de
  // controle) depois que o usuario salvasse um offset pela web --
  // ate la, localOffsets ficava zerado mesmo com currentOffsets/
  // /api/status ja reportando o valor salvo do boot anterior. Isso
  // fazia a primeira tara depois de ligar somar o offset antigo (que
  // NAO estava de fato sendo subtraido da leitura) com o novo valor
  // calculado, dando um offset errado logo na primeira vez -- so
  // acertava no segundo, quando o evento OFFSET_CHANGED sincronizava
  // localOffsets pela primeira vez. Marcando newOffsetsAvailable aqui,
  // a task de controle sincroniza localOffsets = currentOffsets logo
  // na primeira iteracao do loop, antes de qualquer leitura ser usada.
  newOffsetsAvailable = true;

  pid.pitch.kp = currentPidConfig.pitch.kp;
  pid.pitch.ki = currentPidConfig.pitch.ki;
  pid.pitch.kd = currentPidConfig.pitch.kd;
  pid.pitch.integralLimit = currentPidConfig.pitch.integralLimit;

  pid.roll.kp = currentPidConfig.roll.kp;
  pid.roll.ki = currentPidConfig.roll.ki;
  pid.roll.kd = currentPidConfig.roll.kd;
  pid.roll.integralLimit = currentPidConfig.roll.integralLimit;

  pid.yaw.kp = currentPidConfig.yaw.kp;
  pid.yaw.ki = currentPidConfig.yaw.ki;
  pid.yaw.kd = currentPidConfig.yaw.kd;
  pid.yaw.integralLimit = currentPidConfig.yaw.integralLimit;

  // Neutro fisico (ServoConfig::BaseNeutral) + trim configuravel
  // (currentPidConfig, carregado das Preferences acima) -- sem isso,
  // leftAileron/rightAileron ficariam nos 90.0f default de
  // PID::ServoParamters (errado: a base fisica real e' 85/103) ate a
  // primeira mudanca de PID pela pagina web. setAngle tambem e' fixado
  // aqui pro servo ja nascer na posicao certa (fora do modo FLIGHT,
  // computePID() nao roda pra atualizar isso sozinho).
  syncPidNeutralAngles();

  pid.elevator.setAngle    = pid.elevator.neutralAngle;
  pid.leftAileron.setAngle = pid.leftAileron.neutralAngle;
  pid.rightAileron.setAngle = pid.rightAileron.neutralAngle;

  while(!imu.begin()){led.red();}
  Serial.println("[BOOT] IMU ok");

  while(!bmp.begin()){led.red();}
  Serial.println("[BOOT] BMP280 ok");

  Serial.println("[BOOT] Iniciando GPS (beginGps())...");
  gpsSerial.begin(9600, SERIAL_8N1, Pins::GPS_TX, Pins::GPS_RX);
  delay(10);
  while(!beginGps()){led.red();}
  Serial.println("[BOOT] GPS ok");

  // TESTE SEM ADS1115: pulando ads.begin() (sem isso, o while
  // travaria pra sempre esperando um chip que nao esta na placa).
  // while(!ads.begin()){led.red();}
  // Serial.println("[BOOT] ADS1115 ok");

  while(!lora.begin()){led.red();}
  Serial.println("[BOOT] LoRa ok");

  while(!elevator.begin()){led.red();}
  Serial.println("[BOOT] Servo elevator ok");

  while(!leftAlieron.begin()){led.red();}
  Serial.println("[BOOT] Servo aileron esquerdo ok");

  while(!rightAlieron.begin()){led.red();}
  Serial.println("[BOOT] Servo aileron direito ok");

  while(!sd.begin()) {Serial.println("SD FAIL"); led.red();}
  Serial.println("[BOOT] SD ok");

  // Buffer em RAM/PSRAM pros logs de voo -- ver comentarios em
  // SdLogger::beginBuffer()/bufferLine(). Se falhar (placa sem PSRAM
  // ou sem RAM livre suficiente), o firmware continua funcionando,
  // so volta a escrever direto no SD a cada leitura como antes.
  //
  // 512KB (era o default de 64KB) -- o log no SD agora grava a cada
  // SD_LOG_INTERVAL_MS (10ms, 10x mais rapido que antes), entao o
  // buffer tambem precisa ser maior pra continuar enchendo (e forcando
  // o flush real, bloqueante, no cartao) com a mesma frequencia de
  // antes em vez de 10x mais frequente. Cabe tranquilo na PSRAM do
  // ESP32-S3.
  sd.beginBuffer(512 * 1024);

  attitudeQueue = xQueueCreate(1, sizeof(AttitudeData));
  navigationQueue = xQueueCreate(1, sizeof(NavigationData));

  if(attitudeQueue == NULL || navigationQueue == NULL){
    while(true){
      led.red();
      delay(500);
    }
  }
  setupWifi();

  lastTelemetryMillis = millis();

  led.green();
  
  xTaskCreatePinnedToCore(
    taskControl,
    "Control",
    10000,
    NULL,
    2,
    NULL,
    0
  );
  xTaskCreatePinnedToCore(
    taskData,
    "Data",
    10000,
    NULL,
    1,
    NULL,
    1
  );
}
//==========================================================================================================================================================
//================================================================KILLING TASK==============================================================================
void loop(){vTaskDelete(NULL);}
//==========================================================================================================================================================
//=================================================================TASK AT CORE 0===========================================================================
void taskControl(void *pv){

  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(5); //200HZ

  while(true){
    bool useNewValues = true;

    static AttitudeData attitudeData;
    static NavigationData nav; 
    static ImuOffsets localOffsets; 
  //===================TO  ATTITUDE==============
    if(systemConfig.flightMode == FlightMode::CONFIG && newOffsetsAvailable){
      portENTER_CRITICAL(&offsetMux);

      localOffsets = currentOffsets;

      newOffsetsAvailable = false;

      portEXIT_CRITICAL(&offsetMux);
    }

    // Fora do CONFIG, so' processa mudanca de PID/trim se liveTuningEnabled
    // estiver ligado (marcado na hora de "Iniciar Voo", ver
    // handleStartFlight() em WifiAP.cpp) E o modo for FLIGHT de verdade
    // -- nunca em COUNTDOWN/LANDED, que continuam travados como sempre
    // (handlePid() ja rejeita o POST nesses casos antes de chegar aqui).
    bool pidLiveTuningNow =
        systemConfig.liveTuningEnabled &&
        systemConfig.flightMode == FlightMode::FLIGHT;

    if(newPidAvailable && (systemConfig.flightMode == FlightMode::CONFIG || pidLiveTuningNow)){
      portENTER_CRITICAL(&pidMux);

      pid.pitch.kp = currentPidConfig.pitch.kp;
      pid.pitch.ki = currentPidConfig.pitch.ki;
      pid.pitch.kd = currentPidConfig.pitch.kd;
      pid.pitch.integralLimit = currentPidConfig.pitch.integralLimit;

      pid.roll.kp = currentPidConfig.roll.kp;
      pid.roll.ki = currentPidConfig.roll.ki;
      pid.roll.kd = currentPidConfig.roll.kd;
      pid.roll.integralLimit = currentPidConfig.roll.integralLimit;

      pid.yaw.kp = currentPidConfig.yaw.kp;
      pid.yaw.ki = currentPidConfig.yaw.ki;
      pid.yaw.kd = currentPidConfig.yaw.kd;
      pid.yaw.integralLimit = currentPidConfig.yaw.integralLimit;

      syncPidNeutralAngles();

      // So' forca a posicao final (setAngle = neutralAngle) em CONFIG --
      // e' o que faz o servo se mover na hora que o trim muda, "forcar e
      // travar" na pagina Servos/PID (fora do FLIGHT, computePID() nao
      // roda pra atualizar isso sozinho). Durante o FLIGHT (so' chega
      // aqui com pidLiveTuningNow), computePID() ja' roda todo ciclo e
      // aplica o neutro/ganho novo sozinho -- forcar setAngle aqui
      // tambem brigaria com a correcao em tempo real dele.
      if(systemConfig.flightMode == FlightMode::CONFIG){
        pid.elevator.setAngle     = pid.elevator.neutralAngle;
        pid.leftAileron.setAngle  = pid.leftAileron.neutralAngle;
        pid.rightAileron.setAngle = pid.rightAileron.neutralAngle;
      }

      newPidAvailable = false;

      portEXIT_CRITICAL(&pidMux);
    }

    imu.update();

    if(systemConfig.flightMode == FlightMode::FLIGHT){
      pid.setAngles(attitudeData, nav, useNewValues);
    }


    elevator.write(pid.elevator.setAngle);
    leftAlieron.write(pid.leftAileron.setAngle);
    rightAlieron.write(pid.rightAileron.setAngle);

  //===================TO  ATTITUDE==============
  //=====================FOR SEND================
    attitudeData.pitch = imu.getPitch() - localOffsets.pitch;
    attitudeData.roll = imu.getRoll() - localOffsets.roll;
    attitudeData.yaw = imu.getYaw() - localOffsets.yaw;

    // Mesmo valor que acabou de ir pro elevator.write()/leftAlieron.write()/
    // rightAlieron.write() acima -- registrado aqui, e nao lido de volta do
    // servo, porque servo (PWM) nao tem telemetria de posicao real.
    attitudeData.servoElevator = pid.elevator.setAngle;
    attitudeData.servoLeftAileron = pid.leftAileron.setAngle;
    attitudeData.servoRightAileron = pid.rightAileron.setAngle;

    attitudeData.accX = imu.getAccX();
    attitudeData.accY = imu.getAccY();
    attitudeData.accZ = imu.getAccZ();

    attitudeData.gyroX = imu.getGyroX();
    attitudeData.gyroY = imu.getGyroY();
    attitudeData.gyroZ = imu.getGyroZ();

    attitudeData.magX= imu.getMagX();
    attitudeData.magY = imu.getMagY();
    attitudeData.magZ = imu.getMagZ();

    attitudeData.isImuOk = imu.isHealthy();

    xQueueOverwrite(attitudeQueue, &attitudeData);
  //=====================FOR SEND================
  //====================TO  RECEIVE==============
    if(xQueueReceive(navigationQueue,&nav,0)){useNewValues = true;}
  //====================TO  RECEIVE==============
  
    vTaskDelayUntil(&lastWake,period);
  }
}
//==========================================================================================================================================================
//=================================================================TASK AT CORE 1===========================================================================
void taskData(void *pv){

  TickType_t lastWake = xTaskGetTickCount();
  // Era 100ms/10Hz -- baixado pra 10ms/100Hz pra' o log no SD
  // (SD_LOG_INTERVAL_MS, dentro de handleOutputs()) conseguir gravar de
  // verdade a cada 10ms, ja' que ele so' roda quando esse loop roda.
  // O resto do trabalho aqui dentro (GPS/BMP/WiFi) ou se beneficia de
  // rodar mais vezes (GPS.update() drena a UART com mais frequencia,
  // handleWifi() atende requisicoes HTTP mais rapido) ou e' barato o
  // suficiente pra nao importar (leitura do BMP280, montagem de JSON) --
  // exceto o envio LoRa, que continua no proprio periodo configurado
  // (systemConfig.telemetryPeriodMs), sem depender do periodo do loop.
  const TickType_t period = pdMS_TO_TICKS(10); //100HZ
  static bool isImuValid = false;

  while(true){

    static NavigationData navigationData;
    static AttitudeData attitude;

    readAttitudeData(attitude);

    systemStatus.bmpOk = bmp.update();
    systemStatus.imuOk = attitude.isImuOk;

    // pitot.update(); // TESTE SEM ADS1115

    if(gps.update()){led.green(); /*Serial.println("GPS Atualizado");*/}
    else{led.blue();}

    if(systemConfig.flightMode == FlightMode::COUNTDOWN){

        unsigned long elapsed = millis() - countdownStartMillis;

        if(elapsed >= COUNTDOWN_DURATION_MS){
            systemConfig.flightMode = FlightMode::FLIGHT;
            wifi.setFlightMode(systemConfig.flightMode);
            wifi.setCountdownRemaining(0);
        }
        else{
            wifi.setCountdownRemaining(COUNTDOWN_DURATION_MS - elapsed);
        }
    }
    else{
        wifi.setCountdownRemaining(0);
    }

    telemetry.update(buildTelemetryData(attitude));

    sendNavigationData(navigationData);

    handleOutputs(systemConfig.flightMode);//Sends LoRa Packets and registers logs in SD_Card, it update systemStatus.loraOk

    // Modo de voo "WiFi": o unico modo por enquanto mantem o WiFi
    // ativo mesmo durante o FLIGHT, entao o AP continua respondendo
    // (status, Finalizar Voo, telemetria) em vez de ficar mudo ate
    // o pouso. Logs no SD e envio LoRa continuam de qualquer forma,
    // isso e controlado em handleOutputs().
    if(wifiEnabled){
      handleWifi(attitude, navigationData);
      updateGpsStatus();
    }
    else{
      handleWifiTimeout();
    }

    vTaskDelayUntil(&lastWake, period);
  }
}
//==========================================================================================================================================================
//================================================================OTHERS FUNCTIONS==========================================================================
void handleWifi(const AttitudeData& attitude, const NavigationData& navigationData){
    if(!systemConfig.wifiEnabled)
        return;

    wifi.update();

    wifi.setAttitude(attitude);

    wifi.setNavigation(navigationData);

    wifi.setSystemStatus(systemStatus);

    switch(wifi.getPendingEvent())
    {
        case SystemEvent::NONE:
            return;

        case SystemEvent::OFFSET_CHANGED:

            portENTER_CRITICAL(&offsetMux);

            currentOffsets = wifi.getOffsets();

            newOffsetsAvailable = true;

            portEXIT_CRITICAL(&offsetMux);

            nvs.saveOffsets(currentOffsets);

            wifi.setOffsets(currentOffsets);

            // pitot.setZeroVoltage(currentOffsets.pitotZeroVoltage); // TESTE SEM ADS1115

            break;

        case SystemEvent::PID_CHANGED:

            portENTER_CRITICAL(&pidMux);

            currentPidConfig = wifi.getPidConfig();

            newPidAvailable = true;

            portEXIT_CRITICAL(&pidMux);

            nvs.savePid(currentPidConfig);

            wifi.setPidConfig(currentPidConfig);

            break;

        case SystemEvent::SYSTEM_CHANGED:

            systemConfig = wifi.getSystemConfig();

            nvs.saveSystem(systemConfig);

            wifi.setSystemConfig(systemConfig);

            break;

        case SystemEvent::LORA_CHANGED:

            systemConfig = wifi.getSystemConfig();

            nvs.saveLora(systemConfig.lora);

            lora.reconfigure(systemConfig.lora);

            wifi.setSystemConfig(systemConfig);

            break;

        case SystemEvent::START_FLIGHT:

            // handleStartFlight() (WifiAP.cpp) ja' guardou a escolha do
            // checkbox "Permitir alterar PID/trim durante o voo" na copia
            // do systemConfig do WifiAP -- puxa pra' copia autoritativa
            // (a mesma que taskControl() consulta) antes de trocar de
            // modo. setFlightMode() so' mexe no campo flightMode, entao
            // sem isso essa escolha nunca chegaria aqui.
            systemConfig.liveTuningEnabled =
                wifi.getSystemConfig().liveTuningEnabled;

            systemConfig.flightMode =
                FlightMode::COUNTDOWN;

            wifi.setFlightMode(
                systemConfig.flightMode
            );

            sd.createFile();

            {
                // Grava os ganhos de PID que estao REALMENTE em uso
                // (pid.pitch/roll/yaw, o mesmo struct usado dentro de
                // computePID) no inicio do voo, antes do header do
                // CSV -- assim, se um voo se comportar estranho, da
                // pra conferir depois exatamente qual Kp/Ki/Kd estava
                // valendo naquele log, sem depender de lembrar (ou de
                // confiar) qual config estava setada na hora.
                char pidInfoLine[192];

                snprintf(
                    pidInfoLine,
                    sizeof(pidInfoLine),
                    "# PID pitch Kp=%.4f Ki=%.4f Kd=%.4f | roll Kp=%.4f Ki=%.4f Kd=%.4f | yaw Kp=%.4f Ki=%.4f Kd=%.4f",
                    pid.pitch.kp, pid.pitch.ki, pid.pitch.kd,
                    pid.roll.kp,  pid.roll.ki,  pid.roll.kd,
                    pid.yaw.kp,   pid.yaw.ki,   pid.yaw.kd
                );

                sd.bufferLine(pidInfoLine);
            }

            sd.bufferLine(telemetry.getCsvHeader());

            countdownStartMillis = millis();

            break;

        case SystemEvent::END_FLIGHT:

            systemConfig.flightMode =
                FlightMode::LANDED;

            wifi.setFlightMode(
                systemConfig.flightMode
            );

            sd.flushBuffer();
            sd.closeFile();

            break;

        case SystemEvent::DEACTIVATE_FLIGHT:

            systemConfig.flightMode =
                FlightMode::LANDED;

            wifi.setFlightMode(
                systemConfig.flightMode
            );
            sd.flushBuffer();
            sd.closeFile();

            break;

        case SystemEvent::RESET_FLIGHT:

            // Reativa o voo sem precisar reiniciar o ESP32: só
            // permitido a partir de LANDED (checado no
            // handleResetFlight() do WifiAP), volta pra CONFIG e
            // libera offsets/PID/sistema/LoRa pra edicao de novo.
            systemConfig.flightMode =
                FlightMode::CONFIG;

            wifi.setFlightMode(
                systemConfig.flightMode
            );

            break;

        case SystemEvent::CHECK_SD:

            systemStatus.sdOk = sd.selfTest();

            systemStatus.sdTested = true;

            break;

        case SystemEvent::CHECK_LORA:

            systemStatus.loraOk = lora.send(
                "TEST;PING"
            );

            systemStatus.loraTested = true;

            break;

        case SystemEvent::CHECK_GPS:

            // Roda de novo a deteccao de baud + aplicacao dos comandos
            // de gpsSettings.h + leitura de volta, na mesma gpsSerial
            // ja aberta -- ver beginGps() e o botao "Forcar Diagnostico"
            // na pagina web (GPS).
            beginGps();

            wifi.setGpsConfigStatus(gpsConfigStatus);

            break;

        case SystemEvent::DELETE_ALL_LOGS:

            if(sd.removeAllLogs())
            {
                Serial.println("Todos os logs removidos.");
            }
            else
            {
                Serial.println("Falha ao remover logs.");
            }

            break;
        case SystemEvent::DELETE_LOG:

            if(sd.removeFile(wifi.getRequestedLog()))
            {
                Serial.println("Log removido.");
            }
            else
            {
                Serial.println("Falha ao remover log.");
            }

            break;
        case SystemEvent::RENAME_LOG:

            if(sd.renameFile(wifi.getRequestedLog(), wifi.getRenameTarget()))
            {
                Serial.println("Log renomeado.");
            }
            else
            {
                Serial.println("Falha ao renomear log.");
            }

            break;
        case SystemEvent::TELEMETRY_REQUEST:

            wifi.setTelemetryJson(telemetry.buildLoraPacket(false));

            break;

        case SystemEvent::RESTART:

            ESP.restart();

            return;
    }

    wifi.clearPendingEvent();
}
void handleOutputs(FlightMode mode){

    bool lowPower = isLowPowerModeEnabled();
    uint32_t period = lowPower? systemConfig.lowBatteryTelemetryPeriodMs : systemConfig.telemetryPeriodMs;

    if(FlightMode::CONFIG == mode){

        if(systemConfig.preFlightTelemetryEnabled &&
           millis() - lastTelemetryMillis >= period)
        {
            lastTelemetryMillis = millis();

            systemStatus.loraOk = lora.send(telemetry.buildLoraPacket(lowPower));
        }
    }
    else if(FlightMode::COUNTDOWN == mode || FlightMode::FLIGHT == mode){

        // SD e LoRa tem timers independentes agora (ver comentario em
        // lastSdMillis, topo do arquivo) -- um nao adia mais o outro.
        if(millis() - lastSdMillis >= SD_LOG_INTERVAL_MS){

            lastSdMillis = millis();
            systemStatus.sdOk = sd.bufferLine(telemetry.buildCsv());
        }

        if(millis() - lastTelemetryMillis >= period){

            lastTelemetryMillis = millis();
            systemStatus.loraOk = lora.send(telemetry.buildLoraPacket(lowPower));
        }
    }

    else if(FlightMode::LANDED == mode){

        period = systemConfig.lowBatteryTelemetryPeriodMs;
    }
}
void sendNavigationData(NavigationData& navigationData){
    
  const bool gpsValid = gps.isValid();

    navigationData.latitude =
        gpsValid ? gps.getLatitude() : 0.0;

    navigationData.longitude =
        gpsValid ? gps.getLongitude() : 0.0;

    navigationData.gpsAltitude =
        gpsValid ? gps.getAltitude() : 0.0f;

    navigationData.course =
        gpsValid ? gps.getCourse() : 0.0f;

    navigationData.satellites =
        gps.getSatellites();

    navigationData.baroAltitude =
        bmp.getRawAltitude();

    navigationData.temperature =
        bmp.getTemperature();

    // TESTE SEM ADS1115: sem bateria/pitot, campos ficam em 0 (default).
    // navigationData.battery =
    //     ads.batteryLevel();

    // navigationData.airspeed =
    //     pitot.getAirspeed();

    // navigationData.pitotRawVoltage =
    //     pitot.getRawVoltage();

    xQueueOverwrite(
        navigationQueue,
        &navigationData
    );
}
bool readAttitudeData(AttitudeData& attitude){
    return xQueueReceive(
        attitudeQueue,
        &attitude,
        0
    ) == pdTRUE;
}
bool isLowPowerModeEnabled(){
    // TESTE SEM ADS1115: sem leitura de bateria, nunca entra em baixa energia.
    return false;
    // return ads.batteryLevel() < systemConfig.batteryLimit;
}
void updateGpsStatus(){
    if(!gps.hasCommunication())
    {
        systemStatus.gpsStatus = GpsStatus::NO_COMMUNICATION;
        return;
    }

    if(!gps.isValid())
    {
        systemStatus.gpsStatus = GpsStatus::NO_FIX;
        return;
    }

    if(gps.getSatellites() < 6)
    {
        systemStatus.gpsStatus = GpsStatus::POOR_FIX;
        return;
    }

    systemStatus.gpsStatus = GpsStatus::GOOD_FIX;
}
// Aplica a config UBX de gpsSettings.h em gpsSerial (ja aberta -- ver
// setup()) via ApplyGpsConfig, guarda o resultado em gpsConfigStatus
// pra reportar na pagina web (WifiAP::setGpsConfigStatus) e no Serial.
// Chamada tanto no boot (setup()) quanto pelo botao "Forcar
// Diagnostico" da pagina web (SystemEvent::CHECK_GPS).
//
// Retorna false SO' se o GPS nao respondeu em NENHUM baud testado
// (modulo desligado/desconectado/fiacao errada) -- se respondeu mas
// nem todo comando foi confirmado, ainda retorna true (o modulo
// continua dando fix normalmente, so nao fica 100% configurado como
// esperado; ver gpsConfigStatus.ok pro diagnostico fino).
bool beginGps(){
    ApplyGpsConfig applyConfig(gpsSerial);
    GpsConfigResult result = applyConfig.apply(Commands, COMMAND_COUNT);

    gpsConfigStatus.ok             = result.ok;
    gpsConfigStatus.detectedBaud   = result.detectedBaud;
    gpsConfigStatus.finalBaud      = result.finalBaud;
    gpsConfigStatus.confirmedCount = (uint8_t)result.confirmedCount;
    gpsConfigStatus.totalCount     = (uint8_t)result.totalCount;

    strncpy(gpsConfigStatus.configJson, result.configJson, sizeof(gpsConfigStatus.configJson) - 1);
    gpsConfigStatus.configJson[sizeof(gpsConfigStatus.configJson) - 1] = '\0';

    Serial.printf(
        "[GPS] %u/%u comandos confirmados (detectado em %lu, final em %lu)\n",
        (unsigned)result.confirmedCount, (unsigned)result.totalCount,
        (unsigned long)result.detectedBaud, (unsigned long)result.finalBaud
    );
    Serial.print("[GPS] config: ");
    Serial.println(result.configJson);

    return result.detectedBaud != 0;
}
void setupWifi(){
    // Nunca voou: liga normalmente
    /*if(systemConfig.flightMode == FlightMode::CONFIG)
    {
        wifiEnabled = true;
        wifiTemporary = false;
    }
    // Já voou: só liga se detectar 3,3 V no A3
    else if(ads.adcVoltage(3) > 3.0f)
    {
        wifiEnabled = true;
        wifiTemporary = true;
        wifiStartMillis = millis();
    }*/

    //if(!wifiEnabled)
        //return;

    if(wifi.begin())
    {
        wifiEnabled = true;

        wifi.attachSdLogger(&sd);

        wifi.print();

        wifi.setSystemConfig(systemConfig);

        wifi.setOffsets(currentOffsets);

        wifi.setPidConfig(currentPidConfig);

        wifi.setGpsConfigStatus(gpsConfigStatus);
    }
    else
    {
        wifiEnabled = false;
    }
}
void handleWifiTimeout(){
    if(!wifiEnabled || !wifiTemporary)
        return;

    // Se alguém conectou, mantém o AP ligado
    if(wifi.hasClient())
    {
        wifiTemporary = false;
        return;
    }

    // Ninguém conectou durante 20 s
    if(millis() - wifiStartMillis >= 20000)
    {
        wifi.stop();

        wifiEnabled = false;

        wifiTemporary = false;
    }
}
TelemetryData buildTelemetryData(AttitudeData& attitude){

    TelemetryData data{};

    data.state = systemConfig.flightMode;

    data.gpsOk = gps.isValid();
    data.imuOk = attitude.isImuOk;

    if(attitude.isImuOk)
    {
        data.pitch = attitude.pitch;
        data.roll  = attitude.roll;
        data.yaw   = attitude.yaw;

        data.accX = attitude.accX;
        data.accY = attitude.accY;
        data.accZ = attitude.accZ;

        data.gyroX = attitude.gyroX;
        data.gyroY = attitude.gyroY;
        data.gyroZ = attitude.gyroZ;

        data.magX = attitude.magX;
        data.magY = attitude.magY;
        data.magZ = attitude.magZ;
    }

    // Fora do "if(isImuOk)" de proposito: a posicao do servo e valida
    // (ou fica travada no ultimo/neutro valor) independente da saude da
    // IMU, e e util ver isso no log mesmo quando o IMU cai.
    data.servoElevator = attitude.servoElevator;
    data.servoLeftAileron = attitude.servoLeftAileron;
    data.servoRightAileron = attitude.servoRightAileron;

    if(data.gpsOk){
        
        data.latitude   = gps.getLatitude();
        data.longitude  = gps.getLongitude();
        data.gpsAltitude = gps.getAltitude();
        data.course     = gps.getCourse();
        data.gpsSpeed   = gps.getSpeed();

        data.day    = gps.getDay();
        data.month  = gps.getMonth();
        data.year   = gps.getYear();

        data.hour   = gps.getHour();
        data.minute = gps.getMinute();
        data.second = gps.getSecond();
    }

    data.satellites = gps.getSatellites();

    data.baroAltitude = bmp.getRawAltitude();
    data.temperature  = bmp.getTemperature();

    // TESTE SEM ADS1115: sem bateria/pitot, ficam em 0 (default de TelemetryData data{}).
    // data.battery = ads.batteryLevel();
    // data.airspeed = pitot.getAirspeed();

    return data;
}