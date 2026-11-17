#ifndef ACTUATOR_MODULE_H
#define ACTUATOR_MODULE_H

#include <Arduino.h>

// Camada fisica: LEDs, buzzer PASSIVO e servo. Nao decide QUANDO acionar - isso e'
// do FeedbackDispatcher/GameLogic. Exige o core Arduino-ESP32 3.x (API LEDC por pino).

void actuatorModuleInit();

// Barra de 10 LEDs: bit 0 = LED 1 ... bit 9 = LED 10.
void actuatorModuleDefinirLeds(uint16_t mascara);
void actuatorModuleSetLed(uint8_t indice, bool aceso);   // indice 0-9
void actuatorModuleApagarTodosLeds();

// Buzzer PASSIVO (via 2N2222): gera o tom por PWM. 0 Hz = silencio. Nao bloqueia.
// O tempo do tom e' controlado por quem chama (FeedbackDispatcher, com millis()).
void actuatorModuleTom(unsigned int frequenciaHz);

// Servo SG90: angulo 0-180 (neutro = 0). So reescreve o PWM se o valor mudou.
void actuatorModuleMoverServo(uint8_t angulo);

#endif
