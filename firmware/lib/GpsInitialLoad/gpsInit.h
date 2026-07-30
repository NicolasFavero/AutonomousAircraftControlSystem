#pragma once

#include <Arduino.h>

// Modulo enxuto, sem dependencia da ferramenta de diagnostico, pra
// colar no firmware oficial. Reenvia, na ordem, todos os comandos
// listados em GpsSettings::Commands (gpsSettings.h) para o receptor
// GPS -- existe porque o receptor tem memoria persistente limitada
// (bateria de backup / NVRAM), entao em vez de confiar que ele
// guarda a configuracao entre boots, o firmware a reaplica toda vez.
//
// Chame no setup(), DEPOIS de abrir gpsSerial no baudrate em que o
// receptor esta hoje (o baudrate de FABRICA, ou o ultimo baudrate
// que voce configurou manualmente antes de gerar este arquivo -- ver
// README.md desta pasta). Se um dos comandos exportados for uma
// troca de baudrate, loadGpsConfig() reconfigura a propria gpsSerial
// para o baud novo automaticamente antes de continuar.
void loadGpsConfig(HardwareSerial& gpsSerial);
