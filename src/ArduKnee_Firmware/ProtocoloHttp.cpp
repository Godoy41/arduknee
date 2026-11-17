#include "ProtocoloHttp.h"
#include "ValidacaoConfig.h"
#include "JsonPlano.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace {
  // Mensagens fixas, SEM aspas/barras (entram no JSON sem escape).
  void montar(RespostaHttp& r, int codigo, const char* status, const char* mensagem, const char* extra) {
    r.codigo = codigo;
    if (mensagem != nullptr)
      snprintf(r.corpo, sizeof(r.corpo), "{\"status\":\"%s\",\"mensagem\":\"%s\"%s}", status, mensagem, extra ? extra : "");
    else
      snprintf(r.corpo, sizeof(r.corpo), "{\"status\":\"%s\"%s}", status, extra ? extra : "");
  }

  int perfilDeTexto(const char* t) {   // "P1".."P6"
    if (t[0] == 'P' && t[1] >= '1' && t[1] <= '6' && t[2] == 0) return t[1] - '0';
    return 0;
  }
  int tipoDeTexto(const char* t) {     // "E1".."E3"
    if (t[0] == 'E' && t[1] >= '1' && t[1] <= '3' && t[2] == 0) return t[1] - '0';
    return 0;
  }

  // Aceita so caracteres de data/hora ISO 8601: o valor vai para uma linha de cabecalho do CSV.
  bool timestampSeguro(const char* t) {
    size_t n = strlen(t);
    if (n == 0 || n >= TIMESTAMP_ISO_MAX) return false;
    for (size_t i = 0; i < n; i++) {
      char c = t[i];
      bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                c == ':' || c == '+' || c == '-' || c == '.' || c == ' ';
      if (!ok) return false;
    }
    return true;
  }
}

RespostaHttp protocoloConfigurar(const char* corpoReq, const StatusSnapshot& snap,
                                 ParametrosSessao* aceitos, bool* sinalizarInvalida) {
  RespostaHttp r;
  if (sinalizarInvalida) *sinalizarInvalida = false;

  // RF41: Sessao Ativa/Pausada (e comemoracao) nao aceitam nova configuracao.
  if (snap.estado == SESSAO_ATIVA || snap.estado == SESSAO_PAUSADA || snap.comemorando) {
    montar(r, 409, "conflito", "Sessão em andamento. Encerre a sessão no aparelho antes de reconfigurar.", nullptr);
    return r;
  }

  JsonPlano j;
  if (corpoReq == nullptr || !jsonParsePlano(corpoReq, j)) {
    montar(r, 400, "erro", "JSON inválido.", nullptr);
    if (sinalizarInvalida) *sinalizarInvalida = true;
    return r;
  }

  const CampoJson* fPerfil = jsonBuscar(j, "perfil");
  const CampoJson* fTipo = jsonBuscar(j, "tipoExercicio");
  const CampoJson* fIni = jsonBuscar(j, "anguloInicial");
  const CampoJson* fAlvo = jsonBuscar(j, "anguloAlvo");
  const CampoJson* fReps = jsonBuscar(j, "repeticoes");
  const CampoJson* fTs = jsonBuscar(j, "timestampISO");
  const CampoJson* fConf = jsonBuscar(j, "confirmado");

  const char* erro = nullptr;
  if (!fPerfil || !fTipo || !fIni || !fAlvo || !fReps) erro = "Campo obrigatório ausente.";
  else if (fPerfil->tipo != JSON_TEXTO || fTipo->tipo != JSON_TEXTO) erro = "Perfil e tipo de exercício devem ser texto (ex.: P1 e E1).";
  else if (fIni->tipo != JSON_INTEIRO || fAlvo->tipo != JSON_INTEIRO || fReps->tipo != JSON_INTEIRO) erro = "Ângulos e repetições devem ser números inteiros.";
  else if (fConf && fConf->tipo != JSON_BOOLEANO && fConf->tipo != JSON_NULO) erro = "O campo confirmado deve ser verdadeiro ou falso.";

  ParametrosSessao p;
  memset(&p, 0, sizeof(p));
  if (!erro) {
    p.perfil = perfilDeTexto(fPerfil->texto);
    p.tipoExercicio = tipoDeTexto(fTipo->texto);
    p.anguloInicial = (int)fIni->inteiro;
    p.anguloAlvo = (int)fAlvo->inteiro;
    p.repeticoes = (int)fReps->inteiro;
    erro = validarConfig(p.tipoExercicio, p.perfil, p.anguloInicial, p.anguloAlvo, p.repeticoes);
  }
  if (erro) {
    montar(r, 400, "erro", erro, nullptr);
    if (sinalizarInvalida) *sinalizarInvalida = true;
    return r;
  }

  // RF40: aviso clinico exige confirmacao explicita (reenvio com confirmado:true).
  bool confirmado = (fConf && fConf->tipo == JSON_BOOLEANO && fConf->booleano);
  if (requerAvisoClinico(p.tipoExercicio, p.perfil, p.anguloAlvo) && !confirmado) {
    montar(r, 202, "aviso", MENSAGEM_AVISO_CLINICO, ",\"requerConfirmacao\":true");
    return r;
  }

  // timestampISO e' opcional e NAO afeta a validacao: se vier estranho, e' ignorado.
  if (fTs && fTs->tipo == JSON_TEXTO && timestampSeguro(fTs->texto)) {
    strncpy(p.timestampISO, fTs->texto, TIMESTAMP_ISO_MAX - 1);
  }

  if (aceitos) *aceitos = p;
  montar(r, 200, "sucesso", "Configuração recebida", nullptr);
  return r;
}

RespostaHttp protocoloCalibrar(const char* corpoReq, const StatusSnapshot& snap, int* anguloRef) {
  RespostaHttp r;
  if (snap.estado != AGUARDANDO_INICIO) {
    montar(r, 409, "conflito", "A calibração só é aceita em Aguardando Início.", nullptr);
    return r;
  }

  int ref = 0;  // padrao 0 quando o campo e' omitido
  if (corpoReq != nullptr && corpoReq[0] != 0) {
    JsonPlano j;
    if (!jsonParsePlano(corpoReq, j)) {
      montar(r, 400, "erro", "JSON inválido.", nullptr);
      return r;
    }
    const CampoJson* f = jsonBuscar(j, "anguloReferencia");
    if (f && f->tipo != JSON_NULO) {
      if (f->tipo != JSON_INTEIRO || f->inteiro < 0 || f->inteiro > 90) {
        montar(r, 400, "erro", "Ângulo de referência inválido (use um inteiro de 0 a 90).", nullptr);
        return r;
      }
      ref = (int)f->inteiro;
    }
  }

  if (snap.instavel) {
    montar(r, 503, "instavel", "Leitura instável. Mantenha o joelho parado e repita a captura.", nullptr);
    return r;
  }

  if (anguloRef) *anguloRef = ref;
  montar(r, 200, "sucesso", nullptr, ",\"calibrado\":true");
  return r;
}

size_t protocoloMontarStatus(const StatusSnapshot& s, char* buf, size_t cap) {
  int meta = s.configurado ? s.parametros.repeticoes : 0;
  int n = snprintf(buf, cap,
      "{\"estado\":\"%s\",\"repeticoes\":%d,\"metaRepeticoes\":%d,\"anguloAtual\":%d,"
      "\"amplitudeMaxima\":%d,\"sessaoConcluida\":%s,\"calibrado\":%s,\"configurado\":%s",
      nomeEstado(s.estado), s.repeticoes, meta, (int)lroundf(s.anguloAtual),
      (int)lroundf(s.amplitudeMaxima), s.sessaoConcluida ? "true" : "false",
      s.calibrado ? "true" : "false", s.configurado ? "true" : "false");
  if (n < 0 || (size_t)n >= cap) return 0;
  size_t len = (size_t)n;
  if (s.configurado) {   // "parametros" so existe quando ha configuracao vigente
    n = snprintf(buf + len, cap - len,
        ",\"parametros\":{\"perfil\":\"P%d\",\"tipoExercicio\":\"E%d\",\"anguloInicial\":%d,"
        "\"anguloAlvo\":%d,\"repeticoes\":%d}",
        s.parametros.perfil, s.parametros.tipoExercicio, s.parametros.anguloInicial,
        s.parametros.anguloAlvo, s.parametros.repeticoes);
    if (n < 0 || len + (size_t)n >= cap) return 0;
    len += (size_t)n;
  }
  if (len + 2 > cap) return 0;
  buf[len++] = '}';
  buf[len] = 0;
  return len;
}
