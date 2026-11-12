#include "FeedbackDispatcher.h"
#include "ActuatorModule.h"
#include <Arduino.h>

namespace {
  // A instabilidade e' detectada ate 20x por segundo; sem este intervalo o
  // buzzer/LED seriam acionados sem parar (Sprint 2).
  const unsigned long INTERVALO_MIN_INSTABILIDADE_MS = 1000;
  unsigned long ultimoDisparoInstabilidade = 0;
  bool jaDisparouInstabilidade = false;
}

void dispararEvento(TipoEvento evento) {
  unsigned long agora = millis();

  if (evento == INSTABILIDADE) {
    if (jaDisparouInstabilidade &&
        (agora - ultimoDisparoInstabilidade) < INTERVALO_MIN_INSTABILIDADE_MS) {
      return;  // muito cedo desde o ultimo aviso: ignora
    }
    jaDisparouInstabilidade = true;
    ultimoDisparoInstabilidade = agora;
  }

  // Padroes de feedback (secao 9 do guia). Por enquanto so o buzzer de CONFIGURACAO_OK
  // (bipe curto, nao bloqueante: o actuatorModuleTick() desliga). LEDs e servo: Sprint 2.
  if (evento == CONFIGURACAO_OK) {
    actuatorModuleTocarBuzzer(0, 150);
  }

  Serial.print("[FeedbackDispatcher] Evento disparado: ");
  Serial.println(evento);
}
