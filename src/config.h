#pragma once
#include <Arduino.h>

// --- FUNÇÕES GLOBAIS DE LOG (USB + TELNET) ---
void logPrint(const String &msg);
void logPrintln(const String &msg);
void logPrintf(const char *format, ...);

// --- PINAGEM ---
#define PIN_NTC_ADC          34
#define PIN_XSHUT_AQUARIO    16
#define PIN_XSHUT_RESERV     17
#define PIN_SD_CS            4

#define PIN_RELE_DRENAGEM    25
#define PIN_RELE_ENCHIMENTO  26
#define PIN_RELE_RESERVA1    27
#define PIN_RELE_RESERVA2    14

// --- LEDS INDICADORES DO RESERVATORIO ---
#define PIN_LED_VERMELHO     13  // Reservatorio Vazio (Bloqueia TPA)
#define PIN_LED_AMARELO      32  // Nivel Critico (Alerta)
#define PIN_LED_VERDE        33  // Nivel OK / Cheio

// --- ENDEREÇOS I2C ---
#define ADDR_VL53_AQUARIO    0x30
#define ADDR_VL53_RESERV     0x31

struct SystemConfig {
  char wifi_ssid[32];
  char wifi_pass[64];
  
  int tpa_periodicidade;      // 1 = 1x ao dia, 2 = 2x ao dia
  char tpa_hora1[6];          // Ex: "09:00"
  char tpa_hora2[6];          // Ex: "21:00"

  uint32_t proxima_tpa_epoch;
  uint32_t ultima_tpa_epoch;
  float vol_ultima_tpa;

  uint16_t aq_dist_max_mm;    // Nível cheio (mm)
  uint16_t aq_dist_min_mm;    // Nível drenado seguro (mm)
  uint16_t res_dist_max_mm;   // Reservatório cheio (mm)
  uint16_t res_dist_min_mm;   // Reservatório vazio (mm)
  
  float res_volume_total_l;
  float res_percent_critico;

  uint32_t tempo_max_drenagem_seg;
  uint32_t tempo_max_enchimento_seg;
  uint32_t tempo_pausa_pos_dreno_seg;     // Novo: pausa para escoar sifão
  uint32_t tempo_estabilizacao_seg;       // Novo: espera para vasos comunicantes
  uint16_t tolerancia_estabilizacao_mm;   // Novo: margem em mm para pulso complementar
  // --- CONTROLE TÉRMICO (DUPLA VIA) ---
  bool temp_controle_ativo;
  float temp_min_liga;     // Liga aquecedor (GPIO 27)
  float temp_min_desliga;  // Desliga aquecedor
  float temp_max_liga;     // Liga cooler (GPIO 14)
  float temp_max_desliga;  // Desliga cooler

  uint32_t boot_counter;
};