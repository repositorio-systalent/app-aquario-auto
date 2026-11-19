#include <unity.h>
#include <math.h>
#include "../src/logic/AquarioLogic.h"

using namespace AquarioLogic;

void test_ntc_100k_beta_3750() {
  // A 25.0 °C exatos, a resistência nominal do NTC 100k é 100.000 Ohms
  float temp25 = calcularTemperaturaNTC(100000.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.05f, 25.0f, temp25);

  // Resistência maior (~123.5 kOhms) indica água mais fria (~20.0 °C)
  float temp20 = calcularTemperaturaNTC(123500.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 20.0f, temp20);

  // Resistência menor (~81.4 kOhms) indica água mais quente (~30.0 °C)
  float temp30 = calcularTemperaturaNTC(81400.0f);
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 30.0f, temp30);

  // Leitura inválida (0 ou negativa) deve retornar NAN
  TEST_ASSERT_TRUE(isnan(calcularTemperaturaNTC(0.0f)));
}

void test_percentual_reservatorio() {
  // Cheio (180 mm) = 100%, Vazio (680 mm) = 0%, Meio (430 mm) = 50%
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, calcularPercentualReservatorio(180, 180, 680));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, calcularPercentualReservatorio(680, 180, 680));
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, calcularPercentualReservatorio(430, 180, 680));

  // Leitura inválida de I2C (8191 mm) deve retornar 0% por segurança
  TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, calcularPercentualReservatorio(8191, 180, 680));
}

void test_controle_termico_aquecimento_e_histerese() {
  // Caiu para 24.7 °C -> liga aquecedor
  DecisaoTermica d1 = avaliarControleTermico(24.7f, true, false, false, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_TRUE(d1.ligarAquecedor);
  TEST_ASSERT_FALSE(d1.ligarCooler);

  // Subiu para 25.0 °C (dentro da histerese) -> mantém aquecedor ligado
  DecisaoTermica d2 = avaliarControleTermico(25.0f, true, false, true, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_TRUE(d2.ligarAquecedor);

  // Atingiu 25.3 °C -> desliga aquecedor
  DecisaoTermica d3 = avaliarControleTermico(25.3f, true, false, true, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_FALSE(d3.ligarAquecedor);
}

void test_controle_termico_resfriamento_e_histerese() {
  // Subiu para 26.9 °C -> liga cooler
  DecisaoTermica d1 = avaliarControleTermico(26.9f, true, false, false, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_FALSE(d1.ligarAquecedor);
  TEST_ASSERT_TRUE(d1.ligarCooler);

  // Baixou para 26.5 °C (dentro da histerese) -> mantém cooler ligado
  DecisaoTermica d2 = avaliarControleTermico(26.5f, true, false, false, true, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_TRUE(d2.ligarCooler);

  // Baixou para 26.2 °C -> desliga cooler
  DecisaoTermica d3 = avaliarControleTermico(26.2f, true, false, false, true, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_FALSE(d3.ligarCooler);
}

void test_seguranca_tpa_e_sensor_invalido() {
  // Durante TPA -> desliga tudo mesmo com água fria ou quente
  DecisaoTermica dTpa = avaliarControleTermico(22.0f, true, true, true, false, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_FALSE(dTpa.ligarAquecedor);
  TEST_ASSERT_FALSE(dTpa.ligarCooler);

  // Sensor desconectado ou fora da faixa válida (48 °C) -> sinaliza erro e desliga tudo
  DecisaoTermica dErro = avaliarControleTermico(48.0f, true, false, false, true, 24.8f, 25.3f, 26.8f, 26.2f);
  TEST_ASSERT_TRUE(dErro.erroSensor);
  TEST_ASSERT_FALSE(dErro.ligarAquecedor);
  TEST_ASSERT_FALSE(dErro.ligarCooler);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_ntc_100k_beta_3750);
  RUN_TEST(test_percentual_reservatorio);
  RUN_TEST(test_controle_termico_aquecimento_e_histerese);
  RUN_TEST(test_controle_termico_resfriamento_e_histerese);
  RUN_TEST(test_seguranca_tpa_e_sensor_invalido);
  return UNITY_END();
}