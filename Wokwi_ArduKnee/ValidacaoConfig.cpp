#include "ValidacaoConfig.h"

const char* validarConfig(int tipoExercicio, int perfil, int anguloInicial, int anguloAlvo, int repeticoes) {
  if (tipoExercicio < 1 || tipoExercicio > 3) return "Tipo de exercício inválido.";
  if (perfil < 1 || perfil > 6) return "Perfil clínico inválido.";
  if (anguloInicial < ANGULO_MIN_GRAUS || anguloInicial > ANGULO_MAX_GRAUS) return "O ângulo inicial deve estar entre 0 e 90.";
  if (anguloAlvo < ANGULO_MIN_GRAUS || anguloAlvo > ANGULO_MAX_GRAUS) return "O ângulo alvo deve estar entre 0 e 90.";
  if (tipoExercicio == 2) {
    if (anguloAlvo >= anguloInicial) return "Na extensão, o ângulo alvo deve ser menor que o inicial.";
  } else {
    if (anguloAlvo <= anguloInicial) return "Na flexão e no ciclo completo, o ângulo alvo deve ser maior que o inicial.";
  }
  if (repeticoes <= 0 || repeticoes > REPETICOES_MAXIMAS) return "As repetições devem estar entre 1 e 100.";
  return nullptr;
}
