#include "GPSInfo.h"

void GPSInfo::clear()
{
    firmware = "";
    hardware = "";
    model = "";

    baudrate = 0;

    updateRateHz = 0;
    measurementRate = 0;

    dynamicModel = "";
    fixMode = "";

    gps = false;
    sbas = false;
    galileo = false;
    beidou = false;
    qzss = false;
    glonass = false;
    imes = false;

    agc = 0;
    noise = 0;

    hasMONVER = false;
    hasMONHW = false;
    hasMONHW2 = false;
    hasCFGPRT = false;
    hasCFGRATE = false;
    hasCFGNAV5 = false;
    hasCFGGNSS = false;
}

void GPSInfo::printSummary() const
{
    Serial.println();
    Serial.println("======================================");
    Serial.println("           GPS SUMMARY");
    Serial.println("======================================");

    Serial.print("Modelo............. ");
    Serial.println(model.length() ? model : "Desconhecido");

    Serial.print("Firmware........... ");
    Serial.println(firmware.length() ? firmware : "Desconhecido");

    Serial.print("Hardware........... ");
    Serial.println(hardware.length() ? hardware : "Desconhecido");

    Serial.println();

    Serial.print("Baud............... ");
    Serial.println(baudrate);

    Serial.print("Measurement........ ");
    Serial.print(measurementRate);
    Serial.println(" ms");

    Serial.print("Update Rate........ ");
    Serial.print(updateRateHz,1);
    Serial.println(" Hz");

    Serial.print("Dynamic Model...... ");
    Serial.println(dynamicModel);

    Serial.print("Fix Mode........... ");
    Serial.println(fixMode);

    Serial.println();

    Serial.println("GNSS");

    Serial.printf("GPS................. %s\n",gps?"ON":"OFF");
    Serial.printf("SBAS................ %s\n",sbas?"ON":"OFF");
    Serial.printf("GLONASS............. %s\n",glonass?"ON":"OFF");
    Serial.printf("Galileo............. %s\n",galileo?"ON":"OFF");
    Serial.printf("BeiDou.............. %s\n",beidou?"ON":"OFF");
    Serial.printf("QZSS................ %s\n",qzss?"ON":"OFF");
    Serial.printf("IMES................ %s\n",imes?"ON":"OFF");

    Serial.println();

    Serial.print("Noise............... ");
    Serial.println(noise);

    Serial.print("AGC................. ");
    Serial.println(agc);

    Serial.println();

    Serial.println("Consultas realizadas");

    Serial.printf("MON-VER   : %s\n",hasMONVER?"OK":"--");
    Serial.printf("MON-HW    : %s\n",hasMONHW?"OK":"--");
    Serial.printf("MON-HW2   : %s\n",hasMONHW2?"OK":"--");
    Serial.printf("CFG-PRT   : %s\n",hasCFGPRT?"OK":"--");
    Serial.printf("CFG-RATE  : %s\n",hasCFGRATE?"OK":"--");
    Serial.printf("CFG-NAV5  : %s\n",hasCFGNAV5?"OK":"--");
    Serial.printf("CFG-GNSS  : %s\n",hasCFGGNSS?"OK":"--");

    Serial.println();

    Serial.println("Diagnostico");

    if(firmware.indexOf("14")>=0)
        Serial.println("Firmware semelhante ao u-blox 7.");

    if(galileo)
        Serial.println("Suporte a Galileo detectado.");

    if(beidou)
        Serial.println("Suporte a BeiDou detectado.");

    if(hasCFGGNSS)
        Serial.println("Implementa CFG-GNSS.");

    if(hasMONHW2)
        Serial.println("Implementa MON-HW2.");

    if(firmware.indexOf("14")>=0 && hasCFGGNSS && hasMONHW2)
    {
        Serial.println();
        Serial.println("*** CASO INCOMUM ***");
        Serial.println("Firmware PROTVER 14,");
        Serial.println("mas implementa recursos");
        Serial.println("mais modernos.");
        Serial.println("Pode ser clone ou firmware");
        Serial.println("personalizado.");
    }

    Serial.println("======================================");
}