# ArduKnee - firmware (Sprint 1)

Protótipo educacional. Os limites de ângulo e de repetições devem ser definidos com um fisioterapeuta.

## Estrutura (cada pasta = um sketch; o nome da pasta é igual ao do .ino)

| Pasta | Para que serve |
|---|---|
| `ArduKnee_Firmware/` | Firmware oficial, modular. Use este para entrega. |
| `ArduKnee_Unico/` | Mesmo firmware num arquivo só, gerado dos módulos. Só para teste rápido (viola a RNF17). |
| `CalibracaoSensor/` | Sketch separado para medir o potenciômetro em 0° e 90°. |
| `Wokwi_ArduKnee/` | Cópia para o simulador wokwi.com (circuito `diagram.json` + chave `SEM_WIFI`). Não é para a placa real. |

Nunca coloque dois `.ino` na mesma pasta: a Arduino IDE junta todos num programa só e a compilação falha.

## Ambiente

- Arduino IDE, placa **ESP32 Dev Module**, pacote **esp32 (Espressif) versão 3.x** (a compilação foi testada com a 3.3.12).
- Bibliotecas da organização **ESP32Async** (github.com/ESP32Async): `ESPAsyncWebServer` e `AsyncTCP`.
  A versão antiga (me-no-dev) não é confiável no core 3.x.
- Módulo **ESP32-WROOM-32** (os botões usam GPIO 16 e 17, que não existem em módulos WROVER).

## Antes de ir para a bancada

1. **Senha do Wi-Fi.** Este pacote **não traz** o `Segredos.h`: copie `ArduKnee_Firmware/Segredos.h.example` para `ArduKnee_Firmware/Segredos.h` e troque a senha (8 a 63 caracteres). Anote-a na caixa do aparelho. O `Segredos.h` está no `.gitignore`: nunca o coloque num zip ou repositório que for compartilhado. Se usar `ArduKnee_Unico`, a senha-modelo está dentro do próprio arquivo (bloco "Segredos.h.example"): troque-a lá e não compartilhe o arquivo depois.
2. **Calibração** (antes de qualquer teste de ângulo). Grave `CalibracaoSensor` com o 5 V do MT3608 desconectado do VIN. Com a perna em 0° e depois em 90° (transferidor), anote a "Média" estável. O esperado é ~1350 em 0° e ~2666 em 90°; valores muito fora disso indicam ligação errada. Escreva `ADC_EM_0_GRAUS` e `ADC_EM_90_GRAUS` em `SensorModule.cpp` e mude `CALIBRACAO_FEITA` para `true`. Para conferir, acrescente temporariamente no bloco de 20 Hz do `.ino` `Serial.printf("ADC=%u ang=%.1f\n", leitura.adcFiltrado, leitura.anguloFiltrado);`, confira 30° e 60° (erro máximo aceito: ±5°, RNF26) e apague a linha.
3. Ao gravar por USB, desconecte o 5 V do MT3608 do pino VIN.

## Configuração da sessão (página do portal)

Perfil clínico P1 a P6 e tipo de exercício: E1 Flexão, E2 Extensão, E3 Ciclo completo. No JSON o campo `anguloMaximo` significa "ângulo alvo": na flexão e no ciclo completo o alvo fica **acima** do inicial; na extensão, **abaixo**. Repetições de 1 a 100.

## Buzzer

O buzzer é ativo (tom fixo). `actuatorModuleTocarBuzzer()` liga e `actuatorModuleTick()`, chamada a cada `loop()`, desliga sozinho depois do tempo pedido. Não usa `tone()` nem `delay()`. Por enquanto o único gatilho é o bipe curto (150 ms) de `CONFIGURACAO_OK`, disparado pelo `FeedbackDispatcher` quando uma configuração válida chega pelo portal; os demais padrões de som, LEDs e servo são da Sprint 2.

## Teste de aceite na placa (Arduino IDE: ESP32 Dev Module, core 3.x, bibliotecas ESP32Async)

No celular, pelo portal:
- E1 com inicial 10 e alvo 70 deve salvar.
- E2 com inicial 70 e alvo 20 deve salvar.
- E2 com inicial 20 e alvo 70 deve dar erro.
- E1 com alvo menor que o inicial deve dar erro.

O buzzer deve apitar e parar sozinho, sem travar o loop.

## Testes feitos

**Simulador no computador (sem placa):** validação da configuração (tipo, perfil, ângulos e repetições), os quatro casos de aceite acima passando pelos handlers HTTP, rotas do portal cativo (iOS, Android e Windows), buzzer (liga, desliga sozinho e volta do `millis()`), sensor (instabilidade e calibração), debounce e CURTA/LONGA.

**Compilação real:** `ArduKnee_Firmware`, `ArduKnee_Unico`, `CalibracaoSensor` e `Wokwi_ArduKnee` (com e sem `SEM_WIFI`) compilam para ESP32 Dev Module com core esp32 3.3.12, ESP Async WebServer 3.12.1 e Async TCP 4.0.0, sem avisos.

**Falta testar só em placa:** Wi-Fi e portal cativo no celular (Android e iOS), ADC e calibração, buzzer físico e o circuito `diagram.json` aberto no Wokwi.

## Simulador Wokwi

Veja `Wokwi_ArduKnee/LEIAME_WOKWI.md`. A pasta é gerada de `ArduKnee_Firmware/`: se mudar o firmware, copie de novo os módulos, **exceto `WebServerModule.cpp`**, que tem o bloco `SEM_WIFI`: repita as mesmas edições à mão dentro do `#else`. O `ArduKnee_Unico` deve ser regenerado dos módulos sempre que o firmware mudar. O Wokwi (até onde sei) não simula o ESP32 como Access Point, então lá o servidor web fica desligado (`SEM_WIFI`).
