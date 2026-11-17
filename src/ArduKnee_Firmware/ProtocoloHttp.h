#ifndef PROTOCOLO_HTTP_H
#define PROTOCOLO_HTTP_H

#include "Tipos.h"
#include <stddef.h>

// Regras do contrato HTTP (UML v2.5 sec. 5) como FUNCOES PURAS: recebem o corpo do
// pedido e a foto do estado, devolvem o codigo HTTP e o JSON de resposta. Nao tocam em
// Wi-Fi, filas nem variaveis da sessao (RNF20) - por isso sao testaveis no PC.
// Contrato: o campo "status" e o codigo HTTP seguem o documento; o texto de "mensagem"
// e' livre (aqui, mais especifico que o exemplo do documento).

struct RespostaHttp {
  int codigo;
  char corpo[384];
};

// POST /configurar
//  409 em Sessao Ativa/Pausada ou durante a comemoracao; 400 corpo/valores invalidos;
//  202 aviso RF40 sem "confirmado": true; 200 aceita (preenche *aceitos).
// *sinalizarInvalida = true quando devolve 400 (o loop() dispara o feedback fisico).
RespostaHttp protocoloConfigurar(const char* corpoReq, const StatusSnapshot& snap,
                                 ParametrosSessao* aceitos, bool* sinalizarInvalida);

// POST /calibrar
//  409 fora de "Aguardando Inicio"; 400 anguloReferencia invalido (padrao 0 se ausente);
//  503 leitura instavel (extensao: o UML nao define o codigo - ver nota na entrega);
//  200 aceita (preenche *anguloRef).
RespostaHttp protocoloCalibrar(const char* corpoReq, const StatusSnapshot& snap, int* anguloRef);

// GET /status - idempotente (RNF22). Retorna o tamanho do JSON (0 se nao coube).
size_t protocoloMontarStatus(const StatusSnapshot& snap, char* buf, size_t cap);

#endif
