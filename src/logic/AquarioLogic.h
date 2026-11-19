#pragma once
#include <math.h>
#include <stdint.h>

namespace AquarioLogic {

// ============================================================================
// 1. CONTROLE TÉRMICO
// ============================================================================
struct DecisaoTermica {
  bool ligarAquecedor;
  bool ligarCooler;
  bool erroSensor;
};

inline DecisaoTermica avaliarControleTermico(
    float tempAtual,
    bool controleAtivo,
    bool tpaEmAndamento,
    bool aquecedorAtual,
    bool coolerAtual,
    float minLiga,
    float minDesliga,
    float maxLiga,
    float maxDesliga) {

  if (!controleAtivo || tpaEmAndamento) {
    return {false, false, false};
  }

  if (isnan(tempAtual) || tempAtual < 5.0f || tempAtual > 45.0f) {
    return {false, false, true};
  }

  bool proxAquecedor = aquecedorAtual;
  bool proxCooler = coolerAtual;

  if (!proxAquecedor && tempAtual <= minLiga) {
    proxCooler = false;
    proxAquecedor = true;
  } else if (proxAquecedor && tempAtual >= minDesliga) {
    proxAquecedor = false;
  }

  if (!proxCooler && tempAtual >= maxLiga) {
    proxAquecedor = false;
    proxCooler = true;
  } else if (proxCooler && tempAtual <= maxDesliga) {
    proxCooler = false;
  }

  if (proxAquecedor && proxCooler) {
    proxAquecedor = false;
    proxCooler = false;
  }

  return {proxAquecedor, proxCooler, false};
}

// ============================================================================
// 2. SENSORES (NTC 100k Beta 3750 & Reservatório VL53L0X)
// ============================================================================
inline float calcularPercentualReservatorio(uint16_t distAtualMm, uint16_t distCheioMm, uint16_t distVazioMm) {
  if (distVazioMm <= distCheioMm) return 0.0f;
  if (distAtualMm <= distCheioMm) return 100.0f;
  if (distAtualMm >= distVazioMm) return 0.0f;

  float total = (float)(distVazioMm - distCheioMm);
  float restante = (float)(distVazioMm - distAtualMm);
  return (restante / total) * 100.0f;
}

inline float calcularTemperaturaNTC(float resistenciaOhms) {
  if (resistenciaOhms <= 0.0f) return NAN;
  const float R0 = 100000.0f;  // NTC 100k a 25°C
  const float T0 = 298.15f;    // 25°C em Kelvin
  const float BETA = 3750.0f;  // Beta calibrado do sensor

  float steinhart = log(resistenciaOhms / R0) / BETA;
  steinhart += 1.0f / T0;
  return (1.0f / steinhart) - 273.15f;
}

// ============================================================================
// 3. REGRAS DE NEGÓCIO E SEGURANÇA DA TPA
// ============================================================================
enum MotivoFimEtapaTPA {
  TPA_CONTINUAR = 0,
  TPA_FIM_NIVEL_ATINGIDO,
  TPA_FIM_TIMEOUT_SEGURANCA,
  TPA_FIM_RESERVATORIO_VAZIO
};

// Valida se a TPA pode iniciar com base no estado atual e nível do reservatório
inline bool podeIniciarTPA(bool jaEmAndamento, uint16_t distResMm, uint16_t resDistMinMm) {
  if (jaEmAndamento) return false;
  if (distResMm >= resDistMinMm) return false;
  return true;
}

// Avalia condição de parada da Etapa 1 (Drenagem)
inline MotivoFimEtapaTPA avaliarEtapaDrenagem(
    uint16_t distAqMm,
    uint16_t aqDistMinMm,
    uint32_t segDecorridos,
    uint32_t tempoMaxDrenoSeg) {

  bool leituraValida = (distAqMm > 0 && distAqMm < 2000);
  bool atingeNivel = (leituraValida && distAqMm >= aqDistMinMm);
  bool atingeTempo = (tempoMaxDrenoSeg > 0 && segDecorridos >= tempoMaxDrenoSeg);

  if (atingeNivel) return TPA_FIM_NIVEL_ATINGIDO;
  if (atingeTempo) return TPA_FIM_TIMEOUT_SEGURANCA;
  return TPA_CONTINUAR;
}

// Avalia condição de parada da Etapa 2 (Enchimento)
// Prioridade de segurança: Reservatório Vazio > Timeout > Nível Alvo
inline MotivoFimEtapaTPA avaliarEtapaEnchimento(
    uint16_t distAqMm,
    uint16_t aqDistMaxMm,
    uint16_t distResMm,
    uint16_t resDistMinMm,
    uint32_t tempoTotalEnchimentoSeg,
    uint32_t tempoMaxEnchimentoSeg) {

  bool leituraAqValida = (distAqMm > 0 && distAqMm < 2000);
  bool leituraResValida = (distResMm > 0 && distResMm < 2000);

  bool resEsvaziou = (leituraResValida && distResMm >= resDistMinMm);
  if (resEsvaziou) return TPA_FIM_RESERVATORIO_VAZIO;

  bool atingeTempo = (tempoMaxEnchimentoSeg > 0 && tempoTotalEnchimentoSeg >= tempoMaxEnchimentoSeg);
  if (atingeTempo) return TPA_FIM_TIMEOUT_SEGURANCA;

  bool atingeNivel = (leituraAqValida && distAqMm <= aqDistMaxMm);
  if (atingeNivel) return TPA_FIM_NIVEL_ATINGIDO;

  return TPA_CONTINUAR;
}

// Avalia após estabilização se precisa de pulso complementar de enchimento
inline bool precisaPulsoComplementarTPA(
    uint16_t distAqEstabilizadaMm,
    uint16_t aqDistMaxMm,
    uint16_t toleranciaMm) {
  if (distAqEstabilizadaMm == 0 || distAqEstabilizadaMm >= 2000) return false;
  return distAqEstabilizadaMm > (aqDistMaxMm + toleranciaMm);
}

// Calcula o próximo horário de TPA em segundos (epoch) dado o epoch atual e horários configurados
inline uint32_t calcularProximoEpochTPA(
    uint32_t epochAtual,
    int horaAtual,
    int minAtual,
    int h1, int m1,
    int h2, int m2,
    int periodicidade,
    bool aposCicloConcluido) {

  // Zera os segundos do minuto atual para calcular o início exato do dia
  uint32_t segNoDiaAtual = (uint32_t)(horaAtual * 3600 + minAtual * 60);
  uint32_t inicioDiaEpoch = epochAtual - (epochAtual % 86400UL);
  // Ajuste caso epochAtual seja hora local vs UTC: calcula delta relativo a segNoDiaAtual
  uint32_t baseMeiaNoite = epochAtual - segNoDiaAtual;

  uint32_t alvoH1 = baseMeiaNoite + (uint32_t)(h1 * 3600 + m1 * 60);
  uint32_t alvoH2 = baseMeiaNoite + (uint32_t)(h2 * 3600 + m2 * 60);

  // Se acabou de concluir um ciclo, exige que o próximo horário seja estritamente futuro (> epochAtual)
  if (!aposCicloConcluido) {
    if (alvoH1 >= epochAtual) return alvoH1;
    if (periodicidade == 2 && alvoH2 >= epochAtual) return alvoH2;
  } else {
    if (alvoH1 > epochAtual) return alvoH1;
    if (periodicidade == 2 && alvoH2 > epochAtual) return alvoH2;
  }

  return alvoH1 + 86400UL;
}

} // namespace AquarioLogic