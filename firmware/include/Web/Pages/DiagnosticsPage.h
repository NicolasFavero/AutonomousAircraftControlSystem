#pragma once
#include <Arduino.h>

namespace Web::Pages
{

inline constexpr char DIAGNOSTICS_PAGE[] PROGMEM = R"rawliteral(

<section>

<h2>Diagnóstico</h2>

<div class="card">

<h3>Sensores</h3>

<table>

<tr>
<td>IMU</td>
<td>
    <span id="imuLed" class="led"></span>
</td>
</tr>

<tr>
<td>GPS</td>
<td id="gpsLed">●</td>
</tr>

<tr>
<td>BMP280</td>
<td id="bmpLed">●</td>
</tr>

</table>

</div>

<div class="card">

<h3>Sistema</h3>

<table>

<tr>
<td>Clientes WiFi</td>
<td id="wifiClients">-</td>
</tr>

<tr>
<td>Tensão</td>
<td id="battery">-</td>
</tr>

<tr>
<td>Modo</td>
<td id="flightMode">-</td>
</tr>

<tr>
<td>Satélites</td>
<td id="satellites">-</td>
</tr>

</table>

</div>

<div class="card">

<h3>Informações</h3>

<table>

<tr>
<td>Pitch</td>
<td id="pitch">-</td>
</tr>

<tr>
<td>Roll</td>
<td id="roll">-</td>
</tr>

<tr>
<td>Yaw</td>
<td id="yaw">-</td>
</tr>

<tr>
<td>Altitude GPS</td>
<td id="gpsAltitude">-</td>
</tr>

<tr>
<td>Altitude Barômetro</td>
<td id="baroAltitude">-</td>
</tr>

</table>

</div>

</section>

)rawliteral";

}