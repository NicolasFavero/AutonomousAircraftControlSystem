#include "SdLogger.h"
#include <esp_heap_caps.h>

static bool endsWithCsv(const char* name){
    size_t len = strlen(name);

    return len >= 4 &&
        strcasecmp(name + len - 4, ".csv") == 0;
}
SdLogger::SdLogger(uint8_t csPin): csPin(csPin) {}
bool SdLogger::begin(){

    unsigned long start = millis();

    while (millis() - start < 300) {

        if (sd.begin(csPin, SD_SCK_MHZ(20))) {
            mounted = true;
            return true;
        }

        delay(50);
    }

    return false;
}
bool SdLogger::createFile(){

    if (fileOpen) {
        return true;
    }

    generateFileName();

    if (!openFile()) {
        return false;
    }

    Serial.print("LOG INICIADO: ");
    Serial.println(currentFile);

    return true;
}
bool SdLogger::beginBuffer(size_t capacityBytes){

    if (buffer != nullptr)
        return true; // ja alocado, nao aloca de novo

    // Tenta PSRAM primeiro -- tem muito mais espaço livre que a RAM
    // interna do ESP32-S3 e esse buffer nao precisa ser velocissimo,
    // so segura os dados entre um flush e outro. Se a placa nao tiver
    // PSRAM (ou a alocacao falhar por qualquer motivo), cai pra RAM
    // interna normal: o buffer funciona do mesmo jeito, so com menos
    // margem antes de forçar um flush.
    buffer = static_cast<char*>(
        heap_caps_malloc(capacityBytes, MALLOC_CAP_SPIRAM)
    );

    if (buffer == nullptr)
        buffer = static_cast<char*>(malloc(capacityBytes));

    if (buffer == nullptr) {
        bufferCapacity = 0;
        Serial.println("SD BUFFER: falha ao alocar, voltando a escrita direta");
        return false;
    }

    bufferCapacity = capacityBytes;
    bufferUsed = 0;

    return true;
}
bool SdLogger::bufferLine(const char* line){

    // Sem buffer disponivel (alocacao falhou ou beginBuffer() nunca
    // foi chamado): cai no comportamento antigo, escrevendo direto
    // no cartao a cada linha.
    if (buffer == nullptr)
        return saveLine(line);

    if (!fileOpen)
        return false;

    size_t len = strlen(line);

    // Linha maior que o buffer inteiro: caso patologico, nao ha como
    // acumular -- escreve direto pra nao perder o dado.
    if (len + 1 > bufferCapacity)
        return saveLine(line);

    // +1 do '\n' que separa as linhas dentro do buffer. Se nao
    // couber mais, faz o flush de seguranca antes (essa e a unica
    // escrita real no cartao durante o voo -- acontece a cada
    // bufferCapacity bytes acumulados, bem mais raro que a cada
    // 100ms como era antes).
    if (bufferUsed + len + 1 > bufferCapacity) {

        if (!flushBuffer())
            return false;
    }

    memcpy(buffer + bufferUsed, line, len);
    bufferUsed += len;

    buffer[bufferUsed] = '\n';
    bufferUsed += 1;

    return true;
}
bool SdLogger::flushBuffer(){

    if (buffer == nullptr || bufferUsed == 0)
        return true; // nada acumulado pra gravar

    if (!fileOpen)
        return false;

    bool ok = (file.write(buffer, bufferUsed) == bufferUsed);

    if (!ok && reopenCurrentFile())
        ok = (file.write(buffer, bufferUsed) == bufferUsed);

    if (ok)
        ok = file.sync();

    bufferUsed = 0;

    return ok;
}
bool SdLogger::selfTest(){
    // Teste independente do log de voo -- usa um arquivo proprio
    // (testeSD.txt) em vez do "file"/fileOpen usados por
    // createFile()/saveLine(). Antes o teste reaproveitava
    // saveLine(), que só escreve se ja existir um arquivo de voo
    // aberto (fileOpen==true) -- como isso só acontece quando o
    // voo comeca de verdade, testar o SD antes do voo sempre dava
    // falso negativo mesmo com o cartao funcionando.

    if (!mounted && !sd.begin(csPin, SD_SCK_MHZ(20)))
        return false;

    mounted = true;

    SdFile testFile;

    if (!testFile.open("/testeSD.txt", O_WRITE | O_CREAT | O_TRUNC))
        return false;

    bool ok = testFile.println("Teste de SD OK") > 0;

    testFile.close();

    return ok;
}
bool SdLogger::listFiles(String& json){
    
    json = "[";

    if(!mounted && !sd.begin(csPin, SD_SCK_MHZ(20)))
        return false;

    mounted = true;

    SdFile dir;
    SdFile entry;

    if(!dir.open("/"))
        return false;

    bool first = true;

    while(entry.openNext(&dir, O_RDONLY))
    {
        char name[64];

        entry.getName(name, sizeof(name));

        entry.close();

        if(!endsWithCsv(name))
            continue;

        if(!first)
            json += ",";

        json += "\"";
        json += name;
        json += "\"";

        first = false;
    }

    dir.close();

    json += "]";

    return true;
}
bool SdLogger::removeFile(const char* filename){
    closeFile();

    if(!mounted && !sd.begin(csPin, SD_SCK_MHZ(20)))
        return false;

    mounted = true;

    return sd.remove(filename);
}
bool SdLogger::removeAllLogs(){
    closeFile();

    if(!mounted && !sd.begin(csPin, SD_SCK_MHZ(20)))
        return false;

    mounted = true;

    SdFile dir;
    SdFile entry;

    if(!dir.open("/"))
        return false;

    bool ok = true;

    while(entry.openNext(&dir, O_RDONLY))
    {
        char name[64];

        entry.getName(name, sizeof(name));
        entry.close();

        if(endsWithCsv(name))
        {
            if(!sd.remove(name))
                ok = false;
        }
    }

    dir.close();

    return ok;
}
bool SdLogger::renameFile(const char* oldName, const char* newName){
    closeFile();

    if(!mounted && !sd.begin(csPin, SD_SCK_MHZ(20)))
        return false;

    mounted = true;

    if(!sd.exists(oldName))
        return false;

    if(sd.exists(newName))
        return false;

    return sd.rename(oldName, newName);
}
bool SdLogger::openRead(const char* filename, File32& file){
    return file.open(filename, O_RDONLY);
}
void SdLogger::closeFile(){
    if (!fileOpen) {
        return;
    }

    file.sync();
    file.close();

    syncCounter = 0;
    fileOpen = false;

    Serial.println("LOG FINALIZADO");
}
bool SdLogger::isOpen() const{
    return fileOpen;
}
const char* SdLogger::getFileName() const{
    return currentFile;
}
bool SdLogger::openFile(){

    if (!file.open(
            currentFile,
            O_WRITE | O_CREAT | O_TRUNC)) {
        return false;
    }

    fileOpen = true;
    return true;
}
void SdLogger::generateFileName(){

    for (uint16_t i = 1; i < 10000; i++) {

        snprintf(
            currentFile,
            sizeof(currentFile),
            "/voo%u.csv",
            i
        );

        if (!sd.exists(currentFile)) {
            return;
        }
    }

    strcpy(currentFile, "/voo9999.csv");
}
bool SdLogger::reopenCurrentFile(){

    if (fileOpen) {
        file.close();
    }

    fileOpen = false;

    if (!sd.begin(csPin, SD_SCK_MHZ(20))) {
        return false;
    }

    if (!file.open(
            currentFile,
            O_WRITE | O_APPEND)) {
        return false;
    }

    fileOpen = true;
    return true;
}
bool SdLogger::saveLine(const char* line){

    if (!fileOpen) {
        return false;
    }

    if (!file.println(line)) {

        if (!reopenCurrentFile()) {
            return false;
        }

        if (!file.println(line)) {
            return false;
        }
    }

    syncCounter++;

    if (syncCounter >= SYNC_INTERVAL) {

        if (!file.sync()) {

            if (!reopenCurrentFile()) {
                return false;
            }
        }

        syncCounter = 0;
    }

    return true;
}
void SdLogger::flush(){

    if (!fileOpen) {
        return;
    }

    file.sync();
}
