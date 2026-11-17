#include "JsonPlano.h"
#include <string.h>

namespace {
  const char* pularEspacos(const char* p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
  }

  // Le uma string JSON em "saida" (capacidade cap). Retorna ponteiro apos a aspa final, ou nullptr.
  const char* lerString(const char* p, char* saida, int cap) {
    if (*p != '"') return nullptr;
    p++;
    int n = 0;
    while (*p != '"') {
      unsigned char c = (unsigned char)*p;
      if (c == 0 || c < 0x20) return nullptr;  // fim inesperado ou caractere de controle
      if (c == '\\') {
        p++;
        switch (*p) {
          case '"': c = '"'; break;
          case '\\': c = '\\'; break;
          case '/': c = '/'; break;
          case 'n': c = '\n'; break;
          case 't': c = '\t'; break;
          case 'r': c = '\r'; break;
          case 'b': c = '\b'; break;
          case 'f': c = '\f'; break;
          default: return nullptr;  // inclui \uXXXX (nao suportado)
        }
      }
      if (n >= cap - 1) return nullptr;  // texto longo demais
      saida[n++] = (char)c;
      p++;
    }
    saida[n] = 0;
    return p + 1;
  }

  bool igualLiteral(const char* p, const char* lit) {
    size_t n = strlen(lit);
    return strncmp(p, lit, n) == 0;
  }
}

bool jsonParsePlano(const char* texto, JsonPlano& j) {
  j.n = 0;
  if (texto == nullptr) return false;
  const char* p = pularEspacos(texto);
  if (*p != '{') return false;
  p = pularEspacos(p + 1);
  if (*p == '}') return *pularEspacos(p + 1) == 0;  // objeto vazio

  for (;;) {
    if (j.n >= JSON_MAX_CAMPOS) return false;
    CampoJson& c = j.campos[j.n];
    memset(&c, 0, sizeof(c));

    p = pularEspacos(p);
    p = lerString(p, c.chave, JSON_MAX_CHAVE);
    if (!p) return false;
    p = pularEspacos(p);
    if (*p != ':') return false;
    p = pularEspacos(p + 1);

    if (*p == '"') {
      c.tipo = JSON_TEXTO;
      p = lerString(p, c.texto, JSON_MAX_TEXTO);
      if (!p) return false;
    } else if (igualLiteral(p, "true") ) { c.tipo = JSON_BOOLEANO; c.booleano = true; p += 4; }
    else if (igualLiteral(p, "false")) { c.tipo = JSON_BOOLEANO; c.booleano = false; p += 5; }
    else if (igualLiteral(p, "null"))  { c.tipo = JSON_NULO; p += 4; }
    else if (*p == '-' || (*p >= '0' && *p <= '9')) {
      bool negativo = false;
      if (*p == '-') { negativo = true; p++; }
      if (!(*p >= '0' && *p <= '9')) return false;
      if (*p == '0' && p[1] >= '0' && p[1] <= '9') return false;  // zero a esquerda
      long v = 0; int digitos = 0;
      while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); digitos++; p++; if (digitos > 9) return false; }
      if (*p == '.' || *p == 'e' || *p == 'E') {
        // numero nao inteiro: aceito como JSON_REAL (a camada de cima decide rejeitar)
        while ((*p >= '0' && *p <= '9') || *p == '.' || *p == 'e' || *p == 'E' || *p == '+' || *p == '-') p++;
        c.tipo = JSON_REAL;
      } else {
        c.tipo = JSON_INTEIRO;
        c.inteiro = negativo ? -v : v;
      }
    } else {
      return false;  // objeto/array aninhado ou lixo
    }

    for (int i = 0; i < j.n; i++) if (strcmp(j.campos[i].chave, c.chave) == 0) return false;  // duplicada
    j.n++;

    p = pularEspacos(p);
    if (*p == ',') { p++; continue; }
    if (*p == '}') return *pularEspacos(p + 1) == 0;
    return false;
  }
}

const CampoJson* jsonBuscar(const JsonPlano& j, const char* chave) {
  for (int i = 0; i < j.n; i++) if (strcmp(j.campos[i].chave, chave) == 0) return &j.campos[i];
  return nullptr;
}
