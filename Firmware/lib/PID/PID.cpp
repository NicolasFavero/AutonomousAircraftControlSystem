#include "PID.h"

void PID::setAngles(AttitudeData& attitudeData, NavigationData& nav, bool gpsData) {

    // Definição dos alvos do PID
    pitch.target = -3.0f;
    roll.target = 0.0f;

    // Configuração dos ângulos neutros dos servos
    leftAileron.neutralAngle = 85.0f;
    rightAileron.neutralAngle = 103.0f;
    elevator.neutralAngle = 90.0f;

    // Cálculo do Delta Time (dt)
    uint32_t now = micros();
    float dt = (lastTime == 0) ? 0.005f : (now - lastTime) / 1000000.0f;
    lastTime = now;

    // Inicialização do safeAngle no primeiro ciclo para evitar saltos bruscos
    if (elevator.safeAngle == 0.0f) elevator.safeAngle = elevator.neutralAngle;
    if (leftAileron.safeAngle == 0.0f) leftAileron.safeAngle = leftAileron.neutralAngle;
    if (rightAileron.safeAngle == 0.0f) rightAileron.safeAngle = rightAileron.neutralAngle;

    // Configuração do Limitador de Taxa (Slew Rate)
    // Restaurado: estava comentado, mas usado logo abaixo nas 3 chamadas
    // de computePID() -- isso e o erro de compilacao (maxMovementThisCycle
    // nao declarado) apontado antes.
    const float MAX_VELOCITY_DEG_PER_SEC = 90.0f;
    float maxMovementThisCycle = MAX_VELOCITY_DEG_PER_SEC * dt;

    //================ EXECUÇÃO DO PITCH (Profundor) =================
    computePID(pitch, elevator, attitudeData.pitch, dt, pitch.integralLimit, maxMovementThisCycle, 20, 160, true);

    //================ EXECUÇÃO DO ROLL (Ailerons) =================

    computePID(roll, leftAileron,  attitudeData.roll, dt, roll.integralLimit, maxMovementThisCycle, 20, 165, false);
    computePID(roll, rightAileron, attitudeData.roll, dt, roll.integralLimit, maxMovementThisCycle, 20, 165, false);

    // elevator.setAngle / leftAileron.setAngle / rightAileron.setAngle
    // (publicos) ja ficam com o angulo final depois do computePID() --
    // nao ha mais nenhuma copia de saida aqui, o chamador (main.cpp) le
    // esses 3 campos direto do objeto PID.
}

// Instâncias globais ou membros da classe

// Função auxiliar privada para processar o PID e atualizar o Servo
void PID::computePID(PID_variables& pid, ServoParamters& servo, float currentAngle, float dt, float integralLimit, float maxMovement, int minLimit, int maxLimit, bool invert) {
    // 1. Cálculo do erro
    pid.error = pid.target - currentAngle;

    // 2. Integral com Anti-Windup
    // Corrigido: estava comparando/saturando com pid.integralLimit (membro
    // que nunca e configurado em lugar nenhum do firmware -- fica sempre
    // 0.0f -- ver setAngles() logo abaixo), o que zerava a acao integral na
    // pratica mesmo com Ki configurado. Usa o parametro integralLimit, que
    // e o valor de fato passado pelo chamador.
    pid.integral += pid.error * dt;
    if(pid.integral > integralLimit)  pid.integral = integralLimit;
    if(pid.integral < -integralLimit) pid.integral = -integralLimit;

    // 3. Derivada
    pid.derivative = (dt > 0.0f) ? (pid.error - pid.lastError) / dt : 0.0f;

    // 4. Saída do PID
    pid.proporcional = pid.kp * pid.error;
    pid.output = pid.proporcional + (pid.ki * pid.integral) + (pid.kd * pid.derivative);
    pid.lastError = pid.error;

    // 5. Cálculo do ângulo alvo baseado na saída do PID
    // Restaurado: sem isso o offset do neutro do servo (ex.: 85/103/90
    // graus configurados em setAngles()) era perdido, e o setAngle usava
    // pid.output cru como se fosse um angulo absoluto.
    servo.targetAngle = invert
    ? servo.neutralAngle - pid.output
    : servo.neutralAngle + pid.output;

    // 6. Limitador de Taxa (Slew Rate) baseado no safeAngle do ciclo anterior
    servo.safeAngle = constrain(servo.targetAngle, servo.safeAngle - maxMovement, servo.safeAngle + maxMovement);

    // 7. Limite de hardware físico gravado diretamente no setAngle do servo
    servo.setAngle = constrain(servo.safeAngle, minLimit, maxLimit);
}

