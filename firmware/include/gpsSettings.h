#pragma once
#include <Arduino.h>

// Configuracao oficial do GPS desta aeronave -- exportada de verdade
// pelo index.html (Web Serial, GPS real conectado), substituindo a
// versao anterior recuperada do modulo GpsInitialLoad (que mandava
// dynModel/fixMode como dois comandos CFG-NAV5 separados, cada um com
// mascara de 1 bit so -- esse GPS especifico nao aplica isso
// corretamente; sem ACK, sem efeito. Essa versao manda os dois juntos
// num unico CFG-NAV5 com mascara combinada, que e' o que a ferramenta
// ja faz por padrao -- ver comentario em index.html). Requer "Command"
// ja declarado -- inclua ApplyGpsConfig.h antes deste arquivo.
//
// 4 comando(s).

// Baudrate -> 115200
inline constexpr uint8_t CMD_00_BAUDRATE____115200[] = {181,98,6,0,20,0,1,0,0,0,192,8,0,0,0,194,1,0,7,0,3,0,0,0,0,0,176,126};

// CFG-RATE measurementRate=175ms
inline constexpr uint8_t CMD_01_CFG_RATE_MEASUREMENTRATE_175MS[] = {181,98,6,8,6,0,175,0,1,0,1,0,197,212};

// CFG-NAV5 dynamicModel=Airborne2g, fixMode=3Donly
inline constexpr uint8_t CMD_02_CFG_NAV5_DYNAMICMODEL_AIRBORNE2G__FIXMODE_3DONLY[] = {181,98,6,36,36,0,5,0,7,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,92,168};

// CFG-GNSS (ligadas: GPS+SBAS)
inline constexpr uint8_t CMD_03_CFG_GNSS__LIGADAS__GPS_SBAS_[] = {181,98,6,62,36,0,0,22,22,4,0,4,255,0,1,0,0,0,1,1,3,0,1,0,0,0,5,0,3,0,0,0,0,0,6,8,255,0,0,0,0,0,183,7};

inline constexpr Command Commands[] = {
    { "Baudrate -> 115200", CMD_00_BAUDRATE____115200, sizeof(CMD_00_BAUDRATE____115200), true, 115200u },
    { "CFG-RATE measurementRate=175ms", CMD_01_CFG_RATE_MEASUREMENTRATE_175MS, sizeof(CMD_01_CFG_RATE_MEASUREMENTRATE_175MS), false, 0u },
    { "CFG-NAV5 dynamicModel=Airborne2g, fixMode=3Donly", CMD_02_CFG_NAV5_DYNAMICMODEL_AIRBORNE2G__FIXMODE_3DONLY, sizeof(CMD_02_CFG_NAV5_DYNAMICMODEL_AIRBORNE2G__FIXMODE_3DONLY), false, 0u },
    { "CFG-GNSS (ligadas: GPS+SBAS)", CMD_03_CFG_GNSS__LIGADAS__GPS_SBAS_, sizeof(CMD_03_CFG_GNSS__LIGADAS__GPS_SBAS_), false, 0u },
};

constexpr size_t COMMAND_COUNT = sizeof(Commands)/sizeof(Commands[0]);
