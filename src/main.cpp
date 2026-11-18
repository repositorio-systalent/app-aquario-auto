
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>
#include <stdarg.h>
#include <esp_system.h>
#include "config.h"
#include "models/StorageModel.h"
#include "models/SensorModel.h"
#include "controllers/TPAController.h"
#include "controllers/WebController.h"

#define WDT_TIMEOUT_SEG 10

SystemConfig sysConfig;
SensorModel sensores;
TPAController tpa(sysConfig, sensores);
TempController tempCtrl(sysConfig, sensores);
WebController web(sysConfig, sensores, tpa, tempCtrl);

WiFiServer telnetServer(23);
WiFiClient telnetClient;

void logPrint(const String &msg) {
  Serial.print(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(msg);
  }
}

void logPrintln(const String &msg) {
  Serial.println(msg);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.println(msg);
  }
}

void logPrintf(const char *format, ...) {
  char buf[512];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  Serial.print(buf);
  if (telnetClient && telnetClient.connected()) {
    telnetClient.print(buf);
  }
}

void setupOTA() {
  ArduinoOTA.setHostname("aquario-esp32");
   // Opcional: descomente para exigir senha no upload Wi-Fi
  ArduinoOTA.setPassword("admin123");

  ArduinoOTA.onStart([]() {
    tpa.abortar();
    esp_task_wdt_delete(NULL);
    logPrintln("[OTA] Iniciando atualizacao via Wi-Fi...");
  });

  ArduinoOTA.onEnd([]() {
    logPrintln("\n[OTA] Atualizacao concluida! Reiniciando...");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("[OTA] Progresso: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    logPrintf("[OTA] Erro [%u]\n", error);
    ESP.restart();
  });

  ArduinoOTA.begin();
  logPrintln("[OTA] Servico OTA ativo (Hostname: aquario-esp32)");
}

const char* obterMotivoReset(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON:   return "POWERON (Energia ligada/religada)";
    case ESP_RST_EXT:       return "EXT (Pino EN/Reset externo)";
    case ESP_RST_SW:        return "SW (Reinicio via software / ESP.restart)";
    case ESP_RST_PANIC:     return "PANIC (Crash/Excecao no firmware)";
    case ESP_RST_INT_WDT:   return "INT_WDT (Watchdog de interrupcao)";
    case ESP_RST_TASK_WDT:  return "TASK_WDT (Travamento do loop > 10s)";
    case ESP_RST_WDT:       return "WDT (Outro Watchdog de hardware)";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP (Saida de sono profundo)";
    case ESP_RST_BROWNOUT:  return "BROWNOUT (Queda de tensao na alimentacao!)";
    case ESP_RST_SDIO:      return "SDIO (Reset via SDIO)";
    default:                return "DESCONHECIDO";
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.printf("[SYS] SDK Version: %s\n", ESP.getSdkVersion());

  // Reconfigura o Watchdog já inicializado pelo core Arduino 3.x
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_SEG * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_reconfigure(&wdt_config);
  esp_task_wdt_add(NULL);

  LittleFS.begin(true);
  StorageModel::initSD();
  StorageModel::loadConfig(sysConfig);
  StorageModel::incrementBootCounter(sysConfig);

  sensores.initSensors();
  // Registra o motivo exato do último boot no log de erros (ignora reinício intencional via software)
  esp_reset_reason_t motivo = esp_reset_reason();
  if (motivo != ESP_RST_SW) {
    char msgBoot[160];
    snprintf(msgBoot, sizeof(msgBoot), "Boot #%lu | Motivo: %s | Heap livre: %u bytes",
             sysConfig.boot_counter, obterMotivoReset(motivo), ESP.getFreeHeap());
    logPrintln(String("[BOOT-DIAG] ") + msgBoot);
    StorageModel::logError(sensores.getDataHoraAtual().c_str(), msgBoot);
  }

  tpa.init();
  tempCtrl.init();

  WiFi.mode(WIFI_STA);
  WiFi.begin(sysConfig.wifi_ssid, sysConfig.wifi_pass);
  Serial.printf("[WIFI] Conectando a %s", sysConfig.wifi_ssid);

  uint8_t timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial.print(".");
    esp_task_wdt_reset();
    timeout++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Conectado! IP: " + WiFi.localIP().toString());
    sensores.sincronizarNTPeAtualizarRTC();
    setupOTA();
    telnetServer.begin();
    telnetServer.setNoDelay(true);
  } else {
    Serial.println("\n[WIFI] Modo Offline.");
  }

  web.init();
}

void loop() {
  esp_task_wdt_reset();
  ArduinoOTA.handle();

  // Gerencia conexões Telnet
  if (telnetServer.hasClient()) {
    if (!telnetClient || !telnetClient.connected()) {
      if (telnetClient) telnetClient.stop();
      telnetClient = telnetServer.available();
      telnetClient.println("=== TERMINAL REMOTO AQUARIO CONECTADO ===");
    } else {
      telnetServer.available().stop();
    }
  }

  web.handleClient();
  tpa.update();
  tempCtrl.update(tpa.isEmAndamento());
  delay(10);
}