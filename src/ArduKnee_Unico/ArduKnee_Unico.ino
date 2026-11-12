// =====================================================================
// ArduKnee - versao CONSOLIDADA EM UM ARQUIVO SO (somente para teste na bancada)
//
// Gerada a partir dos modulos de ArduKnee_Firmware/ (mesmo comportamento).
// ATENCAO: viola a RNF17 (codigo do servidor web separado da logica de jogo).
// Para entrega / Definition of Done, use a pasta ArduKnee_Firmware (modular).
//
// SENHA DO WI-FI: o bloco "Segredos.h.example" abaixo traz a senha-modelo.
// Troque SENHA_AP (8 a 63 caracteres) ANTES de gravar e nao compartilhe este
// arquivo depois de trocar a senha.
//
// Esta pasta deve conter SOMENTE este .ino (varios .ino na mesma pasta nao compilam).
// Bibliotecas: ESPAsyncWebServer e AsyncTCP da organizacao ESP32Async;
// placa: pacote esp32 da Espressif, versao 3.x.
// =====================================================================



// ---------------------------- Pinout.h ----------------------------
#ifndef PINOUT_H
#define PINOUT_H

// Pinout oficial - ArduKnee Circuito Eletrico v2.7 (fonte unica da verdade).
// Qualquer mudanca de pino e' feita SO AQUI, nunca direto dentro de um modulo.

// Sensor
#define PINO_POTENCIOMETRO 34   // ADC1_CH6 - so entrada, sem pull-up/down interno

// Botoes (INPUT_PULLUP - pressionado = nivel BAIXO)
#define PINO_BOTAO_INICIAR  4
#define PINO_BOTAO_ENCERRAR 16  // ATENCAO: GPIO 16 e 17 so existem no ESP32-WROOM-32
#define PINO_BOTAO_RESET    17  // (DevKit V1). Em modulos WROVER (PSRAM) ficam ocupados.

// Atuadores
#define PINO_SERVO  13  // PWM, 50Hz, alimentado em 5V - NUNCA no 3V3
#define PINO_BUZZER 23  // aciona a BASE do transistor 2N2222 - GPIO nunca liga o buzzer direto

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

// ---------------------------- Segredos.h.example ----------------------------
#ifndef SEGREDOS_H
#define SEGREDOS_H

// MODELO. Copie este arquivo para "Segredos.h" (mesma pasta) e troque a senha.
// O Segredos.h real NAO vai para o repositorio (esta no .gitignore); este .example vai.
// Troque a senha, anote-a na caixa do aparelho e mantenha de 8 a 63 caracteres
// (limite do WPA2 no ESP32).
#define SENHA_AP "TROQUE_ESTA_SENHA"

#endif

// ---------------------------- ValidacaoConfig.h ----------------------------
#ifndef VALIDACAO_CONFIG_H
#define VALIDACAO_CONFIG_H

// Limites da configuracao de sessao. Os valores definitivos de angulo e
// repeticoes devem ser definidos com um fisioterapeuta (prototipo educacional).
// tipoExercicio: 1 = E1 Flexao, 2 = E2 Extensao, 3 = E3 Ciclo completo.
// perfil: 1 a 6 (P1 a P6).
// anguloAlvo: na Flexao e no Ciclo completo fica ACIMA do inicial; na Extensao, ABAIXO.
#define ANGULO_MIN_GRAUS   0
#define ANGULO_MAX_GRAUS   90
#define REPETICOES_MAXIMAS 100

// Funcao PURA (PT 4.2.9): sem Arduino, sem Wi-Fi, sem efeito colateral.
// Retorna nullptr se a configuracao e' valida, ou o texto do erro.
const char* validarConfig(int tipoExercicio, int perfil, int anguloInicial, int anguloAlvo, int repeticoes);

#endif

// ---------------------------- SensorModule.h ----------------------------
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

// ---------------------------- InputManager.h ----------------------------
#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Arduino.h>

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

// ---------------------------- GameLogic.h ----------------------------
#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H


enum EstadoSessao {
  AGUARDANDO_CONFIGURACAO,
  AGUARDANDO_INICIO,
  SESSAO_ATIVA,
  SESSAO_PAUSADA,
  SESSAO_ENCERRADA
};

void gameLogicInit();
void gameLogicProcessarEvento(EventoBotao evento);
void gameLogicProcessarLeitura(LeituraSensor leitura);
void gameLogicTick(); // chamado a cada loop para processar o timer de 5s (RF30)
EstadoSessao gameLogicGetEstado();

#endif

// ---------------------------- ActuatorModule.h ----------------------------
#ifndef ACTUATOR_MODULE_H
#define ACTUATOR_MODULE_H

#include <Arduino.h>

// Controla LEDs, buzzer e servo (secao 9 do guia + circuito eletrico v2.7).
// Nao decide quando acionar - quem decide e' o GameLogic via FeedbackDispatcher (Sprint 2).

void actuatorModuleInit();

// --- Funcoes de baixo nivel, uma por atuador (comportamento completo e' Sprint 2 / PB-18, PB-19, PB-25) ---

// Acende/apaga um LED da barra (indice 0-9, correspondendo a LED 1-10 do circuito)
void actuatorModuleSetLed(uint8_t indice, bool aceso);
void actuatorModuleApagarTodosLeds();

// Liga o buzzer ATIVO (via transistor 2N2222) por duracaoMs, sem bloquear (sem delay()).
// O buzzer ativo tem tom fixo: frequenciaHz e' ignorada. Quem desliga e' actuatorModuleTick().
void actuatorModuleTocarBuzzer(unsigned int frequenciaHz, unsigned long duracaoMs);

// Chamar a cada loop(): desliga o buzzer quando o tempo pedido acabar.
void actuatorModuleTick();

// Servo: angulo 0-180 (a posicao neutra/comemoracao e' definida pelo GameLogic na Sprint 2)
void actuatorModuleMoverServo(uint8_t angulo);

#endif

// ---------------------------- FeedbackDispatcher.h ----------------------------
#ifndef FEEDBACK_DISPATCHER_H
#define FEEDBACK_DISPATCHER_H

enum TipoEvento {
  CONFIGURACAO_OK,
  CONFIGURACAO_INVALIDA,
  REPETICAO_CORRETA,
  META_ATINGIDA,
  INSTABILIDADE,
  RESET,
  QUASE_LA
};

void dispararEvento(TipoEvento evento);

#endif

// ---------------------------- WebServerModule.h ----------------------------
#ifndef WEB_SERVER_MODULE_H
#define WEB_SERVER_MODULE_H

#include <Arduino.h>

// Parametros recebidos via POST /configurar.
struct ParametrosSessao {
  int tipoExercicio;  // 1 = E1 Flexao, 2 = E2 Extensao, 3 = E3 Ciclo completo
  int perfil;         // 1 a 6 (P1 a P6)
  int anguloInicial;
  int anguloMaximo;   // significa "angulo alvo"
  int repeticoes;
};

void webServerModuleInit();
void webServerModuleLoop();  // atende o DNS do Portal Cativo - chamar a cada loop()

// Se chegou uma configuracao nova desde a ultima chamada, copia para "saida" e
// retorna true. Thread-safe: o callback HTTP roda em outra tarefa do FreeRTOS e o
// acesso aos dados e' protegido por secao critica (PB-08 / RNF20). Se chegarem
// duas configuracoes seguidas, vale a ultima, inteira - nunca valores misturados.
bool webServerModuleObterNovaConfig(ParametrosSessao &saida);

#endif

// ---------------------------- PaginaConfiguracao.h ----------------------------
#ifndef PAGINA_CONFIGURACAO_H
#define PAGINA_CONFIGURACAO_H

#include <Arduino.h>

// Pagina servida em "/" - para onde o Portal Cativo redireciona o celular.
const char PAGINA_CONFIGURACAO[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ArduKnee - Configuração</title>
<style>
  body { font-family: Arial, sans-serif; background:#f2f4f7; margin:0; padding:24px; }
  .card { max-width:400px; margin:0 auto; background:#fff; border-radius:12px; padding:24px; box-shadow:0 2px 8px rgba(0,0,0,.1); }
  h1 { font-size:20px; color:#2b6cb0; margin-top:0; }
  label { display:block; margin:16px 0 4px; font-size:14px; color:#333; }
  input, select { width:100%; padding:10px; border:1px solid #ccc; border-radius:8px; box-sizing:border-box; font-size:16px; }
  button { width:100%; margin-top:24px; padding:12px; background:#2b6cb0; color:#fff; border:none; border-radius:8px; font-size:16px; }
  #msg { margin-top:16px; font-size:14px; text-align:center; }
  .aviso { margin-top:20px; font-size:12px; color:#666; text-align:center; }
</style>
</head>
<body>
  <div class="card">
    <h1>ArduKnee - Configurar sessão</h1>
    <form id="formConfig">
      <label>Perfil clínico</label>
      <select id="perfil">
        <option value="1">P1</option>
        <option value="2">P2</option>
        <option value="3">P3</option>
        <option value="4">P4</option>
        <option value="5">P5</option>
        <option value="6">P6</option>
      </select>

      <label>Tipo de exercício</label>
      <select id="tipoExercicio">
        <option value="1">E1 - Flexão</option>
        <option value="2">E2 - Extensão</option>
        <option value="3">E3 - Ciclo completo</option>
      </select>

      <label>Ângulo inicial (0 a 90)</label>
      <input type="number" id="anguloInicial" min="0" max="90" value="10" required>

      <label>Ângulo alvo (0 a 90)</label>
      <input type="number" id="anguloMaximo" min="0" max="90" value="70" required>

      <label>Número de repetições (1 a 100)</label>
      <input type="number" id="repeticoes" min="1" max="100" value="10" required>

      <button type="submit">Salvar configuração</button>
    </form>
    <div id="msg"></div>
    <p class="aviso">Protótipo educacional. Os limites de ângulo e de repetições devem ser definidos com um fisioterapeuta.</p>
  </div>

<script>
document.getElementById('formConfig').addEventListener('submit', async function (e) {
  e.preventDefault();
  const msg = document.getElementById('msg');
  msg.style.color = '#333';
  msg.textContent = 'Enviando...';

  const params = new URLSearchParams();
  params.append('anguloInicial', document.getElementById('anguloInicial').value);
  params.append('anguloMaximo', document.getElementById('anguloMaximo').value);
  params.append('repeticoes', document.getElementById('repeticoes').value);
  params.append('perfil', document.getElementById('perfil').value);
  params.append('tipoExercicio', document.getElementById('tipoExercicio').value);

  try {
    const resp = await fetch('/configurar', { method: 'POST', body: params });
    const data = await resp.json();
    msg.style.color = data.status === 'sucesso' ? 'green' : 'red';
    msg.textContent = data.mensagem;
  } catch (err) {
    msg.style.color = 'red';
    msg.textContent = 'Erro de conexão. Tente novamente.';
  }
});
</script>
</body>
</html>
)rawliteral";

#endif

// ---------------------------- ValidacaoConfig.cpp ----------------------------

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
  if (repeticoes <= 0 || repeticoes > REPETICOES_MAXIMAS) return "As repetições devem estar entre 1 e 100.";
  return nullptr;
}

// ---------------------------- SensorModule.cpp ----------------------------

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

// ---------------------------- InputManager.cpp ----------------------------

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

// ---------------------------- GameLogic.cpp ----------------------------

namespace {
  EstadoSessao estadoAtual = AGUARDANDO_CONFIGURACAO;
}

void gameLogicInit() {
  estadoAtual = AGUARDANDO_CONFIGURACAO;
}

void gameLogicProcessarEvento(EventoBotao evento) {
  // Sprint 1: apenas registra o evento recebido.
  // A tabela estado x evento completa (secao 3 do guia) entra na Sprint 2.
  Serial.print("[GameLogic] Evento - botao: ");
  Serial.print(evento.idBotao);
  Serial.print(" tipo: ");
  Serial.println(evento.tipo == CURTA ? "CURTA" : "LONGA");
}

void gameLogicProcessarLeitura(LeituraSensor leitura) {
  // Sprint 1: sem contagem de repeticoes/meta ainda (isso e' Sprint 2).
  (void)leitura;
}

void gameLogicTick() {
  // Sprint 2: timer de 5s da janela de confirmacao "Aguardando Inicio" (RF30, risco R24/R25).
}

EstadoSessao gameLogicGetEstado() {
  return estadoAtual;
}

// ---------------------------- ActuatorModule.cpp ----------------------------

// Biblioteca ESP32Servo (instalar via Library Manager) - usada so' quando o
// servo entrar em uso na Sprint 2 (comemoracao / indicador fisico).
// #include <ESP32Servo.h>
// Servo servoIndicador;

namespace {
  const uint8_t PINOS_LEDS[10] = {
    PINO_LED_1, PINO_LED_2, PINO_LED_3, PINO_LED_4, PINO_LED_5,
    PINO_LED_6, PINO_LED_7, PINO_LED_8, PINO_LED_9, PINO_LED_10
  };

  unsigned long buzzerFimMs = 0;
  bool buzzerLigado = false;
}

void actuatorModuleInit() {
  for (uint8_t i = 0; i < 10; i++) {
    pinMode(PINOS_LEDS[i], OUTPUT);
    digitalWrite(PINOS_LEDS[i], LOW);
  }

  pinMode(PINO_BUZZER, OUTPUT); // base do 2N2222 - GPIO nunca liga o buzzer direto
  digitalWrite(PINO_BUZZER, LOW);

  // Sprint 2: servoIndicador.attach(PINO_SERVO); posicao neutra inicial.
}

void actuatorModuleSetLed(uint8_t indice, bool aceso) {
  if (indice >= 10) return; // indice invalido, ignora
  digitalWrite(PINOS_LEDS[indice], aceso ? HIGH : LOW);
}

void actuatorModuleApagarTodosLeds() {
  for (uint8_t i = 0; i < 10; i++) {
    digitalWrite(PINOS_LEDS[i], LOW);
  }
}

void actuatorModuleTocarBuzzer(unsigned int frequenciaHz, unsigned long duracaoMs) {
  (void)frequenciaHz;  // buzzer ativo: o tom é fixo
  digitalWrite(PINO_BUZZER, HIGH);
  buzzerLigado = true;
  buzzerFimMs = millis() + duracaoMs;
}
void actuatorModuleTick() {
  if (buzzerLigado && (long)(millis() - buzzerFimMs) >= 0) {
    digitalWrite(PINO_BUZZER, LOW);
    buzzerLigado = false;
  }
}

void actuatorModuleMoverServo(uint8_t angulo) {
  // Sprint 2: servoIndicador.write(angulo);
  (void)angulo;
}

// ---------------------------- FeedbackDispatcher.cpp ----------------------------
#include <Arduino.h>

namespace {
  // A instabilidade e' detectada ate 20x por segundo; sem este intervalo o
  // buzzer/LED seriam acionados sem parar (Sprint 2).
  const unsigned long INTERVALO_MIN_INSTABILIDADE_MS = 1000;
  unsigned long ultimoDisparoInstabilidade = 0;
  bool jaDisparouInstabilidade = false;
}

void dispararEvento(TipoEvento evento) {
  unsigned long agora = millis();

  if (evento == INSTABILIDADE) {
    if (jaDisparouInstabilidade &&
        (agora - ultimoDisparoInstabilidade) < INTERVALO_MIN_INSTABILIDADE_MS) {
      return;  // muito cedo desde o ultimo aviso: ignora
    }
    jaDisparouInstabilidade = true;
    ultimoDisparoInstabilidade = agora;
  }

  // Padroes de feedback (secao 9 do guia). Por enquanto so o buzzer de CONFIGURACAO_OK
  // (bipe curto, nao bloqueante: o actuatorModuleTick() desliga). LEDs e servo: Sprint 2.
  if (evento == CONFIGURACAO_OK) {
    actuatorModuleTocarBuzzer(0, 150);
  }

  Serial.print("[FeedbackDispatcher] Evento disparado: ");
  Serial.println(evento);
}

// ---------------------------- WebServerModule.cpp ----------------------------
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

namespace {
  const byte DNS_PORT = 53;
  DNSServer dnsServer;
  AsyncWebServer servidor(80);
  IPAddress ipAP;

  // Dados compartilhados entre o callback HTTP (tarefa async_tcp) e o loop().
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  ParametrosSessao paramsPendentes = {0, 0, 0, 0, 0};
  bool temNovaConfig = false;

  String gerarSSID() {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char sufixo[5];
    snprintf(sufixo, sizeof(sufixo), "%02X%02X", mac[4], mac[5]);
    return "ArduKnee-Config-" + String(sufixo);  // RF31 - SSID unico por dispositivo
  }

  void responderJson(AsyncWebServerRequest *request, int codigo, const char *status, const char *mensagem) {
    String corpo = String("{\"status\":\"") + status + "\",\"mensagem\":\"" + mensagem + "\"}";
    request->send(codigo, "application/json; charset=utf-8", corpo);
  }

  // Rotas que iOS/Android/Windows usam para detectar "tem internet?".
  // Sem interceptar, o Portal Cativo fecha sozinho (risco R16).
  void configurarRotasPortalCativo() {
    servidor.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");  // iOS / macOS
    });
    servidor.on("/library/test/success.html", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");  // iOS / macOS
    });
    servidor.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");  // Android
    });
    servidor.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");  // Windows
    });
    servidor.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->redirect("/");  // Windows 10/11
    });
  }
}

void webServerModuleInit() {
  String ssid = gerarSSID();

  // PB-04: WPA2 com SSID unico. A senha vem de Segredos.h (fora do repositorio).
  WiFi.softAP(ssid.c_str(), SENHA_AP);
  ipAP = WiFi.softAPIP();

  // Redireciona TODA consulta DNS para o ESP32 (truque do Portal Cativo).
  dnsServer.start(DNS_PORT, "*", ipAP);

  servidor.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html; charset=utf-8", PAGINA_CONFIGURACAO);
  });

  servidor.on("/configurar", HTTP_POST, [](AsyncWebServerRequest *request) {
    ParametrosSessao novos;
    novos.tipoExercicio = request->arg("tipoExercicio").toInt();
    novos.perfil = request->arg("perfil").toInt();
    novos.anguloInicial = request->arg("anguloInicial").toInt();
    novos.anguloMaximo = request->arg("anguloMaximo").toInt();
    novos.repeticoes = request->arg("repeticoes").toInt();

    const char *erro = validarConfig(novos.tipoExercicio, novos.perfil, novos.anguloInicial, novos.anguloMaximo, novos.repeticoes);
    if (erro != nullptr) {
      responderJson(request, 400, "erro", erro);
      return;
    }

    // O callback NUNCA mexe no estado da sessao (RNF20): so deixa a config
    // pendente, copiada de uma vez (secao critica curta, sem alocar memoria).
    portENTER_CRITICAL(&mux);
    paramsPendentes = novos;
    temNovaConfig = true;
    portEXIT_CRITICAL(&mux);

    responderJson(request, 200, "sucesso",
                  "Configuração salva. Para começar, aperte Iniciar duas vezes no aparelho.");
  });

  configurarRotasPortalCativo();

  // Qualquer rota desconhecida cai na pagina de configuracao.
  servidor.onNotFound([](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  servidor.begin();

  Serial.print("[WebServerModule] Access Point criado: ");
  Serial.println(ssid);
  Serial.print("[WebServerModule] IP: ");
  Serial.println(ipAP);
}

void webServerModuleLoop() {
  dnsServer.processNextRequest();  // obrigatorio a cada loop para o DNS responder
}

bool webServerModuleObterNovaConfig(ParametrosSessao &saida) {
  bool nova = false;
  portENTER_CRITICAL(&mux);
  if (temNovaConfig) {
    saida = paramsPendentes;
    temNovaConfig = false;
    nova = true;
  }
  portEXIT_CRITICAL(&mux);
  return nova;
}

// ---------------------------- ArduKnee_Firmware.ino ----------------------------

namespace {
  unsigned long ultimaLeituraSensor = 0;
  const unsigned long INTERVALO_LEITURA_MS = 50;  // 20 Hz
}

void setup() {
  Serial.begin(115200);

  sensorModuleInit();
  inputManagerInit();
  gameLogicInit();
  actuatorModuleInit();
  webServerModuleInit();

  Serial.println("[main] Sprint 1 - PoC inicializado.");
}

void loop() {
  unsigned long agora = millis();

  // Leitura do sensor a 20 Hz - zero delay() no loop principal (RNF21)
  if (agora - ultimaLeituraSensor >= INTERVALO_LEITURA_MS) {
    ultimaLeituraSensor = agora;
    LeituraSensor leitura = sensorModuleLer();
    gameLogicProcessarLeitura(leitura);

    if (leitura.instavel) {
      dispararEvento(INSTABILIDADE);  // o dispatcher limita a 1 aviso por segundo
    }
  }

  // Classificacao de botoes (CURTA/LONGA)
  EventoBotao evento;
  if (inputManagerPoll(evento)) {
    gameLogicProcessarEvento(evento);
  }

  // Servidor web: DNS do Portal Cativo + coleta de configuracao nova (thread-safe)
  webServerModuleLoop();
  ParametrosSessao novaConfig;
  if (webServerModuleObterNovaConfig(novaConfig)) {
    // Sprint 2: gameLogicAplicarConfig(novaConfig) - cancela o timer de 5 s (RF30)
    dispararEvento(CONFIGURACAO_OK);  // bipe curto: a configuracao foi aceita
    Serial.printf("[main] Nova config: exercicio E%d, perfil P%d, %d a %d graus, %d repeticoes\n",
                  novaConfig.tipoExercicio, novaConfig.perfil,
                  novaConfig.anguloInicial, novaConfig.anguloMaximo, novaConfig.repeticoes);
  }

  // Desliga o buzzer quando o tempo pedido acabar (nao bloqueante)
  actuatorModuleTick();

  // Timers da maquina de estados - Sprint 2 (RF30)
  gameLogicTick();
}
