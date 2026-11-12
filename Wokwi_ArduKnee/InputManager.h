#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

enum IdBotao { BOTAO_INICIAR, BOTAO_ENCERRAR, BOTAO_RESET };
enum TipoPressao { CURTA, LONGA };

// O InputManager so classifica. Nao interpreta duplo clique nem conhece a sessao (guia, secao 6).
struct EventoBotao {
  IdBotao idBotao;
  TipoPressao tipo;
};

void inputManagerInit();
// Retorna true se um evento foi classificado neste ciclo
bool inputManagerPoll(EventoBotao &evento);

#endif
