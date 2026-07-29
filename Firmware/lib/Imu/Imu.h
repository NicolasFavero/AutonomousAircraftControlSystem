#pragma once
//  OBS CONFERIR SE OS EIXOS DO MAGNOMETROS JÁ ESTÃO SENDO CORRIGIDOS NA LIB ICM20948 OU SE ESTOU CERTO NO CÓDIGO, O YAW ESTÁ RUIM AINDA
#include "Wire.h"
#include "ICM_20948.h"
#include "MahonyAHRS.h"
#include <Arduino.h>

//Wire.begin(SDA, SCL)  --> main.cpp

class IMU{
    public:

        IMU();

        bool begin();
        bool update();
        bool isHealthy() const;
        void print();

        float getPitch() const;
        float getRoll() const;
        float getYaw() const;

        float getAccX() const;
        float getAccY() const;
        float getAccZ() const;

        float getGyroX() const;
        float getGyroY() const;
        float getGyroZ() const;

        float getMagX() const;
        float getMagY() const;
        float getMagZ() const;

    private:
        ICM_20948_I2C imu;

        // Fusao sensorial feita na mao (sem DMP), le registradores crus a
        // cada chamada de update() -- ver Imu.cpp para o porque da troca.
        MahonyAHRS ahrs;

        // Timestamp (micros) da ultima chamada ao filtro, usado pra medir
        // o dt real entre amostras (independente de STALE_TIMEOUT_US, que
        // e sobre saude do sensor, nao sobre o dt do filtro).
        uint32_t lastFilterMicros = 0;

        float pitch = 0.0f;
        float roll  = 0.0f;
        float yaw   = 0.0f; 

        float accX = 0.0f;
        float accY = 0.0f;
        float accZ = 0.0f;

        float gyroX = 0.0f;
        float gyroY = 0.0f;
        float gyroZ = 0.0f;

        float magX = 0.0f;
        float magY = 0.0f;
        float magZ = 0.0f;

        // Timestamp (micros) da ultima leitura de quaternion valida do
        // DMP. So marca sensorOk=false quando passar STALE_TIMEOUT_US
        // sem nenhuma leitura boa -- assim uma unica falha isolada (ou
        // o DMP simplesmente ainda nao ter gerado amostra nesse ciclo,
        // ja que a taxa dele e bem menor que a do loop de controle) nao
        // e confundida com o sensor realmente travado/desconectado.
        unsigned long lastUpdateMicros = 0;

        static constexpr unsigned long STALE_TIMEOUT_US = 300000; // 300ms

        bool sensorOk = true;

        static constexpr uint8_t ADDRESS = 0; //it corresponds with the 0x68

};
