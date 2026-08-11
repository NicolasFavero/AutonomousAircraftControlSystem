#include "PID.h"
#include "ServomotorConfig.h"

void PID::setAngles(AttitudeData& attitudeData, NavigationData& nav, bool gpsData) {

    // Definição dos alvos do PID
    pitch.target = -3.0f;
    roll.target = 0.0f;

    // Angulos neutros NAO sao mais fixados aqui -- vem de fora
    // (main.cpp, sincronizados a partir de PidConfig::flaperonTrim/
    // elevatorTrim toda vez que a config muda; ver o bloco
    // "newPidAvailable" em taskControl()). setAngles() so' usa o que
    // ja estiver em elevator/leftAileron/rightAileron.neutralAngle.

    // Cálculo do Delta Time (dt)
    uint32_t now = micros();
    float dt = (lastTime == 0) ? 0.005f : (now - lastTime) / 1000000.0f;
    lastTime = now;

    //================ EXECUÇÃO DO PITCH (Profundor) =================
    updatePidVariables(pitch, attitudeData.pitch, dt);
    applyServoOutput(elevator, pitch.output, ServoConfig::SafeRange::ELEVATOR_MIN, ServoConfig::SafeRange::ELEVATOR_MAX, true);

    //================ EXECUÇÃO DO ROLL (Ailerons) =================

    // Corrigido: antes chamava computePID(roll, ...) uma vez pra cada
    // aileron, e as duas chamadas mexiam no MESMO objeto PID_variables
    // roll -- entao o termo integral (roll.integral += error*dt) era
    // acumulado 2x por ciclo (dobrando a acao do Ki na pratica) e ainda
    // ficava assimetrico entre os ailerons (rightAileron sempre calculado
    // em cima de um integral ja "adiantado" pelo leftAileron no mesmo
    // ciclo). error/currentAngle sao os mesmos pros dois lados (e' um
    // unico angulo de roll da aeronave), entao o calculo do PID roda UMA
    // vez so' e o mesmo output e' aplicado aos 2 servos.
    updatePidVariables(roll, attitudeData.roll, dt);
    applyServoOutput(leftAileron,  roll.output, ServoConfig::SafeRange::LEFT_AILERON_MIN,  ServoConfig::SafeRange::LEFT_AILERON_MAX,  false);
    applyServoOutput(rightAileron, roll.output, ServoConfig::SafeRange::RIGHT_AILERON_MIN, ServoConfig::SafeRange::RIGHT_AILERON_MAX, false);

    // elevator.setAngle / leftAileron.setAngle / rightAileron.setAngle
    // (publicos) ja ficam com o angulo final depois do applyServoOutput()
    // -- nao ha mais nenhuma copia de saida aqui, o chamador (main.cpp)
    // le esses 3 campos direto do objeto PID.
}

// Instâncias globais ou membros da classe

// Calcula error/integral/derivative/output de um eixo -- so' isso, nao
// mexe em nenhum servo (ver applyServoOutput logo abaixo). Separado pra
// poder rodar UMA vez por eixo mesmo quando o eixo controla mais de um
// servo (roll -> leftAileron + rightAileron).
void PID::updatePidVariables(PID_variables& pid, float currentAngle, float dt) {
    // 1. Cálculo do erro
    pid.error = pid.target - currentAngle;

    // 2. Integral com Anti-Windup
    pid.integral += pid.error * dt;
    if(pid.integral > pid.integralLimit)  pid.integral = pid.integralLimit;
    if(pid.integral < -pid.integralLimit) pid.integral = -pid.integralLimit;

    // 3. Derivada
    pid.derivative = (dt > 0.0f) ? (pid.error - pid.lastError) / dt : 0.0f;

    // 4. Saída do PID
    pid.proporcional = pid.kp * pid.error;
    pid.output = pid.proporcional + (pid.ki * pid.integral) + (pid.kd * pid.derivative);
    pid.lastError = pid.error;
}

// Converte a saída (já calculada) de um eixo no ângulo final de UM
// servo -- neutro + offset, direto pro alvo (sem slew rate) e limite
// de hardware. Pode ser chamada mais de uma vez por ciclo com o mesmo
// output (roll: uma vez pra cada aileron), já que não mexe em nenhum
// PID_variables.
void PID::applyServoOutput(ServoParamters& servo, float output, float minLimit, float maxLimit, bool invert) {
    // 5. Cálculo do ângulo alvo baseado na saída do PID
    servo.targetAngle = invert
    ? servo.neutralAngle - output
    : servo.neutralAngle + output;

    // 6. Limite de hardware físico (fim de curso, ServoConfig::SafeRange)
    // -- unico limite que sobra: nao e' restricao de velocidade, e' o
    // que impede o servo de ser mandado pra fora do range mecanico
    // seguro (ver comentario em ServomotorConfig.h sobre o servo que
    // ja quebrou por causa disso).
    servo.setAngle = constrain(servo.targetAngle, minLimit, maxLimit);
}

