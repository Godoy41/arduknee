#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "Tipos.h"
#include "InputManager.h"
#include "SensorModule.h"

// Maquina de estados do ArduKnee (UML v2.5 sec. 2). Roda SO no loop(): quem vem de
// outra tarefa (HTTP) entra por webServerModuleObterPendencias() -> aplicarConfig/calibrar.

enum ResultadoCalibracao {
  CALIBRACAO_ACEITA,
  CALIBRACAO_ESTADO_INVALIDO,  // fora de Aguardando Inicio
  CALIBRACAO_INSTAVEL,         // UC14-A3: leitura instavel, repetir a captura
  CALIBRACAO_SEM_LEITURA
};

#define JANELA_CONFIRMACAO_MS       5000UL    // RF30: 2a CURTA em ate 5 s confirma o inicio
#define TIMEOUT_AGUARDANDO_MS       300000UL  // RF28: 5 min
#define JANELA_INSTABILIDADE_MS     1000UL    // leitura "instavel" ate 1 s apos o ultimo salto

void gameLogicInit();
void gameLogicProcessarEvento(EventoBotao evento);   // CURTA/LONGA vindos do InputManager
void gameLogicProcessarLeitura(const LeituraSensor& leitura);  // a cada 50 ms (exceto pausada)
void gameLogicTick();                                // a cada loop(): timers
EstadoSessao gameLogicGetEstado();

// Configuracao validada pelo callback HTTP. Retorna false (e descarta) se o estado ja
// nao a aceita - o callback so consultou uma FOTO do estado, o loop() e' a autoridade.
bool gameLogicAplicarConfig(const ParametrosSessao& params);
ResultadoCalibracao gameLogicCalibrar(int anguloReferencia);

void gameLogicObterSnapshot(StatusSnapshot& saida);

#endif
