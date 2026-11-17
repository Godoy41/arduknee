#ifndef ARMAZENAMENTO_H
#define ARMAZENAMENTO_H

#include "Tipos.h"

// NVS (RF05). So guarda o necessario: o contador de sessoes (sessao_id do CSV) e o
// registro da ULTIMA configuracao recebida, sempre marcado "nao confirmado".
// Os parametros ATIVOS e a calibracao ficam em RAM e sao invalidados no boot,
// no Reset e no timeout (decisao do UML v2.5) - o NVS nunca os restaura.
void armazenamentoInit();
uint32_t armazenamentoProximoSessaoId();                    // incrementa e persiste
void armazenamentoSalvarUltimaConfig(const ParametrosSessao& p);  // chamado so no loop()

#endif
