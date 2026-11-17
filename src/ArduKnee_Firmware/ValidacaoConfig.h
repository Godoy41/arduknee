#ifndef VALIDACAO_CONFIG_H
#define VALIDACAO_CONFIG_H

#include "Tipos.h"

// RF04: limites de projeto do MVP (mapeamento do sensor e arco-guia).
// Repeticoes: apenas "> 0" (decisao da equipe: sem teto de 100).
#define ANGULO_MIN_GRAUS 0
#define ANGULO_MAX_GRAUS 90

// Funcao PURA (PT 4.2.9): sem Arduino, sem Wi-Fi, sem efeito colateral.
// tipoExercicio: 1=E1, 2=E2, 3=E3. perfil: 1 a 6.
// E1/E3: 0 <= inicial < alvo <= 90.   E2: 0 <= alvo < inicial <= 90.
// Retorna nullptr se valida, ou o texto do erro.
const char* validarConfig(int tipoExercicio, int perfil, int anguloInicial, int anguloAlvo, int repeticoes);

// RF40: aviso clinico (E2 + perfil P1/P2/P5 + alvo <= 40). Funcao pura.
bool requerAvisoClinico(int tipoExercicio, int perfil, int anguloAlvo);

extern const char* const MENSAGEM_AVISO_CLINICO;  // texto proposto no RF40 (validar com o fisioterapeuta)

#endif
