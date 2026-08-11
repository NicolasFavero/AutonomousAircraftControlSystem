#include "Pitot.h"
#include <math.h>

Pitot::Pitot(ADS& ads, uint8_t adsChannel) : ads(ads), channel(adsChannel), zeroVoltage(1.8385f) {}

void Pitot::update(){

    rawVoltage = ads.adcVoltage(channel);

    float vSensor = rawVoltage / DIVISOR_RATIO;

    // zeroVoltage e' guardado na tensao CRUA (lado do ADS, antes do
    // divisor -- e' o que getRawVoltage()/a tara na pagina web
    // capturam), entao precisa passar pelo MESMO divisor que vSensor
    // acima antes de entrar na formula -- senao compara tensao crua
    // com tensao dividida, dando um "zero" completamente errado (bug
    // real, confirmado em hardware: ~40 m/s parado).
    float zeroSensor = zeroVoltage / DIVISOR_RATIO;

    // Formula exata do datasheet do MPXV7002DP: Vout = Vcc*(0.2*P + 0.5),
    // P em kPa -- isolando P: P = (Vout/Vcc - 0.5) / 0.2.
    float pressureKpa = ((vSensor / SENSOR_VCC) - 0.5f) / 0.2f;
    float zeroKpa      = ((zeroSensor / SENSOR_VCC) - 0.5f) / 0.2f;

    pressureKpa -= zeroKpa;

    float pressurePa = pressureKpa * 1000.0f;

    // Filtro exponencial (90/10) -- suaviza ruido do ADC sem atrasar
    // demais a resposta a mudancas reais de velocidade.
    filteredPressure = filteredPressure * 0.90f + pressurePa * 0.10f;

    // Zona morta -- abaixo de 1 Pa e' ruido do sensor parado, nao
    // fluxo de ar de verdade.
    if(fabsf(filteredPressure) < 1.0f)
        filteredPressure = 0.0f;

    airspeed = 0.0f;

    if(filteredPressure != 0.0f)
    {
        // Formula de Bernoulli (v = sqrt(2*deltaP/rho), rho do ar ao
        // nivel do mar) -- com sinal, preserva a direcao do fluxo
        // (util pra detectar vento de cauda/ar entrando ao contrario).
        airspeed = sqrtf((2.0f * fabsf(filteredPressure)) / 1.225f);

        if(filteredPressure < 0)
            airspeed = -airspeed;
    }
}

float Pitot::getRawVoltage() const {return rawVoltage;}
float Pitot::getPressurePa() const {return filteredPressure;}
float Pitot::getAirspeed() const {return airspeed;}
float Pitot::getZeroVoltage() const {return zeroVoltage;}
void Pitot::setZeroVoltage(float voltage){zeroVoltage = voltage;}
