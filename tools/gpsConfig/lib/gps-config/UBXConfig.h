#pragma once

#include <Arduino.h>
#include "UBX.h"

// Responsavel por MONTAR e ENVIAR os comandos UBX que alteram a
// configuracao do receptor (baudrate, taxa de atualizacao, dynamic
// model, constelacoes GNSS habilitadas, e salvar tudo na memoria).
//
// UBXInterface so sabe enviar/receber bytes. UBXDatabase/UBXDecoder
// so sabem interpretar pacotes ja recebidos. UBXConfig e quem
// conhece o formato dos payloads de SET de cada comando -- assim
// nem UBXInterface nem o main.cpp precisam saber como um pacote
// CFG-NAV5 ou CFG-GNSS e montado por dentro.
//
// Tamanho maximo de pacote UBX que qualquer metodo aqui pode montar
// (usado tambem pelo UBXExporter pra dimensionar o buffer de cada
// entrada exportada). CFG-GNSS e o maior -- header(8) + payload
// (ate ~512 bytes na pratica, ver UBXInterface::Packet::payload).
inline constexpr size_t UBX_CONFIG_MAX_PACKET = 520;

class UBXConfig
{
public:

    explicit UBXConfig(UBXInterface& ubx);

    // Le a porta atual (poll CFG-PRT), troca so o campo do baudrate
    // e reenvia. Nao espera ACK: o receptor pode passar a falar no
    // baud novo antes de qualquer resposta chegar, entao nao ha como
    // ler uma resposta sincrona numa porta ainda configurada pro
    // baud antigo. Depois de chamar isso, quem chamou precisa
    // reconfigurar a UART local pro baud novo antes de conversar de
    // novo com o receptor.
    //
    // outPacket/outLen (opcionais): se nao-nulos, recebem uma copia
    // do pacote UBX completo que foi enviado -- usado por quem
    // chama pra registrar o comando no UBXExporter. outPacket deve
    // ter pelo menos UBX_CONFIG_MAX_PACKET bytes.
    bool setBaudrate(uint32_t newBaud,
                     uint8_t* outPacket = nullptr,
                     size_t* outLen = nullptr);

    // measurementRateMs: intervalo entre solucoes de navegacao (ex.:
    // 100 = 10Hz, 1000 = 1Hz). navRate fica fixo em 1 (uma solucao
    // por medicao) e timeRef em 1 (tempo GPS), que e o padrao usual.
    bool setUpdateRate(uint16_t measurementRateMs,
                       uint8_t* outPacket = nullptr,
                       size_t* outLen = nullptr);

    // model: mesmos codigos usados em UBXDatabase::dynamicModel()
    // (0=Portable, 4=Automotive, 6=Airborne 1g, etc). So o campo
    // dynModel e alterado (mask=0x0001): o resto da configuracao de
    // navegacao do receptor fica intocado.
    bool setDynamicModel(uint8_t model,
                        uint8_t* outPacket = nullptr,
                        size_t* outLen = nullptr);

    // mode: 1=2D only, 2=3D only, 3=Auto 2D/3D (mesmos codigos do
    // campo fixMode de CFG-NAV5). So o campo fixMode e alterado
    // (mask=0x0004): dynModel e o resto da configuracao ficam
    // intocados. Util pra aeromodelo: forcar 3D only evita que o
    // receptor entregue uma altitude "de mentirinha" quando so
    // consegue fixar em 2D (poucos satelites visiveis).
    bool setFixMode(uint8_t mode,
                    uint8_t* outPacket = nullptr,
                    size_t* outLen = nullptr);

    // gnssId: 0=GPS,1=SBAS,2=Galileo,3=BeiDou,4=IMES,5=QZSS,6=GLONASS
    // (mesma numeracao usada em UBXDatabase/UBXDecoder). Le a
    // configuracao atual (poll CFG-GNSS), so vira o bit de habilitado
    // do bloco correspondente e reenvia -- os outros blocos ficam
    // intocados.
    enum class GnssSetResult
    {
        Ack,          // receptor aceitou a mudanca
        Nack,         // bloco existe, mas o receptor recusou (NACK) --
                      // tipico de constelacoes mutuamente exclusivas
                      // nesse hardware (ex.: GLONASS x GPS em alguns u-blox 7)
        NotSupported, // nao ha bloco dessa constelacao no CFG-GNSS
                      // desse receptor -- hardware nao implementa
        NoResponse    // falha de comunicacao (poll ou envio sem resposta)
    };

    // outPacket/outLen recebem o pacote CFG-GNSS completo (com todos
    // os blocos, so o bit alterado) -- e o pacote que deve ser
    // reenviado no boot pra reproduzir esse estado, entao so faz
    // sentido registrar no exportador quando o resultado for Ack.
    GnssSetResult setGnss(uint8_t gnssId,
                          bool enable,
                          uint8_t* outPacket = nullptr,
                          size_t* outLen = nullptr);

    // Grava a configuracao atual (RAM) na memoria nao-volatil do
    // receptor (BBR + Flash + EEPROM, quando existirem). Sem isso,
    // qualquer alteracao se perde ao desligar.
    //
    // Nao faz sentido exportar este comando pro firmware oficial: o
    // objetivo do modulo exportado e justamente reaplicar a
    // configuracao a cada boot, sem depender da NVRAM do receptor.
    bool saveConfig();

private:

    UBXInterface& ubx;

    static bool isAck(const UBXInterface::Packet& pkt);

    // Monta um pacote UBX completo (sync+cls+id+len+payload+checksum)
    // em 'out'. Retorna o tamanho total ou 0 se nao coube em outCapacity.
    static size_t buildPacket(uint8_t cls,
                              uint8_t id,
                              const uint8_t* payload,
                              uint16_t payloadLen,
                              uint8_t* out,
                              size_t outCapacity);

    // Copia 'packet'/'len' pra outPacket/outLen se ambos foram
    // fornecidos (nao-nulos). Helper comum a todos os metodos acima.
    static void copyOut(const uint8_t* packet,
                        size_t len,
                        uint8_t* outPacket,
                        size_t* outLen);
};
