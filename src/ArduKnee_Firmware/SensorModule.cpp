#include "SensorModule.h"
#include "Pinout.h"
#include "Log.h"

namespace {
  const uint8_t N_AMOSTRAS = 8;                    // tamanho da media movel
  const uint16_t LIMIAR_INSTABILIDADE = 500;       // variacao entre AMOSTRAS CRUAS consecutivas
  // Se ficou mais que isto sem ler (ex.: sessao pausada), a media e a "amostra anterior"
  // estao velhas: reinicia o filtro para nao gerar um falso "instavel" na retomada.
  const unsigned long LACUNA_REINICIA_FILTRO_MS = 500;

  uint16_t buffer[N_AMOSTRAS] = {0};
  uint8_t indiceBuffer = 0;
  bool primeiraLeitura = true;
  bool temLeitura = false;
  uint16_t ultimaAmostraBruta = 0;
  uint16_t ultimoFiltrado = 0;
  unsigned long ultimaLeituraMs = 0;

  bool calibrado = false;
  uint16_t adcRef = 0;
  int anguloRef = 0;
}

float converterAdcParaGraus(uint16_t adc, uint16_t adcReferencia, float anguloReferencia, float k) {
  float a = anguloReferencia + k * ((float)adc - (float)adcReferencia);
  if (a < 0.0f) a = 0.0f;
  if (a > 90.0f) a = 90.0f;
  return a;
}

void sensorModuleInit() {
  analogReadResolution(12);                                // 0-4095
  analogSetPinAttenuation(PINO_POTENCIOMETRO, ADC_11db);   // faixa ~0-3,3 V (circuito v2.7)
  pinMode(PINO_POTENCIOMETRO, INPUT);
  sensorModuleInvalidarCalibracao();
  logInfo("[SensorModule] pronto. Aguardando calibracao (POST /calibrar).");
}

static uint16_t atualizarMediaMovel(uint16_t amostra) {
  if (primeiraLeitura) {
    for (uint8_t i = 0; i < N_AMOSTRAS; i++) buffer[i] = amostra;  // media nao comeca enviesada
  } else {
    buffer[indiceBuffer] = amostra;
    indiceBuffer = (indiceBuffer + 1) % N_AMOSTRAS;
  }
  uint32_t soma = 0;
  for (uint8_t i = 0; i < N_AMOSTRAS; i++) soma += buffer[i];
  return (uint16_t)(soma / N_AMOSTRAS);
}

LeituraSensor sensorModuleLer() {
  unsigned long agora = millis();
  if (!primeiraLeitura && (agora - ultimaLeituraMs) > LACUNA_REINICIA_FILTRO_MS) {
    primeiraLeitura = true;
  }
  ultimaLeituraMs = agora;

  LeituraSensor l;
  l.adcBruto = analogRead(PINO_POTENCIOMETRO);
  l.adcFiltrado = atualizarMediaMovel(l.adcBruto);
  ultimoFiltrado = l.adcFiltrado;
  l.calibrado = calibrado;
  l.anguloFiltrado = calibrado
      ? converterAdcParaGraus(l.adcFiltrado, adcRef, (float)anguloRef, SENSOR_K_NOMINAL)
      : 0.0f;

  // RF20: a instabilidade compara amostras CRUAS (a media movel esconderia saltos).
  if (primeiraLeitura) {
    l.instavel = false;  // sem amostra anterior: sem alarme falso
    primeiraLeitura = false;
  } else {
    int32_t variacao = abs((int32_t)l.adcBruto - (int32_t)ultimaAmostraBruta);
    l.instavel = variacao > LIMIAR_INSTABILIDADE;
  }
  ultimaAmostraBruta = l.adcBruto;
  temLeitura = true;
  return l;
}

bool sensorModuleCapturarReferencia(int anguloReferencia) {
  if (!temLeitura) return false;
  if (anguloReferencia < 0) anguloReferencia = 0;
  if (anguloReferencia > 90) anguloReferencia = 90;
  adcRef = ultimoFiltrado;
  anguloRef = anguloReferencia;
  calibrado = true;
  logInfo("[SensorModule] calibrado: ADC_ref=%u angulo_ref=%d", (unsigned)adcRef, anguloRef);
  return true;
}

void sensorModuleInvalidarCalibracao() {
  calibrado = false;
  adcRef = 0;
  anguloRef = 0;
}

bool sensorModuleCalibrado() { return calibrado; }
int sensorModuleAnguloReferencia() { return anguloRef; }
uint16_t sensorModuleAdcReferencia() { return adcRef; }
