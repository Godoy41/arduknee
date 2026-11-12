// Sketch isolado so para calibrar o potenciometro (Etapa 2 do guia).
// Nao faz parte do firmware final - e' so uma ferramenta de medicao.
// Depois de anotar os valores em 0 e 90 graus, delete este sketch
// e coloque os numeros no SensorModule.cpp do projeto principal.

const uint8_t PINO_POTENCIOMETRO = 34; // ADC1 - GPIO34
const uint8_t N_AMOSTRAS = 8;

uint16_t buffer[N_AMOSTRAS] = {0};
uint8_t indice = 0;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetPinAttenuation(PINO_POTENCIOMETRO, ADC_11db); // mesma atenuacao do firmware (circuito v2.7)
  pinMode(PINO_POTENCIOMETRO, INPUT);
  Serial.println("Calibracao iniciada. Gire o eixo e anote os valores estaveis em 0 e 90 graus.");
}

void loop() {
  uint16_t bruto = analogRead(PINO_POTENCIOMETRO);
  buffer[indice] = bruto;
  indice = (indice + 1) % N_AMOSTRAS;

  uint32_t soma = 0;
  for (uint8_t i = 0; i < N_AMOSTRAS; i++) soma += buffer[i];
  uint16_t media = soma / N_AMOSTRAS;

  Serial.print("ADC bruto: ");
  Serial.print(bruto);
  Serial.print("   |   Media (8 amostras): ");
  Serial.println(media);

  delay(200); // aqui pode usar delay - e' so ferramenta de bancada, nao e' o loop principal do firmware
}
