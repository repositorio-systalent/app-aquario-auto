#include "controllers/TempController.h"

TempController::TempController(SystemConfig &cfg, SensorModel &sens)
  : config(cfg), sensores(sens) {}

void TempController::init() {
  pinMode(PIN_RELE_RESERVA1, OUTPUT);
  pinMode(PIN_RELE_RESERVA2, OUTPUT);
  desligarTudo();
  logPrintln("[TEMP] Controlador termico iniciado (GPIO 27: Aquecedor | GPIO 14: Cooler)");
}

void TempController::desligarTudo() {
  if (aquecedorLigado) {
    aquecedorLigado = false;
    logPrintln("[TEMP] Aquecedor DESLIGADO.");
  }
  if (coolerLigado) {
    coolerLigado = false;
    logPrintln("[TEMP] Cooler DESLIGADO.");
  }
  digitalWrite(PIN_RELE_RESERVA1, HIGH);
  digitalWrite(PIN_RELE_RESERVA2, HIGH);
}

void TempController::update(bool tpaEmAndamento) {
  // Verifica a cada 2 segundos para evitar ruído de leitura instantânea
  if (millis() - ultimoCheckMs < 2000) return;
  ultimoCheckMs = millis();

  if (!config.temp_controle_ativo || tpaEmAndamento) {
    desligarTudo();
    return;
  }

  float temp = sensores.getTemperatura();

  // Proteção contra sensor desconectado, em curto ou sob sol direto (> 45°C)
  if (isnan(temp) || temp < 5.0f || temp > 45.0f) {
    desligarTudo();
    if (!erroSensorRegistrado) {
      erroSensorRegistrado = true;
      char msg[120];
      snprintf(msg, sizeof(msg), "Controle termico suspenso: leitura NTC fora de faixa (%.1f C)", temp);
      logPrintln(String("[TEMP-ALERTA] ") + msg);
      StorageModel::logError(sensores.getDataHoraAtual().c_str(), msg);
    }
    return;
  }
  erroSensorRegistrado = false;

  // --- LÓGICA DE AQUECIMENTO (GPIO 27) ---
  if (!aquecedorLigado && temp <= config.temp_min_liga) {
    coolerLigado = false;
    digitalWrite(PIN_RELE_RESERVA2, HIGH);

    aquecedorLigado = true;
    digitalWrite(PIN_RELE_RESERVA1, LOW);
    logPrintf("[TEMP] Temp %.1f C <= %.1f C -> Aquecedor LIGADO\n", temp, config.temp_min_liga);
  } else if (aquecedorLigado && temp >= config.temp_min_desliga) {
    aquecedorLigado = false;
    digitalWrite(PIN_RELE_RESERVA1, HIGH);
    logPrintf("[TEMP] Temp %.1f C >= %.1f C -> Aquecedor DESLIGADO\n", temp, config.temp_min_desliga);
  }

  // --- LÓGICA DE RESFRIAMENTO / COOLER (GPIO 14) ---
  if (!coolerLigado && temp >= config.temp_max_liga) {
    aquecedorLigado = false;
    digitalWrite(PIN_RELE_RESERVA1, HIGH);

    coolerLigado = true;
    digitalWrite(PIN_RELE_RESERVA2, LOW);
    logPrintf("[TEMP] Temp %.1f C >= %.1f C -> Cooler LIGADO\n", temp, config.temp_max_liga);
  } else if (coolerLigado && temp <= config.temp_max_desliga) {
    coolerLigado = false;
    digitalWrite(PIN_RELE_RESERVA2, HIGH);
    logPrintf("[TEMP] Temp %.1f C <= %.1f C -> Cooler DESLIGADO\n", temp, config.temp_max_desliga);
  }
}