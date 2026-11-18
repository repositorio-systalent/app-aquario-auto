#pragma once
#include <math.h>
#include <stdint.h>

namespace AquarioLogic {

struct DecisaoTermica {
  bool ligarAquecedor;
  bool ligarCooler;
  bool erroSensor;
};

// Avalia a máquina de estados de temperatura com histerese e intertravamento
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

  // Aquecimento
  if (!proxAquecedor && tempAtual <= minLiga) {
    proxCooler = false;
    proxAquecedor = true;
  } else if (proxAquecedor && tempAtual >= minDesliga) {
    proxAquecedor = false;
  }

  // Resfriamento
  if (!proxCooler && tempAtual >= maxLiga) {
    proxAquecedor = false;
    proxCooler = true;
  } else if (proxCooler && tempAtual <= maxDesliga) {
    proxCooler = false;
  }

  // Garantia absoluta de exclusão mútua
  if (proxAquecedor && proxCooler) {
    proxAquecedor = false;
    proxCooler = false;
  }

  return {proxAquecedor, proxCooler, false};
}

// Calcula o percentual útil de água no reservatório (0% a 100%)
inline float calcularPercentualReservatorio(uint16_t distAtualMm, uint16_t distCheioMm, uint16_t distVazioMm) {
  if (distVazioMm <= distCheioMm) return 0.0f;
  if (distAtualMm <= distCheioMm) return 100.0f;
  if (distAtualMm >= distVazioMm) return 0.0f;

  float total = (float)(distVazioMm - distCheioMm);
  float restante = (float)(distVazioMm - distAtualMm);
  return (restante / total) * 100.0f;
}

// Converte resistência do NTC 10k (Beta 3950) para graus Celsius
inline float calcularTemperaturaNTC(float resistenciaOhms) {
  if (resistenciaOhms <= 0.0f) return NAN;
  const float R0 = 10000.0f;
  const float T0 = 298.15f; // 25°C em Kelvin
  const float BETA = 3950.0f;

  float steinhart = log(resistenciaOhms / R0) / BETA;
  steinhart += 1.0f / T0;
  return (1.0f / steinhart) - 273.15f;
}

} // namespace AquarioLogic