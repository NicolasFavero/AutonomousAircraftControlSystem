#include "Imu.h"
#include <Arduino.h>
#include "math.h"
//Wire.begin(SDA, SCL)  --> main.cpp

IMU::IMU():imu() {}

bool IMU::begin() {

    imu.begin(Wire, ADDRESS);

    if (imu.status != ICM_20948_Stat_Ok) {
        Serial.println("Erro Critico: ICM-20948 nao respondeu em 0x68");
        return false;
    }

    bool success = true;

    // ATENCAO: nao usamos mais o DMP. Antes, o "10Hz" de saida vinha do
    // proprio initializeDMP() da lib SparkFun, que hard-codeia o divisor
    // de ODR do acelerometro/giroscopio pra 55-56Hz (mySmplrt.g = mySmplrt.a
    // = 19 no codigo-fonte da lib) alem de jogar o magnetometro pra
    // 68.75Hz via I2C_MST_ODR_CONFIG -- nao era so o teto do DMP, era
    // literalmente o driver derrubando a taxa crua tambem. Como agora
    // fazemos a fusao (Mahony) na mao a partir dos registradores crus,
    // configuramos a ODR real nos mesmos, sem depender do que o
    // initializeDMP() faria.
    //
    // imu.begin() ja chamou startupDefault(minimal=true) internamente
    // (porque a lib foi compilada com ICM_20948_USE_DMP definido) -- isso
    // liga acelerometro/giroscopio mas NAO configura full-scale/DLPF/ODR
    // nem o magnetometro (esse startup minimo assume que o DMP vai
    // configurar o resto depois). Sem DMP, fazemos essa parte abaixo.

    success &= (imu.setSampleMode((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr),
                                   ICM_20948_Sample_Mode_Continuous) == ICM_20948_Stat_Ok);

    ICM_20948_fss_t fss;
    fss.a = gpm4;    // +/- 4g
    fss.g = dps2000; // +/- 2000 dps
    success &= (imu.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), fss) == ICM_20948_Stat_Ok);

    ICM_20948_dlpcfg_t dlpcfg;
    dlpcfg.a = acc_d473bw_n499bw;
    dlpcfg.g = gyr_d361bw4_n376bw5;
    success &= (imu.setDLPFcfg((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), dlpcfg) == ICM_20948_Stat_Ok);
    success &= (imu.enableDLPF(ICM_20948_Internal_Acc, false) == ICM_20948_Stat_Ok);
    success &= (imu.enableDLPF(ICM_20948_Internal_Gyr, true) == ICM_20948_Stat_Ok);

    // Divisor 4 = 225Hz para acc/giro (o mesmo valor que ja estava
    // comentado, e nunca usado, dentro do initializeDMP() original).
    ICM_20948_smplrt_t smplrt;
    smplrt.g = 4;
    smplrt.a = 4;
    success &= (imu.setSampleRate((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), smplrt) == ICM_20948_Stat_Ok);

    // Configura o magnetometro "de verdade" (modo continuo 100Hz + leitura
    // automatica via I2C_SLV0/ST1). Isso e pulado no startup minimo porque,
    // originalmente, ficava a cargo do initializeDMP() (que fazia sua
    // propria configuracao "secreta" do mag pro DMP). Sem chamar isso
    // aqui, magX()/Y()/Z() nunca dariam dado novo.
    success &= (imu.startupMagnetometer(false) == ICM_20948_Stat_Ok);

    if (!success) {
        Serial.println("Erro: Falha ao configurar o ICM-20948 para leitura crua (sem DMP)!");
        return false;
    }

    ahrs.reset();

    Serial.println("Filtro Mahony (leitura crua, sem DMP) pronto. Movimente o ICM20948...");

    lastUpdateMicros = micros();
    lastFilterMicros = 0;

    return true;
}

bool IMU::update(){

    if (!imu.dataReady()) {
        // O sensor ainda nao tem amostra crua nova neste ciclo (bit DRDY
        // do proprio ICM-20948, independente de DMP/FIFO) -- com o loop a
        // 200Hz e o acc/giro configurados a 225Hz isso deve ser raro, mas
        // nao e falha do sensor por si so. So declara defeito de verdade
        // se isso persistir por mais que STALE_TIMEOUT_US seguidos sem
        // nenhuma leitura boa.
        if (micros() - lastUpdateMicros > STALE_TIMEOUT_US) {
            sensorOk = false;
        }

        return false;
    }

    imu.getAGMT();

    accX = imu.accX();
    accY = imu.accY();
    accZ = imu.accZ();

    gyroX = imu.gyrX();
    gyroY = imu.gyrY();
    gyroZ = imu.gyrZ();

    magX = imu.magX();
    magY = imu.magY();
    magZ = imu.magZ();

    // dt real entre chamadas do filtro (nao assume 5ms fixos -- o
    // agendamento do FreeRTOS via vTaskDelayUntil tem jitter, e o Mahony
    // e sensivel a dt errado na parte integral/proporcional).
    uint32_t now = micros();
    float dt = (lastFilterMicros == 0) ? (1.0f / 200.0f) : (now - lastFilterMicros) / 1000000.0f;
    lastFilterMicros = now;

    // O magnetometro roda a 100Hz enquanto o loop roda a 200Hz -- em metade
    // dos ciclos magX/Y/Z ainda vai ser o mesmo valor do ciclo anterior.
    // Isso e inofensivo pro Mahony (o peso da correcao magnetica e pequeno
    // frente ao vetor de gravidade), entao passamos direto sem tratamento
    // especial de "amostra nova".
    ahrs.update(dt, gyroX, gyroY, gyroZ, accX, accY, accZ, magX, magY, magZ);

    // NOTA DE MONTAGEM: mantido o mesmo mapeamento de eixos que o codigo
    // baseado em DMP ja usava (roll = -pitch_padrao, pitch = roll_padrao),
    // pra nao mudar o comportamento em voo por causa da orientacao fisica
    // do sensor na aeronave. Se o sensor for remontado, ajuste aqui.
    roll  = -ahrs.getPitch();
    pitch =  ahrs.getRoll();
    yaw   =  ahrs.getYaw();

    lastUpdateMicros = now;
    sensorOk = true;

    return true;
}

bool IMU::isHealthy() const {
    return sensorOk;
}
void IMU::print(){
    Serial.print("ROLL:");
    Serial.print(roll, 1);
    Serial.print(",");

    Serial.print("PITCH:");
    Serial.print(pitch, 1);
    Serial.print(",");

    Serial.print("YAW:");
    Serial.println(yaw, 1);
}
float IMU::getPitch() const {return pitch;}
float IMU::getRoll() const {return roll;}
float IMU::getYaw() const {return yaw;}

float IMU::getAccX() const {return accX;}
float IMU::getAccY() const {return accY;}
float IMU::getAccZ() const {return accZ;}

float IMU::getGyroX() const {return gyroX;}
float IMU::getGyroY() const {return gyroY;}
float IMU::getGyroZ() const {return gyroZ;}

float IMU::getMagX() const {return magX;}
float IMU::getMagY() const {return magY;}
float IMU::getMagZ() const {return magZ;}