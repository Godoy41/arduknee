#ifndef FEEDBACK_DISPATCHER_H
#define FEEDBACK_DISPATCHER_H

#include <stdint.h>

// Ponto unico dos padroes de LED / buzzer / servo (RF22). Sem logica de jogo.
// Tudo NAO BLOQUEANTE: feedbackTick() avanca padroes com millis() a cada loop().
//
// Dois niveis de LED:
//   - EXIBICAO (base): o que a barra mostra o tempo todo, conforme o modo.
//   - EVENTO (overlay): padrao curto que SUBSTITUI a exibicao enquanto dura.

enum TipoEvento {
  CONFIGURACAO_OK,
  CONFIGURACAO_INVALIDA,
  CALIBRACAO_OK,
  CONFIRMAR_INICIO,        // 1a CURTA em Aguardando Inicio: 3 pulsos (RF30)
  INICIO_RECUSADO,         // sem config/calibracao: vermelho 3 pulsos
  REPETICAO_CORRETA,
  REPETICAO_NAO_CONFIAVEL, // repeticao durante instabilidade (US09): ambar
  META_ATINGIDA,           // comemoracao (servo + buzzer + LEDs)
  ENCERRAMENTO_MANUAL,     // sinal neutro: tom curto + LEDs fixos (RF15)
  PAUSA,
  RETOMADA,
  INSTABILIDADE,           // limitado a 1 aviso por segundo
  RESET,
  QUASE_LA                 // tratado pela exibicao de progresso (80-99%); nao e' overlay
};

enum ModoExibicao {
  EXIBICAO_AGUARDANDO_CONFIG,   // batimento discreto (indica que o aparelho esta ligado)
  EXIBICAO_AGUARDANDO_INICIO,   // verde piscando lentamente
  EXIBICAO_PROGRESSO,           // termometro angular
  EXIBICAO_PAUSA,               // ambar fixo
  EXIBICAO_RESULTADO            // alterna: repeticoes/meta e amplitude/arco (a cada 3 s)
};

#define DURACAO_COMEMORACAO_MS 3000UL

void feedbackInit();
void dispararEvento(TipoEvento evento);
void feedbackDefinirModo(ModoExibicao modo);
void feedbackDefinirProgresso(float progresso01, bool instavel);       // modo PROGRESSO
void feedbackDefinirResultado(float fracRepeticoes, float fracAmplitude);  // modo RESULTADO
void feedbackDefinirServo(float razao01);   // 0 = neutro (0 graus), 1 = 180 graus
void feedbackTick();
bool feedbackComemoracaoAtiva();

// Funcao pura (testavel): mascara do termometro, com "quase la" (>=80%: pulso lento)
// e meta atingida (>=100%: todos piscam rapido).
uint16_t feedbackMascaraProgresso(float progresso01, unsigned long agoraMs);

#endif
