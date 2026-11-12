#ifndef PINOUT_H
#define PINOUT_H

// Pinout oficial - ArduKnee Circuito Eletrico v2.7 (fonte unica da verdade).
// Qualquer mudanca de pino e' feita SO AQUI, nunca direto dentro de um modulo.

// Sensor
#define PINO_POTENCIOMETRO 34   // ADC1_CH6 - so entrada, sem pull-up/down interno

// Botoes (INPUT_PULLUP - pressionado = nivel BAIXO)
#define PINO_BOTAO_INICIAR  4
#define PINO_BOTAO_ENCERRAR 16  // ATENCAO: GPIO 16 e 17 so existem no ESP32-WROOM-32
#define PINO_BOTAO_RESET    17  // (DevKit V1). Em modulos WROVER (PSRAM) ficam ocupados.

// Atuadores
#define PINO_SERVO  13  // PWM, 50Hz, alimentado em 5V - NUNCA no 3V3
#define PINO_BUZZER 23  // aciona a BASE do transistor 2N2222 - GPIO nunca liga o buzzer direto

// Barra de 10 LEDs (progresso angular) - cada um com resistor 220 ohm em serie
#define PINO_LED_1  14  // vermelho
#define PINO_LED_2  27  // vermelho
#define PINO_LED_3  26  // amarelo
#define PINO_LED_4  25  // amarelo
#define PINO_LED_5  33  // amarelo
#define PINO_LED_6  32  // amarelo
#define PINO_LED_7  18  // verde
#define PINO_LED_8  19  // verde
#define PINO_LED_9  21  // verde
#define PINO_LED_10 22  // verde

// Pinos que NAO devem ser usados neste projeto:
//   GPIO 0, 2, 5, 12, 15 -> strapping (interferem no boot)
//   GPIO 6 a 11          -> flash SPI interna (proibidos)
// Confirme que a placa e' ESP32-WROOM-32 antes de montar (GPIO 16/17 dos botoes).

#endif
