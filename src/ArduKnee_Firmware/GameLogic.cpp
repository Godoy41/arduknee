#include "GameLogic.h"

namespace {
  EstadoSessao estadoAtual = AGUARDANDO_CONFIGURACAO;
}

void gameLogicInit() {
  estadoAtual = AGUARDANDO_CONFIGURACAO;
}

void gameLogicProcessarEvento(EventoBotao evento) {
  // Sprint 1: apenas registra o evento recebido.
  // A tabela estado x evento completa (secao 3 do guia) entra na Sprint 2.
  Serial.print("[GameLogic] Evento - botao: ");
  Serial.print(evento.idBotao);
  Serial.print(" tipo: ");
  Serial.println(evento.tipo == CURTA ? "CURTA" : "LONGA");
}

void gameLogicProcessarLeitura(LeituraSensor leitura) {
  // Sprint 1: sem contagem de repeticoes/meta ainda (isso e' Sprint 2).
  (void)leitura;
}

void gameLogicTick() {
  // Sprint 2: timer de 5s da janela de confirmacao "Aguardando Inicio" (RF30, risco R24/R25).
}

EstadoSessao gameLogicGetEstado() {
  return estadoAtual;
}
