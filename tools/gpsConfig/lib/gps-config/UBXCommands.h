#pragma once

#include <Arduino.h>

namespace UBX
{

//--------------------------------------------------
// MON
//--------------------------------------------------

inline constexpr uint8_t MON_VER[] = {
    0xB5,0x62,0x0A,0x04,0x00,0x00,0x0E,0x34
};

inline constexpr uint8_t MON_HW[] = {
    0xB5,0x62,0x0A,0x09,0x00,0x00,0x13,0x43
};

inline constexpr uint8_t MON_HW2[] = {
    0xB5,0x62,0x0A,0x0B,0x00,0x00,0x15,0x47
};

inline constexpr uint8_t MON_GNSS[] = {
    0xB5,0x62,0x0A,0x28,0x00,0x00,0x32,0xA0
};

// NAV-PVT: poll da solucao de navegacao completa (fix type, numero
// de satelites, lat/lon/altitude, precisao, DOP). Fica de fora do
// array Commands[] (que so aceita 1-9 no menu) -- usado direto pelo
// modo de monitoramento/log de fix.
inline constexpr uint8_t NAV_PVT[] = {
    0xB5,0x62,0x01,0x07,0x00,0x00,0x08,0x19
};

//--------------------------------------------------
// CFG POLL
//--------------------------------------------------

inline constexpr uint8_t CFG_PRT[] = {
    0xB5,0x62,0x06,0x00,0x00,0x00,0x06,0x18
};

inline constexpr uint8_t CFG_RATE[] = {
    0xB5,0x62,0x06,0x08,0x00,0x00,0x0E,0x30
};

inline constexpr uint8_t CFG_NAV5[] = {
    0xB5,0x62,0x06,0x24,0x00,0x00,0x2A,0x84
};

inline constexpr uint8_t CFG_GNSS[] = {
    0xB5,0x62,0x06,0x3E,0x00,0x00,0x44,0xD2
};

inline constexpr uint8_t CFG_CFG[] = {
    0xB5,0x62,0x06,0x09,0x00,0x00,0x0F,0x30
};

//--------------------------------------------------
// Helpers
//--------------------------------------------------

struct Command
{
    const char *name;
    const uint8_t *packet;
    size_t length;
};

inline constexpr Command Commands[] =
{
    { "MON-VER"  , MON_VER  , sizeof(MON_VER)  },
    { "MON-HW"   , MON_HW   , sizeof(MON_HW)   },
    { "CFG-PRT"  , CFG_PRT  , sizeof(CFG_PRT)  },
    { "CFG-RATE" , CFG_RATE , sizeof(CFG_RATE) },
    { "CFG-NAV5" , CFG_NAV5 , sizeof(CFG_NAV5) },
    { "CFG-GNSS" , CFG_GNSS , sizeof(CFG_GNSS) },
    { "CFG-CFG"  , CFG_CFG  , sizeof(CFG_CFG)  },
    { "MON-HW2"  , MON_HW2  , sizeof(MON_HW2)  },
    { "MON-GNSS" , MON_GNSS , sizeof(MON_GNSS) }
};

constexpr uint8_t COMMAND_COUNT =
    sizeof(Commands) / sizeof(Command);

}