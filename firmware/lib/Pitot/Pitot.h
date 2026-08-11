#pragma once

#include <Arduino.h>
#include "ADS1X15.h"

// Tubo de Pitot via MPXV7002DP (sensor de pressao diferencial), lido
// pelo mesmo ADS1115 que ja existe pra bateria -- so' converte a
// tensao (ADS::adcVoltage(), ja existente) em pressao/velocidade, nao
// possui o ADC. Sem calibracao estatica no boot: zeroVoltage comeca
// no valor de fabrica (ver DataTypes.h::ImuOffsets::pitotZeroVoltage)
// e so' muda quando alguem clica "Zerar" na pagina Offsets (ver
// setZeroVoltage()).
class Pitot
{
public:

    // 'adsChannel' e' o canal do ADS1115 ligado a saida do
    // MPXV7002DP (default 3, o mesmo default de ADS::adcVoltage()).
    explicit Pitot(ADS& ads, uint8_t adsChannel = 3);

    // Le o ADS, atualiza pressao filtrada + velocidade. Chame na
    // mesma core/task do resto da telemetria (BMP/GPS/bateria).
    void update();

    // Tensao CRUA do ultimo update() (sem zero, sem filtro) -- usada
    // pra popular o campo de tara na pagina web (o navegador manda
    // esse valor de volta como novo zeroVoltage).
    float getRawVoltage() const;

    float getPressurePa() const; // filtrada, com zero e zona morta ja aplicados
    float getAirspeed() const;   // m/s, com sinal (positivo = fluxo no sentido normal do tubo)

    float getZeroVoltage() const;
    void setZeroVoltage(float voltage);

private:

    ADS& ads;
    uint8_t channel;

    float zeroVoltage;

    float rawVoltage = 0.0f;
    float filteredPressure = 0.0f;
    float airspeed = 0.0f;

    // Razao do divisor resistivo entre a saida do MPXV7002DP (0-5V) e
    // a entrada do ADS1115 -- ajuste se trocar os resistores.
    static constexpr float DIVISOR_RATIO = 0.667f;

    // Alimentacao do MPXV7002DP -- entra na formula de pressao do
    // datasheet (Vout = Vcc*(0.2*P + 0.5)).
    static constexpr float SENSOR_VCC = 5.0f;
};
