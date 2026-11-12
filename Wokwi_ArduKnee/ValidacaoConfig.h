#ifndef VALIDACAO_CONFIG_H
#define VALIDACAO_CONFIG_H

// Limites da configuracao de sessao. Os valores definitivos de angulo e
// repeticoes devem ser definidos com um fisioterapeuta (prototipo educacional).
// tipoExercicio: 1 = E1 Flexao, 2 = E2 Extensao, 3 = E3 Ciclo completo.
// perfil: 1 a 6 (P1 a P6).
// anguloAlvo: na Flexao e no Ciclo completo fica ACIMA do inicial; na Extensao, ABAIXO.
#define ANGULO_MIN_GRAUS   0
#define ANGULO_MAX_GRAUS   90
#define REPETICOES_MAXIMAS 100

// Funcao PURA (PT 4.2.9): sem Arduino, sem Wi-Fi, sem efeito colateral.
// Retorna nullptr se a configuracao e' valida, ou o texto do erro.
const char* validarConfig(int tipoExercicio, int perfil, int anguloInicial, int anguloAlvo, int repeticoes);

#endif
