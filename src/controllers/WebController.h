#pragma once
#include <WebServer.h>
#include <ArduinoJson.h>
#include "config.h"
#include "models/SensorModel.h"
#include "models/StorageModel.h"
#include "controllers/TPAController.h"
#include "controllers/TempController.h"

class WebController {
private:
  WebServer server;
  SystemConfig &config;
  SensorModel &sensores;
  TPAController &tpa;
  TempController &tempCtrl;

  bool deveReiniciar = false;
  uint32_t tempoPedidoRestart = 0;

  void handleStatus();
  void handleSaveConfig();
  void handleBomba();
  void handleResetBoot();
  void handleNotFound();
  void handleSaveConfigTPA();
  void handleSaveConfigTemp();
  void handleSaveConfigWiFi();
  void handleWifiScan();
  

public:
  WebController(SystemConfig &cfg, SensorModel &sens, TPAController &tpaCtrl, TempController &temp);
  void init();
  void handleClient();
};