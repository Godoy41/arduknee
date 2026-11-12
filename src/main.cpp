// ArduKnee - ponto de entrada no PlatformIO (src/main.cpp).
// Equivale ao ArduKnee_Firmware.ino: so orquestra os modulos que ficam em
// src/ArduKnee_Firmware/. Nao ha "classe main" em C++/Arduino: o programa comeca
// em setup() (uma vez) e repete loop() para sempre.
#include <Arduino.h>

#include "ArduKnee_Firmware/Pinout.h"
#include "ArduKnee_Firmware/SensorModule.h"
#include "ArduKnee_Firmware/InputManager.h"
#include "ArduKnee_Firmware/GameLogic.h"
#include "ArduKnee_Firmware/ActuatorModule.h"
#include "ArduKnee_Firmware/WebServerModule.h"
#include "ArduKnee_Firmware/FeedbackDispatcher.h"

namespace {
  unsigned long ultimaLeituraSensor = 0;
  const unsigned long INTERVALO_LEITURA_MS = 50;  // 20 Hz
}

void setup() {
  Serial.begin(115200);

  sensorModuleInit();
  inputManagerInit();
  gameLogicInit();
  actuatorModuleInit();
  webServerModuleInit();

  Serial.println("[main] Sprint 1 - PoC inicializado.");
}

void loop() {
  unsigned long agora = millis();

  // Leitura do sensor a 20 Hz - zero delay() no loop principal (RNF21)
  if (agora - ultimaLeituraSensor >= INTERVALO_LEITURA_MS) {
    ultimaLeituraSensor = agora;
    LeituraSensor leitura = sensorModuleLer();
    gameLogicProcessarLeitura(leitura);

    if (leitura.instavel) {
      dispararEvento(INSTABILIDADE);  // o dispatcher limita a 1 aviso por segundo
    }
  }

  // Classificacao de botoes (CURTA/LONGA)
  EventoBotao evento;
  if (inputManagerPoll(evento)) {
    gameLogicProcessarEvento(evento);
  }

  // Servidor web: DNS do Portal Cativo + coleta de configuracao nova (thread-safe)
  webServerModuleLoop();
  ParametrosSessao novaConfig;
  if (webServerModuleObterNovaConfig(novaConfig)) {
    // Sprint 2: gameLogicAplicarConfig(novaConfig) - cancela o timer de 5 s (RF30)
    dispararEvento(CONFIGURACAO_OK);  // bipe curto: a configuracao foi aceita
    Serial.printf("[main] Nova config: exercicio E%d, perfil P%d, %d a %d graus, %d repeticoes\n",
                  novaConfig.tipoExercicio, novaConfig.perfil,
                  novaConfig.anguloInicial, novaConfig.anguloMaximo, novaConfig.repeticoes);
  }

  // Desliga o buzzer quando o tempo pedido acabar (nao bloqueante)
  actuatorModuleTick();

  // Timers da maquina de estados - Sprint 2 (RF30)
  gameLogicTick();
}
