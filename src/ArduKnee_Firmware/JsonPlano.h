#ifndef JSON_PLANO_H
#define JSON_PLANO_H

// Leitor MINIMO de JSON "plano": um unico objeto cujos valores sao texto, inteiro,
// booleano ou null. Objetos/arrays aninhados sao REJEITADOS (o contrato do POST nao
// os usa). Sem alocacao dinamica, sem Arduino: funciona no PC e no ESP32.

#define JSON_MAX_CAMPOS 10
#define JSON_MAX_CHAVE  24
#define JSON_MAX_TEXTO  40

enum TipoJson { JSON_TEXTO, JSON_INTEIRO, JSON_REAL, JSON_BOOLEANO, JSON_NULO };

struct CampoJson {
  char chave[JSON_MAX_CHAVE];
  TipoJson tipo;
  char texto[JSON_MAX_TEXTO];  // JSON_TEXTO
  long inteiro;                // JSON_INTEIRO (ate 9 digitos)
  bool booleano;               // JSON_BOOLEANO
};

struct JsonPlano {
  CampoJson campos[JSON_MAX_CAMPOS];
  int n;
};

// true se o texto e' um objeto JSON plano valido. Chave repetida = invalido.
bool jsonParsePlano(const char* texto, JsonPlano& saida);
const CampoJson* jsonBuscar(const JsonPlano& j, const char* chave);

#endif
