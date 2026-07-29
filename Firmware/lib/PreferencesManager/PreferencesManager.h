#pragma once

#include <Arduino.h>
#include <Preferences.h>

#include "DataTypes.h"

// Cada função abre e fecha a NVS individualmente.
// Isso torna cada operação independente e evita
// manter a namespace aberta desnecessariamente.

class PreferencesManager
{
public:

    bool initialize();

    bool loadSystem(SystemConfig& config);
    bool loadOffsets(ImuOffsets& offsets);
    bool loadPid(PidConfig& config);
    bool loadLora(LoraConfig& config);

    bool loadAll(
        SystemConfig& system,
        ImuOffsets& offsets
    );

    bool saveSystem(const SystemConfig& config);
    bool saveOffsets(const ImuOffsets& offsets);
    bool savePid(const PidConfig& config);
    bool saveLora(const LoraConfig& config);

    bool saveAll(
        const SystemConfig& system,
        const ImuOffsets& offsets
    );

    bool restoreDefaults();

    void printAll(Stream& stream);

private:

    bool openNamespace(
        Preferences& nvs,
        bool readOnly
    );
};
