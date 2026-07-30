#include "UBXDatabase.h"

UBXDatabase::UBXDatabase(GPSInfo& gps)
    : info(gps)
{
}

uint16_t UBXDatabase::U2(const uint8_t* p)
{
    return p[0] | (p[1] << 8);
}

uint32_t UBXDatabase::U4(const uint8_t* p)
{
    return
        (uint32_t)p[0] |
        ((uint32_t)p[1] << 8) |
        ((uint32_t)p[2] << 16) |
        ((uint32_t)p[3] << 24);
}

void UBXDatabase::update(const UBXInterface::Packet& pkt)
{
    if(!pkt.validChecksum)
        return;

    switch((pkt.cls<<8)|pkt.id)
    {
        case 0x0A04:
            parseMONVER(pkt);
            break;

        case 0x0A09:
            parseMONHW(pkt);
            break;

        case 0x0A0B:
            parseMONHW2(pkt);
            break;

        case 0x0600:
            parseCFGPRT(pkt);
            break;

        case 0x0608:
            parseCFGRATE(pkt);
            break;

        case 0x0624:
            parseCFGNAV5(pkt);
            break;

        case 0x063E:
            parseCFGGNSS(pkt);
            break;

        default:
            break;
    }
}

const char* UBXDatabase::dynamicModel(uint8_t model)
{
    switch(model)
    {
        case 0: return "Portable";
        case 2: return "Stationary";
        case 3: return "Pedestrian";
        case 4: return "Automotive";
        case 5: return "Sea";
        case 6: return "Airborne 1g";
        case 7: return "Airborne 2g";
        case 8: return "Airborne 4g";
        default:return "Unknown";
    }
}

const char* UBXDatabase::fixMode(uint8_t mode)
{
    switch(mode)
    {
        case 1:return "2D";
        case 2:return "3D";
        case 3:return "Auto";
        default:return "Unknown";
    }
}

void UBXDatabase::parseMONVER(const UBXInterface::Packet& pkt)
{
    info.hasMONVER = true;

    String texto;

    for(uint16_t i=0;i<pkt.length;i++)
    {
        char c=(char)pkt.payload[i];

        if(c>=32 && c<=126)
            texto += c;
        else
            texto += '\n';
    }

    int p = texto.indexOf("PROTVER");

    if(p>=0)
    {
        int e = texto.indexOf('\n',p);

        info.firmware = texto.substring(p,e);
    }

    if(texto.indexOf("00070000")>=0)
        info.model="u-blox 7 / Clone";

    if(texto.indexOf("00080000")>=0)
        info.model="u-blox M8";

    if(texto.indexOf("00090000")>=0)
        info.model="u-blox M9";

    if(texto.indexOf("00100000")>=0)
        info.model="u-blox M10";
}

void UBXDatabase::parseMONHW(const UBXInterface::Packet& pkt)
{
    info.hasMONHW = true;

    if(pkt.length<60)
        return;

    info.noise = U4(pkt.payload+16);

    info.agc = U2(pkt.payload+20);

    info.hardware="MON-HW disponível";
}

void UBXDatabase::parseMONHW2(const UBXInterface::Packet& pkt)
{
    info.hasMONHW2 = true;

    info.hardware="MON-HW2 disponível";
}

void UBXDatabase::parseCFGPRT(const UBXInterface::Packet& pkt)
{
    info.hasCFGPRT = true;

    if(pkt.length<20)
        return;

    info.baudrate = U4(pkt.payload+8);
}

void UBXDatabase::parseCFGRATE(const UBXInterface::Packet& pkt)
{
    info.hasCFGRATE = true;

    if(pkt.length<6)
        return;

    uint16_t measRate = U2(pkt.payload);

    info.measurementRate = measRate;
    info.updateRateHz = measRate ? (1000.0f/measRate) : 0.0f;
}

void UBXDatabase::parseCFGNAV5(const UBXInterface::Packet& pkt)
{
    info.hasCFGNAV5 = true;

    if(pkt.length<36)
        return;

    info.dynamicModel = dynamicModel(pkt.payload[2]);
    info.fixMode = fixMode(pkt.payload[3]);
}

void UBXDatabase::parseCFGGNSS(const UBXInterface::Packet& pkt)
{
    info.hasCFGGNSS = true;

    if(pkt.length<4)
        return;

    uint8_t blocks = pkt.payload[3];
    uint16_t pos = 4;

    for(uint8_t i=0; i<blocks; i++)
    {
        if(pos+8 > pkt.length)
            break;

        uint8_t id = pkt.payload[pos];

        uint32_t flags = U4(pkt.payload+pos+4);

        bool enabled = flags & 1;

        switch(id)
        {
            case 0: info.gps     = enabled; break;
            case 1: info.sbas    = enabled; break;
            case 2: info.galileo = enabled; break;
            case 3: info.beidou  = enabled; break;
            case 4: info.imes    = enabled; break;
            case 5: info.qzss    = enabled; break;
            case 6: info.glonass = enabled; break;
        }

        pos += 8;
    }
}