# Como usar no Wokwi (wokwi.com)

1. Crie um projeto **ESP32** (DevKit V1) com **Arduino**.
2. Apague o `sketch.ino` padrao e envie TODOS os arquivos desta pasta (o principal e' o `sketch.ino`).
3. Cole o `diagram.json` na aba de mesmo nome. Pinos: potenciometro GPIO34; botoes GPIO4, 16 e 17;
   LEDs 14,27,26,25,33,32,18,19,21,22 (resistor 220 ohm); buzzer 23; servo 13.
4. Clique em Play e abra o Serial Monitor.

## O que da para testar
- Leitura do angulo girando o potenciometro (aviso "sensor NAO calibrado" e' normal ate calibrar).
- Botoes: curta (< 2 s) e longa (>= 2 s) aparecem no Serial.
- Instabilidade: gire o potenciometro de um extremo ao outro de uma vez; no maximo 1 aviso por segundo.

## Limitacoes
- WiFi/portal cativo desligado (SEM_WIFI): teste isso numa placa real.
- Buzzer ligado direto no GPIO (na placa real passa pelo transistor 2N2222).
- LEDs e servo so' reagem quando a Sprint 2 for implementada.
- Este diagrama nao foi aberto no Wokwi por quem o gerou: se alguma peca/fio estiver errado, ajuste pela tela.
