#include <unity.h>
#include <math.h>
#include "../src/logic/AquarioLogic.h"

using namespace AquarioLogic;

// ============================================================================
// SUÍTE 1: SENSORES (NTC 100k Beta 3750 & RESERVATÓRIO)
// ============================================================================
void test_ntc_100k_beta_3750() {
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 25.0f, calcularTemperaturaNTC(100000.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 20.0f, calcularTemperaturaNTC(123500.0f));
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 30.0f, calcularTemperaturaNTC(81400.0f));
  TEST_ASSERT_TRUE(isnan(calcularTemperaturaNTC(0.0f)));
  TEST_ASSERT_TRUE(isnan(calcularTemperaturaNTC(-10.0f)));
}

void test_percentual_reservatorio() {
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, calcularPercentualReservatorio(180, 180, 680));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, calcularPercentualReservatorio(680, 180, 680));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, calcularPercentualReservatorio(430, 180, 680));
  // Acima do topo ou leitura inválida I2C (8191 mm)
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, calcularPercentualReservatorio(150, 180, 680));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, calcularPercentualReservatorio(8191, 180, 680));
}

// ============================================================================
// SUÍTE 2: CONTROLE TÉRMICO DE DUPLA VIA
// ============================================================================
void test_controle_termico_aquecimento_e_histerese() {
  DecisaoTermica d1 = avaliarControleTermico(24.7f, true, false, false, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_TRUE(d1.ligarAquecedor);
  TEST_ASSERT_FALSE(d1.ligarCooler);

  // Dentro da banda morta (25.0°C) mantém ligado
  DecisaoTermica d2 = avaliarControleTermico(25.0f, true, false, true, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_TRUE(d2.ligarAquecedor);

  // Atingiu 25.3°C desliga
  DecisaoTermica d3 = avaliarControleTermico(25.3f, true, false, true, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_FALSE(d3.ligarAquecedor);
}

void test_controle_termico_resfriamento_e_histerese() {
  DecisaoTermica d1 = avaliarControleTermico(26.9f, true, false, false, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_FALSE(d1.ligarAquecedor);
  TEST_ASSERT_TRUE(d1.ligarCooler);

  // Dentro da banda morta (26.5°C) mantém ligado
  DecisaoTermica d2 = avaliarControleTermico(26.5f, true, false, false, true, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_TRUE(d2.ligarCooler);

  // Baixou para 26.2°C desliga
  DecisaoTermica d3 = avaliarControleTermico(26.2f, true, false, false, true, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_FALSE(d3.ligarCooler);
}

void test_seguranca_termica_tpa_e_sensor_invalido() {
  DecisaoTermica dTpa = avaliarControleTermico(22.0f, true, true, true, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_FALSE(dTpa.ligarAquecedor);
  TEST_ASSERT_FALSE(dTpa.ligarCooler);

  DecisaoTermica dErro = avaliarControleTermico(48.0f, true, false, false, true, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_TRUE(dErro.erroSensor);
  TEST_ASSERT_FALSE(dErro.ligarAquecedor);
  TEST_ASSERT_FALSE(dErro.ligarCooler);

  DecisaoTermica dDesativado = avaliarControleTermico(20.0f, false, false, true, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_FALSE(dDesativado.ligarAquecedor);
  TEST_ASSERT_FALSE(dDesativado.ligarCooler);
}

// ============================================================================
// SUÍTE 3: CICLO COMPLETO E SEGURANÇA DA TPA
// ============================================================================
void test_tpa_bloqueio_inicio() {
  // Pode iniciar quando IDLE e reservatório com água (300 mm < 680 mm)
  TEST_ASSERT_TRUE(podeIniciarTPA(false, 300, 680));

  // Bloqueia se já existe TPA em andamento
  TEST_ASSERT_FALSE(podeIniciarTPA(true, 300, 680));

  // Bloqueia se reservatório vazio (680 mm >= 680 mm) ou leitura de erro I2C (8191 mm)
  TEST_ASSERT_FALSE(podeIniciarTPA(false, 680, 680));
  TEST_ASSERT_FALSE(podeIniciarTPA(false, 8191, 680));
}

void test_tpa_etapa_drenagem() {
  // Ainda drenando (160 mm < 185 mm alvo, 10s < 30s timeout)
  TEST_ASSERT_EQUAL(TPA_CONTINUAR, avaliarEtapaDrenagem(160, 185, 10, 30));

  // Atingiu nível mínimo de drenagem (185 mm) -> para por nível
  TEST_ASSERT_EQUAL(TPA_FIM_NIVEL_ATINGIDO, avaliarEtapaDrenagem(185, 185, 12, 30));

  // Estouro de tempo (30s) mesmo sem atingir nível -> para por timeout de segurança
  TEST_ASSERT_EQUAL(TPA_FIM_TIMEOUT_SEGURANCA, avaliarEtapaDrenagem(165, 185, 30, 30));

  // Leitura espúria I2C (8191 mm) não deve encerrar drenagem por nível falso, apenas por timeout
  TEST_ASSERT_EQUAL(TPA_CONTINUAR, avaliarEtapaDrenagem(8191, 185, 10, 30));
  TEST_ASSERT_EQUAL(TPA_FIM_TIMEOUT_SEGURANCA, avaliarEtapaDrenagem(8191, 185, 30, 30));
}

void test_tpa_etapa_enchimento() {
  // Enchendo normalmente (160 mm > 140 mm alvo, res 300 mm < 680 mm, tempo 40s < 150s)
  TEST_ASSERT_EQUAL(TPA_CONTINUAR, avaliarEtapaEnchimento(160, 140, 300, 680, 40, 150));

  // Atingiu nível máximo do aquário (140 mm) -> vai para estabilização
  TEST_ASSERT_EQUAL(TPA_FIM_NIVEL_ATINGIDO, avaliarEtapaEnchimento(140, 140, 350, 680, 65, 150));

  // Reservatório esvaziou no meio do enchimento (680 mm) -> prioridade máxima de corte
  TEST_ASSERT_EQUAL(TPA_FIM_RESERVATORIO_VAZIO, avaliarEtapaEnchimento(155, 140, 680, 680, 50, 150));

  // Estouro de tempo total de enchimento (150s) -> corta por timeout
  TEST_ASSERT_EQUAL(TPA_FIM_TIMEOUT_SEGURANCA, avaliarEtapaEnchimento(150, 140, 400, 680, 150, 150));
}

void test_tpa_estabilizacao_e_pulso_complementar() {
  // Alvo 140 mm, tolerância 2 mm (aceita até 142 mm)
  TEST_ASSERT_FALSE(precisaPulsoComplementarTPA(140, 140, 2));
  TEST_ASSERT_FALSE(precisaPulsoComplementarTPA(142, 140, 2));

  // Água baixou para 144 mm após acomodar sifão (> 142 mm) -> exige pulso complementar
  TEST_ASSERT_TRUE(precisaPulsoComplementarTPA(144, 140, 2));

  // Leitura inválida I2C (8191 mm) não dispara pulso infinito
  TEST_ASSERT_FALSE(precisaPulsoComplementarTPA(8191, 140, 2));
}

void test_tpa_agendamento_1x_e_2x_dia() {
  // Base: 08:00 (28800s no dia), epoch fictício 1000000 + 28800 = 1028800
  uint32_t baseDia = 1000000UL;
  uint32_t epoch08h00 = baseDia + (8 * 3600);

  // 1x ao dia às 09:00 -> antes das 09:00 agenda para 09:00 de hoje
  uint32_t prox1 = calcularProximoEpochTPA(epoch08h00, 8, 0, 9, 0, 21, 0, 1, false);
  TEST_ASSERT_EQUAL_UINT32(baseDia + (9 * 3600), prox1);

  // 1x ao dia às 09:00 -> às 10:00 agenda para 09:00 de amanhã (+86400s)
  uint32_t epoch10h00 = baseDia + (10 * 3600);
  uint32_t prox2 = calcularProximoEpochTPA(epoch10h00, 10, 0, 9, 0, 21, 0, 1, false);
  TEST_ASSERT_EQUAL_UINT32(baseDia + (9 * 3600) + 86400UL, prox2);

  // 2x ao dia (09:00 e 21:00) -> às 10:00 agenda para 21:00 de hoje
  uint32_t prox3 = calcularProximoEpochTPA(epoch10h00, 10, 0, 9, 0, 21, 0, 2, false);
  TEST_ASSERT_EQUAL_UINT32(baseDia + (21 * 3600), prox3);

  // 2x ao dia -> acabou de concluir a TPA das 09:00 exatamente às 09:00 -> agenda para 21:00 de hoje
  uint32_t epoch09h00 = baseDia + (9 * 3600);
  uint32_t prox4 = calcularProximoEpochTPA(epoch09h00, 9, 0, 9, 0, 21, 0, 2, true);
  TEST_ASSERT_EQUAL_UINT32(baseDia + (21 * 3600), prox4);

  // 2x ao dia -> às 21:30 agenda para 09:00 de amanhã
  uint32_t epoch21h30 = baseDia + (21 * 3600) + (30 * 60);
  uint32_t prox5 = calcularProximoEpochTPA(epoch21h30, 21, 30, 9, 0, 21, 0, 2, true);
  TEST_ASSERT_EQUAL_UINT32(baseDia + (9 * 3600) + 86400UL, prox5);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  // Sensores
  RUN_TEST(test_ntc_100k_beta_3750);
  RUN_TEST(test_percentual_reservatorio);
  // Controle Térmico
  RUN_TEST(test_controle_termico_aquecimento_e_histerese);
  RUN_TEST(test_controle_termico_resfriamento_e_histerese);
  RUN_TEST(test_seguranca_termica_tpa_e_sensor_invalido);
  // Máquina de Estados e Segurança da TPA
  RUN_TEST(test_tpa_bloqueio_inicio);
  RUN_TEST(test_tpa_etapa_drenagem);
  RUN_TEST(test_tpa_etapa_enchimento);
  RUN_TEST(test_tpa_estabilizacao_e_pulso_complementar);
  RUN_TEST(test_tpa_agendamento_1x_e_2x_dia);
  return UNITY_END();
}