#pragma once
#include <Arduino.h>
#include "config.h"
#include "models/SensorModel.h"
#include "models/StorageModel.h"

enum EstadoTPA { IDLE, DRENANDO, PAUSA_POS_DRENO, ENCHENDO, ESTABILIZANDO, ALARME_ERRO };

class TPAController {
private:
  EstadoTPA estado;
  uint32_t tempoInicioEtapa;
  uint32_t ultimoCheckMinuto;
  bool tpaManualEmAndamento = false;
  SystemConfig &config;
  SensorModel &sensores;

  void finalizarCicloTPA(uint32_t tempoEnche, uint16_t nivelEncheMm, const char* motivoEnche);
  void atualizarLedsReservatorio();

public:
  TPAController(SystemConfig &cfg, SensorModel &sens);
  void init();
  void update();
  void dispararTPA();
  void dispararTPAManual();
  void abortar();
  void interromperManualmente();

  bool isEmAndamento() const { return estado != IDLE; }
  String getNomeEstado() const;

  void ligarBombaDrenagemManual();
  void desligarBombaDrenagemManual();
  void ligarBombaEnchimentoManual();
  void desligarBombaEnchimentoManual();

  void recalcularProximaTPAAposCiclo();
  static uint32_t calcularProximaTPAConfiguracao(const char* hora1Str, const char* hora2Str, int periodicidade, SensorModel &sens);

  String getDataUltimaTPA();
  String getDataProximaTPA();
  float getVolumeUltimaTPA() { return config.vol_ultima_tpa; }
};