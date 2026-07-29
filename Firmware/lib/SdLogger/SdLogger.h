#pragma once

#include <Arduino.h>
#include <SdFat.h>

class SdLogger {
public:
    explicit SdLogger(uint8_t csPin);

    bool begin();

    bool createFile();

    // Buffer em RAM/PSRAM: usado durante COUNTDOWN/FLIGHT pra nao
    // bloquear o loop de controle com uma escrita no SD a cada
    // leitura (100ms). As linhas se acumulam em memoria e so viram
    // escrita de verdade no cartao quando o buffer enche (flush de
    // seguranca automatico) ou quando flushBuffer() e chamado
    // explicitamente (ex: ao finalizar o voo). Ver comentario em
    // beginBuffer() sobre o trade-off de perda de dados em caso de
    // queda de energia no meio do voo.
    bool beginBuffer(size_t capacityBytes = 64 * 1024);
    bool bufferLine(const char* line);
    bool flushBuffer();

    bool selfTest();

    bool listFiles(String& json);
    bool removeFile(const char* filename);
    bool removeAllLogs();
    bool renameFile(const char* oldName, const char* newName);
    bool openRead(const char* filename, File32& file);

    void closeFile();

    bool isOpen() const;
    const char* getFileName() const;


private:
    SdFat sd;
    SdFile file;

    uint8_t csPin = 0;

    bool fileOpen = false;

    bool mounted = false;

    uint16_t syncCounter = 0;

    char currentFile[16] = "";

    // Buffer em RAM/PSRAM (ver beginBuffer()/bufferLine()/flushBuffer()).
    char* buffer = nullptr;
    size_t bufferCapacity = 0;
    size_t bufferUsed = 0;

    static constexpr uint8_t SYNC_INTERVAL = 10;

    bool openFile();
    void generateFileName();
    bool reopenCurrentFile();

    // So usadas internamente por bufferLine()/flushBuffer() -- nenhum
    // chamador externo (main.cpp/WifiAP) grava direto no cartao sem
    // passar pelo buffer, entao nao precisam ser publicas.
    bool saveLine(const char* line);
    void flush();
};
