#ifndef TIPOS_H
#define TIPOS_H

// Tipos compartilhados entre os modulos. Sem dependencia de Arduino (testavel no PC).

#include <stdint.h>

enum EstadoSessao {
  AGUARDANDO_CONFIGURACAO,
  AGUARDANDO_INICIO,
  SESSAO_ATIVA,
  SESSAO_PAUSADA,
  SESSAO_ENCERRADA
};

// Nome exato exposto no /status (RNF23: sempre um dos 5 valores, nunca vazio).
inline const char* nomeEstado(EstadoSessao e) {
  static const char* const n[] = {"Aguardando Configuração", "Aguardando Início",
                                  "Sessão Ativa", "Sessão Pausada", "Sessão Encerrada"};
  return ((unsigned)e < 5) ? n[e] : n[0];
}

// Nome usado na coluna estado_sessao do CSV.
inline const char* nomeEstadoCsv(EstadoSessao e) {
  static const char* const n[] = {"AGUARDANDO_CONFIGURACAO", "AGUARDANDO_INICIO",
                                  "SESSAO_ATIVA", "SESSAO_PAUSADA", "SESSAO_ENCERRADA"};
  return ((unsigned)e < 5) ? n[e] : n[0];
}

#define TIMESTAMP_ISO_MAX 32

// Parametros da sessao (POST /configurar). Struct simples (POD): pode ser copiada
// inteira dentro de uma secao critica.
struct ParametrosSessao {
  int perfil;           // 1 a 6 (P1 a P6)
  int tipoExercicio;    // 1 = E1 Flexao, 2 = E2 Extensao, 3 = E3 Ciclo completo
  int anguloInicial;    // ponto de partida (graus)
  int anguloAlvo;       // ponto de chegada (graus)
  int repeticoes;       // meta de repeticoes (> 0, sem teto de projeto)
  char timestampISO[TIMESTAMP_ISO_MAX];  // hora do celular (opcional); "" se ausente
};

// Foto do estado do dispositivo. Montada pelo loop() (GameLogic) e PUBLICADA para o
// callback HTTP: o callback so LE esta copia, nunca toca nas variaveis da sessao (RNF20).
struct StatusSnapshot {
  EstadoSessao estado;
  int repeticoes;
  int metaRepeticoes;
  float anguloAtual;       // 0 se nao calibrado
  float amplitudeMaxima;   // max |angulo - anguloInicial| da sessao
  bool sessaoConcluida;    // encerrada por meta de repeticoes (RF26)
  bool calibrado;
  bool configurado;
  bool instavel;           // janela de instabilidade ativa (nao vai no /status)
  bool comemorando;        // rotina de comemoracao em curso (nao vai no /status)
  ParametrosSessao parametros;
};

#endif
