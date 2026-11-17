// =====================================================================================
// ArduKnee - firmware MODULAR - versao PlatformIO (src/main.cpp)
// Substitui o ArduKnee_Firmware.ino: o main.cpp SO ORQUESTRA (setup/loop); cada modulo
// (.h/.cpp) fica em src/ArduKnee_Firmware/ e cuida da sua responsabilidade (RNF07/RNF17).
//
// Regras de ouro: zero delay() no loop (RNF21); dados compartilhados com o callback HTTP
// so por secao critica (RNF20); /status idempotente (RNF22); CSV nao bloqueante (RF34).
//
// Requisitos de ambiente:
//  - plataforma pioarduino (Arduino-ESP32 3.x) no platformio.ini
//  - bibliotecas ESP32Async/ESPAsyncWebServer e ESP32Async/AsyncTCP (lib_deps)
//  - src/ArduKnee_Firmware/Segredos.h (copie Segredos.h.example e defina a senha do Wi-Fi)
// =====================================================================================

#include <Arduino.h>

#include "ArduKnee_Firmware/Pinout.h"
#include "ArduKnee_Firmware/Tipos.h"
#include "ArduKnee_Firmware/Log.h"
#include "ArduKnee_Firmware/Armazenamento.h"
#include "ArduKnee_Firmware/SensorModule.h"
#include "ArduKnee_Firmware/InputManager.h"
#include "ArduKnee_Firmware/GameLogic.h"
#include "ArduKnee_Firmware/FeedbackDispatcher.h"
#include "ArduKnee_Firmware/ActuatorModule.h"
#include "ArduKnee_Firmware/CsvExport.h"
#include "ArduKnee_Firmware/WebServerModule.h"

namespace {
  const unsigned long INTERVALO_LEITURA_MS = 50;       // 20 Hz (RF13 / RF29)
  const unsigned long INTERVALO_PUBLICACAO_MS = 10;    // foto do estado para o HTTP (polling e' de 2 s)
  unsigned long ultimaAmostragemMs = 0;
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
  if (agora - ultimaAmostragemMs >= INTERVALO_LEITURA_MS) {
    ultimaAmostragemMs = agora;
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
