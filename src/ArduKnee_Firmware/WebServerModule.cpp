#include "WebServerModule.h"
#include "ProtocoloHttp.h"
#include "PaginaConfiguracao.h"
#include "Segredos.h"
#include "Log.h"
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <string.h>
#include <stdlib.h>

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
