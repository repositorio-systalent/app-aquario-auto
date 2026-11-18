#include "TPAController.h"

TPAController::TPAController(SystemConfig &cfg, SensorModel &sens)
  : config(cfg), sensores(sens), estado(IDLE), tempoInicioEtapa(0), ultimoCheckMinuto(0) {}

static uint32_t duracaoDrenagemReal = 0;
static uint16_t nivelDrenagemRealMm = 0;
static String motivoDrenagemReal = "";
static uint32_t tempoTotalEnchimento = 0;

void TPAController::init() {
  pinMode(PIN_RELE_DRENAGEM, OUTPUT);
  pinMode(PIN_RELE_ENCHIMENTO, OUTPUT);
  pinMode(PIN_RELE_RESERVA1, OUTPUT);
  pinMode(PIN_RELE_RESERVA2, OUTPUT);

  pinMode(PIN_LED_VERMELHO, OUTPUT);
  pinMode(PIN_LED_AMARELO, OUTPUT);
  pinMode(PIN_LED_VERDE, OUTPUT);

  digitalWrite(PIN_RELE_DRENAGEM, HIGH);
  digitalWrite(PIN_RELE_ENCHIMENTO, HIGH);
  digitalWrite(PIN_RELE_RESERVA1, HIGH);
  digitalWrite(PIN_RELE_RESERVA2, HIGH);

  digitalWrite(PIN_LED_VERMELHO, LOW);
  digitalWrite(PIN_LED_AMARELO, LOW);
  digitalWrite(PIN_LED_VERDE, LOW);

  uint32_t agoraEpoch = sensores.getEpochAtual();

  if (config.proxima_tpa_epoch == 0) {
    config.proxima_tpa_epoch = calcularProximaTPAConfiguracao(config.tpa_hora1, config.tpa_hora2, config.tpa_periodicidade, sensores);
    StorageModel::saveConfig(config);
  } else if (agoraEpoch >= config.proxima_tpa_epoch) {
    logPrintln("[TPA-SEGURANCA] TPA pendente detectada no boot!");
    StorageModel::logError(sensores.getDataHoraAtual().c_str(), "TPA pendente executada no Boot");
    dispararTPA();
  }
}

void TPAController::atualizarLedsReservatorio() {
  uint16_t distRes = sensores.getDistanciaReservatorio();
  if (distRes == 0 || distRes >= 8000) return; // Ignora falha momentânea de I2C

  float percRes = sensores.getPercentualReservatorio(config);

  if (distRes >= config.res_dist_min_mm) {
    digitalWrite(PIN_LED_VERMELHO, HIGH);
    digitalWrite(PIN_LED_AMARELO, LOW);
    digitalWrite(PIN_LED_VERDE, LOW);
  } else if (percRes <= config.res_percent_critico) {
    digitalWrite(PIN_LED_VERMELHO, LOW);
    digitalWrite(PIN_LED_AMARELO, HIGH);
    digitalWrite(PIN_LED_VERDE, LOW);
  } else {
    digitalWrite(PIN_LED_VERMELHO, LOW);
    digitalWrite(PIN_LED_AMARELO, LOW);
    digitalWrite(PIN_LED_VERDE, HIGH);
  }
}

void TPAController::dispararTPA() {
  if (estado != IDLE) return;

  uint16_t distRes = sensores.getDistanciaReservatorio();
  if (distRes >= config.res_dist_min_mm) {
    logPrintln("[TPA-BLOQUEIO] Reservatorio vazio! TPA abortada por seguranca.");
    StorageModel::logError(sensores.getDataHoraAtual().c_str(), "TPA abortada: Reservatorio vazio");
    recalcularProximaTPAAposCiclo();
    return;
  }

  logPrintln("[TPA] Iniciando drenagem do aquario...");
  tempoTotalEnchimento = 0; // Zera acumulador de tempo para o novo ciclo
  tempoInicioEtapa = millis();
  estado = DRENANDO;
  digitalWrite(PIN_RELE_DRENAGEM, LOW);
}

void TPAController::abortar() {
  digitalWrite(PIN_RELE_DRENAGEM, HIGH);
  digitalWrite(PIN_RELE_ENCHIMENTO, HIGH);
  estado = IDLE;
}

void TPAController::ligarBombaDrenagemManual() { digitalWrite(PIN_RELE_DRENAGEM, LOW); }
void TPAController::desligarBombaDrenagemManual() { digitalWrite(PIN_RELE_DRENAGEM, HIGH); }
void TPAController::ligarBombaEnchimentoManual() { digitalWrite(PIN_RELE_ENCHIMENTO, LOW); }
void TPAController::desligarBombaEnchimentoManual() { digitalWrite(PIN_RELE_ENCHIMENTO, HIGH); }

uint32_t TPAController::calcularProximaTPAConfiguracao(const char* hora1Str, const char* hora2Str, int periodicidade, SensorModel &sens) {
  RtcDateTime agora = sens.getDateTimeAtual();
  int h1 = 9, m1 = 0;
  sscanf(hora1Str, "%d:%d", &h1, &m1);

  uint32_t minutosAgora = agora.Hour() * 60 + agora.Minute();
  uint32_t minutosH1 = h1 * 60 + m1;

  RtcDateTime alvo;
  if (minutosH1 >= minutosAgora) {
    alvo = RtcDateTime(agora.Year(), agora.Month(), agora.Day(), h1, m1, 0);
  } else {
    if (periodicidade == 2) {
      int h2 = 21, m2 = 0;
      sscanf(hora2Str, "%d:%d", &h2, &m2);
      uint32_t minutosH2 = h2 * 60 + m2;
      if (minutosH2 >= minutosAgora) {
        alvo = RtcDateTime(agora.Year(), agora.Month(), agora.Day(), h2, m2, 0);
        return alvo.TotalSeconds();
      }
    }
    RtcDateTime amanha(agora.TotalSeconds() + 86400UL);
    alvo = RtcDateTime(amanha.Year(), amanha.Month(), amanha.Day(), h1, m1, 0);
  }
  return alvo.TotalSeconds();
}

void TPAController::recalcularProximaTPAAposCiclo() {
  RtcDateTime agora = sensores.getDateTimeAtual();
  uint32_t minutosAgora = agora.Hour() * 60 + agora.Minute();

  int h1 = 9, m1 = 0, h2 = 21, m2 = 0;
  sscanf(config.tpa_hora1, "%d:%d", &h1, &m1);
  sscanf(config.tpa_hora2, "%d:%d", &h2, &m2);

  RtcDateTime alvo;
  if (config.tpa_periodicidade == 2) {
    uint32_t minutosH2 = h2 * 60 + m2;
    if (minutosH2 >= minutosAgora) {
      alvo = RtcDateTime(agora.Year(), agora.Month(), agora.Day(), h2, m2, 0);
    } else {
      RtcDateTime amanha(agora.TotalSeconds() + 86400UL);
      alvo = RtcDateTime(amanha.Year(), amanha.Month(), amanha.Day(), h1, m1, 0);
    }
  } else {
    RtcDateTime amanha(agora.TotalSeconds() + 86400UL);
    alvo = RtcDateTime(amanha.Year(), amanha.Month(), amanha.Day(), h1, m1, 0);
  }

  config.proxima_tpa_epoch = alvo.TotalSeconds();
  StorageModel::saveConfig(config);
}

void TPAController::finalizarCicloTPA(uint32_t tempoEnche, uint16_t nivelEncheMm, const char* motivoEnche) {
  config.vol_ultima_tpa = 1.0;

  StorageModel::logTPA(
    sensores.getDataHoraAtual().c_str(),
    duracaoDrenagemReal,
    nivelDrenagemRealMm,
    motivoDrenagemReal.c_str(),
    tempoEnche,
    nivelEncheMm,
    motivoEnche
  );

  if (tpaManualEmAndamento) {
    // Registra a hora real da execução manual e mantém a próxima TPA intacta
    config.ultima_tpa_epoch = sensores.getEpochAtual();
    tpaManualEmAndamento = false;
    StorageModel::saveConfig(config);
    logPrintln("[TPA] Ciclo manual concluido. Agendamento programado mantido sem alteracoes.");
  } else {
    config.ultima_tpa_epoch = config.proxima_tpa_epoch;
    recalcularProximaTPAAposCiclo();
  }

  estado = IDLE;
}

void TPAController::update() {
  atualizarLedsReservatorio();

  if (estado == IDLE && (millis() - ultimoCheckMinuto >= 15000 || ultimoCheckMinuto == 0)) {
    ultimoCheckMinuto = millis();
    uint32_t agoraEpoch = sensores.getEpochAtual();
    if (config.proxima_tpa_epoch > 0 && agoraEpoch >= config.proxima_tpa_epoch) {
      logPrintln("[TPA] Horario alcancado pelo RTC.");
      dispararTPA();
    }
  }

  // Etapa 1: Drenagem
  if (estado == DRENANDO) {
    uint32_t agoraMs = millis();
    uint32_t segDecorridos = (agoraMs >= tempoInicioEtapa) ? ((agoraMs - tempoInicioEtapa) / 1000) : 0;
    uint16_t distAq = sensores.getDistanciaAquario();

    bool leituraValida = (distAq > 0 && distAq < 2000);
    bool atingeNivel = (leituraValida && distAq >= config.aq_dist_min_mm);
    bool atingeTempo = (config.tempo_max_drenagem_seg > 0 && segDecorridos >= config.tempo_max_drenagem_seg);

    if (atingeNivel || atingeTempo) {
      digitalWrite(PIN_RELE_DRENAGEM, HIGH);
      duracaoDrenagemReal = segDecorridos;
      nivelDrenagemRealMm = distAq;
      motivoDrenagemReal = atingeNivel ? "NIVEL_MINIMO" : "TIMEOUT_SEGURANCA";
      logPrintf("[TPA] Drenagem finalizada por %s (%u s) - Nivel: %u mm\n", motivoDrenagemReal.c_str(), segDecorridos, distAq);

      logPrintf("[TPA] Aguardando %u s para escoamento final do sifao...\n", config.tempo_pausa_pos_dreno_seg);
      tempoInicioEtapa = millis();
      estado = PAUSA_POS_DRENO;
    }
  }
  // Etapa 1.5: Pausa configurável após drenagem (não-bloqueante)
  else if (estado == PAUSA_POS_DRENO) {
    uint32_t agoraMs = millis();
    uint32_t segPausa = (agoraMs >= tempoInicioEtapa) ? ((agoraMs - tempoInicioEtapa) / 1000) : 0;

    if (segPausa >= config.tempo_pausa_pos_dreno_seg) {
      logPrintln("[TPA] Pausa concluida. Iniciando reposicao de agua...");
      tempoInicioEtapa = millis();
      estado = ENCHENDO;
      digitalWrite(PIN_RELE_ENCHIMENTO, LOW);
    }
  }
  // Etapa 2: Enchimento
  else if (estado == ENCHENDO) {
    uint32_t agoraMs = millis();
    uint32_t segDecorridos = (agoraMs >= tempoInicioEtapa) ? ((agoraMs - tempoInicioEtapa) / 1000) : 0;
    uint16_t distAq = sensores.getDistanciaAquario();
    uint16_t distRes = sensores.getDistanciaReservatorio();

    bool leituraAqValida = (distAq > 0 && distAq < 2000);
    bool leituraResValida = (distRes > 0 && distRes < 2000);

    bool atingeNivel = (leituraAqValida && distAq <= config.aq_dist_max_mm);
    bool atingeTempo = (config.tempo_max_enchimento_seg > 0 && (tempoTotalEnchimento + segDecorridos) >= config.tempo_max_enchimento_seg);
    bool resEsvaziou = (leituraResValida && distRes >= config.res_dist_min_mm);

    if (atingeNivel || atingeTempo || resEsvaziou) {
      digitalWrite(PIN_RELE_ENCHIMENTO, HIGH);
      tempoTotalEnchimento += segDecorridos;

      if (atingeNivel && !atingeTempo && !resEsvaziou) {
        logPrintf("[TPA] Nivel alvo atingido (%u mm). Estabilizando por %u s...\n", distAq, config.tempo_estabilizacao_seg);
        tempoInicioEtapa = millis();
        estado = ESTABILIZANDO;
      } else {
        const char* motivo = resEsvaziou ? "RESERVATORIO_VAZIO" : "TIMEOUT_SEGURANCA";
        logPrintf("[TPA] Enchimento finalizado por %s (%u s totais) - Nivel: %u mm\n", motivo, tempoTotalEnchimento, distAq);
        finalizarCicloTPA(tempoTotalEnchimento, distAq, motivo);
      }
    }
  }
  // Etapa 3: Estabilização configurável pós-enchimento
  else if (estado == ESTABILIZANDO) {
    uint32_t agoraMs = millis();
    uint32_t segEstab = (agoraMs >= tempoInicioEtapa) ? ((agoraMs - tempoInicioEtapa) / 1000) : 0;

    if (segEstab >= config.tempo_estabilizacao_seg) {
      uint16_t distAqEstabilizada = sensores.getDistanciaAquario();

      if (distAqEstabilizada > (config.aq_dist_max_mm + config.tolerancia_estabilizacao_mm) && distAqEstabilizada < 2000) {
        logPrintf("[TPA] Nivel estabilizou em %u mm (alvo %u + tol %u mm). Acionando pulso complementar...\n",
                  distAqEstabilizada, config.aq_dist_max_mm, config.tolerancia_estabilizacao_mm);
        tempoInicioEtapa = millis();
        estado = ENCHENDO;
        digitalWrite(PIN_RELE_ENCHIMENTO, LOW);
      } else {
        logPrintf("[TPA] Enchimento concluido e estabilizado em %u mm (%u s totais)\n", distAqEstabilizada, tempoTotalEnchimento);
        finalizarCicloTPA(tempoTotalEnchimento, distAqEstabilizada, "NIVEL_MAXIMO_ESTABILIZADO");
      }
    }
  }
}

void TPAController::interromperManualmente() {
  if (estado == IDLE) return;

  String etapaAnterior = getNomeEstado();
  digitalWrite(PIN_RELE_DRENAGEM, HIGH);
  digitalWrite(PIN_RELE_ENCHIMENTO, HIGH);
  estado = IDLE;

  String msg = "TPA interrompida manualmente durante a etapa: " + etapaAnterior;
  logPrintln("[TPA-ABORT] " + msg);
  StorageModel::logError(sensores.getDataHoraAtual().c_str(), msg.c_str());

  if (tpaManualEmAndamento) {
    tpaManualEmAndamento = false;
  } else {
    recalcularProximaTPAAposCiclo();
  }
}

String TPAController::getNomeEstado() const {
  switch (estado) {
    case DRENANDO:        return "DRENANDO";
    case PAUSA_POS_DRENO: return "AGUARDANDO SIFÃO";
    case ENCHENDO:        return "ENCHENDO";
    case ESTABILIZANDO:   return "ESTABILIZANDO NÍVEL";
    case ALARME_ERRO:     return "ALARME DE ERRO";
    default:              return "OCIOSO";
  }
}

void TPAController::dispararTPAManual() {
  if (estado != IDLE) return;

  uint16_t distRes = sensores.getDistanciaReservatorio();
  if (distRes >= config.res_dist_min_mm) {
    logPrintln("[TPA-BLOQUEIO] Reservatorio vazio! TPA manual abortada por seguranca.");
    StorageModel::logError(sensores.getDataHoraAtual().c_str(), "TPA manual abortada: Reservatorio vazio");
    return; // Não recalcula o agendamento futuro
  }

  logPrintln("[TPA] Iniciando TPA MANUAL a pedido do usuario...");
  tpaManualEmAndamento = true;
  tempoTotalEnchimento = 0;
  tempoInicioEtapa = millis();
  estado = DRENANDO;
  digitalWrite(PIN_RELE_DRENAGEM, LOW);
}

String TPAController::getDataUltimaTPA() { return sensores.formatarDataHora(config.ultima_tpa_epoch); }
String TPAController::getDataProximaTPA() { return sensores.formatarDataHora(config.proxima_tpa_epoch); }