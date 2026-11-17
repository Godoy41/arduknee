#include "Armazenamento.h"
#include <Preferences.h>

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
