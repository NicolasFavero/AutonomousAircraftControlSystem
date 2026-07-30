#include "UBXExporter.h"

void UBXExporter::clear()
{
    entryCount = 0;
}

bool UBXExporter::record(const char* description,
                         const uint8_t* packet,
                         size_t length,
                         bool isBaudrateChange,
                         uint32_t newBaud)
{
    if (entryCount >= MAX_ENTRIES)
        return false;

    if (length > UBX_CONFIG_MAX_PACKET)
        return false;

    Entry& e = entries[entryCount];

    e.description = description;
    memcpy(e.packet, packet, length);
    e.length = length;
    e.isBaudrateChange = isBaudrateChange;
    e.newBaud = newBaud;

    entryCount++;

    return true;
}

String UBXExporter::makeIdentifier(uint8_t index, const String& description)
{
    String id = "CMD_";

    if (index < 10)
        id += "0";

    id += String(index);
    id += "_";

    for (size_t i = 0; i < description.length(); i++)
    {
        char c = description[i];

        bool alnum = (c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') ||
                     (c >= '0' && c <= '9');

        id += alnum ? (char)toupper(c) : '_';
    }

    return id;
}

void UBXExporter::printExport() const
{
    Serial.println();
    Serial.println("======================================================");

    if (entryCount == 0)
    {
        Serial.println("Nada para exportar ainda.");
        Serial.println();
        Serial.println("Use as opcoes B/F/M/X/G para testar e aplicar uma");
        Serial.println("configuracao no receptor primeiro -- so entram no");
        Serial.println("export os comandos que o receptor confirmou.");
        Serial.println("======================================================");
        return;
    }

    Serial.println("// ---- COLE O BLOCO ABAIXO EM gpsSettings.h ----");
    Serial.println("// Gerado pela ferramenta de diagnostico (opcao E).");
    Serial.print("// ");
    Serial.print(entryCount);
    Serial.println(" comando(s) aplicado(s) com sucesso nesta sessao.");
    Serial.println();

    // Um array de bytes por comando aplicado, na ordem em que foi
    // aplicado -- a ordem importa (ex.: baudrate no meio da lista).
    for (uint8_t i = 0; i < entryCount; i++)
    {
        const Entry& e = entries[i];

        String id = makeIdentifier(i, e.description);

        Serial.print("// ");
        Serial.println(e.description);
        Serial.print("inline constexpr uint8_t ");
        Serial.print(id);
        Serial.print("[] = {");

        for (size_t b = 0; b < e.length; b++)
        {
            if (b > 0)
                Serial.print(",");

            Serial.print("0x");

            if (e.packet[b] < 0x10)
                Serial.print("0");

            Serial.print(e.packet[b], HEX);
        }

        Serial.println("};");
        Serial.println();
    }

    // Tabela final que o gpsInit.cpp percorre em ordem no boot.
    Serial.println("inline constexpr Command Commands[] = {");

    for (uint8_t i = 0; i < entryCount; i++)
    {
        const Entry& e = entries[i];

        String id = makeIdentifier(i, e.description);

        Serial.print("    { \"");
        Serial.print(e.description);
        Serial.print("\", ");
        Serial.print(id);
        Serial.print(", sizeof(");
        Serial.print(id);
        Serial.print("), ");
        Serial.print(e.isBaudrateChange ? "true" : "false");
        Serial.print(", ");
        Serial.print(e.newBaud);
        Serial.println("u },");
    }

    Serial.println("};");
    Serial.println();
    Serial.println("constexpr size_t COMMAND_COUNT = sizeof(Commands)/sizeof(Commands[0]);");
    Serial.println();
    Serial.println("// ---- FIM DO BLOCO ----");
    Serial.println("======================================================");
}
