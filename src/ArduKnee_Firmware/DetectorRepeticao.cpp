#include "DetectorRepeticao.h"

void DetectorRepeticao::iniciar(int tipoExercicio, int anguloInicial, int anguloAlvo) {
  tipo_ = tipoExercicio;
  inicial_ = (float)anguloInicial;
  alvo_ = (float)anguloAlvo;
  float arco = alvo_ > inicial_ ? alvo_ - inicial_ : inicial_ - alvo_;
  limiarAlvo_ = (arco > 0.0f) ? 1.0f - TOLERANCIA_ALVO_GRAUS / arco : 1.0f;
  if (limiarAlvo_ < 0.5f) limiarAlvo_ = 0.5f;      // arco minusculo: nunca menos que metade do caminho
  fase_ = FORA_DA_POSICAO;
  p_ = 0.0f;
  tInicioRep_ = 0;
}

float DetectorRepeticao::progresso() const {
  float x = p_ / limiarAlvo_;
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

bool DetectorRepeticao::atualizar(float angulo, unsigned long agoraMs, unsigned long* duracaoMs) {
  float den = alvo_ - inicial_;
  p_ = (den == 0.0f) ? 0.0f : (angulo - inicial_) / den;

  // Uma unica amostra pode atravessar mais de uma fase (ex.: salto da posicao inicial
  // direto ao alvo): reavalia a MESMA amostra ate a fase estabilizar (no maximo 3 passos).
  for (int passo = 0; passo < 3; passo++) {
    Fase antes = fase_;
    switch (fase_) {
      case FORA_DA_POSICAO:
        if (p_ <= LIMIAR_POSICAO_INICIAL) fase_ = NA_POSICAO;
        break;
      case NA_POSICAO:
        if (p_ > LIMIAR_POSICAO_INICIAL) { fase_ = IDA; tInicioRep_ = agoraMs; }
        break;
      case IDA:
        if (p_ >= limiarAlvo_) fase_ = NO_ALVO;
        else if (p_ <= LIMIAR_POSICAO_INICIAL) fase_ = NA_POSICAO;  // parcial: nao conta
        break;
      case NO_ALVO:
        if (p_ <= LIMIAR_POSICAO_INICIAL) {
          fase_ = NA_POSICAO;
          if (duracaoMs) *duracaoMs = agoraMs - tInicioRep_;
          return true;
        }
        break;
    }
    if (fase_ == antes) break;
  }
  return false;
}
