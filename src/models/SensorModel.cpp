#include "SensorModel.h"
#include "logic/AquarioLogic.h"
#include <math.h>

const float R_FIXO = 9800.0;     // Seu resistor fixo medido no multímetro
const float R0_NTC = 10000.0;    // NTC 10k a 25°C
const float T0_KELVIN = 298.15;
const float VCC_MV = 3280.0;     // Tensão real medida no pino 3.3V
const float BETA = 3950.0;       // Beta 3950

SensorModel::SensorModel() : rtc(Wire) {}

bool SensorModel::initSensors() {
  Wire.begin(21, 22);
  Wire.setTimeOut(50);

  pinMode(PIN_XSHUT_AQUARIO, OUTPUT);
  pinMode(PIN_XSHUT_RESERV, OUTPUT);
  digitalWrite(PIN_XSHUT_AQUARIO, LOW);
  digitalWrite(PIN_XSHUT_RESERV, LOW);
  delay(20);

  digitalWrite(PIN_XSHUT_AQUARIO, HIGH);
  delay(10);
  if (sensorAq.init()) {
    sensorAq.setAddress(ADDR_VL53_AQUARIO);
    sensorAq.setMeasurementTimingBudget(200000);
    sensorAq.startContinuous();
  }

  digitalWrite(PIN_XSHUT_RESERV, HIGH);
  delay(10);
  if (sensorRes.init()) {
    sensorRes.setAddress(ADDR_VL53_RESERV);
    sensorRes.setMeasurementTimingBudget(200000);
    sensorRes.startContinuous();
  }

  rtc.Begin();
  return true;
}

void SensorModel::sincronizarNTPeAtualizarRTC() {
  // Configura fuso horário de Brasília (UTC-3) e servidores NTP brasileiros
  configTime(-3 * 3600, 0, "a.st1.ntp.br", "b.st1.ntp.br", "pool.ntp.org");
  Serial.println("[RTC] Aguardando sincronizacao NTP...");

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 5000)) { // Aguarda até 5 segundos
    RtcDateTime ntpTime(
      timeinfo.tm_year + 1900,
      timeinfo.tm_mon + 1,
      timeinfo.tm_mday,
      timeinfo.tm_hour,
      timeinfo.tm_min,
      timeinfo.tm_sec
    );
    rtc.SetDateTime(ntpTime);
    Serial.println("[RTC] Data e Hora atualizadas no DS3231 com sucesso via NTP!");
  } else {
    Serial.println("[RTC] Falha ao consultar NTP. Mantendo horario gravado no DS3231.");
  }
}

float SensorModel::getTemperatura() {
  long somaMv = 0;
  for (int i = 0; i < 16; i++) {
    somaMv += analogReadMilliVolts(PIN_NTC_ADC);
    delay(2);
  }
  float vOut_mv = (float)somaMv / 16.0;
  if (vOut_mv < 50.0 || vOut_mv > (VCC_MV - 50.0)) return -99.0;

  float rNtc = R_FIXO * (vOut_mv / (VCC_MV - vOut_mv));
  return AquarioLogic::calcularTemperaturaNTC(rNtc);
}

static uint8_t falhasConsecutivasI2C = 0;

uint16_t SensorModel::getDistanciaAquario() {
  uint16_t dist = sensorAq.readRangeContinuousMillimeters();
  if (sensorAq.timeoutOccurred() || dist >= 8000) {
    delay(5);
    dist = sensorAq.readRangeContinuousMillimeters();
  }

  if (dist >= 8000) {
    falhasConsecutivasI2C++;
    if (falhasConsecutivasI2C >= 10) {
      logPrintln("[I2C-RECUPERACAO] Barramento I2C travado (8191). Reinicializando sensores VL53L0X...");
      falhasConsecutivasI2C = 0;
      Wire.end();
      delay(20);
      initSensors();
    }
  } else {
    falhasConsecutivasI2C = 0;
  }
  return dist;
}

uint16_t SensorModel::getDistanciaReservatorio() {
  uint16_t dist = sensorRes.readRangeContinuousMillimeters();
  if (sensorRes.timeoutOccurred() || dist >= 8000) {
    delay(5);
    dist = sensorRes.readRangeContinuousMillimeters();
  }
  return dist;
}

float SensorModel::getVolumeAquario(const SystemConfig &config) {
  uint16_t d = getDistanciaAquario();
  if (d <= config.aq_dist_max_mm) return 12.0;
  if (d >= config.aq_dist_min_mm) return 0.0;
  float perc = (float)(config.aq_dist_min_mm - d) / (float)(config.aq_dist_min_mm - config.aq_dist_max_mm);
  return perc * 12.0;
}

float SensorModel::getVolumeReservatorio(const SystemConfig &config) {
  uint16_t d = getDistanciaReservatorio();
  if (d <= config.res_dist_max_mm) return config.res_volume_total_l;
  if (d >= config.res_dist_min_mm) return 0.0;
  float perc = (float)(config.res_dist_min_mm - d) / (float)(config.res_dist_min_mm - config.res_dist_max_mm);
  return perc * config.res_volume_total_l;
}

float SensorModel::getPercentualReservatorio(const SystemConfig &config) {
  float v = getVolumeReservatorio(config);
  if (config.res_volume_total_l <= 0) return 0.0;
  return (v / config.res_volume_total_l) * 100.0;
}

String SensorModel::getDataHoraAtual() {
  RtcDateTime now = rtc.GetDateTime();
  char buf[20];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d:%02d",
           now.Day(), now.Month(), now.Year(), now.Hour(), now.Minute(), now.Second());
  return String(buf);
}

uint32_t SensorModel::getEpochAtual() {
  RtcDateTime now = rtc.GetDateTime();
  return now.TotalSeconds();
}

RtcDateTime SensorModel::getDateTimeAtual() {
  return rtc.GetDateTime();
}

String SensorModel::formatarDataHora(uint32_t epoch) {
  if (epoch == 0) return "Nenhuma";
  
  // Passa o timestamp diretamente no construtor
  RtcDateTime dt(epoch);
  
  char buf[25];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d",
           dt.Day(), dt.Month(), dt.Year(), dt.Hour(), dt.Minute());
  return String(buf);
}