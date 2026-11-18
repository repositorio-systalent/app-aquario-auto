#include "controllers/WebController.h"
#include "views/WebViews.h"
#include <LittleFS.h>
#include <WiFi.h>

WebController::WebController(SystemConfig &cfg, SensorModel &sens, TPAController &tpaCtrl, TempController &temp)
  : server(80), config(cfg), sensores(sens), tpa(tpaCtrl), tempCtrl(temp) {}

void WebController::init() {
  if (!LittleFS.begin(true)) {
    Serial.println("[WEB] Erro ao montar LittleFS!");
  } else {
    Serial.println("[WEB] LittleFS montado com sucesso. Arquivos encontrados:");
    
    // Lista todos os arquivos presentes na raiz do LittleFS
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while (file) {
      Serial.printf("  -> %s (%d bytes)\n", file.name(), file.size());
      file = root.openNextFile();
    }
  }

  server.on("/", HTTP_GET, [this]() {
    if (LittleFS.exists("/index.html")) {
      File f = LittleFS.open("/index.html", "r");
      server.streamFile(f, "text/html");
      f.close();
    } else {
      server.send(404, "text/plain", "index.html nao encontrado");
    }
  });

  server.on("/style.css", HTTP_GET, [this]() {
    if (LittleFS.exists("/style.css")) {
      File f = LittleFS.open("/style.css", "r");
      server.streamFile(f, "text/css");
      f.close();
    }
  });

  server.on("/script.js", HTTP_GET, [this]() {
    if (LittleFS.exists("/script.js")) {
      File f = LittleFS.open("/script.js", "r");
      server.streamFile(f, "application/javascript");
      f.close();
    }
  });

  server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
  server.on("/api/config", HTTP_GET, [this]() {
    server.send(200, "application/json", WebViews::renderConfigJson(config));
  });
  
  // Rotas separadas para TPA e Wi-Fi
  server.on("/api/config_tpa", HTTP_POST, [this]() { handleSaveConfigTPA(); });
  server.on("/api/config_temp", HTTP_POST, [this]() { handleSaveConfigTemp(); });
  server.on("/api/config_wifi", HTTP_POST, [this]() { handleSaveConfigWiFi(); });
  server.on("/api/wifi_scan", HTTP_GET, [this]() { handleWifiScan(); });

  server.on("/api/bomba", HTTP_GET, [this]() { handleBomba(); });
  server.on("/api/reset_boot", HTTP_POST, [this]() { handleResetBoot(); });

  server.on("/api/logs", HTTP_GET, [this]() {
    StaticJsonDocument<512> doc;
    // Envia os textos puros via JSON ou separados
    String tpaLog = StorageModel::readLogFile("/tpa_history.csv");
    String errLog = StorageModel::readLogFile("/errors.log");
    
    DynamicJsonDocument respDoc(8192);
    respDoc["tpa_log"] = tpaLog;
    respDoc["err_log"] = errLog;
    
    String out;
    serializeJson(respDoc, out);
    server.send(200, "application/json", out);
  });

  server.on("/api/logs/clear", HTTP_POST, [this]() {
    if (server.hasArg("tipo")) {
      String tipo = server.arg("tipo");
      if (tipo == "tpa") StorageModel::clearLogFile("/tpa_history.csv");
      else if (tipo == "erro") StorageModel::clearLogFile("/errors.log");
    }
    server.send(200, "application/json", WebViews::renderSuccessJson("Log limpo"));
  });

  server.on("/api/tpa/abort", HTTP_POST, [this]() {
    tpa.interromperManualmente();
    server.send(200, "application/json", WebViews::renderSuccessJson("TPA interrompida com seguranca"));
  });

  server.on("/api/tpa/start", HTTP_POST, [this]() {
    if (tpa.isEmAndamento()) {
      server.send(400, "application/json", WebViews::renderErrorJson("Ja existe uma TPA em andamento"));
      return;
    }
    tpa.dispararTPAManual();
    server.send(200, "application/json", WebViews::renderSuccessJson("TPA manual iniciada!"));
  });

  server.onNotFound([this]() { handleNotFound(); });

  server.begin();
  Serial.println("[WEB] Servidor HTTP pronto na porta 80");
}

void WebController::handleClient() {
  server.handleClient();

  // Executa o reinício somente após a resposta HTTP já ter sido entregue ao navegador
  if (deveReiniciar && (millis() - tempoPedidoRestart >= 1500)) {
    ESP.restart();
  }
}

void WebController::handleStatus() {
  server.send(200, "application/json", WebViews::renderStatusJson(sensores, config, tpa, tempCtrl));
}

// Salva APENAS parâmetros da tela Configurar TPA (não mexe no Wi-Fi)
void WebController::handleSaveConfigTPA() {
  logPrintln("[WEB] Recebido POST /api/config_tpa");

  if (server.hasArg("plain")) {
    String body = server.arg("plain");
    logPrintf("[WEB] Payload: %s\n", body.c_str());

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
      logPrintf("[WEB] Erro JSON: %s\n", err.c_str());
      server.send(400, "application/json", WebViews::renderErrorJson("JSON invalido"));
      return;
    }

    config.tpa_periodicidade = doc["tpa_periodicidade"] | config.tpa_periodicidade;
    strlcpy(config.tpa_hora1, doc["tpa_hora1"] | config.tpa_hora1, sizeof(config.tpa_hora1));
    strlcpy(config.tpa_hora2, doc["tpa_hora2"] | config.tpa_hora2, sizeof(config.tpa_hora2));
    config.aq_dist_max_mm = doc["aq_dist_max_mm"] | config.aq_dist_max_mm;
    config.aq_dist_min_mm = doc["aq_dist_min_mm"] | config.aq_dist_min_mm;
    config.res_dist_max_mm = doc["res_dist_max_mm"] | config.res_dist_max_mm;
    config.res_dist_min_mm = doc["res_dist_min_mm"] | config.res_dist_min_mm;
    config.res_volume_total_l = doc["res_volume_total_l"] | config.res_volume_total_l;
    config.res_percent_critico = doc["res_percent_critico"] | config.res_percent_critico;
    config.tempo_max_drenagem_seg = doc["tempo_max_drenagem_seg"] | config.tempo_max_drenagem_seg;
    config.tempo_max_enchimento_seg = doc["tempo_max_enchimento_seg"] | config.tempo_max_enchimento_seg;
    config.tempo_pausa_pos_dreno_seg = doc["tempo_pausa_pos_dreno_seg"] | config.tempo_pausa_pos_dreno_seg;
    config.tempo_estabilizacao_seg = doc["tempo_estabilizacao_seg"] | config.tempo_estabilizacao_seg;
    config.tolerancia_estabilizacao_mm = doc["tolerancia_estabilizacao_mm"] | config.tolerancia_estabilizacao_mm;

    config.proxima_tpa_epoch = TPAController::calcularProximaTPAConfiguracao(
      config.tpa_hora1, config.tpa_hora2, config.tpa_periodicidade, sensores
    );

    bool ok = StorageModel::saveConfig(config);
    if (ok) {
      server.send(200, "application/json", WebViews::renderSuccessJson("Configuracoes de TPA salvas. Reiniciando..."));
      deveReiniciar = true;
      tempoPedidoRestart = millis();
    } else {
      server.send(500, "application/json", WebViews::renderErrorJson("Falha ao gravar no SD Card"));
    }
  } else {
    logPrintln("[WEB] Erro: Payload vazio em /api/config_tpa");
    server.send(400, "application/json", WebViews::renderErrorJson("Payload vazio"));
  }
}

void WebController::handleSaveConfigTemp() {
  logPrintln("[WEB] Recebido POST /api/config_temp");

  if (!server.hasArg("plain")) {
    server.send(400, "application/json", WebViews::renderErrorJson("Payload vazio"));
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "application/json", WebViews::renderErrorJson("JSON invalido"));
    return;
  }

  if (doc.containsKey("temp_controle_ativo")) {
    config.temp_controle_ativo = doc["temp_controle_ativo"];
  }
  config.temp_min_liga = doc["temp_min_liga"] | config.temp_min_liga;
  config.temp_min_desliga = doc["temp_min_desliga"] | config.temp_min_desliga;
  config.temp_max_liga = doc["temp_max_liga"] | config.temp_max_liga;
  config.temp_max_desliga = doc["temp_max_desliga"] | config.temp_max_desliga;

  bool ok = StorageModel::saveConfig(config);
  if (ok) {
    logPrintf("[TEMP-CFG] Salvo: Ativo=%d | Aquec: %.1f->%.1f | Cooler: %.1f->%.1f\n",
              config.temp_controle_ativo, config.temp_min_liga, config.temp_min_desliga,
              config.temp_max_liga, config.temp_max_desliga);
    server.send(200, "application/json", WebViews::renderSuccessJson("Configuracoes de temperatura salvas com sucesso!"));
  } else {
    server.send(500, "application/json", WebViews::renderErrorJson("Falha ao gravar configuracoes de temperatura"));
  }
}

// Salva APENAS parâmetros de Wi-Fi (preserva senha antiga se campo vier vazio)
void WebController::handleSaveConfigWiFi() {
  logPrintln("[WEB] Recebido POST /api/config_wifi");

  if (server.hasArg("plain")) {
    StaticJsonDocument<512> doc;
    if (deserializeJson(doc, server.arg("plain"))) {
      server.send(400, "application/json", WebViews::renderErrorJson("JSON invalido"));
      return;
    }

    const char* novoSsid = doc["wifi_ssid"] | "";
    const char* novaSenha = doc["wifi_pass"] | "";

    if (strlen(novoSsid) > 0) {
      strlcpy(config.wifi_ssid, novoSsid, sizeof(config.wifi_ssid));
    }

    // Só altera a senha se o usuário digitou algo no campo
    if (strlen(novaSenha) > 0) {
      strlcpy(config.wifi_pass, novaSenha, sizeof(config.wifi_pass));
    }

    StorageModel::saveConfig(config);
    server.send(200, "application/json", WebViews::renderSuccessJson("Configuracoes de TPA salvas. Reiniciando..."));
    
    // Agenda o restart para daqui a 1,5s, permitindo o fechamento limpo da conexão TCP
    deveReiniciar = true;
    tempoPedidoRestart = millis();

  } else {
    logPrintln("[WEB] Erro: Payload vazio em /api/config_wifi");
    server.send(400, "application/json", WebViews::renderErrorJson("Payload vazio"));
  }
}

// Escaneia redes Wi-Fi próximas e devolve lista JSON
void WebController::handleWifiScan() {
  int n = WiFi.scanNetworks();
  DynamicJsonDocument doc(2048);
  JsonArray redes = doc.to<JsonArray>();

  for (int i = 0; i < n; i++) {
    JsonObject item = redes.createNestedObject();
    item["ssid"] = WiFi.SSID(i);
    item["rssi"] = WiFi.RSSI(i);
  }
  WiFi.scanDelete();

  String resp;
  serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void WebController::handleBomba() {
  if (server.hasArg("tipo") && server.hasArg("acao")) {
    String tipo = server.arg("tipo");
    String acao = server.arg("acao");

    if (tipo == "dreno") {
      if (acao == "liga") tpa.ligarBombaDrenagemManual();
      else tpa.desligarBombaDrenagemManual();
    } else if (tipo == "enche") {
      if (acao == "liga") tpa.ligarBombaEnchimentoManual();
      else tpa.desligarBombaEnchimentoManual();
    }
    server.send(200, "text/plain", "OK");
  }
}

void WebController::handleResetBoot() {
  StorageModel::resetBootCounter(config);
  server.send(200, "application/json", WebViews::renderSuccessJson("Contador zerado"));
}

void WebController::handleNotFound() { server.send(404, "text/plain", "Recurso nao encontrado"); }