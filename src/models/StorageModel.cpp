#include "StorageModel.h"

static const char* CONFIG_FILE = "/config.json";
static const char* LOG_FILE = "/tpa_history.csv";
static bool sdDisponivel = false;

bool StorageModel::initSD() {
  // Clock de 4MHz garante estabilidade total em protoboard/jumpers
  if (!SD.begin(PIN_SD_CS, SPI, 4000000)) {
    logPrintln("[STORAGE] Aviso: Cartao SD nao montado. Usando Flash interna (LittleFS) como backup.");
    sdDisponivel = false;
    return false;
  }
  sdDisponivel = true;
  logPrintln("[STORAGE] Cartao SD montado com sucesso (4MHz)");
  return true;
}

bool StorageModel::loadConfig(SystemConfig &config) {
  File f;
  if (sdDisponivel && SD.exists(CONFIG_FILE)) {
    f = SD.open(CONFIG_FILE, FILE_READ);
  } else if (LittleFS.exists(CONFIG_FILE)) {
    logPrintln("[STORAGE] Carregando configuracao da Flash interna (LittleFS)...");
    f = LittleFS.open(CONFIG_FILE, "r");
  }

  if (!f) {
    strlcpy(config.wifi_ssid, "BARROS1", sizeof(config.wifi_ssid));
    strlcpy(config.wifi_pass, "sofia2410", sizeof(config.wifi_pass));
    config.tpa_periodicidade = 1;
    strlcpy(config.tpa_hora1, "09:00", sizeof(config.tpa_hora1));
    strlcpy(config.tpa_hora2, "21:00", sizeof(config.tpa_hora2));
    config.proxima_tpa_epoch = 0;
    config.ultima_tpa_epoch = 0;
    config.vol_ultima_tpa = 0.0;
    config.aq_dist_max_mm = 140;
    config.aq_dist_min_mm = 185;
    config.res_dist_max_mm = 180;
    config.res_dist_min_mm = 680;
    config.res_volume_total_l = 20.0;
    config.res_percent_critico = 15.0;
    config.tempo_max_drenagem_seg = 30;
    config.tempo_max_enchimento_seg = 150;
    config.tempo_pausa_pos_dreno_seg = 5;
    config.tempo_estabilizacao_seg = 3;
    config.tolerancia_estabilizacao_mm = 2;
    config.boot_counter = 0;
    config.temp_controle_ativo = true;
    config.temp_min_liga = 24.8;
    config.temp_min_desliga = 25.3;
    config.temp_max_liga = 26.8;
    config.temp_max_desliga = 26.2;
    saveConfig(config);
    return true;
  }

  DynamicJsonDocument doc(1536);
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) return false;

  strlcpy(config.wifi_ssid, doc["wifi_ssid"] | "SEU_WIFI", sizeof(config.wifi_ssid));
  strlcpy(config.wifi_pass, doc["wifi_pass"] | "SUA_SENHA", sizeof(config.wifi_pass));
  config.tpa_periodicidade = doc["tpa_periodicidade"] | 1;
  strlcpy(config.tpa_hora1, doc["tpa_hora1"] | "09:00", sizeof(config.tpa_hora1));
  strlcpy(config.tpa_hora2, doc["tpa_hora2"] | "21:00", sizeof(config.tpa_hora2));
  config.proxima_tpa_epoch = doc["proxima_tpa_epoch"] | 0;
  config.ultima_tpa_epoch = doc["ultima_tpa_epoch"] | 0;
  config.vol_ultima_tpa = doc["vol_ultima_tpa"] | 0.0;
  config.aq_dist_max_mm = doc["aq_dist_max_mm"] | 140;
  config.aq_dist_min_mm = doc["aq_dist_min_mm"] | 185;
  config.res_dist_max_mm = doc["res_dist_max_mm"] | 180;
  config.res_dist_min_mm = doc["res_dist_min_mm"] | 680;
  config.res_volume_total_l = doc["res_volume_total_l"] | 20.0;
  config.res_percent_critico = doc["res_percent_critico"] | 15.0;
  config.tempo_max_drenagem_seg = doc["tempo_max_drenagem_seg"] | 30;
  config.tempo_max_enchimento_seg = doc["tempo_max_enchimento_seg"] | 150;
  config.tempo_pausa_pos_dreno_seg = doc["tempo_pausa_pos_dreno_seg"] | 5;
  config.tempo_estabilizacao_seg = doc["tempo_estabilizacao_seg"] | 3;
  config.tolerancia_estabilizacao_mm = doc["tolerancia_estabilizacao_mm"] | 2;
  config.boot_counter = doc["boot_counter"] | 0;
  config.temp_controle_ativo = doc["temp_controle_ativo"] | true;
  config.temp_min_liga = doc["temp_min_liga"] | 24.8;
  config.temp_min_desliga = doc["temp_min_desliga"] | 25.3;
  config.temp_max_liga = doc["temp_max_liga"] | 26.8;
  config.temp_max_desliga = doc["temp_max_desliga"] | 26.2;

  return true;
}

bool StorageModel::saveConfig(const SystemConfig &config) {
  DynamicJsonDocument doc(1536);
  doc["wifi_ssid"] = config.wifi_ssid;
  doc["wifi_pass"] = config.wifi_pass;
  doc["tpa_periodicidade"] = config.tpa_periodicidade;
  doc["tpa_hora1"] = config.tpa_hora1;
  doc["tpa_hora2"] = config.tpa_hora2;
  doc["proxima_tpa_epoch"] = config.proxima_tpa_epoch;
  doc["ultima_tpa_epoch"] = config.ultima_tpa_epoch;
  doc["vol_ultima_tpa"] = config.vol_ultima_tpa;
  doc["aq_dist_max_mm"] = config.aq_dist_max_mm;
  doc["aq_dist_min_mm"] = config.aq_dist_min_mm;
  doc["res_dist_max_mm"] = config.res_dist_max_mm;
  doc["res_dist_min_mm"] = config.res_dist_min_mm;
  doc["res_volume_total_l"] = config.res_volume_total_l;
  doc["res_percent_critico"] = config.res_percent_critico;
  doc["tempo_max_drenagem_seg"] = config.tempo_max_drenagem_seg;
  doc["tempo_max_enchimento_seg"] = config.tempo_max_enchimento_seg;
  doc["boot_counter"] = config.boot_counter;
  doc["tempo_pausa_pos_dreno_seg"] = config.tempo_pausa_pos_dreno_seg;
  doc["tempo_estabilizacao_seg"] = config.tempo_estabilizacao_seg;
  doc["tolerancia_estabilizacao_mm"] = config.tolerancia_estabilizacao_mm;
  doc["temp_controle_ativo"] = config.temp_controle_ativo;
  doc["temp_min_liga"] = config.temp_min_liga;
  doc["temp_min_desliga"] = config.temp_min_desliga;
  doc["temp_max_liga"] = config.temp_max_liga;
  doc["temp_max_desliga"] = config.temp_max_desliga;

  // 1. Grava cópia rápida na Flash interna (LittleFS)
  bool okFlash = false;
  File fFlash = LittleFS.open(CONFIG_FILE, "w");
  if (fFlash) {
    okFlash = (serializeJsonPretty(doc, fFlash) > 0);
    fFlash.close();
  }

  // 2. Grava no Cartão SD se estiver disponível
  bool okSD = false;
  if (sdDisponivel) {
    if (SD.exists(CONFIG_FILE)) SD.remove(CONFIG_FILE);
    File fSD = SD.open(CONFIG_FILE, FILE_WRITE);
    if (fSD) {
      okSD = (serializeJsonPretty(doc, fSD) > 0);
      fSD.flush();
      fSD.close();
    }
  }

  logPrintf("[STORAGE] Config salva (Flash: %s | SD: %s)\n", okFlash ? "OK" : "ERRO", okSD ? "OK" : "FALHA");
  return (okFlash || okSD);
}

void StorageModel::logTPA(const char* dataHora, uint32_t tempoDreno, const char* motivoDreno, uint32_t tempoEnche, const char* motivoEnche) {
  logTPA(dataHora, tempoDreno, 0, motivoDreno, tempoEnche, 0, motivoEnche);
}

void StorageModel::logTPA(const char* dataHora, uint32_t tempoDreno, uint16_t nivelDrenoMm, const char* motivoDreno, uint32_t tempoEnche, uint16_t nivelEncheMm, const char* motivoEnche) {
  if (!sdDisponivel) {
    logPrintln("[STORAGE] ERRO: Cartao SD indisponivel para gravar log de TPA.");
    return;
  }

  File f = SD.open(LOG_FILE, FILE_APPEND);
  if (f) {
    f.printf("%s | Dreno: %us (%umm - %s) | Enche: %us (%umm - %s)\n",
             dataHora, tempoDreno, nivelDrenoMm, motivoDreno, tempoEnche, nivelEncheMm, motivoEnche);
    f.flush();
    f.close();
    logPrintln("[STORAGE] Log de TPA gravado no Cartao SD com sucesso.");
  } else {
    logPrintln("[STORAGE] ERRO: Falha ao abrir /tpa_history.csv no Cartao SD.");
  }
}

void StorageModel::logError(const char* dataHora, const char* mensagem) {
  if (!sdDisponivel) {
    logPrintln("[STORAGE] ERRO: Cartao SD indisponivel para gravar log de erro.");
    return;
  }

  File f = SD.open("/errors.log", FILE_APPEND);
  if (f) {
    f.printf("[%s] %s\n", dataHora, mensagem);
    f.flush();
    f.close();
    logPrintf("[STORAGE] Log de erro/evento gravado no SD: %s\n", mensagem);
  } else {
    logPrintln("[STORAGE] ERRO: Falha ao abrir /errors.log no Cartao SD.");
  }
}

void StorageModel::incrementBootCounter(SystemConfig &config) {
  config.boot_counter++;
  saveConfig(config);
}

void StorageModel::resetBootCounter(SystemConfig &config) {
  config.boot_counter = 0;
  saveConfig(config);
}

String StorageModel::readLogFile(const char* path) {
  if (!sdDisponivel || !SD.exists(path)) return "Nenhum registro encontrado no Cartao SD.";
  
  File f = SD.open(path, FILE_READ);
  if (!f) return "Erro ao abrir arquivo no Cartao SD.";

  size_t tamanho = f.size();
  if (tamanho > 4096) {
    f.seek(tamanho - 4096);
  }

  String conteudo = "";
  while (f.available()) {
    conteudo += (char)f.read();
  }
  f.close();
  return (conteudo.length() > 0) ? conteudo : "Arquivo vazio.";
}

void StorageModel::clearLogFile(const char* path) {
  if (sdDisponivel && SD.exists(path)) {
    SD.remove(path);
    logPrintf("[STORAGE] Arquivo removido do Cartao SD: %s\n", path);
  }
}