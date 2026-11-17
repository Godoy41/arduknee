#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <Arduino.h>

// ---- Conversao ADC -> graus (RF09 / UML v2.5) ----
//   angulo = anguloRef + k * (ADC - ADC_ref),  limitado a 0-90 graus
// k nominal = SENSOR_SINAL_K * SENSOR_CURSO_ELETRICO_GRAUS / 4095.
// O SINAL de k depende da montagem do potenciometro: confirme na bancada (PT 5.1) com
// CalibracaoSensor.ino - se o ADC DIMINUI quando o joelho flexiona, troque para -1.0f.
#define SENSOR_SINAL_K               (+1.0f)
#define SENSOR_CURSO_ELETRICO_GRAUS  280.0f   // curso tipico de um potenciometro de 270-300 graus
#define SENSOR_K_NOMINAL             (SENSOR_SINAL_K * SENSOR_CURSO_ELETRICO_GRAUS / 4095.0f)

struct LeituraSensor {
  uint16_t adcBruto;      // ultima amostra crua do ADC (0-4095)
  uint16_t adcFiltrado;   // media movel das ultimas amostras (0-4095)
  float anguloFiltrado;   // 0.0-90.0 graus; 0 enquanto NAO calibrado
  bool instavel;          // true se a amostra crua variou mais que o limiar (RF20)
  bool calibrado;
};

// Funcao pura (testavel no PC).
float converterAdcParaGraus(uint16_t adc, uint16_t adcRef, float anguloRef, float k);

void sensorModuleInit();
LeituraSensor sensorModuleLer();

// Calibracao em tempo de execucao (POST /calibrar, UC14): registra ADC_ref = ultimo ADC
// filtrado e anguloRef. Retorna false se ainda nao houve nenhuma leitura.
bool sensorModuleCapturarReferencia(int anguloReferencia);
// Boot, Reset e timeout de 5 min invalidam a calibracao (RF09).
void sensorModuleInvalidarCalibracao();
bool sensorModuleCalibrado();
int sensorModuleAnguloReferencia();
uint16_t sensorModuleAdcReferencia();

#endif
