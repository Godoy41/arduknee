// =====================================================================================
// ArduKnee - firmware em ARQUIVO UNICO (Sprint 2)
// GERADO AUTOMATICAMENTE por testes_pc/gerar_unico.py a partir de ArduKnee_Firmware/.
// NAO EDITE AQUI: edite os modulos e gere de novo. Comportamento identico ao sketch modular.
//
// Requisitos: pacote "esp32" da Espressif 3.x; bibliotecas "ESP Async WebServer" e
// "Async TCP" (ESP32Async). Senha do Wi-Fi: crie um Segredos.h na mesma pasta com
//   #define SENHA_AP "sua-senha-de-8-a-63-caracteres"
// ou troque o valor de SENHA_AP logo abaixo.
// =====================================================================================

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <Preferences.h>
#include <math.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <stdlib.h>

// ---- Senha do AP (RF27: WPA2, 8 a 63 caracteres) ----
#if __has_include("Segredos.h")
#include "Segredos.h"
#endif
#ifndef SENHA_AP
#define SENHA_AP "TROQUE_ESTA_SENHA"   // so para teste de bancada; NAO versione a senha real
#endif

// ======================================================================
// Pinout.h
// ======================================================================
#ifndef PINOUT_H
#define PINOUT_H

// Pinout oficial - ArduKnee Circuito Eletrico v2.7 / layout ArduKnee_v3_4.json
// (fonte unica da verdade). Qualquer mudanca de pino e' feita SO AQUI.

// Sensor
#define PINO_POTENCIOMETRO 34   // ADC1_CH6 - so entrada, sem pull-up/down interno

// Botoes (INPUT_PULLUP - pressionado = nivel BAIXO)
#define PINO_BOTAO_INICIAR  4
#define PINO_BOTAO_ENCERRAR 16  // ATENCAO: GPIO 16 e 17 so existem no ESP32-WROOM-32
#define PINO_BOTAO_RESET    17  // (DevKit V1). Em modulos WROVER (PSRAM) ficam ocupados.

// Atuadores
#define PINO_SERVO  13  // PWM 50 Hz, alimentado em 5 V (boost) - NUNCA no 3V3
#define PINO_BUZZER 23  // BASE do 2N2222 (via 1 kohm). Buzzer PASSIVO: o tom e' gerado por PWM (LEDC)

// Canais LEDC fixos. No core 3.x cada PAR de canais (0-1, 2-3, ...) compartilha um timer:
// servo e buzzer ficam em pares diferentes para que mudar a frequencia do buzzer
// NAO altere os 50 Hz do servo.
#define LEDC_CANAL_SERVO  0
#define LEDC_CANAL_BUZZER 2

// Barra de 10 LEDs (progresso angular) - cada um com resistor 220 ohm em serie
#define PINO_LED_1  14  // vermelho
#define PINO_LED_2  27  // vermelho
#define PINO_LED_3  26  // amarelo
#define PINO_LED_4  25  // amarelo
#define PINO_LED_5  33  // amarelo
#define PINO_LED_6  32  // amarelo
#define PINO_LED_7  18  // verde
#define PINO_LED_8  19  // verde
#define PINO_LED_9  21  // verde
#define PINO_LED_10 22  // verde

// Pinos que NAO devem ser usados neste projeto:
//   GPIO 0, 2, 5, 12, 15 -> strapping (interferem no boot)
//   GPIO 6 a 11          -> flash SPI interna (proibidos)
// Confirme que a placa e' ESP32-WROOM-32 antes de montar (GPIO 16/17 dos botoes).

#endif

// ======================================================================
// Tipos.h
// ======================================================================
#ifndef TIPOS_H
#define TIPOS_H

// Tipos compartilhados entre os modulos. Sem dependencia de Arduino (testavel no PC).


enum EstadoSessao {
  AGUARDANDO_CONFIGURACAO,
  AGUARDANDO_INICIO,
  SESSAO_ATIVA,
  SESSAO_PAUSADA,
  SESSAO_ENCERRADA
};

// Nome exato exposto no /status (RNF23: sempre um dos 5 valores, nunca vazio).
inline const char* nomeEstado(EstadoSessao e) {
  static const char* const n[] = {"Aguardando Configuração", "Aguardando Início",
                                  "Sessão Ativa", "Sessão Pausada", "Sessão Encerrada"};
  return ((unsigned)e < 5) ? n[e] : n[0];
}

// Nome usado na coluna estado_sessao do CSV.
inline const char* nomeEstadoCsv(EstadoSessao e) {
  static const char* const n[] = {"AGUARDANDO_CONFIGURACAO", "AGUARDANDO_INICIO",
                                  "SESSAO_ATIVA", "SESSAO_PAUSADA", "SESSAO_ENCERRADA"};
  return ((unsigned)e < 5) ? n[e] : n[0];
}

#define TIMESTAMP_ISO_MAX 32

// Parametros da sessao (POST /configurar). Struct simples (POD): pode ser copiada
// inteira dentro de uma secao critica.
struct ParametrosSessao {
  int perfil;           // 1 a 6 (P1 a P6)
  int tipoExercicio;    // 1 = E1 Flexao, 2 = E2 Extensao, 3 = E3 Ciclo completo
  int anguloInicial;    // ponto de partida (graus)
  int anguloAlvo;       // ponto de chegada (graus)
  int repeticoes;       // meta de repeticoes (> 0, sem teto de projeto)
  char timestampISO[TIMESTAMP_ISO_MAX];  // hora do celular (opcional); "" se ausente
};

// Foto do estado do dispositivo. Montada pelo loop() (GameLogic) e PUBLICADA para o
// callback HTTP: o callback so LE esta copia, nunca toca nas variaveis da sessao (RNF20).
struct StatusSnapshot {
  EstadoSessao estado;
  int repeticoes;
  int metaRepeticoes;
  float anguloAtual;       // 0 se nao calibrado
  float amplitudeMaxima;   // max |angulo - anguloInicial| da sessao
  bool sessaoConcluida;    // encerrada por meta de repeticoes (RF26)
  bool calibrado;
  bool configurado;
  bool instavel;           // janela de instabilidade ativa (nao vai no /status)
  bool comemorando;        // rotina de comemoracao em curso (nao vai no /status)
  ParametrosSessao parametros;
};

#endif

// ======================================================================
// ValidacaoConfig.h
// ======================================================================
#ifndef VALIDACAO_CONFIG_H
#define VALIDACAO_CONFIG_H


// RF04: limites de projeto do MVP (mapeamento do sensor e arco-guia).
// Repeticoes: apenas "> 0" (decisao da equipe: sem teto de 100).
#define ANGULO_MIN_GRAUS 0
#define ANGULO_MAX_GRAUS 90

// Funcao PURA (PT 4.2.9): sem Arduino, sem Wi-Fi, sem efeito colateral.
// tipoExercicio: 1=E1, 2=E2, 3=E3. perfil: 1 a 6.
// E1/E3: 0 <= inicial < alvo <= 90.   E2: 0 <= alvo < inicial <= 90.
// Retorna nullptr se valida, ou o texto do erro.
const char* validarConfig(int tipoExercicio, int perfil, int anguloInicial, int anguloAlvo, int repeticoes);

// RF40: aviso clinico (E2 + perfil P1/P2/P5 + alvo <= 40). Funcao pura.
bool requerAvisoClinico(int tipoExercicio, int perfil, int anguloAlvo);

extern const char* const MENSAGEM_AVISO_CLINICO;  // texto proposto no RF40 (validar com o fisioterapeuta)

#endif

// ======================================================================
// JsonPlano.h
// ======================================================================
#ifndef JSON_PLANO_H
#define JSON_PLANO_H

// Leitor MINIMO de JSON "plano": um unico objeto cujos valores sao texto, inteiro,
// booleano ou null. Objetos/arrays aninhados sao REJEITADOS (o contrato do POST nao
// os usa). Sem alocacao dinamica, sem Arduino: funciona no PC e no ESP32.

#define JSON_MAX_CAMPOS 10
#define JSON_MAX_CHAVE  24
#define JSON_MAX_TEXTO  40

enum TipoJson { JSON_TEXTO, JSON_INTEIRO, JSON_REAL, JSON_BOOLEANO, JSON_NULO };

struct CampoJson {
  char chave[JSON_MAX_CHAVE];
  TipoJson tipo;
  char texto[JSON_MAX_TEXTO];  // JSON_TEXTO
  long inteiro;                // JSON_INTEIRO (ate 9 digitos)
  bool booleano;               // JSON_BOOLEANO
};

struct JsonPlano {
  CampoJson campos[JSON_MAX_CAMPOS];
  int n;
};

// true se o texto e' um objeto JSON plano valido. Chave repetida = invalido.
bool jsonParsePlano(const char* texto, JsonPlano& saida);
const CampoJson* jsonBuscar(const JsonPlano& j, const char* chave);

#endif

// ======================================================================
// ProtocoloHttp.h
// ======================================================================
#ifndef PROTOCOLO_HTTP_H
#define PROTOCOLO_HTTP_H


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

// ======================================================================
// Log.h
// ======================================================================
#ifndef LOG_H
#define LOG_H

// Log de depuracao NAO BLOQUEANTE na mesma serial do CSV.
// Toda linha comeca com "# " : um leitor de CSV (pandas, Excel) trata como comentario,
// entao o log nunca corrompe os dados. Se o buffer da serial nao tem espaco, a linha e'
// DESCARTADA (RF34 / RNF21) - nunca bloqueia o loop().
void logInfo(const char* formato, ...);

#endif

// ======================================================================
// Armazenamento.h
// ======================================================================
#ifndef ARMAZENAMENTO_H
#define ARMAZENAMENTO_H


// NVS (RF05). So guarda o necessario: o contador de sessoes (sessao_id do CSV) e o
// registro da ULTIMA configuracao recebida, sempre marcado "nao confirmado".
// Os parametros ATIVOS e a calibracao ficam em RAM e sao invalidados no boot,
// no Reset e no timeout (decisao do UML v2.5) - o NVS nunca os restaura.
void armazenamentoInit();
uint32_t armazenamentoProximoSessaoId();                    // incrementa e persiste
void armazenamentoSalvarUltimaConfig(const ParametrosSessao& p);  // chamado so no loop()

#endif

// ======================================================================
// CsvExport.h
// ======================================================================
#ifndef CSV_EXPORT_H
#define CSV_EXPORT_H


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

// ======================================================================
// SensorModule.h
// ======================================================================
#ifndef SENSOR_MODULE_H
#define SENSOR_MODULE_H


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

// ======================================================================
// InputManager.h
// ======================================================================
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H


enum IdBotao { BOTAO_INICIAR, BOTAO_ENCERRAR, BOTAO_RESET };
enum TipoPressao { CURTA, LONGA };

// O InputManager so classifica. Nao interpreta duplo clique nem conhece a sessao (guia, secao 6).
struct EventoBotao {
  IdBotao idBotao;
  TipoPressao tipo;
};

void inputManagerInit();
// Retorna true se um evento foi classificado neste ciclo
bool inputManagerPoll(EventoBotao &evento);

#endif

// ======================================================================
// DetectorRepeticao.h
// ======================================================================
#ifndef DETECTOR_REPETICAO_H
#define DETECTOR_REPETICAO_H

// Deteccao de repeticao (UML v2.5 sec. 5.7) - classe PURA, sem Arduino.
//
// Progresso normalizado p = (angulo - inicial) / (alvo - inicial):
//   E1/E3: alvo > inicial (p cresce subindo).  E2: alvo < inicial (p cresce descendo).
// Uma repeticao = sair da posicao inicial, ATINGIR o alvo e VOLTAR a posicao inicial.
// Movimento parcial (volta sem atingir o alvo) nao conta (US04-CA2).
// A posicao inicial precisa ser VISTA antes de contar (evita contar a primeira volta
// se a sessao comeca no meio do arco).
//
// NOTA: a documentacao nao distingue o comportamento de E1 e E3; ambos usam esta regra.

#define LIMIAR_POSICAO_INICIAL 0.10f  // p <= isto = "de volta ao ponto de partida" (a validar na bancada)
// O sensor tem erro de ate +-5 graus (RNF26) e o ADC e' quantizado: exigir 100% exato do
// alvo faria um alvo de 30 graus nunca ser atingido ao ler 29,97. Vale como "alvo atingido"
// chegar a TOLERANCIA_ALVO_GRAUS do alvo (ou alem). O progresso exportado e' normalizado
// por esse criterio, entao a barra de LEDs mostra 100% exatamente quando a repeticao
// passa a poder fechar. Parametro de projeto: validar no PT 5.1.
#define TOLERANCIA_ALVO_GRAUS  2.0f

class DetectorRepeticao {
 public:
  void iniciar(int tipoExercicio, int anguloInicial, int anguloAlvo);
  // Retorna true quando este amostra COMPLETA uma repeticao; *duracaoMs = ida+volta.
  bool atualizar(float angulo, unsigned long agoraMs, unsigned long* duracaoMs);
  float progresso() const;  // 0.0-1.0 (limitado) - alimenta o termometro de LEDs

 private:
  enum Fase { FORA_DA_POSICAO, NA_POSICAO, IDA, NO_ALVO };
  Fase fase_ = FORA_DA_POSICAO;
  int tipo_ = 1;
  float inicial_ = 0.0f;
  float alvo_ = 1.0f;
  float limiarAlvo_ = 1.0f;   // p a partir do qual o alvo conta como atingido
  float p_ = 0.0f;
  unsigned long tInicioRep_ = 0;
};

#endif

// ======================================================================
// FeedbackDispatcher.h
// ======================================================================
#ifndef FEEDBACK_DISPATCHER_H
#define FEEDBACK_DISPATCHER_H


// Ponto unico dos padroes de LED / buzzer / servo (RF22). Sem logica de jogo.
// Tudo NAO BLOQUEANTE: feedbackTick() avanca padroes com millis() a cada loop().
//
// Dois niveis de LED:
//   - EXIBICAO (base): o que a barra mostra o tempo todo, conforme o modo.
//   - EVENTO (overlay): padrao curto que SUBSTITUI a exibicao enquanto dura.

enum TipoEvento {
  CONFIGURACAO_OK,
  CONFIGURACAO_INVALIDA,
  CALIBRACAO_OK,
  CONFIRMAR_INICIO,        // 1a CURTA em Aguardando Inicio: 3 pulsos (RF30)
  INICIO_RECUSADO,         // sem config/calibracao: vermelho 3 pulsos
  REPETICAO_CORRETA,
  REPETICAO_NAO_CONFIAVEL, // repeticao durante instabilidade (US09): ambar
  META_ATINGIDA,           // comemoracao (servo + buzzer + LEDs)
  ENCERRAMENTO_MANUAL,     // sinal neutro: tom curto + LEDs fixos (RF15)
  PAUSA,
  RETOMADA,
  INSTABILIDADE,           // limitado a 1 aviso por segundo
  RESET,
  QUASE_LA                 // tratado pela exibicao de progresso (80-99%); nao e' overlay
};

enum ModoExibicao {
  EXIBICAO_AGUARDANDO_CONFIG,   // batimento discreto (indica que o aparelho esta ligado)
  EXIBICAO_AGUARDANDO_INICIO,   // verde piscando lentamente
  EXIBICAO_PROGRESSO,           // termometro angular
  EXIBICAO_PAUSA,               // ambar fixo
  EXIBICAO_RESULTADO            // alterna: repeticoes/meta e amplitude/arco (a cada 3 s)
};

#define DURACAO_COMEMORACAO_MS 3000UL

void feedbackInit();
void dispararEvento(TipoEvento evento);
void feedbackDefinirModo(ModoExibicao modo);
void feedbackDefinirProgresso(float progresso01, bool instavel);       // modo PROGRESSO
void feedbackDefinirResultado(float fracRepeticoes, float fracAmplitude);  // modo RESULTADO
void feedbackDefinirServo(float razao01);   // 0 = neutro (0 graus), 1 = 180 graus
void feedbackTick();
bool feedbackComemoracaoAtiva();

// Funcao pura (testavel): mascara do termometro, com "quase la" (>=80%: pulso lento)
// e meta atingida (>=100%: todos piscam rapido).
uint16_t feedbackMascaraProgresso(float progresso01, unsigned long agoraMs);

#endif

// ======================================================================
// ActuatorModule.h
// ======================================================================
#ifndef ACTUATOR_MODULE_H
#define ACTUATOR_MODULE_H


// Camada fisica: LEDs, buzzer PASSIVO e servo. Nao decide QUANDO acionar - isso e'
// do FeedbackDispatcher/GameLogic. Exige o core Arduino-ESP32 3.x (API LEDC por pino).

void actuatorModuleInit();

// Barra de 10 LEDs: bit 0 = LED 1 ... bit 9 = LED 10.
void actuatorModuleDefinirLeds(uint16_t mascara);
void actuatorModuleSetLed(uint8_t indice, bool aceso);   // indice 0-9
void actuatorModuleApagarTodosLeds();

// Buzzer PASSIVO (via 2N2222): gera o tom por PWM. 0 Hz = silencio. Nao bloqueia.
// O tempo do tom e' controlado por quem chama (FeedbackDispatcher, com millis()).
void actuatorModuleTom(unsigned int frequenciaHz);

// Servo SG90: angulo 0-180 (neutro = 0). So reescreve o PWM se o valor mudou.
void actuatorModuleMoverServo(uint8_t angulo);

#endif

// ======================================================================
// GameLogic.h
// ======================================================================
#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H


// Maquina de estados do ArduKnee (UML v2.5 sec. 2). Roda SO no loop(): quem vem de
// outra tarefa (HTTP) entra por webServerModuleObterPendencias() -> aplicarConfig/calibrar.

enum ResultadoCalibracao {
  CALIBRACAO_ACEITA,
  CALIBRACAO_ESTADO_INVALIDO,  // fora de Aguardando Inicio
  CALIBRACAO_INSTAVEL,         // UC14-A3: leitura instavel, repetir a captura
  CALIBRACAO_SEM_LEITURA
};

#define JANELA_CONFIRMACAO_MS       5000UL    // RF30: 2a CURTA em ate 5 s confirma o inicio
#define TIMEOUT_AGUARDANDO_MS       300000UL  // RF28: 5 min
#define JANELA_INSTABILIDADE_MS     1000UL    // leitura "instavel" ate 1 s apos o ultimo salto

void gameLogicInit();
void gameLogicProcessarEvento(EventoBotao evento);   // CURTA/LONGA vindos do InputManager
void gameLogicProcessarLeitura(const LeituraSensor& leitura);  // a cada 50 ms (exceto pausada)
void gameLogicTick();                                // a cada loop(): timers
EstadoSessao gameLogicGetEstado();

// Configuracao validada pelo callback HTTP. Retorna false (e descarta) se o estado ja
// nao a aceita - o callback so consultou uma FOTO do estado, o loop() e' a autoridade.
bool gameLogicAplicarConfig(const ParametrosSessao& params);
ResultadoCalibracao gameLogicCalibrar(int anguloReferencia);

void gameLogicObterSnapshot(StatusSnapshot& saida);

#endif

// ======================================================================
// PaginaConfiguracao.h
// ======================================================================
#ifndef PAGINA_CONFIGURACAO_H
#define PAGINA_CONFIGURACAO_H


// Pagina servida em "/" (Portal Cativo). Sem recursos externos (a rede nao tem internet)
// e sem innerHTML (so textContent). Alvos de toque >= 48 px, contraste >= 4,5:1 (RF24/PB-28).
// TODO(TAP 5.8): preencher PERFIS com os valores padrao oficiais de P1-P6. Enquanto isso,
// todos usam o padrao generico 10 / 70 / 10 em E1.
const char PAGINA_CONFIGURACAO[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ArduKnee</title>
<style>
:root{--tinta:#15231f;--papel:#f3f5f1;--linha:#b9c5bd;--acento:#0b5d4f;--erro:#9b1c1c;--ok:#14602d;--aviso-fundo:#fff3d1;--aviso:#5c3b00}
*{box-sizing:border-box}
body{margin:0;background:var(--papel);color:var(--tinta);font:16px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif}
main{max-width:30rem;margin:0 auto;padding:1rem 1rem 3rem}
h1{font-size:1.1rem;margin:0;letter-spacing:.02em}
header{display:flex;justify-content:space-between;align-items:center;padding:.25rem 0 1rem}
#conexao{font-size:.85rem}
#conexao::before{content:"";display:inline-block;width:.6rem;height:.6rem;border-radius:50%;background:var(--ok);margin-right:.4rem}
#conexao.off::before{background:var(--erro)}
.painel{border:2px solid var(--tinta);padding:1rem;background:#fff}
#estado{font-size:1.15rem;font-weight:700;margin:0}
.leitura{display:flex;align-items:baseline;gap:.25rem;margin:.5rem 0 .25rem}
#angulo{font-size:3.5rem;font-weight:800;line-height:1;font-variant-numeric:tabular-nums}
.arco{height:.75rem;background:#e3e9e5;border:1px solid var(--linha);margin:.5rem 0}
.arco i{display:block;height:100%;width:0;background:var(--acento)}
dl{display:grid;grid-template-columns:auto 1fr;gap:.2rem 1rem;margin:.75rem 0 0}
dt{color:#44544d}dd{margin:0;font-weight:600;text-align:right}
#concluida{margin:.75rem 0 0;padding:.5rem .75rem;background:var(--ok);color:#fff;font-weight:700}
section{margin-top:1.5rem}
h2{font-size:1rem;margin:0 0 .5rem;display:flex;gap:.6rem;align-items:center}
h2 b{display:inline-grid;place-items:center;width:1.7rem;height:1.7rem;background:var(--tinta);color:#fff;border-radius:50%;font-size:.9rem}
label{display:block;margin:.9rem 0 .25rem;font-weight:600}
select,input,button{font:inherit;width:100%;min-height:3rem;padding:.5rem .75rem;border:2px solid var(--tinta);border-radius:0;background:#fff;color:var(--tinta)}
button{background:var(--acento);color:#fff;border-color:var(--acento);font-weight:700;margin-top:1rem;cursor:pointer}
button.sec{background:#fff;color:var(--acento)}
button:disabled{background:#d7ddd9;border-color:#d7ddd9;color:#44544d;cursor:not-allowed}
:focus-visible{outline:3px solid #1d4ed8;outline-offset:2px}
.dica{font-size:.85rem;color:#44544d;margin:.25rem 0 0}
.msg{margin:.75rem 0 0;font-weight:600;min-height:1.4rem}
.msg.erro{color:var(--erro)}.msg.ok{color:var(--ok)}
#aviso{margin-top:.75rem;padding:.75rem;background:var(--aviso-fundo);color:var(--aviso);border-left:6px solid var(--aviso)}
#aviso p{margin:0}
[hidden]{display:none!important}
.rodape{margin-top:1.5rem;font-size:.85rem;color:#44544d}
</style>
</head>
<body>
<main>
<header><h1>ArduKnee</h1><span id="conexao">Conectado</span></header>

<div class="painel" role="region" aria-label="Estado do aparelho">
  <p id="estado">Consultando…</p>
  <div class="leitura"><span id="angulo">–</span><span>°</span></div>
  <div class="arco" id="arco" hidden><i id="barra"></i></div>
  <dl>
    <dt>Repetições</dt><dd id="reps">–</dd>
    <dt>Amplitude máxima</dt><dd id="amp">–</dd>
    <dt>Calibração</dt><dd id="calib">–</dd>
    <dt>Parâmetros</dt><dd id="params">não configurado</dd>
  </dl>
  <p id="concluida" hidden>Sessão concluída</p>
</div>

<section>
<h2><b>1</b>Configurar a sessão</h2>
<form id="cfg" novalidate>
  <label for="perfil">Perfil clínico</label>
  <select id="perfil"><option>P1</option><option>P2</option><option>P3</option><option>P4</option><option>P5</option><option>P6</option></select>
  <label for="tipo">Tipo de exercício</label>
  <select id="tipo"><option value="E1">E1 · Flexão</option><option value="E2">E2 · Extensão</option><option value="E3">E3 · Ciclo completo</option></select>
  <label for="ini" id="lIni">Ângulo inicial (0 a 90)</label>
  <input id="ini" type="number" inputmode="numeric" min="0" max="90" step="1" required>
  <label for="alvo" id="lAlvo">Ângulo alvo (0 a 90)</label>
  <input id="alvo" type="number" inputmode="numeric" min="0" max="90" step="1" required>
  <p class="dica" id="dicaAng"></p>
  <label for="reps">Repetições</label>
  <input id="reps_in" type="number" inputmode="numeric" min="1" step="1" required>
  <div id="aviso" role="alert" hidden><p id="avisoTxt"></p><button type="button" id="confirmar">Confirmar e enviar</button></div>
  <button type="submit" id="enviar">Enviar configuração</button>
  <p class="msg" id="msgCfg" aria-live="polite"></p>
</form>
</section>

<section>
<h2><b>2</b>Calibrar o sensor</h2>
<p class="dica">Posicione o joelho num ângulo conhecido, informe-o abaixo e capture. Disponível depois que a configuração for aceita.</p>
<label for="ref">Ângulo de referência (0 a 90)</label>
<input id="ref" type="number" inputmode="numeric" min="0" max="90" step="1" value="0">
<button type="button" class="sec" id="calibrar" disabled>Capturar referência</button>
<p class="msg" id="msgCal" aria-live="polite"></p>
</section>

<section>
<h2><b>3</b>Iniciar no aparelho</h2>
<p>Aperte <strong>Iniciar</strong> duas vezes, com no máximo 5 segundos entre os toques. Encerrar e Reset existem só no aparelho (segure 2 s para encerrar ou resetar).</p>
</section>
<p class="rodape">Protótipo educacional. Os limites de ângulo e de repetições devem ser definidos com um fisioterapeuta.</p>
</main>

<script>
var PADRAO={tipo:"E1",ini:10,alvo:70,reps:10};
var PERFIS={P1:PADRAO,P2:PADRAO,P3:PADRAO,P4:PADRAO,P5:PADRAO,P6:PADRAO};
function $(i){return document.getElementById(i)}
function msg(id,t,c){var e=$(id);e.textContent=t;e.className="msg"+(c?" "+c:"")}
function pad(n){return(n<10?"0":"")+n}
function agoraISO(){var d=new Date(),o=-d.getTimezoneOffset(),s=o<0?"-":"+";o=Math.abs(o);
return d.getFullYear()+"-"+pad(d.getMonth()+1)+"-"+pad(d.getDate())+"T"+pad(d.getHours())+":"+pad(d.getMinutes())+":"+pad(d.getSeconds())+s+pad(Math.floor(o/60))+":"+pad(o%60)}

function aplicarPerfil(){var p=PERFIS[$("perfil").value];$("tipo").value=p.tipo;$("ini").value=p.ini;$("alvo").value=p.alvo;$("reps_in").value=p.reps;rotulos()}
function rotulos(){var e2=$("tipo").value==="E2";
$("lIni").textContent=e2?"Ângulo inicial (partida, maior que o alvo)":"Ângulo inicial (partida)";
$("lAlvo").textContent=e2?"Ângulo alvo (chegada)":"Ângulo alvo (chegada, maior que o inicial)";
$("dicaAng").textContent=e2?"Extensão: o alvo é menor que o ângulo inicial.":"Flexão e ciclo completo: o alvo é maior que o ângulo inicial."}

async function pedir(url,corpo){
 var c=("AbortController" in window)?new AbortController():null,t=setTimeout(function(){if(c)c.abort()},4000);
 try{var r=await fetch(url,{method:corpo?"POST":"GET",headers:corpo?{"Content-Type":"application/json"}:{},body:corpo?JSON.stringify(corpo):undefined,signal:c?c.signal:undefined,cache:"no-store"});
 var j={};try{j=await r.json()}catch(e){}return{http:r.status,j:j}}finally{clearTimeout(t)}}

function montarCorpo(conf){return{perfil:$("perfil").value,tipoExercicio:$("tipo").value,anguloInicial:parseInt($("ini").value,10),anguloAlvo:parseInt($("alvo").value,10),repeticoes:parseInt($("reps_in").value,10),timestampISO:agoraISO(),confirmado:conf}}

async function enviar(conf){
 $("aviso").hidden=true;msg("msgCfg","Enviando…");
 try{var r=await pedir("/configurar",montarCorpo(conf));
  if(r.http===200)msg("msgCfg","Configuração recebida. Agora calibre o sensor (passo 2).","ok");
  else if(r.http===202){$("avisoTxt").textContent=r.j.mensagem;$("aviso").hidden=false;msg("msgCfg","")}
  else if(r.http===409)msg("msgCfg","Há uma sessão em andamento. Encerre no aparelho antes de reconfigurar.","erro");
  else msg("msgCfg",r.j.mensagem||"Não foi possível enviar.","erro")}
 catch(e){msg("msgCfg","Sem resposta do aparelho. Confira a conexão e tente de novo.","erro")}}

async function calibrar(){
 var v=parseInt($("ref").value,10);
 if(isNaN(v)||v<0||v>90){msg("msgCal","Informe um ângulo de 0 a 90.","erro");return}
 msg("msgCal","Capturando…");
 try{var r=await pedir("/calibrar",{anguloReferencia:v});
  if(r.http===200)msg("msgCal","Referência capturada.","ok");
  else if(r.http===503)msg("msgCal","Leitura instável. Mantenha o joelho parado e capture de novo.","erro");
  else if(r.http===409)msg("msgCal","A calibração só é aceita depois da configuração e antes do início.","erro");
  else msg("msgCal",r.j.mensagem||"Não foi possível calibrar.","erro")}
 catch(e){msg("msgCal","Sem resposta do aparelho. Tente de novo.","erro")}}

function mostrar(s){
 $("estado").textContent=s.estado;
 $("angulo").textContent=s.calibrado?s.anguloAtual:"–";
 $("reps").textContent=s.configurado?s.repeticoes+" / "+s.metaRepeticoes:"–";
 $("amp").textContent=s.configurado?s.amplitudeMaxima+"°":"–";
 $("calib").textContent=s.calibrado?"Calibrado":"Pendente";
 var p=s.parametros;
 $("params").textContent=p?p.perfil+" · "+p.tipoExercicio+" · "+p.anguloInicial+"°→"+p.anguloAlvo+"° · "+p.repeticoes+"×":"não configurado";
 $("concluida").hidden=!s.sessaoConcluida;
 var arco=$("arco");
 if(p&&s.calibrado){var f=(s.anguloAtual-p.anguloInicial)/(p.anguloAlvo-p.anguloInicial);f=Math.max(0,Math.min(1,f));arco.hidden=false;$("barra").style.width=(f*100)+"%"}else arco.hidden=true;
 var emSessao=s.estado==="Sessão Ativa"||s.estado==="Sessão Pausada";
 $("enviar").disabled=emSessao;
 $("calibrar").disabled=s.estado!=="Aguardando Início"}

async function ciclo(){
 try{var r=await pedir("/status");if(r.http===200){mostrar(r.j);$("conexao").textContent="Conectado";$("conexao").className=""}}
 catch(e){$("conexao").textContent="Sem resposta";$("conexao").className="off"}
 setTimeout(ciclo,2000)}

$("perfil").addEventListener("change",aplicarPerfil);
$("tipo").addEventListener("change",rotulos);
$("cfg").addEventListener("submit",function(e){e.preventDefault();enviar(false)});
$("cfg").addEventListener("input",function(){$("aviso").hidden=true});
$("confirmar").addEventListener("click",function(){enviar(true)});
$("calibrar").addEventListener("click",calibrar);
aplicarPerfil();ciclo();
</script>
</body>
</html>
)rawliteral";

#endif

// ======================================================================
// WebServerModule.h
// ======================================================================
#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H


// Access Point + Portal Cativo + endpoints (UML v2.5 sec. 5):
//   GET /  |  GET /status  |  POST /configurar  |  POST /calibrar
// NAO existem /reset nem /encerrar (RF41): Reset e Encerrar sao so fisicos.
//
// Regra de concorrencia (RNF20): o callback HTTP roda na tarefa async_tcp. Ele so
//   (1) le a FOTO do estado publicada pelo loop(),
//   (2) valida o pedido (funcoes puras de ProtocoloHttp),
//   (3) deixa o pedido numa caixa de correio protegida por secao critica.
// Quem escreve nas variaveis da sessao e' sempre o loop().

struct Pendencias {
  bool config;                 // chegou uma configuracao valida
  ParametrosSessao params;
  bool calibrar;               // chegou um pedido de calibracao valido
  int anguloReferencia;
  bool configInvalida;         // um POST /configurar foi rejeitado com 400 (feedback fisico)
};

void webServerModuleInit();
void webServerModuleLoop();                                     // DNS do Portal Cativo - a cada loop()
void webServerModulePublicarStatus(const StatusSnapshot& snap); // a cada loop(): foto para o callback
// Copia e LIMPA os pedidos pendentes (atomico). true se havia algo.
bool webServerModuleObterPendencias(Pendencias& saida);

#endif

// ======================================================================
// Log.cpp
// ======================================================================
void logInfo(const char* formato, ...) {
  char buf[160];
  buf[0] = '#'; buf[1] = ' ';
  va_list args;
  va_start(args, formato);
  int n = vsnprintf(buf + 2, sizeof(buf) - 4, formato, args);
  va_end(args);
  if (n < 0) return;
  size_t len = 2 + (size_t)n;
  if (len > sizeof(buf) - 3) len = sizeof(buf) - 3;  // truncou
  buf[len++] = '\r';
  buf[len++] = '\n';
  if ((size_t)Serial.availableForWrite() < len) return;  // buffer cheio: descarta
  Serial.write((const uint8_t*)buf, len);
}

// ======================================================================
// Armazenamento.cpp
// ======================================================================
namespace {
  const char* const ESPACO = "arduknee";
  Preferences prefs;
}

void armazenamentoInit() {
  // Cria o espaco de nomes se ainda nao existir (evita aviso de NOT_FOUND na 1a leitura).
  prefs.begin(ESPACO, false);
  prefs.end();
}

uint32_t armazenamentoProximoSessaoId() {
  prefs.begin(ESPACO, false);
  uint32_t id = prefs.getUInt("sessao", 0) + 1;
  prefs.putUInt("sessao", id);
  prefs.end();
  return id;
}

void armazenamentoSalvarUltimaConfig(const ParametrosSessao& p) {
  prefs.begin(ESPACO, false);
  prefs.putBytes("cfg", &p, sizeof(p));
  prefs.putBool("cfgConf", false);  // sempre "nao confirmado" (so registro)
  prefs.end();
}

// ======================================================================
// CsvExport.cpp
// ======================================================================
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

// ======================================================================
// ValidacaoConfig.cpp
// ======================================================================
const char* const MENSAGEM_AVISO_CLINICO =
    "Faixa angular sinalizada para atenção clínica em extensão em cadeia cinética aberta. "
    "Confirme com base no protocolo clínico do paciente antes de prosseguir.";

const char* validarConfig(int tipoExercicio, int perfil, int anguloInicial, int anguloAlvo, int repeticoes) {
  if (tipoExercicio < 1 || tipoExercicio > 3) return "Tipo de exercício inválido.";
  if (perfil < 1 || perfil > 6) return "Perfil clínico inválido.";
  if (anguloInicial < ANGULO_MIN_GRAUS || anguloInicial > ANGULO_MAX_GRAUS) return "O ângulo inicial deve estar entre 0 e 90.";
  if (anguloAlvo < ANGULO_MIN_GRAUS || anguloAlvo > ANGULO_MAX_GRAUS) return "O ângulo alvo deve estar entre 0 e 90.";
  if (tipoExercicio == 2) {
    if (anguloAlvo >= anguloInicial) return "Na extensão, o ângulo alvo deve ser menor que o inicial.";
  } else {
    if (anguloAlvo <= anguloInicial) return "Na flexão e no ciclo completo, o ângulo alvo deve ser maior que o inicial.";
  }
  if (repeticoes <= 0) return "O número de repetições deve ser maior que zero.";
  return nullptr;
}

bool requerAvisoClinico(int tipoExercicio, int perfil, int anguloAlvo) {
  return tipoExercicio == 2 && (perfil == 1 || perfil == 2 || perfil == 5) && anguloAlvo <= 40;
}

// ======================================================================
// JsonPlano.cpp
// ======================================================================
namespace {
  const char* pularEspacos(const char* p) {
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') p++;
    return p;
  }

  // Le uma string JSON em "saida" (capacidade cap). Retorna ponteiro apos a aspa final, ou nullptr.
  const char* lerString(const char* p, char* saida, int cap) {
    if (*p != '"') return nullptr;
    p++;
    int n = 0;
    while (*p != '"') {
      unsigned char c = (unsigned char)*p;
      if (c == 0 || c < 0x20) return nullptr;  // fim inesperado ou caractere de controle
      if (c == '\\') {
        p++;
        switch (*p) {
          case '"': c = '"'; break;
          case '\\': c = '\\'; break;
          case '/': c = '/'; break;
          case 'n': c = '\n'; break;
          case 't': c = '\t'; break;
          case 'r': c = '\r'; break;
          case 'b': c = '\b'; break;
          case 'f': c = '\f'; break;
          default: return nullptr;  // inclui \uXXXX (nao suportado)
        }
      }
      if (n >= cap - 1) return nullptr;  // texto longo demais
      saida[n++] = (char)c;
      p++;
    }
    saida[n] = 0;
    return p + 1;
  }

  bool igualLiteral(const char* p, const char* lit) {
    size_t n = strlen(lit);
    return strncmp(p, lit, n) == 0;
  }
}

bool jsonParsePlano(const char* texto, JsonPlano& j) {
  j.n = 0;
  if (texto == nullptr) return false;
  const char* p = pularEspacos(texto);
  if (*p != '{') return false;
  p = pularEspacos(p + 1);
  if (*p == '}') return *pularEspacos(p + 1) == 0;  // objeto vazio

  for (;;) {
    if (j.n >= JSON_MAX_CAMPOS) return false;
    CampoJson& c = j.campos[j.n];
    memset(&c, 0, sizeof(c));

    p = pularEspacos(p);
    p = lerString(p, c.chave, JSON_MAX_CHAVE);
    if (!p) return false;
    p = pularEspacos(p);
    if (*p != ':') return false;
    p = pularEspacos(p + 1);

    if (*p == '"') {
      c.tipo = JSON_TEXTO;
      p = lerString(p, c.texto, JSON_MAX_TEXTO);
      if (!p) return false;
    } else if (igualLiteral(p, "true") ) { c.tipo = JSON_BOOLEANO; c.booleano = true; p += 4; }
    else if (igualLiteral(p, "false")) { c.tipo = JSON_BOOLEANO; c.booleano = false; p += 5; }
    else if (igualLiteral(p, "null"))  { c.tipo = JSON_NULO; p += 4; }
    else if (*p == '-' || (*p >= '0' && *p <= '9')) {
      bool negativo = false;
      if (*p == '-') { negativo = true; p++; }
      if (!(*p >= '0' && *p <= '9')) return false;
      if (*p == '0' && p[1] >= '0' && p[1] <= '9') return false;  // zero a esquerda
      long v = 0; int digitos = 0;
      while (*p >= '0' && *p <= '9') { v = v * 10 + (*p - '0'); digitos++; p++; if (digitos > 9) return false; }
      if (*p == '.' || *p == 'e' || *p == 'E') {
        // numero nao inteiro: aceito como JSON_REAL (a camada de cima decide rejeitar)
        while ((*p >= '0' && *p <= '9') || *p == '.' || *p == 'e' || *p == 'E' || *p == '+' || *p == '-') p++;
        c.tipo = JSON_REAL;
      } else {
        c.tipo = JSON_INTEIRO;
        c.inteiro = negativo ? -v : v;
      }
    } else {
      return false;  // objeto/array aninhado ou lixo
    }

    for (int i = 0; i < j.n; i++) if (strcmp(j.campos[i].chave, c.chave) == 0) return false;  // duplicada
    j.n++;

    p = pularEspacos(p);
    if (*p == ',') { p++; continue; }
    if (*p == '}') return *pularEspacos(p + 1) == 0;
    return false;
  }
}

const CampoJson* jsonBuscar(const JsonPlano& j, const char* chave) {
  for (int i = 0; i < j.n; i++) if (strcmp(j.campos[i].chave, chave) == 0) return &j.campos[i];
  return nullptr;
}

// ======================================================================
// ProtocoloHttp.cpp
// ======================================================================
namespace {
  // Mensagens fixas, SEM aspas/barras (entram no JSON sem escape).
  void montar(RespostaHttp& r, int codigo, const char* status, const char* mensagem, const char* extra) {
    r.codigo = codigo;
    if (mensagem != nullptr)
      snprintf(r.corpo, sizeof(r.corpo), "{\"status\":\"%s\",\"mensagem\":\"%s\"%s}", status, mensagem, extra ? extra : "");
    else
      snprintf(r.corpo, sizeof(r.corpo), "{\"status\":\"%s\"%s}", status, extra ? extra : "");
  }

  int perfilDeTexto(const char* t) {   // "P1".."P6"
    if (t[0] == 'P' && t[1] >= '1' && t[1] <= '6' && t[2] == 0) return t[1] - '0';
    return 0;
  }
  int tipoDeTexto(const char* t) {     // "E1".."E3"
    if (t[0] == 'E' && t[1] >= '1' && t[1] <= '3' && t[2] == 0) return t[1] - '0';
    return 0;
  }

  // Aceita so caracteres de data/hora ISO 8601: o valor vai para uma linha de cabecalho do CSV.
  bool timestampSeguro(const char* t) {
    size_t n = strlen(t);
    if (n == 0 || n >= TIMESTAMP_ISO_MAX) return false;
    for (size_t i = 0; i < n; i++) {
      char c = t[i];
      bool ok = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                c == ':' || c == '+' || c == '-' || c == '.' || c == ' ';
      if (!ok) return false;
    }
    return true;
  }
}

RespostaHttp protocoloConfigurar(const char* corpoReq, const StatusSnapshot& snap,
                                 ParametrosSessao* aceitos, bool* sinalizarInvalida) {
  RespostaHttp r;
  if (sinalizarInvalida) *sinalizarInvalida = false;

  // RF41: Sessao Ativa/Pausada (e comemoracao) nao aceitam nova configuracao.
  if (snap.estado == SESSAO_ATIVA || snap.estado == SESSAO_PAUSADA || snap.comemorando) {
    montar(r, 409, "conflito", "Sessão em andamento. Encerre a sessão no aparelho antes de reconfigurar.", nullptr);
    return r;
  }

  JsonPlano j;
  if (corpoReq == nullptr || !jsonParsePlano(corpoReq, j)) {
    montar(r, 400, "erro", "JSON inválido.", nullptr);
    if (sinalizarInvalida) *sinalizarInvalida = true;
    return r;
  }

  const CampoJson* fPerfil = jsonBuscar(j, "perfil");
  const CampoJson* fTipo = jsonBuscar(j, "tipoExercicio");
  const CampoJson* fIni = jsonBuscar(j, "anguloInicial");
  const CampoJson* fAlvo = jsonBuscar(j, "anguloAlvo");
  const CampoJson* fReps = jsonBuscar(j, "repeticoes");
  const CampoJson* fTs = jsonBuscar(j, "timestampISO");
  const CampoJson* fConf = jsonBuscar(j, "confirmado");

  const char* erro = nullptr;
  if (!fPerfil || !fTipo || !fIni || !fAlvo || !fReps) erro = "Campo obrigatório ausente.";
  else if (fPerfil->tipo != JSON_TEXTO || fTipo->tipo != JSON_TEXTO) erro = "Perfil e tipo de exercício devem ser texto (ex.: P1 e E1).";
  else if (fIni->tipo != JSON_INTEIRO || fAlvo->tipo != JSON_INTEIRO || fReps->tipo != JSON_INTEIRO) erro = "Ângulos e repetições devem ser números inteiros.";
  else if (fConf && fConf->tipo != JSON_BOOLEANO && fConf->tipo != JSON_NULO) erro = "O campo confirmado deve ser verdadeiro ou falso.";

  ParametrosSessao p;
  memset(&p, 0, sizeof(p));
  if (!erro) {
    p.perfil = perfilDeTexto(fPerfil->texto);
    p.tipoExercicio = tipoDeTexto(fTipo->texto);
    p.anguloInicial = (int)fIni->inteiro;
    p.anguloAlvo = (int)fAlvo->inteiro;
    p.repeticoes = (int)fReps->inteiro;
    erro = validarConfig(p.tipoExercicio, p.perfil, p.anguloInicial, p.anguloAlvo, p.repeticoes);
  }
  if (erro) {
    montar(r, 400, "erro", erro, nullptr);
    if (sinalizarInvalida) *sinalizarInvalida = true;
    return r;
  }

  // RF40: aviso clinico exige confirmacao explicita (reenvio com confirmado:true).
  bool confirmado = (fConf && fConf->tipo == JSON_BOOLEANO && fConf->booleano);
  if (requerAvisoClinico(p.tipoExercicio, p.perfil, p.anguloAlvo) && !confirmado) {
    montar(r, 202, "aviso", MENSAGEM_AVISO_CLINICO, ",\"requerConfirmacao\":true");
    return r;
  }

  // timestampISO e' opcional e NAO afeta a validacao: se vier estranho, e' ignorado.
  if (fTs && fTs->tipo == JSON_TEXTO && timestampSeguro(fTs->texto)) {
    strncpy(p.timestampISO, fTs->texto, TIMESTAMP_ISO_MAX - 1);
  }

  if (aceitos) *aceitos = p;
  montar(r, 200, "sucesso", "Configuração recebida", nullptr);
  return r;
}

RespostaHttp protocoloCalibrar(const char* corpoReq, const StatusSnapshot& snap, int* anguloRef) {
  RespostaHttp r;
  if (snap.estado != AGUARDANDO_INICIO) {
    montar(r, 409, "conflito", "A calibração só é aceita em Aguardando Início.", nullptr);
    return r;
  }

  int ref = 0;  // padrao 0 quando o campo e' omitido
  if (corpoReq != nullptr && corpoReq[0] != 0) {
    JsonPlano j;
    if (!jsonParsePlano(corpoReq, j)) {
      montar(r, 400, "erro", "JSON inválido.", nullptr);
      return r;
    }
    const CampoJson* f = jsonBuscar(j, "anguloReferencia");
    if (f && f->tipo != JSON_NULO) {
      if (f->tipo != JSON_INTEIRO || f->inteiro < 0 || f->inteiro > 90) {
        montar(r, 400, "erro", "Ângulo de referência inválido (use um inteiro de 0 a 90).", nullptr);
        return r;
      }
      ref = (int)f->inteiro;
    }
  }

  if (snap.instavel) {
    montar(r, 503, "instavel", "Leitura instável. Mantenha o joelho parado e repita a captura.", nullptr);
    return r;
  }

  if (anguloRef) *anguloRef = ref;
  montar(r, 200, "sucesso", nullptr, ",\"calibrado\":true");
  return r;
}

size_t protocoloMontarStatus(const StatusSnapshot& s, char* buf, size_t cap) {
  int meta = s.configurado ? s.parametros.repeticoes : 0;
  int n = snprintf(buf, cap,
      "{\"estado\":\"%s\",\"repeticoes\":%d,\"metaRepeticoes\":%d,\"anguloAtual\":%d,"
      "\"amplitudeMaxima\":%d,\"sessaoConcluida\":%s,\"calibrado\":%s,\"configurado\":%s",
      nomeEstado(s.estado), s.repeticoes, meta, (int)lroundf(s.anguloAtual),
      (int)lroundf(s.amplitudeMaxima), s.sessaoConcluida ? "true" : "false",
      s.calibrado ? "true" : "false", s.configurado ? "true" : "false");
  if (n < 0 || (size_t)n >= cap) return 0;
  size_t len = (size_t)n;
  if (s.configurado) {   // "parametros" so existe quando ha configuracao vigente
    n = snprintf(buf + len, cap - len,
        ",\"parametros\":{\"perfil\":\"P%d\",\"tipoExercicio\":\"E%d\",\"anguloInicial\":%d,"
        "\"anguloAlvo\":%d,\"repeticoes\":%d}",
        s.parametros.perfil, s.parametros.tipoExercicio, s.parametros.anguloInicial,
        s.parametros.anguloAlvo, s.parametros.repeticoes);
    if (n < 0 || len + (size_t)n >= cap) return 0;
    len += (size_t)n;
  }
  if (len + 2 > cap) return 0;
  buf[len++] = '}';
  buf[len] = 0;
  return len;
}

// ======================================================================
// SensorModule.cpp
// ======================================================================
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

// ======================================================================
// InputManager.cpp
// ======================================================================
namespace {
  const unsigned long DEBOUNCE_MS = 50;
  const unsigned long LIMIAR_LONGA_MS = 2000;

  struct EstadoBotao {
    uint8_t pino;
    IdBotao id;
    bool pressionadoAnterior;
    unsigned long tInicioPressao;
    unsigned long tUltimaMudanca;
  };

  // Pinout real - ArduKnee Circuito Eletrico v2.7 (ver Pinout.h)
  EstadoBotao botoes[3] = {
    {PINO_BOTAO_INICIAR, BOTAO_INICIAR, false, 0, 0},
    {PINO_BOTAO_ENCERRAR, BOTAO_ENCERRAR, false, 0, 0},
    {PINO_BOTAO_RESET, BOTAO_RESET, false, 0, 0}
  };
}

void inputManagerInit() {
  for (auto &b : botoes) {
    pinMode(b.pino, INPUT_PULLUP); // botao ligado ao GND quando pressionado
  }
}

bool inputManagerPoll(EventoBotao &evento) {
  unsigned long agora = millis();

  for (auto &b : botoes) {
    bool pressionadoAgora = (digitalRead(b.pino) == LOW);

    if (pressionadoAgora != b.pressionadoAnterior) {
      if (agora - b.tUltimaMudanca < DEBOUNCE_MS) {
        continue; // trepidar mecanico, ignora (nao atualiza estado ainda)
      }
      b.tUltimaMudanca = agora;
      b.pressionadoAnterior = pressionadoAgora;

      if (pressionadoAgora) {
        b.tInicioPressao = agora;
      } else {
        unsigned long duracao = agora - b.tInicioPressao;
        evento.idBotao = b.id;
        evento.tipo = (duracao >= LIMIAR_LONGA_MS) ? LONGA : CURTA;
        return true;
      }
    }
  }
  return false;
}

// ======================================================================
// DetectorRepeticao.cpp
// ======================================================================
void DetectorRepeticao::iniciar(int tipoExercicio, int anguloInicial, int anguloAlvo) {
  tipo_ = tipoExercicio;
  inicial_ = (float)anguloInicial;
  alvo_ = (float)anguloAlvo;
  float arco = alvo_ > inicial_ ? alvo_ - inicial_ : inicial_ - alvo_;
  limiarAlvo_ = (arco > 0.0f) ? 1.0f - TOLERANCIA_ALVO_GRAUS / arco : 1.0f;
  if (limiarAlvo_ < 0.5f) limiarAlvo_ = 0.5f;      // arco minusculo: nunca menos que metade do caminho
  fase_ = FORA_DA_POSICAO;
  p_ = 0.0f;
  tInicioRep_ = 0;
}

float DetectorRepeticao::progresso() const {
  float x = p_ / limiarAlvo_;
  if (x < 0.0f) return 0.0f;
  if (x > 1.0f) return 1.0f;
  return x;
}

bool DetectorRepeticao::atualizar(float angulo, unsigned long agoraMs, unsigned long* duracaoMs) {
  float den = alvo_ - inicial_;
  p_ = (den == 0.0f) ? 0.0f : (angulo - inicial_) / den;

  // Uma unica amostra pode atravessar mais de uma fase (ex.: salto da posicao inicial
  // direto ao alvo): reavalia a MESMA amostra ate a fase estabilizar (no maximo 3 passos).
  for (int passo = 0; passo < 3; passo++) {
    Fase antes = fase_;
    switch (fase_) {
      case FORA_DA_POSICAO:
        if (p_ <= LIMIAR_POSICAO_INICIAL) fase_ = NA_POSICAO;
        break;
      case NA_POSICAO:
        if (p_ > LIMIAR_POSICAO_INICIAL) { fase_ = IDA; tInicioRep_ = agoraMs; }
        break;
      case IDA:
        if (p_ >= limiarAlvo_) fase_ = NO_ALVO;
        else if (p_ <= LIMIAR_POSICAO_INICIAL) fase_ = NA_POSICAO;  // parcial: nao conta
        break;
      case NO_ALVO:
        if (p_ <= LIMIAR_POSICAO_INICIAL) {
          fase_ = NA_POSICAO;
          if (duracaoMs) *duracaoMs = agoraMs - tInicioRep_;
          return true;
        }
        break;
    }
    if (fase_ == antes) break;
  }
  return false;
}

// ======================================================================
// ActuatorModule.cpp
// ======================================================================
#if !defined(ESP_ARDUINO_VERSION_MAJOR) || (ESP_ARDUINO_VERSION_MAJOR < 3)
#error "ArduKnee exige o pacote esp32 da Espressif versao 3.x (API ledcAttachChannel/ledcWriteTone)."
#endif

namespace {
  const uint8_t PINOS_LEDS[10] = {
    PINO_LED_1, PINO_LED_2, PINO_LED_3, PINO_LED_4, PINO_LED_5,
    PINO_LED_6, PINO_LED_7, PINO_LED_8, PINO_LED_9, PINO_LED_10
  };

  // SG90: pulso de ~0,5 ms (0 graus) a ~2,4 ms (180 graus) em 50 Hz. Se o servo "zumbir"
  // nos extremos, estreite estes limites (ex.: 600-2300) em vez de forcar o batente.
  const uint32_t SERVO_FREQ_HZ = 50;
  const uint8_t  SERVO_RESOLUCAO_BITS = 14;
  const uint32_t SERVO_US_MIN = 500;
  const uint32_t SERVO_US_MAX = 2400;

  const uint32_t BUZZER_FREQ_INICIAL_HZ = 2000;
  const uint8_t  BUZZER_RESOLUCAO_BITS = 10;

  int ultimoServo = -1;
  unsigned int ultimoTom = 0;
}

void actuatorModuleInit() {
  for (uint8_t i = 0; i < 10; i++) {
    pinMode(PINOS_LEDS[i], OUTPUT);
    digitalWrite(PINOS_LEDS[i], LOW);
  }
  // Canais explicitos: servo e buzzer em PARES de canais diferentes (timers diferentes).
  ledcAttachChannel(PINO_BUZZER, BUZZER_FREQ_INICIAL_HZ, BUZZER_RESOLUCAO_BITS, LEDC_CANAL_BUZZER);
  ledcWriteTone(PINO_BUZZER, 0);
  ledcAttachChannel(PINO_SERVO, SERVO_FREQ_HZ, SERVO_RESOLUCAO_BITS, LEDC_CANAL_SERVO);
  ultimoServo = -1;
  actuatorModuleMoverServo(0);  // posicao neutra
}

void actuatorModuleDefinirLeds(uint16_t mascara) {
  for (uint8_t i = 0; i < 10; i++) {
    digitalWrite(PINOS_LEDS[i], (mascara >> i) & 1 ? HIGH : LOW);
  }
}

void actuatorModuleSetLed(uint8_t indice, bool aceso) {
  if (indice >= 10) return;
  digitalWrite(PINOS_LEDS[indice], aceso ? HIGH : LOW);
}

void actuatorModuleApagarTodosLeds() { actuatorModuleDefinirLeds(0); }

void actuatorModuleTom(unsigned int frequenciaHz) {
  if (frequenciaHz == ultimoTom) return;
  ultimoTom = frequenciaHz;
  ledcWriteTone(PINO_BUZZER, frequenciaHz);  // 0 = duty 0 = silencio
}

void actuatorModuleMoverServo(uint8_t angulo) {
  if (angulo > 180) angulo = 180;
  if ((int)angulo == ultimoServo) return;
  ultimoServo = angulo;
  uint32_t us = SERVO_US_MIN + ((uint32_t)angulo * (SERVO_US_MAX - SERVO_US_MIN)) / 180;
  uint32_t duty = (us * (1UL << SERVO_RESOLUCAO_BITS)) / (1000000UL / SERVO_FREQ_HZ);
  ledcWrite(PINO_SERVO, duty);
}

// ======================================================================
// FeedbackDispatcher.cpp
// ======================================================================
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

  float limitarFeedback01(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

  uint16_t termometro(float p) {
    int n = (int)(limitarFeedback01(p) * 10.0f + 0.5f);
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

void feedbackDefinirProgresso(float p, bool instavel) { progresso = limitarFeedback01(p); instavelExibicao = instavel; }
void feedbackDefinirResultado(float r, float a) { fracRep = limitarFeedback01(r); fracAmp = limitarFeedback01(a); tModo0 = millis(); }
void feedbackDefinirServo(float razao) { servoRazao = limitarFeedback01(razao); }
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

// ======================================================================
// GameLogic.cpp
// ======================================================================
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

  float limitarFracao(float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); }

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
    return (configurado && params.repeticoes > 0) ? limitarFracao((float)repeticoes / (float)params.repeticoes) : 0.0f;
  }
  float fracaoAmplitude() {
    float arco = fabsf((float)(params.anguloAlvo - params.anguloInicial));
    return (configurado && arco > 0.0f) ? limitarFracao(amplitudeMaxima / arco) : 0.0f;
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

// ======================================================================
// WebServerModule.cpp
// ======================================================================
namespace {
  const byte DNS_PORT = 53;
  const size_t CORPO_MAX = 512;   // JSON de configuracao tem ~200 bytes

  DNSServer dnsServer;
  AsyncWebServer servidor(80);
  IPAddress ipAP;

  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  StatusSnapshot snapshot;        // static: nasce zerada = Aguardando Configuracao
  Pendencias pendencias;          // static: nasce zerada

  String gerarSSID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char sufixo[5];
    snprintf(sufixo, sizeof(sufixo), "%02X%02X", mac[4], mac[5]);
    return "ArduKnee-Config-" + String(sufixo);  // RF31 - SSID unico por dispositivo
  }

  void copiarSnapshot(StatusSnapshot& saida) {
    portENTER_CRITICAL(&mux);
    saida = snapshot;
    portEXIT_CRITICAL(&mux);
  }

  // ATENCAO: o corpo vai como String (copiado). Passar um char[] local para
  // request->send(code, tipo, const char*) NAO copia - a resposta e' enviada depois que o
  // callback retorna e o ponteiro ficaria pendurado na pilha.
  void responder(AsyncWebServerRequest* req, int codigo, const char* json) {
    String corpo(json);
    req->send(codigo, "application/json; charset=utf-8", corpo);
  }

  // Acumula o corpo do POST num buffer do heap guardado em _tempObject (a biblioteca o
  // libera quando o pedido termina). Corpo vazio ou maior que CORPO_MAX: fica sem buffer.
  void aoReceberCorpo(AsyncWebServerRequest* req, uint8_t* dados, size_t len, size_t indice, size_t total) {
    if (total == 0 || total > CORPO_MAX) return;
    if (indice == 0) req->_tempObject = malloc(total + 1);
    char* buf = (char*)req->_tempObject;
    if (buf == nullptr || indice + len > total) return;
    memcpy(buf + indice, dados, len);
    if (indice + len == total) buf[total] = 0;
  }

  const char* corpoDe(AsyncWebServerRequest* req) { return (const char*)req->_tempObject; }

  void aoConfigurar(AsyncWebServerRequest* req) {
    StatusSnapshot snap;
    copiarSnapshot(snap);
    ParametrosSessao novos;
    bool invalida = false;
    RespostaHttp r = protocoloConfigurar(corpoDe(req), snap, &novos, &invalida);

    portENTER_CRITICAL(&mux);
    if (r.codigo == 200) { pendencias.params = novos; pendencias.config = true; }
    else if (invalida) pendencias.configInvalida = true;
    portEXIT_CRITICAL(&mux);

    responder(req, r.codigo, r.corpo);
  }

  void aoCalibrar(AsyncWebServerRequest* req) {
    StatusSnapshot snap;
    copiarSnapshot(snap);
    int ref = 0;
    RespostaHttp r = protocoloCalibrar(corpoDe(req), snap, &ref);

    if (r.codigo == 200) {
      portENTER_CRITICAL(&mux);
      pendencias.anguloReferencia = ref;
      pendencias.calibrar = true;
      portEXIT_CRITICAL(&mux);
    }
    responder(req, r.codigo, r.corpo);
  }

  void aoConsultarStatus(AsyncWebServerRequest* req) {
    // RNF22: so LE a foto - nenhum efeito colateral, nenhuma flag consumida.
    StatusSnapshot snap;
    copiarSnapshot(snap);
    char buf[512];
    if (protocoloMontarStatus(snap, buf, sizeof(buf)) == 0) {
      responder(req, 500, "{\"status\":\"erro\",\"mensagem\":\"Falha ao montar o status\"}");
      return;
    }
    responder(req, 200, buf);
  }

  // Rotas que iOS/Android/Windows usam para detectar "tem internet?".
  // Sem interceptar, o Portal Cativo fecha sozinho (risco R16).
  void configurarRotasPortalCativo() {
    servidor.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest* r) { r->redirect("/"); });        // iOS/macOS
    servidor.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest* r) { r->redirect("/"); });  // iOS/macOS
    servidor.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest* r) { r->redirect("/"); });                // Android
    servidor.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest* r) { r->redirect("/"); });                    // Windows
    servidor.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest* r) { r->redirect("/"); });             // Windows 10/11
  }
}

void webServerModuleInit() {
  WiFi.mode(WIFI_AP);  // antes de ler o MAC: garante o driver Wi-Fi inicializado
  String ssid = gerarSSID();

  // RF27: WPA2 com senha de 8 a 63 caracteres (Segredos.h, fora do repositorio).
  if (!WiFi.softAP(ssid.c_str(), SENHA_AP)) {
    logInfo("[WebServerModule] ERRO: softAP falhou (senha de 8 a 63 caracteres?)");
  }
  ipAP = WiFi.softAPIP();
  dnsServer.start(DNS_PORT, "*", ipAP);  // todo DNS aponta para o ESP32 (Portal Cativo)

  servidor.on("/", HTTP_GET, [](AsyncWebServerRequest* req) {
    req->send(200, "text/html; charset=utf-8", PAGINA_CONFIGURACAO);  // const char[] estatico: sem copia
  });
  servidor.on("/status", HTTP_GET, aoConsultarStatus);
  servidor.on("/configurar", HTTP_POST, aoConfigurar, nullptr, aoReceberCorpo);
  servidor.on("/calibrar", HTTP_POST, aoCalibrar, nullptr, aoReceberCorpo);
  configurarRotasPortalCativo();

  servidor.onNotFound([](AsyncWebServerRequest* req) {
    if (req->method() == HTTP_GET) req->redirect("/");   // Portal Cativo
    else req->send(404, "application/json; charset=utf-8", "{\"status\":\"erro\",\"mensagem\":\"Rota inexistente\"}");
  });

  servidor.begin();
  logInfo("[WebServerModule] AP: %s  IP: %s", ssid.c_str(), ipAP.toString().c_str());
}

void webServerModuleLoop() { dnsServer.processNextRequest(); }

void webServerModulePublicarStatus(const StatusSnapshot& snap) {
  portENTER_CRITICAL(&mux);
  snapshot = snap;
  portEXIT_CRITICAL(&mux);
}

bool webServerModuleObterPendencias(Pendencias& saida) {
  bool algo;
  portENTER_CRITICAL(&mux);
  algo = pendencias.config || pendencias.calibrar || pendencias.configInvalida;
  if (algo) {
    saida = pendencias;
    memset(&pendencias, 0, sizeof(pendencias));
  }
  portEXIT_CRITICAL(&mux);
  return algo;
}

// ======================================================================
// ArduKnee_Firmware.ino (setup/loop)
// ======================================================================
// ArduKnee - firmware (Epic Firmware). Sprint 2: maquina de estados completa.
// O .ino SO ORQUESTRA: cada modulo cuida da sua responsabilidade (RNF07/RNF17).
// Regras de ouro: zero delay() no loop (RNF21); dados compartilhados com o callback HTTP
// so por secao critica (RNF20); /status idempotente (RNF22); CSV nao bloqueante (RF34).
//
// Requisitos de ambiente:
//  - pacote "esp32" da Espressif 3.x (Arduino IDE > Boards Manager)
//  - bibliotecas "ESP Async WebServer" e "Async TCP" (ESP32Async)
//  - copiar Segredos.h.example para Segredos.h e definir a senha do Wi-Fi


namespace {
  const unsigned long INTERVALO_LEITURA_MS = 50;       // 20 Hz (RF13 / RF29)
  const unsigned long INTERVALO_PUBLICACAO_MS = 10;    // foto do estado para o HTTP (polling e' de 2 s)
  unsigned long ultimaAmostragemMs = 0;
  unsigned long ultimaPublicacaoMs = 0;
}

void setup() {
  Serial.begin(115200);
  armazenamentoInit();
  sensorModuleInit();
  inputManagerInit();
  actuatorModuleInit();
  feedbackInit();
  gameLogicInit();
  webServerModuleInit();
  logInfo("[main] ArduKnee pronto. Aguardando configuracao via Wi-Fi.");
}

void loop() {
  unsigned long agora = millis();

  // 1) Sensor a cada 50 ms. Em Sessao Pausada a leitura fica suspensa (UML v2.5).
  if (agora - ultimaAmostragemMs >= INTERVALO_LEITURA_MS) {
    ultimaAmostragemMs = agora;
    if (gameLogicGetEstado() != SESSAO_PAUSADA) {
      LeituraSensor leitura = sensorModuleLer();
      gameLogicProcessarLeitura(leitura);
    }
  }

  // 2) Botoes -> eventos classificados (CURTA/LONGA) -> maquina de estados
  EventoBotao evento;
  if (inputManagerPoll(evento)) gameLogicProcessarEvento(evento);

  // 3) Pedidos vindos do HTTP (ja validados pelo callback) entram AQUI, no loop()
  webServerModuleLoop();
  Pendencias pend;
  if (webServerModuleObterPendencias(pend)) {
    if (pend.configInvalida) dispararEvento(CONFIGURACAO_INVALIDA);
    if (pend.config) gameLogicAplicarConfig(pend.params);
    if (pend.calibrar) gameLogicCalibrar(pend.anguloReferencia);   // depois da config: mesma ordem do fluxo
  }

  // 4) Timers, atuadores e CSV (todos nao bloqueantes)
  gameLogicTick();
  feedbackTick();
  csvExportTick();

  // 5) Foto do estado para o callback de GET /status e para os 409. A cada 10 ms basta:
  //    o loop() re-valida tudo ao consumir um pedido, entao uma foto levemente velha e' inofensiva
  //    - e evita entrar na secao critica centenas de milhares de vezes por segundo.
  if (agora - ultimaPublicacaoMs >= INTERVALO_PUBLICACAO_MS) {
    ultimaPublicacaoMs = agora;
    StatusSnapshot snap;
    gameLogicObterSnapshot(snap);
    webServerModulePublicarStatus(snap);
  }
}
