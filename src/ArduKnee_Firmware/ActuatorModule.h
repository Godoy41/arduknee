#ifndef ACTUATOR_MODULE_H
#define ACTUATOR_MODULE_H

#include <Arduino.h>

// Controla LEDs, buzzer e servo (secao 9 do guia + circuito eletrico v2.7).
// Nao decide quando acionar - quem decide e' o GameLogic via FeedbackDispatcher (Sprint 2).

void actuatorModuleInit();

// --- Funcoes de baixo nivel, uma por atuador (comportamento completo e' Sprint 2 / PB-18, PB-19, PB-25) ---

// Acende/apaga um LED da barra (indice 0-9, correspondendo a LED 1-10 do circuito)
void actuatorModuleSetLed(uint8_t indice, bool aceso);
void actuatorModuleApagarTodosLeds();

// Liga o buzzer ATIVO (via transistor 2N2222) por duracaoMs, sem bloquear (sem delay()).
// O buzzer ativo tem tom fixo: frequenciaHz e' ignorada. Quem desliga e' actuatorModuleTick().
void actuatorModuleTocarBuzzer(unsigned int frequenciaHz, unsigned long duracaoMs);

// Chamar a cada loop(): desliga o buzzer quando o tempo pedido acabar.
void actuatorModuleTick();

// Servo: angulo 0-180 (a posicao neutra/comemoracao e' definida pelo GameLogic na Sprint 2)
void actuatorModuleMoverServo(uint8_t angulo);

#endif
