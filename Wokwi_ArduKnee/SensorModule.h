#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H

#include <Arduino.h>

// Resultado de uma leitura do sensor (guia, secao 7)
struct LeituraSensor {
  uint16_t adcBruto;      // ultima amostra crua do ADC (0-4095)
  uint16_t adcFiltrado;   // media movel das ultimas amostras (0-4095)
  float anguloFiltrado;   // 0.0-90.0 graus, calculado a partir do valor filtrado
  bool instavel;          // true se a amostra crua variou mais que o limiar (RF20)
};

void sensorModuleInit();
LeituraSensor sensorModuleLer();

#endif
