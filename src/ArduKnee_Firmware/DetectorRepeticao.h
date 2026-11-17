#ifndef DETECTOR_REPETICAO_H
#define DETECTOR_REPETICAO_H

// Deteccao de repeticao (UML v2.5 sec. 5.7) - classe PURA, sem Arduino.
//
// Progresso normalizado p = (angulo - inicial) / (alvo - inicial):
//   E1/E3: alvo > inicial (p cresce subindo).  E2: alvo < inicial (p cresce descendo).
// Uma repeticao = sair da posicao inicial, ATINGIR o alvo e VOLTAR a posicao inicial.
// Movimento parcial (volta sem atingir o alvo) nao conta (US04-CA2).
// A posicao inicial precisa ser VISTA antes de contar (evita contar a primeira volta
// se a sessao comeca no meio do arco).
//
// NOTA: a documentacao nao distingue o comportamento de E1 e E3; ambos usam esta regra.

#define LIMIAR_POSICAO_INICIAL 0.10f  // p <= isto = "de volta ao ponto de partida" (a validar na bancada)
// O sensor tem erro de ate +-5 graus (RNF26) e o ADC e' quantizado: exigir 100% exato do
// alvo faria um alvo de 30 graus nunca ser atingido ao ler 29,97. Vale como "alvo atingido"
// chegar a TOLERANCIA_ALVO_GRAUS do alvo (ou alem). O progresso exportado e' normalizado
// por esse criterio, entao a barra de LEDs mostra 100% exatamente quando a repeticao
// passa a poder fechar. Parametro de projeto: validar no PT 5.1.
#define TOLERANCIA_ALVO_GRAUS  2.0f

class DetectorRepeticao {
 public:
  void iniciar(int tipoExercicio, int anguloInicial, int anguloAlvo);
  // Retorna true quando este amostra COMPLETA uma repeticao; *duracaoMs = ida+volta.
  bool atualizar(float angulo, unsigned long agoraMs, unsigned long* duracaoMs);
  float progresso() const;  // 0.0-1.0 (limitado) - alimenta o termometro de LEDs

 private:
  enum Fase { FORA_DA_POSICAO, NA_POSICAO, IDA, NO_ALVO };
  Fase fase_ = FORA_DA_POSICAO;
  int tipo_ = 1;
  float inicial_ = 0.0f;
  float alvo_ = 1.0f;
  float limiarAlvo_ = 1.0f;   // p a partir do qual o alvo conta como atingido
  float p_ = 0.0f;
  unsigned long tInicioRep_ = 0;
};

#endif
