#include "MahonyAHRS.h"
#include <math.h>

static const float DEG_TO_RAD_F = 0.017453292519943295f; // PI / 180
static const float RAD_TO_DEG_F = 57.29577951308232f;     // 180 / PI

MahonyAHRS::MahonyAHRS(float twoKp_, float twoKi_)
    : twoKp(twoKp_), twoKi(twoKi_),
      q0(1.0f), q1(0.0f), q2(0.0f), q3(0.0f),
      integralFBx(0.0f), integralFBy(0.0f), integralFBz(0.0f)
{
}

void MahonyAHRS::begin(float twoKp_, float twoKi_) {
    twoKp = twoKp_;
    twoKi = twoKi_;
}

void MahonyAHRS::reset() {
    q0 = 1.0f; q1 = 0.0f; q2 = 0.0f; q3 = 0.0f;
    integralFBx = 0.0f; integralFBy = 0.0f; integralFBz = 0.0f;
}

void MahonyAHRS::update(float dt,
                         float gx, float gy, float gz,
                         float ax, float ay, float az,
                         float mx, float my, float mz) {

    // Sem magnetometro valido (ainda nao leu / saturado / todo zero):
    // cai pro 6DOF em vez de deixar a correcao de yaw quebrada.
    if ((mx == 0.0f) && (my == 0.0f) && (mz == 0.0f)) {
        updateIMU(dt, gx, gy, gz, ax, ay, az);
        return;
    }

    float recipNorm;
    float q0q0, q0q1, q0q2, q0q3, q1q1, q1q2, q1q3, q2q2, q2q3, q3q3;
    float hx, hy, bx, bz;
    float halfvx, halfvy, halfvz, halfwx, halfwy, halfwz;
    float halfex, halfey, halfez;
    float qa, qb, qc;

    gx *= DEG_TO_RAD_F;
    gy *= DEG_TO_RAD_F;
    gz *= DEG_TO_RAD_F;

    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

        // Normaliza acelerometro
        recipNorm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;

        // Normaliza magnetometro
        recipNorm = invSqrt(mx * mx + my * my + mz * mz);
        mx *= recipNorm;
        my *= recipNorm;
        mz *= recipNorm;

        // Produtos auxiliares do quaternion, pra nao recalcular varias vezes
        q0q0 = q0 * q0;
        q0q1 = q0 * q1;
        q0q2 = q0 * q2;
        q0q3 = q0 * q3;
        q1q1 = q1 * q1;
        q1q2 = q1 * q2;
        q1q3 = q1 * q3;
        q2q2 = q2 * q2;
        q2q3 = q2 * q3;
        q3q3 = q3 * q3;

        // Referencia do campo magnetico no frame do corpo -> projeta no
        // plano horizontal (bx) e no eixo vertical (bz) do frame terrestre,
        // pra so precisar estimar a componente horizontal do heading.
        hx = 2.0f * (mx * (0.5f - q2q2 - q3q3) + my * (q1q2 - q0q3) + mz * (q1q3 + q0q2));
        hy = 2.0f * (mx * (q1q2 + q0q3) + my * (0.5f - q1q1 - q3q3) + mz * (q2q3 - q0q1));
        bx = sqrtf(hx * hx + hy * hy);
        bz = 2.0f * (mx * (q1q3 - q0q2) + my * (q2q3 + q0q1) + mz * (0.5f - q1q1 - q2q2));

        // Vetores de gravidade e campo magnetico estimados (forma "half")
        halfvx = q1q3 - q0q2;
        halfvy = q0q1 + q2q3;
        halfvz = q0q0 - 0.5f + q3q3;
        halfwx = bx * (0.5f - q2q2 - q3q3) + bz * (q1q3 - q0q2);
        halfwy = bx * (q1q2 - q0q3) + bz * (q0q1 + q2q3);
        halfwz = bx * (q0q2 + q1q3) + bz * (0.5f - q1q1 - q2q2);

        // Erro = soma dos produtos vetoriais entre vetores medidos e
        // estimados (gravidade + campo magnetico)
        halfex = (ay * halfvz - az * halfvy) + (my * halfwz - mz * halfwy);
        halfey = (az * halfvx - ax * halfvz) + (mz * halfwx - mx * halfwz);
        halfez = (ax * halfvy - ay * halfvx) + (mx * halfwy - my * halfwx);

        if (twoKi > 0.0f) {
            integralFBx += twoKi * halfex * dt;
            integralFBy += twoKi * halfey * dt;
            integralFBz += twoKi * halfez * dt;
            gx += integralFBx;
            gy += integralFBy;
            gz += integralFBz;
        } else {
            integralFBx = 0.0f;
            integralFBy = 0.0f;
            integralFBz = 0.0f;
        }

        gx += twoKp * halfex;
        gy += twoKp * halfey;
        gz += twoKp * halfez;
    }

    gx *= (0.5f * dt);
    gy *= (0.5f * dt);
    gz *= (0.5f * dt);
    qa = q0;
    qb = q1;
    qc = q2;
    q0 += (-qb * gx - qc * gy - q3 * gz);
    q1 += (qa * gx + qc * gz - q3 * gy);
    q2 += (qa * gy - qb * gz + q3 * gx);
    q3 += (qa * gz + qb * gy - qc * gx);

    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
}

void MahonyAHRS::updateIMU(float dt,
                            float gx, float gy, float gz,
                            float ax, float ay, float az) {
    float recipNorm;
    float halfvx, halfvy, halfvz;
    float halfex, halfey, halfez;
    float qa, qb, qc;

    // Giroscopio: graus/s -> rad/s
    gx *= DEG_TO_RAD_F;
    gy *= DEG_TO_RAD_F;
    gz *= DEG_TO_RAD_F;

    // So usa a correcao do acelerometro se a leitura nao for invalida
    // (evita divisao por zero se ax=ay=az=0, ex.: sensor ainda nao leu nada)
    if (!((ax == 0.0f) && (ay == 0.0f) && (az == 0.0f))) {

        // Normaliza o vetor de aceleracao medido
        recipNorm = invSqrt(ax * ax + ay * ay + az * az);
        ax *= recipNorm;
        ay *= recipNorm;
        az *= recipNorm;

        // Vetor "gravidade estimada" a partir do quaternion atual (metade,
        // ja simplificado algebricamente -- forma padrao da referencia
        // Mahony/Madgwick de codigo aberto)
        halfvx = q1 * q3 - q0 * q2;
        halfvy = q0 * q1 + q2 * q3;
        halfvz = q0 * q0 - 0.5f + q3 * q3;

        // Erro = produto vetorial entre a gravidade medida e a estimada
        halfex = (ay * halfvz - az * halfvy);
        halfey = (az * halfvx - ax * halfvz);
        halfez = (ax * halfvy - ay * halfvx);

        // Feedback integral (estima e compensa o bias do giroscopio)
        if (twoKi > 0.0f) {
            integralFBx += twoKi * halfex * dt;
            integralFBy += twoKi * halfey * dt;
            integralFBz += twoKi * halfez * dt;
            gx += integralFBx;
            gy += integralFBy;
            gz += integralFBz;
        } else {
            integralFBx = 0.0f;
            integralFBy = 0.0f;
            integralFBz = 0.0f;
        }

        // Feedback proporcional
        gx += twoKp * halfex;
        gy += twoKp * halfey;
        gz += twoKp * halfez;
    }

    // Integracao do quaternion (metodo de Euler de 1a ordem)
    gx *= (0.5f * dt);
    gy *= (0.5f * dt);
    gz *= (0.5f * dt);
    qa = q0;
    qb = q1;
    qc = q2;
    q0 += (-qb * gx - qc * gy - q3 * gz);
    q1 += (qa * gx + qc * gz - q3 * gy);
    q2 += (qa * gy - qb * gz + q3 * gx);
    q3 += (qa * gz + qb * gy - qc * gx);

    // Normaliza o quaternion
    recipNorm = invSqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;
}

float MahonyAHRS::getRoll() const {
    float raw = atan2f(2.0f * (q0 * q1 + q2 * q3), 1.0f - 2.0f * (q1 * q1 + q2 * q2));
    return raw * RAD_TO_DEG_F;
}

float MahonyAHRS::getPitch() const {
    float temp = 2.0f * (q0 * q2 - q3 * q1);
    if (temp > 1.0f) temp = 1.0f;
    if (temp < -1.0f) temp = -1.0f;
    return asinf(temp) * RAD_TO_DEG_F;
}

float MahonyAHRS::getYaw() const {
    float raw = atan2f(2.0f * (q0 * q3 + q1 * q2), 1.0f - 2.0f * (q2 * q2 + q3 * q3));
    return raw * RAD_TO_DEG_F;
}

void MahonyAHRS::getQuaternion(float &w, float &x, float &y, float &z) const {
    w = q0; x = q1; y = q2; z = q3;
}

float MahonyAHRS::invSqrt(float x) {
    // 1/sqrt(x) direto -- nao usamos o truque de bit-hack "fast inverse
    // sqrt" (aquele do Quake): no ESP32-S3 o FPU de hardware calcula sqrtf
    // rapido o bastante, e o truque de bits perde precisao à toa aqui.
    return 1.0f / sqrtf(x);
}
