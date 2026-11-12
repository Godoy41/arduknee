#include "SensorModule.h"
#include "Pinout.h"

namespace {
  const uint8_t N_AMOSTRAS = 8;               // tamanho da media movel
  const uint16_t LIMIAR_INSTABILIDADE = 500;  // variacao entre AMOSTRAS CRUAS consecutivas

  // ===== CALIBRACAO (Etapa 2 do guia) =====
  // 1) Rode CalibracaoSensor.ino e anote o valor em 0 e em 90 graus (com transferidor).
  // 2) Escreva os dois valores abaixo e mude CALIBRACAO_FEITA para true.
  // Funciona tambem com o potenciometro montado invertido (0 grau com ADC alto).
  const bool CALIBRACAO_FEITA = false;
  const uint16_t ADC_EM_0_GRAUS = 100;    // <-- placeholder, substitua pelo valor medido
  const uint16_t ADC_EM_90_GRAUS = 3950;  // <-- placeholder, substitua pelo valor medido

  uint16_t buffer[N_AMOSTRAS] = {0};
  uint8_t indiceBuffer = 0;
  bool primeiraLeitura = true;
  uint16_t ultimaAmostraBruta = 0;
}

void sensorModuleInit() {
  analogReadResolution(12);                                   // 0-4095
  analogSetPinAttenuation(PINO_POTENCIOMETRO, ADC_11db);      // faixa ~0-3,3 V (circuito v2.7)
  pinMode(PINO_POTENCIOMETRO, INPUT);

  if (!CALIBRACAO_FEITA) {
    Serial.println("[SensorModule] AVISO: sensor NAO calibrado - os angulos sao apenas estimativas.");
  }
}

static uint16_t atualizarMediaMovel(uint16_t amostra) {
  if (primeiraLeitura) {
    // Enche o buffer com a primeira amostra para a media nao comecar enviesada.
    for (uint8_t i = 0; i < N_AMOSTRAS; i++) buffer[i] = amostra;
  } else {
    buffer[indiceBuffer] = amostra;
    indiceBuffer = (indiceBuffer + 1) % N_AMOSTRAS;
  }

  uint32_t soma = 0;
  for (uint8_t i = 0; i < N_AMOSTRAS; i++) soma += buffer[i];
  return (uint16_t)(soma / N_AMOSTRAS);
}

// Mapeamento linear usando os dois pontos medidos, limitado a 0-90 graus.
static float converterParaGraus(uint16_t adc) {
  if (ADC_EM_90_GRAUS == ADC_EM_0_GRAUS) return 0.0f;  // evita divisao por zero
  float angulo = (float)((int32_t)adc - (int32_t)ADC_EM_0_GRAUS) * 90.0f
                 / (float)((int32_t)ADC_EM_90_GRAUS - (int32_t)ADC_EM_0_GRAUS);
  if (angulo < 0.0f) angulo = 0.0f;
  if (angulo > 90.0f) angulo = 90.0f;
  return angulo;
}

LeituraSensor sensorModuleLer() {
  LeituraSensor leitura;
  leitura.adcBruto = analogRead(PINO_POTENCIOMETRO);
  leitura.adcFiltrado = atualizarMediaMovel(leitura.adcBruto);
  leitura.anguloFiltrado = converterParaGraus(leitura.adcFiltrado);

  // RF20: a instabilidade compara amostras CRUAS (a media movel suaviza demais e
  // esconderia saltos). Na primeira leitura nao ha amostra anterior: sem alarme falso.
  if (primeiraLeitura) {
    leitura.instavel = false;
    primeiraLeitura = false;
  } else {
    int32_t variacao = abs((int32_t)leitura.adcBruto - (int32_t)ultimaAmostraBruta);
    leitura.instavel = variacao > LIMIAR_INSTABILIDADE;
  }
  ultimaAmostraBruta = leitura.adcBruto;

  return leitura;
}
