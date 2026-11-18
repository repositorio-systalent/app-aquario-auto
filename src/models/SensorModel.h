#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include <RtcDS3231.h>
#include <time.h>
#include "config.h"

class SensorModel {
private:
  VL53L0X sensorAq;
  VL53L0X sensorRes;
  RtcDS3231<TwoWire> rtc;

public:
  SensorModel();
  bool initSensors();
  void sincronizarNTPeAtualizarRTC();
  float getTemperatura();
  uint16_t getDistanciaAquario();
  uint16_t getDistanciaReservatorio();
  float getVolumeAquario(const SystemConfig &config);
  float getVolumeReservatorio(const SystemConfig &config);
  float getPercentualReservatorio(const SystemConfig &config);
  String getDataHoraAtual();
  uint32_t getEpochAtual();
  RtcDateTime getDateTimeAtual();
  String formatarDataHora(uint32_t epoch);
};