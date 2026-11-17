#ifndef PINOUT_H
#define PINOUT_H

// Pinout oficial - ArduKnee Circuito Eletrico v2.7 / layout ArduKnee_v3_4.json
// (fonte unica da verdade). Qualquer mudanca de pino e' feita SO AQUI.

// Sensor
#define PINO_POTENCIOMETRO 34   // ADC1_CH6 - so entrada, sem pull-up/down interno

// Botoes (INPUT_PULLUP - pressionado = nivel BAIXO)
#define PINO_BOTAO_INICIAR  4
#define PINO_BOTAO_ENCERRAR 16  // ATENCAO: GPIO 16 e 17 so existem no ESP32-WROOM-32
#define PINO_BOTAO_RESET    17  // (DevKit V1). Em modulos WROVER (PSRAM) ficam ocupados.

// Atuadores
#define PINO_SERVO  13  // PWM 50 Hz, alimentado em 5 V (boost) - NUNCA no 3V3
#define PINO_BUZZER 23  // BASE do 2N2222 (via 1 kohm). Buzzer PASSIVO: o tom e' gerado por PWM (LEDC)

// Canais LEDC fixos. No core 3.x cada PAR de canais (0-1, 2-3, ...) compartilha um timer:
// servo e buzzer ficam em pares diferentes para que mudar a frequencia do buzzer
// NAO altere os 50 Hz do servo.
#define LEDC_CANAL_SERVO  0
#define LEDC_CANAL_BUZZER 2

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
