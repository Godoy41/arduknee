#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <Arduino.h>

// Parametros recebidos via POST /configurar.
struct ParametrosSessao {
  int tipoExercicio;  // 1 = E1 Flexao, 2 = E2 Extensao, 3 = E3 Ciclo completo
  int perfil;         // 1 a 6 (P1 a P6)
  int anguloInicial;
  int anguloMaximo;   // significa "angulo alvo"
  int repeticoes;
};

void webServerModuleInit();
void webServerModuleLoop();  // atende o DNS do Portal Cativo - chamar a cada loop()

// Se chegou uma configuracao nova desde a ultima chamada, copia para "saida" e
// retorna true. Thread-safe: o callback HTTP roda em outra tarefa do FreeRTOS e o
// acesso aos dados e' protegido por secao critica (PB-08 / RNF20). Se chegarem
// duas configuracoes seguidas, vale a ultima, inteira - nunca valores misturados.
bool webServerModuleObterNovaConfig(ParametrosSessao &saida);

#endif
