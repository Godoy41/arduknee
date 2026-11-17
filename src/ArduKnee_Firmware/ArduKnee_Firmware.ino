// ArduKnee - firmware (Epic Firmware). Sprint 2: maquina de estados completa.
// O .ino SO ORQUESTRA: cada modulo cuida da sua responsabilidade (RNF07/RNF17).
// Regras de ouro: zero delay() no loop (RNF21); dados compartilhados com o callback HTTP
// so por secao critica (RNF20); /status idempotente (RNF22); CSV nao bloqueante (RF34).
//
// Requisitos de ambiente:
//  - pacote "esp32" da Espressif 3.x (Arduino IDE > Boards Manager)
//  - bibliotecas "ESP Async WebServer" e "Async TCP" (ESP32Async)
//  - copiar Segredos.h.example para Segredos.h e definir a senha do Wi-Fi

#include "Pinout.h"
#include "Tipos.h"
#include "Log.h"
#include "Armazenamento.h"
#include "SensorModule.h"
#include "InputManager.h"
#include "GameLogic.h"
#include "FeedbackDispatcher.h"
#include "ActuatorModule.h"
#include "CsvExport.h"
#include "WebServerModule.h"

namespace {
  const unsigned long INTERVALO_LEITURA_MS = 50;       // 20 Hz (RF13 / RF29)
  const unsigned long INTERVALO_PUBLICACAO_MS = 10;    // foto do estado para o HTTP (polling e' de 2 s)
  unsigned long ultimaLeituraMs = 0;
  unsigned long ultimaPublicacaoMs = 0;
}

void setup() {
  Serial.begin(115200);
  armazenamentoInit();
  sensorModuleInit();
  inputManagerInit();
  actuatorModuleInit();
  feedbackInit();
  gameLogicInit();
  webServerModuleInit();
  logInfo("[main] ArduKnee pronto. Aguardando configuracao via Wi-Fi.");
}

void loop() {
  unsigned long agora = millis();

  // 1) Sensor a cada 50 ms. Em Sessao Pausada a leitura fica suspensa (UML v2.5).
  if (agora - ultimaLeituraMs >= INTERVALO_LEITURA_MS) {
    ultimaLeituraMs = agora;
    if (gameLogicGetEstado() != SESSAO_PAUSADA) {
      LeituraSensor leitura = sensorModuleLer();
      gameLogicProcessarLeitura(leitura);
    }
  }

  // 2) Botoes -> eventos classificados (CURTA/LONGA) -> maquina de estados
  EventoBotao evento;
  if (inputManagerPoll(evento)) gameLogicProcessarEvento(evento);

  // 3) Pedidos vindos do HTTP (ja validados pelo callback) entram AQUI, no loop()
  webServerModuleLoop();
  Pendencias pend;
  if (webServerModuleObterPendencias(pend)) {
    if (pend.configInvalida) dispararEvento(CONFIGURACAO_INVALIDA);
    if (pend.config) gameLogicAplicarConfig(pend.params);
    if (pend.calibrar) gameLogicCalibrar(pend.anguloReferencia);   // depois da config: mesma ordem do fluxo
  }

  // 4) Timers, atuadores e CSV (todos nao bloqueantes)
  gameLogicTick();
  feedbackTick();
  csvExportTick();

  // 5) Foto do estado para o callback de GET /status e para os 409. A cada 10 ms basta:
  //    o loop() re-valida tudo ao consumir um pedido, entao uma foto levemente velha e' inofensiva
  //    - e evita entrar na secao critica centenas de milhares de vezes por segundo.
  if (agora - ultimaPublicacaoMs >= INTERVALO_PUBLICACAO_MS) {
    ultimaPublicacaoMs = agora;
    StatusSnapshot snap;
    gameLogicObterSnapshot(snap);
    webServerModulePublicarStatus(snap);
  }
}
