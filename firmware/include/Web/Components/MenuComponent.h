#pragma once

#include <Arduino.h>

namespace Web::Components
{inline constexpr char MENU_COMPONENT[] PROGMEM = R"rawliteral(

<button data-page="status">

Status

</button>

<button data-page="offsets">

Offsets

</button>

<button data-page="system">

Sistema

</button>

<button data-page="lora">

LoRa

</button>

<button data-page="flight">

Controle de Voo

</button>

<button data-page="diagnostics">

Diagnóstico

</button>

)rawliteral";
}