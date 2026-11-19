#include "controllers/TempController.h"
#include "logic/AquarioLogic.h"

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
  if (millis() - ultimoCheckMs < 2000) return;
  ultimoCheckMs = millis();

  float temp = sensores.getTemperatura();

  AquarioLogic::DecisaoTermica decisao = AquarioLogic::avaliarControleTermico(
    temp,
    config.temp_controle_ativo,
    tpaEmAndamento,
    aquecedorLigado,
    coolerLigado,
    config.temp_min_liga,
    config.temp_min_desliga,
    config.temp_max_liga,
    config.temp_max_desliga
  );

  // Tratamento de alerta caso o sensor NTC saia da faixa válida (5°C a 45°C)
  if (decisao.erroSensor) {
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

  // Aplica estado do Aquecedor (GPIO 27) somente quando houver mudança
  if (decisao.ligarAquecedor != aquecedorLigado) {
    aquecedorLigado = decisao.ligarAquecedor;
    digitalWrite(PIN_RELE_RESERVA1, aquecedorLigado ? LOW : HIGH);
    logPrintf("[TEMP] Temp %.1f C -> Aquecedor %s\n", temp, aquecedorLigado ? "LIGADO" : "DESLIGADO");
  }

  // Aplica estado do Cooler (GPIO 14) somente quando houver mudança
  if (decisao.ligarCooler != coolerLigado) {
    coolerLigado = decisao.ligarCooler;
    digitalWrite(PIN_RELE_RESERVA2, coolerLigado ? LOW : HIGH);
    logPrintf("[TEMP] Temp %.1f C -> Cooler %s\n", temp, coolerLigado ? "LIGADO" : "DESLIGADO");
  }
}