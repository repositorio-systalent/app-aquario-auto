#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "models/SensorModel.h"
#include "controllers/TPAController.h"
#include "controllers/TempController.h"

class WebViews {
public:
  static String renderStatusJson(SensorModel &sensores, const SystemConfig &config, TPAController &tpa, TempController &tempCtrl) {
    StaticJsonDocument<768> doc;
    uint16_t distRes = sensores.getDistanciaReservatorio();
    float percRes = sensores.getPercentualReservatorio(config);

    doc["temperatura"] = sensores.getTemperatura();
    doc["aquecedor_ligado"] = tempCtrl.isAquecedorLigado();
    doc["cooler_ligado"] = tempCtrl.isCoolerLigado();
    doc["volume_aquario"] = sensores.getVolumeAquario(config);
    doc["dist_aquario"] = sensores.getDistanciaAquario();
    doc["volume_reservatorio"] = sensores.getVolumeReservatorio(config);
    doc["dist_reservatorio"] = distRes;
    doc["percent_reservatorio"] = percRes;
    doc["reservatorio_vazio"] = (distRes >= config.res_dist_min_mm);
    doc["nivel_critico"] = (percRes <= config.res_percent_critico);
    doc["data_hora"] = sensores.getDataHoraAtual();
    doc["ultima_tpa"] = tpa.getDataUltimaTPA();
    doc["vol_ultima_tpa"] = tpa.getVolumeUltimaTPA();
    doc["proxima_tpa"] = tpa.getDataProximaTPA();
    doc["boot_counter"] = config.boot_counter;
    doc["tpa_em_andamento"] = tpa.isEmAndamento();
    doc["tpa_estado"] = tpa.getNomeEstado();

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    return jsonResponse;
  }

  static String renderConfigJson(const SystemConfig &config) {
    StaticJsonDocument<1024> doc;
    doc["tpa_periodicidade"] = config.tpa_periodicidade;
    doc["tpa_hora1"] = config.tpa_hora1;
    doc["tpa_hora2"] = config.tpa_hora2;
    doc["aq_dist_max_mm"] = config.aq_dist_max_mm;
    doc["aq_dist_min_mm"] = config.aq_dist_min_mm;
    doc["res_dist_max_mm"] = config.res_dist_max_mm;
    doc["res_dist_min_mm"] = config.res_dist_min_mm;
    doc["res_volume_total_l"] = config.res_volume_total_l;
    doc["res_percent_critico"] = config.res_percent_critico;
    doc["tempo_max_drenagem_seg"] = config.tempo_max_drenagem_seg;
    doc["tempo_max_enchimento_seg"] = config.tempo_max_enchimento_seg;
    doc["tempo_pausa_pos_dreno_seg"] = config.tempo_pausa_pos_dreno_seg;
    doc["tempo_estabilizacao_seg"] = config.tempo_estabilizacao_seg;
    doc["tolerancia_estabilizacao_mm"] = config.tolerancia_estabilizacao_mm;
    doc["temp_controle_ativo"] = config.temp_controle_ativo;
    doc["temp_min_liga"] = config.temp_min_liga;
    doc["temp_min_desliga"] = config.temp_min_desliga;
    doc["temp_max_liga"] = config.temp_max_liga;
    doc["temp_max_desliga"] = config.temp_max_desliga;
    doc["wifi_ssid"] = config.wifi_ssid;

    String jsonResponse;
    serializeJson(doc, jsonResponse);
    return jsonResponse;
  }

  static String renderSuccessJson(const char* mensagem = "ok") {
    StaticJsonDocument<128> doc;
    doc["status"] = "ok";
    doc["mensagem"] = mensagem;
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    return jsonResponse;
  }

  static String renderErrorJson(const char* erro) {
    StaticJsonDocument<128> doc;
    doc["status"] = "erro";
    doc["mensagem"] = erro;
    String jsonResponse;
    serializeJson(doc, jsonResponse);
    return jsonResponse;
  }
};