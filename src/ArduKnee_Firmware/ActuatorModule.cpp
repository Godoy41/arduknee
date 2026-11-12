#include "ActuatorModule.h"
#include "Pinout.h"

// Biblioteca ESP32Servo (instalar via Library Manager) - usada so' quando o
// servo entrar em uso na Sprint 2 (comemoracao / indicador fisico).
// #include <ESP32Servo.h>
// Servo servoIndicador;

namespace {
  const uint8_t PINOS_LEDS[10] = {
    PINO_LED_1, PINO_LED_2, PINO_LED_3, PINO_LED_4, PINO_LED_5,
    PINO_LED_6, PINO_LED_7, PINO_LED_8, PINO_LED_9, PINO_LED_10
  };

  unsigned long buzzerFimMs = 0;
  bool buzzerLigado = false;
}

void actuatorModuleInit() {
  for (uint8_t i = 0; i < 10; i++) {
    pinMode(PINOS_LEDS[i], OUTPUT);
    digitalWrite(PINOS_LEDS[i], LOW);
  }

  pinMode(PINO_BUZZER, OUTPUT); // base do 2N2222 - GPIO nunca liga o buzzer direto
  digitalWrite(PINO_BUZZER, LOW);

  // Sprint 2: servoIndicador.attach(PINO_SERVO); posicao neutra inicial.
}

void actuatorModuleSetLed(uint8_t indice, bool aceso) {
  if (indice >= 10) return; // indice invalido, ignora
  digitalWrite(PINOS_LEDS[indice], aceso ? HIGH : LOW);
}

void actuatorModuleApagarTodosLeds() {
  for (uint8_t i = 0; i < 10; i++) {
    digitalWrite(PINOS_LEDS[i], LOW);
  }
}

void actuatorModuleTocarBuzzer(unsigned int frequenciaHz, unsigned long duracaoMs) {
  (void)frequenciaHz;  // buzzer ativo: o tom é fixo
  digitalWrite(PINO_BUZZER, HIGH);
  buzzerLigado = true;
  buzzerFimMs = millis() + duracaoMs;
}
void actuatorModuleTick() {
  if (buzzerLigado && (long)(millis() - buzzerFimMs) >= 0) {
    digitalWrite(PINO_BUZZER, LOW);
    buzzerLigado = false;
  }
}

void actuatorModuleMoverServo(uint8_t angulo) {
  // Sprint 2: servoIndicador.write(angulo);
  (void)angulo;
}
