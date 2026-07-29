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
            float safeAngle = 90.0f;
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

        void computePID(PID_variables& pid, ServoParamters& servo, float currentAngle, float dt, float integralLimit, float maxMovement, int minLimit, int maxLimit, bool invert = false);

};
