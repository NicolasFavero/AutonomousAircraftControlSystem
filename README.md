# Sistema de Controle e Telemetria para Asa Autônoma

Este projeto é um **protótipo em desenvolvimento** de um sistema embarcado para uma **asa autônoma / aeromodelo experimental**, baseado em **ESP32-C3**.

O sistema realiza a aquisição de dados de sensores inerciais e ambientais, controla os servomotores das superfícies de comando e registra os dados de voo em um cartão microSD para análise posterior.

---

## Objetivo

Desenvolver uma plataforma embarcada capaz de:

- Adquirir dados de sensores inerciais;
- Medir pressão atmosférica e altitude;
- Medir velocidade do ar através de tubo de Pitot;
- Estimar a atitude da aeronave;
- Controlar superfícies móveis da aeronave;
- Registrar os dados de voo para análise posterior;
- Servir como base para futuras versões do sistema de navegação e telemetria.

---

## Funcionalidades

- Leitura de acelerômetro;
- Leitura de giroscópio;
- Leitura de magnetômetro;
- Estimativa de Pitch, Roll e Yaw;
- Medição de pressão atmosférica;
- Cálculo de altitude utilizando BMP280;
- Medição de velocidade do ar via tubo de Pitot;
- Controle de servomotores;
- Controle de:
  - Flaperon(Flap + Aileron);

- Registro automático dos dados em cartão microSD;
- Estrutura preparada para expansão futura.

---

## Status do Projeto

🚧 **Protótipo em desenvolvimento**

O hardware e o firmware encontram-se em constante evolução e podem sofrer alterações frequentes.

---

## Hardware Utilizado

- ESP32-C3;
- GY-91 (MPU9250 + BMP280);
- Tubo de Pitot;
- Módulo para cartão microSD;
- Servomotores.

---

## Tecnologias Utilizadas

- C / C++;
- Arduino Framework;
- Arduino IDE.

---

## Dependências

### Sensores

- MPU9250_asukiaaa — v1.5.13
- Adafruit BMP280 Library — v3.0.0

### Armazenamento

- SdFat — v2.3.0

### Core Arduino

- SPI
- Wire

### ESP32

- esp_adc_cal
- esp_task_wdt

---

## Ambiente de Desenvolvimento

- Arduino IDE 2.3.7
- Arduino Core for ESP32 v2.x
- Placa alvo: ESP32-C3

---

## Estrutura do Projeto

```text
Firmware/
└── Firmware.ino
```

---

## Aplicação

Projeto desenvolvido para fins educacionais e experimentais, integrando conhecimentos de:

- Programação embarcada;
- Eletrônica;
- Sensores inerciais;
- Aquisição de dados;
- Sistemas aeronáuticos;
- Controle de voo.

O projeto serve como base para o desenvolvimento de futuras versões do sistema de controle, navegação e telemetria da aeronave.