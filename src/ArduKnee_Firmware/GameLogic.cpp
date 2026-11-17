#include "GameLogic.h"
#include "DetectorRepeticao.h"
#include "FeedbackDispatcher.h"
#include "CsvExport.h"
#include "Armazenamento.h"
#include "Log.h"
#include <Arduino.h>
#include <string.h>
#include <math.h>

namespace {
  EstadoSessao estado = AGUARDANDO_CONFIGURACAO;
  unsigned long tEntradaEstado = 0;   // base dos timeouts de 5 min

  ParametrosSessao params;
  bool configurado = false;
  unsigned long tConfigMs = 0;        // timestamp_config_ms do CSV

  bool confirmacaoArmada = false;     // RF30: 1a CURTA ja deu
  unsigned long tConfirmacaoMs = 0;

  DetectorRepeticao detector;
  int repeticoes = 0;
  float amplitudeMaxima = 0.0f;
  bool concluidaPorMeta = false;
  unsigned long tFimSessao = 0;

  LeituraSensor ultima = {0, 0, 0.0f, false, false};
  bool temUltima = false;
  unsigned long tUltimaInstabilidade = 0;
  bool houveInstabilidade = false;

  uint32_t sessaoId = 0;
  bool csvAberto = false;             // entre INICIO e o encerramento/reset

  float limitar01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

  bool instabilidadeAtiva(unsigned long agora) {
    return houveInstabilidade && (agora - tUltimaInstabilidade) < JANELA_INSTABILIDADE_MS;
  }

  bool comemorando(unsigned long agora) {
    return estado == SESSAO_ENCERRADA && concluidaPorMeta && (agora - tFimSessao) < DURACAO_COMEMORACAO_MS;
  }

  void entrarEstado(EstadoSessao novo, unsigned long agora) {
    logInfo("[GameLogic] %s -> %s", nomeEstadoCsv(estado), nomeEstadoCsv(novo));
    estado = novo;
    tEntradaEstado = agora;
  }

  // Linha de evento no CSV (so existe sessao CSV entre INICIO e o fim).
  void emitirEvento(const char* evento, long duracaoRepMs) {
    if (!csvAberto) return;
    csvExportLinha(millis(), ultima.adcBruto, ultima.anguloFiltrado, nomeEstadoCsv(estado), evento, duracaoRepMs);
  }

  float fracaoRepeticoes() {
    return (configurado && params.repeticoes > 0) ? limitar01((float)repeticoes / (float)params.repeticoes) : 0.0f;
  }
  float fracaoAmplitude() {
    float arco = fabsf((float)(params.anguloAlvo - params.anguloInicial));
    return (configurado && arco > 0.0f) ? limitar01(amplitudeMaxima / arco) : 0.0f;
  }

  void invalidarParametrosECalibracao() {
    configurado = false;
    memset(&params, 0, sizeof(params));
    confirmacaoArmada = false;
    sensorModuleInvalidarCalibracao();
  }

  void iniciarSessao(unsigned long agora) {
    repeticoes = 0;                 // RF08: contador e amplitude zeram a cada sessao
    amplitudeMaxima = 0.0f;
    concluidaPorMeta = false;
    confirmacaoArmada = false;
    detector.iniciar(params.tipoExercicio, params.anguloInicial, params.anguloAlvo);
    sessaoId = armazenamentoProximoSessaoId();
    feedbackDefinirServo(0.0f);
    feedbackDefinirProgresso(0.0f, false);
    feedbackDefinirModo(EXIBICAO_PROGRESSO);

    CsvCabecalho c;
    c.sessaoId = sessaoId;
    c.perfil = params.perfil;
    c.tipoExercicio = params.tipoExercicio;
    c.anguloInicial = params.anguloInicial;
    c.anguloAlvo = params.anguloAlvo;
    c.repeticoes = params.repeticoes;
    c.anguloReferencia = sensorModuleAnguloReferencia();
    c.adcReferencia = sensorModuleAdcReferencia();
    c.timestampConfigMs = tConfigMs;
    c.timestampWallClock = params.timestampISO;
    csvExportIniciarSessao(c);
    csvAberto = true;

    entrarEstado(SESSAO_ATIVA, agora);
    emitirEvento("INICIO", -1);
  }

  void pausar(unsigned long agora) {
    entrarEstado(SESSAO_PAUSADA, agora);
    feedbackDefinirModo(EXIBICAO_PAUSA);
    dispararEvento(PAUSA);
    emitirEvento("PAUSA", -1);
  }

  void retomar(unsigned long agora) {
    entrarEstado(SESSAO_ATIVA, agora);
    feedbackDefinirModo(EXIBICAO_PROGRESSO);
    dispararEvento(RETOMADA);
    emitirEvento("RETOMADA", -1);
  }

  void encerrar(bool porMeta, unsigned long agora) {
    concluidaPorMeta = porMeta;
    tFimSessao = agora;
    entrarEstado(SESSAO_ENCERRADA, agora);
    emitirEvento(porMeta ? "ENCERRAMENTO_META" : "ENCERRAMENTO_MANUAL", -1);
    csvAberto = false;
    feedbackDefinirResultado(fracaoRepeticoes(), fracaoAmplitude());
    feedbackDefinirModo(EXIBICAO_RESULTADO);
    dispararEvento(porMeta ? META_ATINGIDA : ENCERRAMENTO_MANUAL);  // Wi-Fi continua ativo (RF23)
  }

  // Reset LONGA: descarta a sessao e os parametros; aborta a comemoracao e poe o servo no neutro.
  void resetar(unsigned long agora) {
    if (estado == AGUARDANDO_CONFIGURACAO) return;  // tabela do UML: no-op
    emitirEvento("RESET", -1);
    csvAberto = false;
    repeticoes = 0;
    amplitudeMaxima = 0.0f;
    concluidaPorMeta = false;
    invalidarParametrosECalibracao();
    entrarEstado(AGUARDANDO_CONFIGURACAO, agora);
    feedbackDefinirModo(EXIBICAO_AGUARDANDO_CONFIG);
    dispararEvento(RESET);
  }

  void tratarIniciarCurta(unsigned long agora) {
    // Sem parametros ou sem calibracao: recusa e NAO arma o timer (UC14 / RF30).
    if (!configurado || !sensorModuleCalibrado()) {
      confirmacaoArmada = false;
      dispararEvento(INICIO_RECUSADO);
      return;
    }
    if (confirmacaoArmada && (agora - tConfirmacaoMs) <= JANELA_CONFIRMACAO_MS) {
      iniciarSessao(agora);        // 2a CURTA dentro de 5 s
      return;
    }
    confirmacaoArmada = true;      // 1a CURTA (ou a janela anterior ja expirou)
    tConfirmacaoMs = agora;
    dispararEvento(CONFIRMAR_INICIO);
  }
}

void gameLogicInit() {
  estado = AGUARDANDO_CONFIGURACAO;
  tEntradaEstado = millis();
  repeticoes = 0;
  amplitudeMaxima = 0.0f;
  concluidaPorMeta = false;
  csvAberto = false;
  houveInstabilidade = false;
  temUltima = false;
  invalidarParametrosECalibracao();   // o boot nunca restaura parametros nem calibracao
  feedbackDefinirModo(EXIBICAO_AGUARDANDO_CONFIG);
  logInfo("[GameLogic] pronto: %s", nomeEstadoCsv(estado));
}

void gameLogicProcessarEvento(EventoBotao ev) {
  unsigned long agora = millis();

  if (ev.idBotao == BOTAO_RESET) {            // Reset CURTA e' ignorado; so LONGA reseta
    if (ev.tipo == LONGA) resetar(agora);
    return;
  }
  if (ev.idBotao == BOTAO_INICIAR) {
    if (ev.tipo != CURTA) return;             // Iniciar LONGA: no-op em todos os estados
    if (estado == AGUARDANDO_INICIO) tratarIniciarCurta(agora);
    else if (estado == SESSAO_PAUSADA) retomar(agora);
    return;
  }
  // BOTAO_ENCERRAR
  if (ev.tipo == CURTA) {
    if (estado == SESSAO_ATIVA) pausar(agora);
    return;
  }
  if (estado == SESSAO_ATIVA || estado == SESSAO_PAUSADA) encerrar(false, agora);
}

void gameLogicProcessarLeitura(const LeituraSensor& l) {
  unsigned long agora = millis();
  ultima = l;
  temUltima = true;

  bool subiuInstabilidade = false;
  if (l.instavel) {
    subiuInstabilidade = !instabilidadeAtiva(agora);
    tUltimaInstabilidade = agora;
    houveInstabilidade = true;
  }

  if (estado == AGUARDANDO_INICIO) {
    if (l.instavel) dispararEvento(INSTABILIDADE);   // sinal ambar (limitado a 1/s)
    return;
  }
  if (estado != SESSAO_ATIVA) return;

  float dev = fabsf(l.anguloFiltrado - (float)params.anguloInicial);
  if (dev > amplitudeMaxima) amplitudeMaxima = dev;

  if (subiuInstabilidade) emitirEvento("INSTABILIDADE", -1);

  unsigned long duracao = 0;
  bool completou = detector.atualizar(l.anguloFiltrado, agora, &duracao);
  bool instavelAgora = instabilidadeAtiva(agora);

  if (completou) {
    repeticoes++;
    feedbackDefinirServo(fracaoRepeticoes());
    emitirEvento("REPETICAO", (long)duracao);
    if (repeticoes >= params.repeticoes) {
      encerrar(true, agora);                          // meta atingida: comemoracao
      return;
    }
    dispararEvento(instavelAgora ? REPETICAO_NAO_CONFIAVEL : REPETICAO_CORRETA);
  } else {
    csvExportLinha(agora, l.adcBruto, l.anguloFiltrado, nomeEstadoCsv(estado), "", -1);
  }
  feedbackDefinirProgresso(detector.progresso(), instavelAgora);
}

void gameLogicTick() {
  unsigned long agora = millis();
  switch (estado) {
    case AGUARDANDO_INICIO:
      if (confirmacaoArmada && (agora - tConfirmacaoMs) > JANELA_CONFIRMACAO_MS) {
        confirmacaoArmada = false;                    // janela de 5 s expirou sem 2a CURTA
        logInfo("[GameLogic] janela de confirmacao expirou");
      }
      if ((agora - tEntradaEstado) >= TIMEOUT_AGUARDANDO_MS) {
        // RF28: timeout invalida parametros E calibracao
        invalidarParametrosECalibracao();
        entrarEstado(AGUARDANDO_CONFIGURACAO, agora);
        feedbackDefinirModo(EXIBICAO_AGUARDANDO_CONFIG);
      }
      break;
    case AGUARDANDO_CONFIGURACAO:
      if ((agora - tEntradaEstado) >= TIMEOUT_AGUARDANDO_MS) {
        tEntradaEstado = agora;                       // self-loop: so registra
        logInfo("[GameLogic] timeout de 5 min em Aguardando Configuracao (sem efeito)");
      }
      break;
    default:
      break;
  }
}

EstadoSessao gameLogicGetEstado() { return estado; }

bool gameLogicAplicarConfig(const ParametrosSessao& p) {
  unsigned long agora = millis();
  bool aceita = (estado == AGUARDANDO_CONFIGURACAO) || (estado == AGUARDANDO_INICIO) ||
                (estado == SESSAO_ENCERRADA && !comemorando(agora));
  if (!aceita) {
    logInfo("[GameLogic] configuracao descartada (estado %s)", nomeEstadoCsv(estado));
    return false;
  }
  params = p;
  configurado = true;
  tConfigMs = agora;
  confirmacaoArmada = false;          // nova config cancela o timer de 5 s: ritual recomeca (RF30-CA5)
  armazenamentoSalvarUltimaConfig(p);
  if (estado != AGUARDANDO_INICIO) entrarEstado(AGUARDANDO_INICIO, agora);
  else tEntradaEstado = agora;        // atividade do fisioterapeuta reinicia o timeout de 5 min
  feedbackDefinirModo(EXIBICAO_AGUARDANDO_INICIO);
  dispararEvento(CONFIGURACAO_OK);
  return true;
}

ResultadoCalibracao gameLogicCalibrar(int anguloReferencia) {
  unsigned long agora = millis();
  if (estado != AGUARDANDO_INICIO) return CALIBRACAO_ESTADO_INVALIDO;
  if (instabilidadeAtiva(agora)) {
    dispararEvento(INSTABILIDADE);                    // UC14-A3: sinal ambar, repetir a captura
    return CALIBRACAO_INSTAVEL;
  }
  if (!sensorModuleCapturarReferencia(anguloReferencia)) return CALIBRACAO_SEM_LEITURA;
  confirmacaoArmada = false;          // a confirmacao vale para parametros + calibracao vigentes
  tEntradaEstado = agora;
  dispararEvento(CALIBRACAO_OK);
  return CALIBRACAO_ACEITA;
}

void gameLogicObterSnapshot(StatusSnapshot& s) {
  unsigned long agora = millis();
  s.estado = estado;
  s.repeticoes = repeticoes;
  s.metaRepeticoes = configurado ? params.repeticoes : 0;
  s.calibrado = sensorModuleCalibrado();
  s.anguloAtual = (s.calibrado && temUltima) ? ultima.anguloFiltrado : 0.0f;
  s.amplitudeMaxima = amplitudeMaxima;
  s.sessaoConcluida = (estado == SESSAO_ENCERRADA) && concluidaPorMeta;
  s.configurado = configurado;
  s.instavel = instabilidadeAtiva(agora);
  s.comemorando = comemorando(agora);
  s.parametros = params;
}
