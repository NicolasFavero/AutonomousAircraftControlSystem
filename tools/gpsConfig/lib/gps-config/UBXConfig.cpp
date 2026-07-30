#include "UBXConfig.h"
#include "UBXCommands.h"

UBXConfig::UBXConfig(UBXInterface& ubx)
    : ubx(ubx)
{
}

bool UBXConfig::isAck(const UBXInterface::Packet& pkt)
{
    return pkt.received &&
           pkt.validChecksum &&
           pkt.cls == 0x05 &&
           pkt.id  == 0x01;
}

void UBXConfig::copyOut(const uint8_t* packet,
                        size_t len,
                        uint8_t* outPacket,
                        size_t* outLen)
{
    if (!outPacket || !outLen)
        return;

    memcpy(outPacket, packet, len);
    *outLen = len;
}

size_t UBXConfig::buildPacket(uint8_t cls,
                              uint8_t id,
                              const uint8_t* payload,
                              uint16_t payloadLen,
                              uint8_t* out,
                              size_t outCapacity)
{
    size_t total = 8 + (size_t)payloadLen; // sync(2)+cls+id+len(2)+payload+ck(2)

    if (outCapacity < total)
        return 0;

    out[0] = 0xB5;
    out[1] = 0x62;
    out[2] = cls;
    out[3] = id;
    out[4] = payloadLen & 0xFF;
    out[5] = (payloadLen >> 8) & 0xFF;

    memcpy(out + 6, payload, payloadLen);

    uint8_t ckA, ckB;

    UBXInterface::ubxChecksum(out + 2, 4 + payloadLen, ckA, ckB);

    out[6 + payloadLen] = ckA;
    out[7 + payloadLen] = ckB;

    return total;
}

bool UBXConfig::setBaudrate(uint32_t newBaud, uint8_t* outPacket, size_t* outLen)
{
    UBXInterface::Packet resp;

    if (!ubx.request(UBX::CFG_PRT, sizeof(UBX::CFG_PRT), resp))
        return false;

    if (!resp.validChecksum || resp.length < 20)
        return false;

    uint8_t payload[20];
    memcpy(payload, resp.payload, sizeof(payload));

    payload[8]  = newBaud & 0xFF;
    payload[9]  = (newBaud >> 8) & 0xFF;
    payload[10] = (newBaud >> 16) & 0xFF;
    payload[11] = (newBaud >> 24) & 0xFF;

    uint8_t packet[32];
    size_t len = buildPacket(0x06, 0x00, payload, sizeof(payload), packet, sizeof(packet));

    if (len == 0)
        return false;

    // Fire-and-forget -- ver comentario em UBXConfig.h/UBXInterface::send().
    ubx.send(packet, len);

    copyOut(packet, len, outPacket, outLen);

    return true;
}

bool UBXConfig::setUpdateRate(uint16_t measurementRateMs, uint8_t* outPacket, size_t* outLen)
{
    uint8_t payload[6] = {
        (uint8_t)(measurementRateMs & 0xFF),
        (uint8_t)(measurementRateMs >> 8),
        0x01, 0x00, // navRate = 1
        0x01, 0x00  // timeRef = 1 (GPS time)
    };

    uint8_t packet[16];
    size_t len = buildPacket(0x06, 0x08, payload, sizeof(payload), packet, sizeof(packet));

    if (len == 0)
        return false;

    UBXInterface::Packet resp;

    if (!ubx.request(packet, len, resp))
        return false;

    bool ok = isAck(resp);

    if (ok)
        copyOut(packet, len, outPacket, outLen);

    return ok;
}

bool UBXConfig::setDynamicModel(uint8_t model, uint8_t* outPacket, size_t* outLen)
{
    uint8_t payload[36] = {0};

    payload[0] = 0x01; // mask = 0x0001 -> aplica so o campo dynModel
    payload[1] = 0x00;
    payload[2] = model;

    uint8_t packet[48];
    size_t len = buildPacket(0x06, 0x24, payload, sizeof(payload), packet, sizeof(packet));

    if (len == 0)
        return false;

    UBXInterface::Packet resp;

    if (!ubx.request(packet, len, resp))
        return false;

    bool ok = isAck(resp);

    if (ok)
        copyOut(packet, len, outPacket, outLen);

    return ok;
}

bool UBXConfig::setFixMode(uint8_t mode, uint8_t* outPacket, size_t* outLen)
{
    uint8_t payload[36] = {0};

    payload[0] = 0x04; // mask = 0x0004 -> aplica so o campo fixMode
    payload[1] = 0x00;
    payload[3] = mode;

    uint8_t packet[48];
    size_t len = buildPacket(0x06, 0x24, payload, sizeof(payload), packet, sizeof(packet));

    if (len == 0)
        return false;

    UBXInterface::Packet resp;

    if (!ubx.request(packet, len, resp))
        return false;

    bool ok = isAck(resp);

    if (ok)
        copyOut(packet, len, outPacket, outLen);

    return ok;
}

UBXConfig::GnssSetResult UBXConfig::setGnss(uint8_t gnssId, bool enable, uint8_t* outPacket, size_t* outLen)
{
    UBXInterface::Packet resp;

    if (!ubx.request(UBX::CFG_GNSS, sizeof(UBX::CFG_GNSS), resp))
        return GnssSetResult::NoResponse;

    if (!resp.validChecksum || resp.length < 4)
        return GnssSetResult::NoResponse;

    uint8_t payload[512];
    uint16_t len = resp.length;

    memcpy(payload, resp.payload, len);

    uint8_t blocks = payload[3];
    bool found = false;

    for (uint8_t i = 0; i < blocks; i++)
    {
        uint16_t pos = 4 + i * 8;

        if (pos + 8 > len)
            break;

        if (payload[pos] != gnssId)
            continue;

        uint32_t flags =
            payload[pos+4] |
            (payload[pos+5] << 8) |
            (payload[pos+6] << 16) |
            ((uint32_t)payload[pos+7] << 24);

        if (enable)
            flags |= 0x01;
        else
            flags &= ~((uint32_t)0x01);

        payload[pos+4] = flags & 0xFF;
        payload[pos+5] = (flags >> 8) & 0xFF;
        payload[pos+6] = (flags >> 16) & 0xFF;
        payload[pos+7] = (flags >> 24) & 0xFF;

        found = true;
        break;
    }

    if (!found)
        return GnssSetResult::NotSupported;

    uint8_t packet[600];
    size_t plen = buildPacket(0x06, 0x3E, payload, len, packet, sizeof(packet));

    if (plen == 0)
        return GnssSetResult::NoResponse;

    UBXInterface::Packet ackResp;

    if (!ubx.request(packet, plen, ackResp))
        return GnssSetResult::NoResponse;

    GnssSetResult result = isAck(ackResp) ? GnssSetResult::Ack : GnssSetResult::Nack;

    if (result == GnssSetResult::Ack)
        copyOut(packet, plen, outPacket, outLen);

    return result;
}

bool UBXConfig::saveConfig()
{
    uint8_t payload[13] = {
        0x00,0x00,0x00,0x00, // clearMask
        0x1F,0x06,0x00,0x00, // saveMask (ioPort,msgConf,infMsg,navConf,rxmConf,antConf)
        0x00,0x00,0x00,0x00, // loadMask
        0x17                 // deviceMask (BBR + Flash + EEPROM)
    };

    uint8_t packet[24];
    size_t len = buildPacket(0x06, 0x09, payload, sizeof(payload), packet, sizeof(packet));

    if (len == 0)
        return false;

    UBXInterface::Packet resp;

    if (!ubx.request(packet, len, resp))
        return false;

    return isAck(resp);
}
