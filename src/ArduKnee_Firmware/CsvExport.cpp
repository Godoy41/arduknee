#include "CsvExport.h"
#include <Arduino.h>
#include <stdio.h>
#include <string.h>

namespace {
  // Fila de saida em ordem. Quando esvazia, volta ao inicio (sem fragmentacao).
  char fila[1024];
  size_t filaTam = 0;
  size_t filaLida = 0;

  bool filaVazia() { return filaLida >= filaTam; }

  bool anexar(const char* dados, size_t n) {
    if (filaVazia()) { filaTam = 0; filaLida = 0; }
    if (filaTam + n > sizeof(fila)) return false;   // cheia: descarta a linha inteira
    memcpy(fila + filaTam, dados, n);
    filaTam += n;
    return true;
  }
}

size_t csvFormatarCabecalho(char* buf, size_t cap, const CsvCabecalho& c) {
  int n = snprintf(buf, cap,
      "# sessao_id=%lu\r\n"
      "# perfil=P%d\r\n"
      "# tipo_exercicio=E%d\r\n"
      "# angulo_inicial=%d\r\n"
      "# angulo_alvo=%d\r\n"
      "# repeticoes=%d\r\n"
      "# angulo_referencia=%d\r\n"
      "# adc_referencia=%u\r\n"
      "# timestamp_config_ms=%lu\r\n",
      (unsigned long)c.sessaoId, c.perfil, c.tipoExercicio, c.anguloInicial, c.anguloAlvo,
      c.repeticoes, c.anguloReferencia, (unsigned)c.adcReferencia, c.timestampConfigMs);
  if (n < 0 || (size_t)n >= cap) return 0;
  size_t len = (size_t)n;
  if (c.timestampWallClock != nullptr && c.timestampWallClock[0] != 0) {
    n = snprintf(buf + len, cap - len, "# timestamp_wall_clock=%s\r\n", c.timestampWallClock);
    if (n < 0 || len + (size_t)n >= cap) return 0;
    len += (size_t)n;
  }
  n = snprintf(buf + len, cap - len,
               "timestamp_ms,adc_bruto,angulo_filtrado_graus,estado_sessao,evento,duracao_rep_ms\r\n");
  if (n < 0 || len + (size_t)n >= cap) return 0;
  return len + (size_t)n;
}

size_t csvFormatarLinha(char* buf, size_t cap, unsigned long ts, uint16_t adc, float ang,
                        const char* estadoCsv, const char* evento, long duracaoRepMs) {
  int n;
  if (duracaoRepMs >= 0) {
    n = snprintf(buf, cap, "%lu,%u,%.1f,%s,%s,%ld\r\n", ts, (unsigned)adc, (double)ang,
                 estadoCsv, evento ? evento : "", duracaoRepMs);
  } else {
    n = snprintf(buf, cap, "%lu,%u,%.1f,%s,%s,\r\n", ts, (unsigned)adc, (double)ang,
                 estadoCsv, evento ? evento : "");
  }
  if (n < 0 || (size_t)n >= cap) return 0;
  return (size_t)n;
}

void csvExportIniciarSessao(const CsvCabecalho& cab) {
  char buf[704];
  filaTam = 0; filaLida = 0;                       // descarta resto de uma sessao anterior
  size_t n = csvFormatarCabecalho(buf, sizeof(buf), cab);
  if (n > 0) anexar(buf, n);
  csvExportTick();                                 // ja manda o que couber no buffer da serial
}

bool csvExportPendente() { return !filaVazia(); }

void csvExportTick() {
  if (filaVazia()) return;
  int livre = Serial.availableForWrite();
  if (livre <= 0) return;                          // terminal lento: tenta de novo no proximo loop
  size_t resto = filaTam - filaLida;
  size_t quanto = ((size_t)livre < resto) ? (size_t)livre : resto;
  Serial.write((const uint8_t*)(fila + filaLida), quanto);
  filaLida += quanto;
}

bool csvExportLinha(unsigned long ts, uint16_t adc, float ang, const char* estadoCsv,
                    const char* evento, long duracaoRepMs) {
  char buf[112];
  size_t n = csvFormatarLinha(buf, sizeof(buf), ts, adc, ang, estadoCsv, evento, duracaoRepMs);
  if (n == 0) return false;
  if (!filaVazia()) return anexar(buf, n);         // mantem a ordem atras do que ainda esta saindo
  if ((size_t)Serial.availableForWrite() < n) return false;   // buffer cheio: descarta a linha (RF34)
  Serial.write((const uint8_t*)buf, n);
  return true;
}
