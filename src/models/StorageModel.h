#pragma once
#include <Arduino.h>
#include <SD.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "config.h"

class StorageModel {
public:
  static bool initSD();
  static bool loadConfig(SystemConfig &config);
  static bool saveConfig(const SystemConfig &config);
  static void logTPA(const char* dataHora, uint32_t tempoDreno, const char* motivoDreno, uint32_t tempoEnche, const char* motivoEnche);
  static void logError(const char* dataHora, const char* mensagem);
  static void incrementBootCounter(SystemConfig &config);
  static void resetBootCounter(SystemConfig &config);
  static void logTPA(const char* dataHora, uint32_t tempoDreno, uint16_t nivelDrenoMm, const char* motivoDreno, uint32_t tempoEnche, uint16_t nivelEncheMm, const char* motivoEnche);
  static String readLogFile(const char* path);
  static void clearLogFile(const char* path);
};