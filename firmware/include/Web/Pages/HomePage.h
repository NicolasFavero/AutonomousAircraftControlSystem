#pragma once

#include <Arduino.h>

namespace Web::Pages{

    inline constexpr char HOME_PAGE[] PROGMEM = R"rawliteral(

    <!DOCTYPE html>

    <html lang="pt-BR">

    <head>

    <meta charset="UTF-8">

    <meta name="viewport" content="width=device-width,initial-scale=1">

    <title>Asa Autônoma</title>

    <style>

    %s

    </style>

    </head>

    <body>

    <header>

    <button id="menuButton">

    ☰

    </button>

    <h1>

    Asa Autônoma

    </h1>

    </header>

    <nav id="menu">

    %s

    </nav>

    <main id="content">

    </main>

    <script>

    %s

    </script>

    </body>

    </html>

    )rawliteral";

}