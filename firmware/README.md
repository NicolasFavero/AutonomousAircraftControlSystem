# Sistema de Controle e Telemetria para Aeromodelo Autônomo

## Sobre o projeto

Este projeto consiste no desenvolvimento de um **sistema embarcado de controle, navegação e telemetria para uma asa autônoma / aeromodelo planador experimental**.

O sistema é baseado em um **ESP32-S3** e tem como finalidade realizar a aquisição de dados de voo, processamento de sensores, controle das superfícies móveis da aeronave e armazenamento das informações para análise posterior.

O projeto encontra-se em fase de **protótipo e desenvolvimento**, sendo utilizado para testes de hardware, sensores, algoritmos de controle e integração de sistemas embarcados.

---

## Objetivo

Desenvolver uma plataforma experimental para estudo e implementação de um sistema de controle autônomo para aeronaves de pequeno porte, integrando:

* Sensoriamento inercial e ambiental;
* Controle eletrônico das superfícies de comando;
* Registro de dados de voo;
* Desenvolvimento de firmware embarcado;
* Testes e validação de sistemas aplicados à aviação experimental.

---

## Funcionalidades

* Leitura de sensores inerciais:

  * Aceleração;
  * Velocidade angular;
  * Campo magnético;
* Medição de altitude e pressão atmosférica utilizando BMP280;
* Leitura de velocidade do ar através de tubo de Pitot;
* Controle das superfícies de comando:

  * Leme;
  * Aileron;
  * Profundor;
* Controle de servomotores;
* Registro de dados de voo em cartão SD;
* Estrutura preparada para futuras expansões, como telemetria e algoritmos de controle autônomo.

---

## Status do projeto

🚧 **Protótipo em desenvolvimento**

O firmware e o hardware ainda estão em fase experimental. Alterações na arquitetura, sensores utilizados e funcionalidades podem ocorrer durante o desenvolvimento.

---

## Tecnologias utilizadas

### Hardware

* ESP32-S3 SuperMini;
* ICM-20948 (IMU 9DOF);
* BMP280 (pressão e altitude);
* Tubo de Pitot;
* Servomotores;
* Cartão SD.

### Software

* C / C++;
* PlatformIO;
* Arduino Framework.

---

## Dependências

```ini
lib_deps =
	sparkfun/SparkFun 9DoF IMU Breakout - ICM 20948 - Arduino Library@^1.3.2
	adafruit/Adafruit ADS1X15@^2.6.2
	adafruit/Adafruit BMP280 Library@^3.0.0
	mikalhart/TinyGPSPlus@^1.1.0
	greiman/SdFat@^2.3.1
	jgromes/RadioLib@^7.7.1
```

---

## Ambiente de desenvolvimento

* IDE: PlatformIO;
* Framework: Arduino;
* Placa alvo: ESP32-S3 SuperMini.

---

## Estrutura do projeto

```
Firmware/
├── src/
├── include/
├── lib/
└── platformio.ini
```

---

## Aplicação

Projeto desenvolvido para fins educacionais e experimentais, integrando conhecimentos de **eletrônica, programação embarcada, sensores, controle e sistemas aeronáuticos** dentro de uma formação técnica em eletrônica.
