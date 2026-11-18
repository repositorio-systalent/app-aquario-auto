#pragma once
#include <Arduino.h>
#include "config.h"
#include "models/SensorModel.h"
#include "models/StorageModel.h"

class TempController {
private:
  SystemConfig &config;
  SensorModel &sensores;
  bool aquecedorLigado = false;
  bool coolerLigado = false;
  bool erroSensorRegistrado = false;
  uint32_t ultimoCheckMs = 0;

  void desligarTudo();

public:
  TempController(SystemConfig &cfg, SensorModel &sens);
  void init();
  void update(bool tpaEmAndamento);

  bool isAquecedorLigado() const { return aquecedorLigado; }
  bool isCoolerLigado() const { return coolerLigado; }
};