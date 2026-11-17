#include "ActuatorModule.h"
#include "Pinout.h"

#if !defined(ESP_ARDUINO_VERSION_MAJOR) || (ESP_ARDUINO_VERSION_MAJOR < 3)
#error "ArduKnee exige o pacote esp32 da Espressif versao 3.x (API ledcAttachChannel/ledcWriteTone)."
#endif

namespace {
  const uint8_t PINOS_LEDS[10] = {
    PINO_LED_1, PINO_LED_2, PINO_LED_3, PINO_LED_4, PINO_LED_5,
    PINO_LED_6, PINO_LED_7, PINO_LED_8, PINO_LED_9, PINO_LED_10
  };

  // SG90: pulso de ~0,5 ms (0 graus) a ~2,4 ms (180 graus) em 50 Hz. Se o servo "zumbir"
  // nos extremos, estreite estes limites (ex.: 600-2300) em vez de forcar o batente.
  const uint32_t SERVO_FREQ_HZ = 50;
  const uint8_t  SERVO_RESOLUCAO_BITS = 14;
  const uint32_t SERVO_US_MIN = 500;
  const uint32_t SERVO_US_MAX = 2400;

  const uint32_t BUZZER_FREQ_INICIAL_HZ = 2000;
  const uint8_t  BUZZER_RESOLUCAO_BITS = 10;

  int ultimoServo = -1;
  unsigned int ultimoTom = 0;
}

void actuatorModuleInit() {
  for (uint8_t i = 0; i < 10; i++) {
    pinMode(PINOS_LEDS[i], OUTPUT);
    digitalWrite(PINOS_LEDS[i], LOW);
  }
  // Canais explicitos: servo e buzzer em PARES de canais diferentes (timers diferentes).
  ledcAttachChannel(PINO_BUZZER, BUZZER_FREQ_INICIAL_HZ, BUZZER_RESOLUCAO_BITS, LEDC_CANAL_BUZZER);
  ledcWriteTone(PINO_BUZZER, 0);
  ledcAttachChannel(PINO_SERVO, SERVO_FREQ_HZ, SERVO_RESOLUCAO_BITS, LEDC_CANAL_SERVO);
  ultimoServo = -1;
  actuatorModuleMoverServo(0);  // posicao neutra
}

void actuatorModuleDefinirLeds(uint16_t mascara) {
  for (uint8_t i = 0; i < 10; i++) {
    digitalWrite(PINOS_LEDS[i], (mascara >> i) & 1 ? HIGH : LOW);
  }
}

void actuatorModuleSetLed(uint8_t indice, bool aceso) {
  if (indice >= 10) return;
  digitalWrite(PINOS_LEDS[indice], aceso ? HIGH : LOW);
}

void actuatorModuleApagarTodosLeds() { actuatorModuleDefinirLeds(0); }

void actuatorModuleTom(unsigned int frequenciaHz) {
  if (frequenciaHz == ultimoTom) return;
  ultimoTom = frequenciaHz;
  ledcWriteTone(PINO_BUZZER, frequenciaHz);  // 0 = duty 0 = silencio
}

void actuatorModuleMoverServo(uint8_t angulo) {
  if (angulo > 180) angulo = 180;
  if ((int)angulo == ultimoServo) return;
  ultimoServo = angulo;
  uint32_t us = SERVO_US_MIN + ((uint32_t)angulo * (SERVO_US_MAX - SERVO_US_MIN)) / 180;
  uint32_t duty = (us * (1UL << SERVO_RESOLUCAO_BITS)) / (1000000UL / SERVO_FREQ_HZ);
  ledcWrite(PINO_SERVO, duty);
}
