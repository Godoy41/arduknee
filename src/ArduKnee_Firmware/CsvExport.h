#ifndef CSV_EXPORT_H
#define CSV_EXPORT_H

#include <stddef.h>
#include <stdint.h>

// Exportacao CSV pela serial USB (RF29, RF34) - NAO BLOQUEANTE.
//  - Cabecalho e linhas de evento passam por uma FILA pequena (1 KB) em ordem: o cabecalho e'
//    maior que o buffer da serial, entao sai aos poucos por csvExportTick() e a linha
//    INICIO que vem logo depois espera atras dele (nao e' perdida).
//  - Se a fila esta vazia e a linha nao cabe no buffer da serial, a linha INTEIRA e'
//    descartada. Se a fila esta cheia, tambem (RF34). Nunca ha linha parcial.
//  - Sem terminal conectado o jogo segue igual (US17-CA3).

struct CsvCabecalho {
  uint32_t sessaoId;
  int perfil;              // 1..6  -> "P1".."P6"
  int tipoExercicio;       // 1..3  -> "E1".."E3"
  int anguloInicial;
  int anguloAlvo;
  int repeticoes;
  int anguloReferencia;
  uint16_t adcReferencia;
  unsigned long timestampConfigMs;
  const char* timestampWallClock;  // "" ou nullptr = omitir a linha
};

// Formatadores puros (testaveis no PC). Retornam o numero de bytes (0 se nao coube).
size_t csvFormatarCabecalho(char* buf, size_t cap, const CsvCabecalho& c);
size_t csvFormatarLinha(char* buf, size_t cap, unsigned long timestampMs, uint16_t adcBruto,
                        float anguloFiltrado, const char* estadoCsv, const char* evento,
                        long duracaoRepMs);  // duracaoRepMs < 0 => campo vazio

void csvExportIniciarSessao(const CsvCabecalho& c);
// Retorna true se a linha foi escrita ou enfileirada, false se descartada.
bool csvExportLinha(unsigned long timestampMs, uint16_t adcBruto, float anguloFiltrado,
                    const char* estadoCsv, const char* evento, long duracaoRepMs);
void csvExportTick();  // chamar a cada loop()
bool csvExportPendente();   // ainda ha bytes na fila

#endif
