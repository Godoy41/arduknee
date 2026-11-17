#include "FeedbackDispatcher.h"
#include "ActuatorModule.h"
#include <Arduino.h>

namespace {
  // Grupos de cor da barra (Pinout.h): LEDs 1-2 vermelhos, 3-6 amarelos, 7-10 verdes.
  const uint16_t M_VERMELHO = 0x003;
  const uint16_t M_AMBAR    = 0x03C;
  const uint16_t M_VERDE    = 0x3C0;
  const uint16_t M_TODOS    = 0x3FF;

  struct Passo { uint16_t freqHz; uint16_t ms; };  // freqHz 0 = silencio

  const Passo SOM_BIPE_CURTO[]  = {{2000, 120}};
  const Passo SOM_BIPE_LONGO[]  = {{400, 700}};
  const Passo SOM_RECUSA[]      = {{400, 300}};
  const Passo SOM_REPETICAO[]   = {{2800, 120}};
  const Passo SOM_CALIBRACAO[]  = {{2000, 100}, {2600, 100}};
  const Passo SOM_NEUTRO[]      = {{1200, 150}};
  const Passo SOM_PAUSA[]       = {{1000, 100}};
  const Passo SOM_RETOMADA[]    = {{1800, 100}};
  const Passo SOM_RESET[]       = {{1000, 100}};
  // Escala ascendente duas vezes + nota final = 3000 ms.
  const Passo SOM_COMEMORACAO[] = {
    {1500, 200}, {1800, 200}, {2100, 200}, {2400, 200}, {2700, 200}, {3000, 200},
    {1500, 200}, {1800, 200}, {2100, 200}, {2400, 200}, {2700, 200}, {3000, 200},
    {3200, 600}};

  enum ModoLed { LED_FIXO, LED_PISCA, LED_PULSOS };

  struct Padrao {
    uint16_t mascara;
    ModoLed modo;
    uint16_t ligadoMs, desligadoMs;
    uint8_t pulsos;          // so LED_PULSOS
    uint16_t duracaoMs;      // 0 = sem overlay de LED (so som)
    const Passo* som;
    uint8_t nSom;
  };
  #define N(a) (uint8_t)(sizeof(a) / sizeof((a)[0]))

  const Padrao P_CONFIG_OK     = {M_VERDE,    LED_FIXO,   0,   0,   0, 1000, SOM_BIPE_CURTO, N(SOM_BIPE_CURTO)};
  const Padrao P_CONFIG_INV    = {M_VERMELHO, LED_PISCA,  150, 150, 0, 1500, SOM_BIPE_LONGO, N(SOM_BIPE_LONGO)};
  const Padrao P_CALIB_OK      = {M_VERDE,    LED_PULSOS, 150, 150, 2, 600,  SOM_CALIBRACAO, N(SOM_CALIBRACAO)};
  const Padrao P_CONFIRMAR     = {M_TODOS,    LED_PULSOS, 200, 200, 3, 1200, nullptr, 0};
  const Padrao P_RECUSADO      = {M_VERMELHO, LED_PULSOS, 200, 200, 3, 1200, SOM_RECUSA, N(SOM_RECUSA)};
  const Padrao P_REP_OK        = {M_TODOS,    LED_PULSOS, 100, 100, 2, 400,  SOM_REPETICAO, N(SOM_REPETICAO)};
  const Padrao P_REP_INSTAVEL  = {M_AMBAR,    LED_FIXO,   0,   0,   0, 400,  SOM_REPETICAO, N(SOM_REPETICAO)};
  const Padrao P_META          = {M_TODOS,    LED_PISCA,  100, 100, 0, (uint16_t)DURACAO_COMEMORACAO_MS, SOM_COMEMORACAO, N(SOM_COMEMORACAO)};
  const Padrao P_MANUAL        = {M_TODOS,    LED_FIXO,   0,   0,   0, 1000, SOM_NEUTRO, N(SOM_NEUTRO)};
  const Padrao P_PAUSA         = {0,          LED_FIXO,   0,   0,   0, 0,    SOM_PAUSA, N(SOM_PAUSA)};
  const Padrao P_RETOMADA      = {0,          LED_FIXO,   0,   0,   0, 0,    SOM_RETOMADA, N(SOM_RETOMADA)};
  const Padrao P_INSTAVEL      = {M_AMBAR,    LED_PISCA,  125, 125, 0, 1000, nullptr, 0};
  const Padrao P_RESET         = {0,          LED_FIXO,   0,   0,   0, 300,  SOM_RESET, N(SOM_RESET)};  // todos apagam

  #undef N

  // Instabilidade e' detectada ate 20x/s: limita o aviso a 1 por segundo.
  const unsigned long INTERVALO_MIN_INSTABILIDADE_MS = 1000;
  unsigned long ultimoDisparoInstabilidade = 0;
  bool jaDisparouInstabilidade = false;

  // ---- estado do overlay de LED ----
  bool ovAtivo = false;
  Padrao ov;
  unsigned long ovT0 = 0;

  // ---- estado do som ----
  const Passo* somAtual = nullptr;
  uint8_t somN = 0, somI = 0;
  unsigned long somT0 = 0;
  bool somAtivo = false;

  // ---- exibicao base ----
  ModoExibicao modo = EXIBICAO_AGUARDANDO_CONFIG;
  unsigned long tModo0 = 0;
  float progresso = 0.0f;
  bool instavelExibicao = false;
  float fracRep = 0.0f, fracAmp = 0.0f;

  // ---- servo ----
  float servoRazao = 0.0f;
  bool comemoracao = false;
  unsigned long comemoracaoT0 = 0;

  uint16_t ultimaMascara = 0xFFFF;  // forca a primeira escrita

  float limitar01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

  uint16_t termometro(float p) {
    int n = (int)(limitar01(p) * 10.0f + 0.5f);
    return (uint16_t)((1u << n) - 1u);
  }

  bool ledOverlayLigado(const Padrao& p, unsigned long t) {
    switch (p.modo) {
      case LED_FIXO: return true;
      case LED_PISCA: return (t % (unsigned long)(p.ligadoMs + p.desligadoMs)) < p.ligadoMs;
      case LED_PULSOS: {
        unsigned long ciclo = p.ligadoMs + p.desligadoMs;
        return (t / ciclo) < p.pulsos && (t % ciclo) < p.ligadoMs;
      }
    }
    return false;
  }

  void iniciarSom(const Passo* passos, uint8_t n, unsigned long agora) {
    if (passos == nullptr || n == 0) return;
    somAtual = passos; somN = n; somI = 0; somT0 = agora; somAtivo = true;
    actuatorModuleTom(somAtual[0].freqHz);
  }

  uint16_t mascaraBase(unsigned long agora) {
    switch (modo) {
      case EXIBICAO_AGUARDANDO_CONFIG:
        return ((agora % 2000UL) < 100UL) ? 0x040 : 0;  // LED 7 (verde): batimento de 100 ms a cada 2 s
      case EXIBICAO_AGUARDANDO_INICIO:
        return ((agora % 1000UL) < 500UL) ? M_VERDE : 0;  // verde pisca lentamente
      case EXIBICAO_PAUSA:
        return M_AMBAR;  // ambar fixo, contador preservado
      case EXIBICAO_RESULTADO: {
        unsigned long t = agora - tModo0;
        if ((t % 3000UL) < 150UL) return 0;  // pausa curta: deixa visivel a troca de tela
        return termometro(((t / 3000UL) % 2UL) == 0 ? fracRep : fracAmp);
      }
      case EXIBICAO_PROGRESSO: {
        uint16_t m = feedbackMascaraProgresso(progresso, agora);
        if (instavelExibicao) {
          // instabilidade: LEDs ambar piscam sem interromper o termometro (RF20)
          m = (uint16_t)(m & ~M_AMBAR);
          if ((agora / 125UL) % 2UL == 0) m |= M_AMBAR;
        }
        return m;
      }
    }
    return 0;
  }
}

uint16_t feedbackMascaraProgresso(float p, unsigned long agora) {
  if (p >= 1.0f) return ((agora / 100UL) % 2UL == 0) ? M_TODOS : 0;           // meta: todos piscam rapido
  uint16_t t = termometro(p);
  if (p >= 0.8f) return ((agora / 500UL) % 2UL == 0) ? t : 0;                 // "quase la": pulso lento
  return t;
}

void feedbackInit() {
  ovAtivo = false; somAtivo = false; comemoracao = false;
  servoRazao = 0.0f; modo = EXIBICAO_AGUARDANDO_CONFIG; ultimaMascara = 0xFFFF;
  jaDisparouInstabilidade = false;
  actuatorModuleTom(0);
  actuatorModuleMoverServo(0);
}

void dispararEvento(TipoEvento evento) {
  unsigned long agora = millis();
  const Padrao* p = nullptr;

  switch (evento) {
    case CONFIGURACAO_OK:        p = &P_CONFIG_OK; break;
    case CONFIGURACAO_INVALIDA:  p = &P_CONFIG_INV; break;
    case CALIBRACAO_OK:          p = &P_CALIB_OK; break;
    case CONFIRMAR_INICIO:       p = &P_CONFIRMAR; break;
    case INICIO_RECUSADO:        p = &P_RECUSADO; break;
    case REPETICAO_CORRETA:      p = &P_REP_OK; break;
    case REPETICAO_NAO_CONFIAVEL: p = &P_REP_INSTAVEL; break;
    case ENCERRAMENTO_MANUAL:    p = &P_MANUAL; break;
    case PAUSA:                  p = &P_PAUSA; break;
    case RETOMADA:               p = &P_RETOMADA; break;
    case META_ATINGIDA:
      p = &P_META;
      comemoracao = true;
      comemoracaoT0 = agora;
      break;
    case RESET:
      p = &P_RESET;
      comemoracao = false;       // aborta a comemoracao em curso (PT 5.15)
      servoRazao = 0.0f;         // servo volta a posicao NEUTRA antes de reconfigurar
      break;
    case INSTABILIDADE:
      if (jaDisparouInstabilidade && (agora - ultimoDisparoInstabilidade) < INTERVALO_MIN_INSTABILIDADE_MS) return;
      jaDisparouInstabilidade = true;
      ultimoDisparoInstabilidade = agora;
      p = &P_INSTAVEL;
      break;
    case QUASE_LA: return;  // exibicao de progresso
  }

  if (p->duracaoMs > 0) { ov = *p; ovAtivo = true; ovT0 = agora; }
  else if (evento == RESET) { ovAtivo = false; }
  somAtivo = false;
  actuatorModuleTom(0);
  iniciarSom(p->som, p->nSom, agora);
}

void feedbackDefinirModo(ModoExibicao m) {
  if (m != modo) tModo0 = millis();
  modo = m;
  if (m != EXIBICAO_PROGRESSO) instavelExibicao = false;
}

void feedbackDefinirProgresso(float p, bool instavel) { progresso = limitar01(p); instavelExibicao = instavel; }
void feedbackDefinirResultado(float r, float a) { fracRep = limitar01(r); fracAmp = limitar01(a); tModo0 = millis(); }
void feedbackDefinirServo(float razao) { servoRazao = limitar01(razao); }
bool feedbackComemoracaoAtiva() { return comemoracao; }

void feedbackTick() {
  unsigned long agora = millis();

  // fim do overlay de LED
  if (ovAtivo && (agora - ovT0) >= ov.duracaoMs) {
    ovAtivo = false;
    if (modo == EXIBICAO_RESULTADO) tModo0 = agora;  // a tela de resultado comeca "do zero" apos o overlay
  }
  // fim da comemoracao
  if (comemoracao && (agora - comemoracaoT0) >= DURACAO_COMEMORACAO_MS) comemoracao = false;

  // som: avanca os passos do padrao
  if (somAtivo) {
    while (somAtivo && (agora - somT0) >= somAtual[somI].ms) {
      somT0 += somAtual[somI].ms;
      somI++;
      if (somI >= somN) { somAtivo = false; actuatorModuleTom(0); }
      else actuatorModuleTom(somAtual[somI].freqHz);
    }
  }

  // LEDs
  uint16_t mascara;
  if (ovAtivo) mascara = ledOverlayLigado(ov, agora - ovT0) ? ov.mascara : 0;
  else mascara = mascaraBase(agora);
  if (mascara != ultimaMascara) { actuatorModuleDefinirLeds(mascara); ultimaMascara = mascara; }

  // servo: oscila durante a comemoracao; senao segue repeticoes/meta
  if (comemoracao) actuatorModuleMoverServo((((agora - comemoracaoT0) / 250UL) % 2UL) ? 120 : 180);
  else actuatorModuleMoverServo((uint8_t)(servoRazao * 180.0f + 0.5f));
}
