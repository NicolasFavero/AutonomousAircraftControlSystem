#pragma once
// =========================================================================
// MahonyAHRS - filtro de fusao sensorial por quaternion (Mahony / MARG)
// =========================================================================
// Mesma familia de algoritmo usada no AHRS complementar do ArduPilot e do
// Betaflight: um filtro complementar em forma de quaternion, com feedback
// proporcional+integral corrigindo o giroscopio a partir dos vetores de
// referencia (gravidade via acelerometro, norte magnetico via magnetometro).
// Nao e um EKF (nao ha covariancia/matriz de estados), o que o torna leve o
// bastante pra rodar todo ciclo de loop num microcontrolador, sem precisar
// do FIFO/DMP do sensor.
//
// Le os dados CRUS do sensor (acc/gyro/mag), diretamente dos registradores,
// entao a cadencia de atualizacao e a cadencia real do loop de controle
// (ex.: 200Hz) e nao fica presa a taxa de saida do DMP.
// =========================================================================

class MahonyAHRS {
public:
    // twoKp / twoKi = 2x os ganhos proporcional/integral do artigo original
    // de Mahony (convencao usual das implementacoes de referencia).
    // twoKp: o quanto o acelerometro/magnetometro corrigem o giroscopio a
    //        cada amostra (resposta mais rapida = mais ruido do acc entra).
    // twoKi: o quanto o filtro estima e compensa o bias do giroscopio ao
    //        longo do tempo (ajuda a zerar deriva residual; comece em 0 e
    //        so aumente aos poucos depois de validar o twoKp em bancada).
    MahonyAHRS(float twoKp = 2.0f * 1.0f, float twoKi = 0.0f);

    // (Re)configura os ganhos em tempo de execucao (ex.: vindo de uma tela
    // de configuracao, igual ao kp/ki do PID).
    void begin(float twoKp, float twoKi);

    // Reinicia a orientacao para a identidade e zera o termo integral do
    // bias do giroscopio. Chame isso ao trocar de FlightMode/relogar, se
    // fizer sentido no fluxo do main.cpp -- hoje nao e chamado automaticamente.
    void reset();

    // Atualizacao completa 9DOF (acc + gyro + mag).
    // dt: segundos desde a ultima chamada (use o dt real medido via micros(),
    //     nao um valor fixo -- o FreeRTOS com vTaskDelayUntil tem jitter).
    // gx,gy,gz: giroscopio em graus/segundo (gyrX()/gyrY()/gyrZ() da lib
    //           ICM_20948 ja retornam nessa unidade -- passe direto).
    // ax,ay,az: acelerometro em qualquer unidade consistente (o filtro
    //           normaliza internamente) -- accX()/Y()/Z() da lib servem direto.
    // mx,my,mz: magnetometro em qualquer unidade consistente (idem, sem
    //           conversao previa) -- magX()/Y()/Z() da lib servem direto.
    // Se o magnetometro nao tiver dado amostra nova neste ciclo, pode
    // reenviar o ultimo valor bom sem problema: o peso da correcao
    // magnetica e propositalmente pequeno perto do vetor de gravidade.
    void update(float dt,
                float gx, float gy, float gz,
                float ax, float ay, float az,
                float mx, float my, float mz);

    // Atualizacao 6DOF (sem magnetometro) -- use quando o mag estiver
    // indisponivel, saturado ou ainda em calibracao.
    void updateIMU(float dt,
                   float gx, float gy, float gz,
                   float ax, float ay, float az);

    float getRoll()  const; // graus
    float getPitch() const; // graus
    float getYaw()   const; // graus (heading magnetico se update() com mag
                             //        foi usado; senao, so integra o giro
                             //        e vai derivar como qualquer 6DOF)

    void getQuaternion(float &w, float &x, float &y, float &z) const;

private:
    float twoKp;
    float twoKi;

    float q0, q1, q2, q3;

    float integralFBx, integralFBy, integralFBz;

    static float invSqrt(float x);
};
