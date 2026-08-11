#include "CurrentConfig.h"

SystemConfig CurrentConfig::currentConfig;

SystemStatus CurrentConfig::currentStatus;

portMUX_TYPE CurrentConfig::configMux =
    portMUX_INITIALIZER_UNLOCKED;

void CurrentConfig::begin(){}
void CurrentConfig::copy(SystemConfig& config){

    portENTER_CRITICAL(&configMux);

    config = currentConfig;

    portEXIT_CRITICAL(&configMux);
}
void CurrentConfig::update(const SystemConfig& config){

    portENTER_CRITICAL(&configMux);

    currentConfig = config;

    portEXIT_CRITICAL(&configMux);
}
SystemStatus& CurrentConfig::getStatus(){

    return currentStatus;
}