#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "InputManager.h"
#include "SensorModule.h"

enum EstadoSessao {
  AGUARDANDO_CONFIGURACAO,
  AGUARDANDO_INICIO,
  SESSAO_ATIVA,
  SESSAO_PAUSADA,
  SESSAO_ENCERRADA
};

void gameLogicInit();
void gameLogicProcessarEvento(EventoBotao evento);
void gameLogicProcessarLeitura(LeituraSensor leitura);
void gameLogicTick(); // chamado a cada loop para processar o timer de 5s (RF30)
EstadoSessao gameLogicGetEstado();

#endif
