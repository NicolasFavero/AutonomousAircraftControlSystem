#pragma once

#include <Arduino.h>
#include <DataTypes.h>



class PID {

    public:
        PID(){};

        void setAngles(AttitudeData& attitudeData, NavigationData& nav, bool gpsData);

        struct PID_variables{
            // Antes sem valor inicial nenhum -- kp/ki/kd/integralLimit
            // ficavam com lixo de memoria ate a primeira config ser
            // aplicada. Zerado por seguranca: com ganho 0 o PID nao
            // produz nenhuma correcao ate ser configurado de verdade.
            float kp = 0.0f;
            float ki = 0.0f;
            float kd = 0.0f;

            float lastError = 0.0f;
            float error = 0.0f;
            float target = 0.0f;

            float proporcional = 0.0f;
            float integral = 0.0f;
            float integralLimit = 0.0f;
            float derivative = 0.0f;

            float output = 0.0f;
        };

        PID_variables pitch;
        PID_variables roll;
        PID_variables yaw; // Ainda nao usado em computePID, so guarda o valor pro futuro.

        struct ServoParamters{

            float setAngle = 90.0f;
            float targetAngle = 90.0f;
            float neutralAngle = 90.0f;
        };

        // Publicos: antes o angulo final de cada servo saia daqui via
        // uma struct ServoPositions separada (em DataTypes.h), so pra
        // copiar de volta esses mesmos 3 valores pro chamador. Como
        // ninguem alem do main.cpp lia essa copia, o main.cpp agora le
        // o angulo direto daqui (ex: pid.elevator.setAngle) e a copia
        // -- e a struct ServoPositions inteira -- deixou de existir.
        ServoParamters elevator;
        ServoParamters leftAileron;
        ServoParamters rightAileron;

    private:

        // Histórico de tempo
        uint32_t lastTime = 0;

        // Separado de computePID() em dois passos -- ver comentario em
        // setAngles() (PID.cpp) sobre o roll ser compartilhado por 2
        // servos (leftAileron/rightAileron) e precisar rodar so' UMA vez
        // por ciclo, nao uma vez por servo.
        void updatePidVariables(PID_variables& pid, float currentAngle, float dt);

        // Sem limitador de taxa (slew rate) -- tirado a pedido, o servo
        // vai direto pro angulo alvo do PID, sem rampa/atraso nenhum.
        // minLimit/maxLimit continuam (fim de curso mecanico, ver
        // ServoConfig::SafeRange) -- isso e' limite de POSICAO, nao de
        // velocidade, protege o servo de forcar contra o batente.
        void applyServoOutput(ServoParamters& servo, float output, float minLimit, float maxLimit, bool invert = false);

};
