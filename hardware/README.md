# Hardware

Esta pasta contém os arquivos de hardware do projeto, incluindo esquemáticos, layouts e demais arquivos relacionados às placas de circuito impresso (PCBs).

O sistema foi dividido em **duas PCBs empilháveis**, conectadas através de **headers macho e fêmea (2,54 mm)**. Essa arquitetura facilita a montagem, manutenção e futuras revisões do hardware, além de permitir uma melhor organização dos circuitos.

## PCB-Top

A **PCB-Top** concentra os circuitos de alimentação, armazenamento e navegação da aeronave, incluindo:

- Receptor GPS;
- Slot para cartão microSD;
- Conector da bateria;
- Conectores para servomotores;
- Conversor Buck (Step-Down);
- Capacitores de bulk para estabilização da alimentação;
- Distribuição das tensões de alimentação.

Também é nesta placa que ocorre a separação entre as linhas de alimentação de **3,3 V**, utilizando **ferrite beads** para isolar os diferentes domínios de alimentação, reduzindo a propagação de ruído entre os circuitos analógicos, digitais e de RF.

## PCB-Bottom

A **PCB-Bottom** concentra os circuitos responsáveis pelo processamento, sensoriamento e comunicação da aeronave, incluindo:

- ESP32-S3 SuperMini;
- IMU ICM-20948;
- BMP280;
- ADS1115;
- Módulo LoRa (RF96W ou módulos compatíveis da mesma família, como CC68);
- Conector para antena;
- Interface para tubo de Pitot.

A proximidade entre esses componentes reduz o comprimento das trilhas dos barramentos de comunicação e dos sinais de alta frequência, contribuindo para uma melhor integridade dos sinais.

## Interconexão

As duas placas são interligadas através de **headers macho e fêmea**, formando um conjunto compacto e removível.

Essa abordagem oferece diversas vantagens:

- Organização dos circuitos por função;
- Facilidade de montagem e manutenção;
- Possibilidade de substituir apenas uma das placas em futuras revisões;
- Redução da complexidade do roteamento;
- Melhor gerenciamento da distribuição de alimentação.

## Estrutura

```text
Hardware/
├── PCB-Top/
└── PCB-Bottom/
```