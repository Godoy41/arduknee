#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <Arduino.h>
#include "Tipos.h"

// Access Point + Portal Cativo + endpoints (UML v2.5 sec. 5):
//   GET /  |  GET /status  |  POST /configurar  |  POST /calibrar
// NAO existem /reset nem /encerrar (RF41): Reset e Encerrar sao so fisicos.
//
// Regra de concorrencia (RNF20): o callback HTTP roda na tarefa async_tcp. Ele so
//   (1) le a FOTO do estado publicada pelo loop(),
//   (2) valida o pedido (funcoes puras de ProtocoloHttp),
//   (3) deixa o pedido numa caixa de correio protegida por secao critica.
// Quem escreve nas variaveis da sessao e' sempre o loop().

struct Pendencias {
  bool config;                 // chegou uma configuracao valida
  ParametrosSessao params;
  bool calibrar;               // chegou um pedido de calibracao valido
  int anguloReferencia;
  bool configInvalida;         // um POST /configurar foi rejeitado com 400 (feedback fisico)
};

void webServerModuleInit();
void webServerModuleLoop();                                     // DNS do Portal Cativo - a cada loop()
void webServerModulePublicarStatus(const StatusSnapshot& snap); // a cada loop(): foto para o callback
// Copia e LIMPA os pedidos pendentes (atomico). true se havia algo.
bool webServerModuleObterPendencias(Pendencias& saida);

#endif
