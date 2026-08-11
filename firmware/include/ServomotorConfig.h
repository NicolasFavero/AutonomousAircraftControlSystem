#pragma once
#include <Arduino.h>
namespace ServoConfig{

    inline constexpr uint8_t FREQUENCY = 50;

    inline constexpr uint8_t RESOLUTION = 14;

    namespace Channel{
        inline constexpr uint8_t ELEVATOR = 0;

        inline constexpr uint8_t LEFT_AILERON = 1;

        inline constexpr uint8_t RIGHT_AILERON = 2;

        //inline constexpr uint8_t RUDDER = 3;
    }

    // Neutro FISICO de cada servo (graus) -- calibracao de montagem,
    // nao muda em uso normal. Assimetrico entre os ailerons (85/103,
    // nao e' espelho perfeito em torno de 90) por causa de como cada
    // horn foi montado fisicamente -- e' exatamente essa esquisitice
    // que o trim configuravel (PidConfig::flaperonTrim/elevatorTrim,
    // ver DataTypes.h) existe pra esconder: o piloto/operador so mexe
    // no trim (um valor simetrico), nunca nesses numeros base.
    namespace BaseNeutral{
        inline constexpr float ELEVATOR = 90.0f;
        inline constexpr float LEFT_AILERON = 85.0f;
        inline constexpr float RIGHT_AILERON = 103.0f;
    }

    // Fim de curso mecanico de cada servo (graus) -- o quanto cada um
    // pode girar de verdade antes de trombar no batente fisico da
    // ligacao/horn (bem mais estreito que os 0-180 genericos que o
    // proprio motor aceitaria). Usada pelo PID (PID::applyServoOutput),
    // pelo trim direto (main.cpp::syncPidNeutralAngles) E pelos proprios
    // objetos Servo (lib/Servo, minAngle/maxAngle no construtor) --
    // ultima linha de defesa: mesmo um caminho novo que esqueca de
    // clampar cai nesse limite dentro da propria lib.
    //
    // Ailerons NAO compartilham mais um unico min/max: cada lado tem o
    // seu, porque o fim de curso fisico de cada servo e' diferente (e
    // o neutro ja e' assimetrico mesmo, ver BaseNeutral acima).
    namespace SafeRange{
        inline constexpr float ELEVATOR_MIN = 20.0f;
        inline constexpr float ELEVATOR_MAX = 160.0f;

        inline constexpr float LEFT_AILERON_MIN = 53.0f;
        inline constexpr float LEFT_AILERON_MAX = 170.0f;

        inline constexpr float RIGHT_AILERON_MIN = 15.0f;
        inline constexpr float RIGHT_AILERON_MAX = 135.0f;
    }
}
