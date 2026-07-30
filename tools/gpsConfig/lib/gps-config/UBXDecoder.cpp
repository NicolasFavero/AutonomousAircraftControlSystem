#include "UBXDecoder.h"

static uint16_t U2(const uint8_t* p)
{
    return p[0] | (p[1] << 8);
}

static uint32_t U4(const uint8_t* p)
{
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int32_t I4(const uint8_t* p)
{
    return (int32_t)U4(p);
}

static const char* pvtFixName(uint8_t fixType)
{
    switch (fixType)
    {
        case 1: return "DR";
        case 2: return "2D";
        case 3: return "3D";
        case 4: return "3D+DR";
        case 5: return "TimeOnly";
        default: return "SemFix";
    }
}

void UBXDecoder::decode(const UBXInterface::Packet& pkt)
{
    Serial.println();
    Serial.println("====================================");

    if (!pkt.validChecksum)
    {
        Serial.println("Checksum invalido");
        return;
    }

    switch ((pkt.cls << 8) | pkt.id)
    {
        case 0x0A04: decodeMONVER(pkt); break;
        case 0x0A09: decodeMONHW(pkt); break;
        case 0x0A0B: decodeMONHW2(pkt); break;

        case 0x0600: decodeCFGPRT(pkt); break;
        case 0x0608: decodeCFGRATE(pkt); break;
        case 0x0624: decodeCFGNAV5(pkt); break;
        case 0x063E: decodeCFGGNSS(pkt); break;
        case 0x0609: decodeCFGCFG(pkt); break;

        case 0x0501:
            Serial.println("ACK");
            break;

        case 0x0500:
            Serial.println("NACK");
            break;

        case 0x0107: decodeNAVPVT(pkt); break;

        default:
            Serial.printf("Classe %02X  ID %02X\n",pkt.cls,pkt.id);
    }

    Serial.println("====================================");
}

void UBXDecoder::decodeMONVER(const UBXInterface::Packet& pkt)
{
    Serial.println("MON-VER");

    Serial.println();

    for(uint16_t i=0;i<pkt.length;i++)
    {
        char c=(char)pkt.payload[i];

        if(c>=32 && c<=126)
            Serial.write(c);
        else
            Serial.println();
    }
}

void UBXDecoder::decodeMONHW(const UBXInterface::Packet& pkt)
{
    Serial.println("MON-HW");

    Serial.print("Payload: ");
    Serial.print(pkt.length);
    Serial.println(" bytes");

    if(pkt.length<60)
        return;

    uint32_t noisePerMS =
            pkt.payload[16] |
            (pkt.payload[17]<<8) |
            (pkt.payload[18]<<16) |
            (pkt.payload[19]<<24);

    Serial.print("Noise: ");
    Serial.println(noisePerMS);

    Serial.print("AGC: ");
    Serial.println(U2(pkt.payload+20));
}

void UBXDecoder::decodeMONHW2(const UBXInterface::Packet& pkt)
{
    Serial.println("MON-HW2");

    Serial.print("Payload: ");
    Serial.print(pkt.length);
    Serial.println(" bytes");
}

void UBXDecoder::decodeCFGPRT(const UBXInterface::Packet& pkt)
{
    Serial.println("CFG-PRT");

    if(pkt.length<20)
        return;

    uint32_t baud =
            pkt.payload[8] |
            (pkt.payload[9]<<8) |
            (pkt.payload[10]<<16) |
            (pkt.payload[11]<<24);

    Serial.print("Baudrate: ");
    Serial.println(baud);

    uint16_t inMask=U2(pkt.payload+12);
    uint16_t outMask=U2(pkt.payload+14);

    Serial.print("Input Mask : 0x");
    Serial.println(inMask,HEX);

    Serial.print("Output Mask: 0x");
    Serial.println(outMask,HEX);
}

void UBXDecoder::decodeCFGRATE(const UBXInterface::Packet& pkt)
{
    Serial.println("CFG-RATE");

    if(pkt.length<6)
        return;

    uint16_t measRate=U2(pkt.payload);

    Serial.print("Measurement Rate : ");
    Serial.print(measRate);
    Serial.println(" ms");

    Serial.print("Update Rate      : ");

    if(measRate)
        Serial.print(1000.0f/measRate);

    Serial.println(" Hz");
}

const char* UBXDecoder::dynamicModel(uint8_t model)
{
    switch(model)
    {
        case 0:return "Portable";
        case 2:return "Stationary";
        case 3:return "Pedestrian";
        case 4:return "Automotive";
        case 5:return "Sea";
        case 6:return "Airborne 1g";
        case 7:return "Airborne 2g";
        case 8:return "Airborne 4g";
        default:return "Unknown";
    }
}

void UBXDecoder::decodeCFGNAV5(const UBXInterface::Packet& pkt)
{
    Serial.println("CFG-NAV5");

    if(pkt.length<36)
        return;

    Serial.print("Dynamic Model : ");
    Serial.println(dynamicModel(pkt.payload[2]));

    Serial.print("Fix Mode      : ");

    switch(pkt.payload[3])
    {
        case 1: Serial.println("2D"); break;
        case 2: Serial.println("3D"); break;
        case 3: Serial.println("Auto"); break;
        default: Serial.println("Unknown");
    }
}

void UBXDecoder::decodeCFGGNSS(const UBXInterface::Packet& pkt)
{
    Serial.println("CFG-GNSS");

    if(pkt.length<4)
        return;

    uint8_t blocks=pkt.payload[3];

    Serial.print("GNSS Blocks: ");
    Serial.println(blocks);

    uint16_t pos=4;

    for(uint8_t i=0;i<blocks;i++)
    {
        if(pos+8>pkt.length)
            break;

        uint8_t id=pkt.payload[pos];

        uint32_t flags=
            pkt.payload[pos+4]|
            (pkt.payload[pos+5]<<8)|
            (pkt.payload[pos+6]<<16)|
            (pkt.payload[pos+7]<<24);

        bool enabled=flags&1;

        const char* name="Unknown";

        switch(id)
        {
            case 0:name="GPS";break;
            case 1:name="SBAS";break;
            case 2:name="Galileo";break;
            case 3:name="BeiDou";break;
            case 4:name="IMES";break;
            case 5:name="QZSS";break;
            case 6:name="GLONASS";break;
        }

        Serial.printf("%-10s : %s\n",
                      name,
                      enabled?"ON":"OFF");

        pos+=8;
    }
}

void UBXDecoder::decodeCFGCFG(const UBXInterface::Packet&)
{
    Serial.println("CFG-CFG");
    Serial.println("Configuracao salva na memoria.");
}

void UBXDecoder::decodeNAVPVT(const UBXInterface::Packet& pkt)
{
    Serial.println("NAV-PVT");

    if (pkt.length < 78)
    {
        Serial.println("Payload curto demais (poucos bytes recebidos).");
        return;
    }

    uint8_t fixType = pkt.payload[20];
    uint8_t numSV   = pkt.payload[23];

    int32_t lon  = I4(pkt.payload + 24);
    int32_t lat  = I4(pkt.payload + 28);
    int32_t hMSL = I4(pkt.payload + 36);
    uint32_t hAcc = U4(pkt.payload + 40);
    uint16_t pDOP = U2(pkt.payload + 76);

    Serial.print("Fix Type........... "); Serial.println(pvtFixName(fixType));
    Serial.print("Satelites usados... "); Serial.println(numSV);
    Serial.print("Latitude........... "); Serial.println(lat / 1e7, 7);
    Serial.print("Longitude.......... "); Serial.println(lon / 1e7, 7);
    Serial.print("Altitude (MSL)..... "); Serial.print(hMSL / 1000.0f, 1); Serial.println(" m");
    Serial.print("Precisao horiz..... "); Serial.print(hAcc / 1000.0f, 1); Serial.println(" m");
    Serial.print("PDOP............... "); Serial.println(pDOP / 100.0f, 2);
}

// Uma linha so, sem cabecalho/rodape -- pensada pro modo de
// monitoramento continuo (ex.: pra julgar se vale a pena aumentar a
// taxa de atualizacao), onde imprimir o bloco inteiro a cada fix
// deixaria o serial ilegivel/pesado.
void UBXDecoder::printPVTLine(const UBXInterface::Packet& pkt)
{
    if (!pkt.validChecksum || pkt.length < 78)
    {
        Serial.println("(sem fix / resposta incompleta)");
        return;
    }

    uint8_t fixType = pkt.payload[20];
    uint8_t numSV   = pkt.payload[23];

    int32_t lon  = I4(pkt.payload + 24);
    int32_t lat  = I4(pkt.payload + 28);
    int32_t hMSL = I4(pkt.payload + 36);
    uint32_t hAcc = U4(pkt.payload + 40);
    uint16_t pDOP = U2(pkt.payload + 76);

    Serial.print(pvtFixName(fixType));
    Serial.print("  Sats=");
    Serial.print(numSV);
    Serial.print("  Lat=");
    Serial.print(lat / 1e7, 7);
    Serial.print("  Lon=");
    Serial.print(lon / 1e7, 7);
    Serial.print("  Alt=");
    Serial.print(hMSL / 1000.0f, 1);
    Serial.print("m  HAcc=");
    Serial.print(hAcc / 1000.0f, 1);
    Serial.print("m  PDOP=");
    Serial.println(pDOP / 100.0f, 2);
}