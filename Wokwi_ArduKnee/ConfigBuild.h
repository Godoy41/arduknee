#ifndef CONFIG_BUILD_H
#define CONFIG_BUILD_H

// Chave so' para o simulador Wokwi (que nao simula o ESP32 como Access Point).
// Para a placa real, use a pasta ArduKnee_Firmware - NAO este Wokwi_ArduKnee.
// Se quiser testar o WiFi aqui mesmo, apague a linha abaixo, copie ../ArduKnee_Firmware/Segredos.h.example para Segredos.h
// e descomente o libraries.txt.
#define SEM_WIFI

#endif
