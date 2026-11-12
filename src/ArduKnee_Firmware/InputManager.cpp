#include "Pinout.h"
#include "InputManager.h"

namespace {
  const unsigned long DEBOUNCE_MS = 50;
  const unsigned long LIMIAR_LONGA_MS = 2000;

  struct EstadoBotao {
    uint8_t pino;
    IdBotao id;
    bool pressionadoAnterior;
    unsigned long tInicioPressao;
    unsigned long tUltimaMudanca;
  };

  // Pinout real - ArduKnee Circuito Eletrico v2.7 (ver Pinout.h)
  EstadoBotao botoes[3] = {
    {PINO_BOTAO_INICIAR, BOTAO_INICIAR, false, 0, 0},
    {PINO_BOTAO_ENCERRAR, BOTAO_ENCERRAR, false, 0, 0},
    {PINO_BOTAO_RESET, BOTAO_RESET, false, 0, 0}
  };
}

void inputManagerInit() {
  for (auto &b : botoes) {
    pinMode(b.pino, INPUT_PULLUP); // botao ligado ao GND quando pressionado
  }
}

bool inputManagerPoll(EventoBotao &evento) {
  unsigned long agora = millis();

  for (auto &b : botoes) {
    bool pressionadoAgora = (digitalRead(b.pino) == LOW);

    if (pressionadoAgora != b.pressionadoAnterior) {
      if (agora - b.tUltimaMudanca < DEBOUNCE_MS) {
        continue; // trepidar mecanico, ignora (nao atualiza estado ainda)
      }
      b.tUltimaMudanca = agora;
      b.pressionadoAnterior = pressionadoAgora;

      if (pressionadoAgora) {
        b.tInicioPressao = agora;
      } else {
        unsigned long duracao = agora - b.tInicioPressao;
        evento.idBotao = b.id;
        evento.tipo = (duracao >= LIMIAR_LONGA_MS) ? LONGA : CURTA;
        return true;
      }
    }
  }
  return false;
}
