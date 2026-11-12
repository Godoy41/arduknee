#include "WebServerModule.h"
#include "ValidacaoConfig.h"
#include "PaginaConfiguracao.h"
#include "Segredos.h"
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
