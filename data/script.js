let pausarPolling = false;

function trocarTela(id) {
  document.getElementById('tela-dashboard').classList.add('hidden');
  document.getElementById('tela-config').classList.add('hidden');
  document.getElementById('tela-logs').classList.add('hidden');
  document.getElementById('tela-sistema').classList.add('hidden');
  document.getElementById('tela-' + id).classList.remove('hidden');

  document.querySelectorAll('nav button').forEach(btn => btn.classList.remove('active'));
  const navBtn = document.getElementById('nav-' + id);
  if (navBtn) navBtn.classList.add('active');

  if (id === 'config' || id === 'sistema') {
    carregarConfiguracoes();
  } else if (id === 'logs') {
    carregarLogs();
  }
}

document.getElementById('rad_1x').addEventListener('change', () => {
  document.getElementById('box_hora2').style.display = 'none';
});
document.getElementById('rad_2x').addEventListener('change', () => {
  document.getElementById('box_hora2').style.display = 'block';
});

let drenoLigado = false, timerDrenoInterval = null, segDreno = 0;
let encheLigado = false, timerEncheInterval = null, segEnche = 0;

function toggleBomba(tipo) {
  if (tipo === 'dreno') {
    drenoLigado = !drenoLigado;
    fetch('/api/bomba?tipo=dreno&acao=' + (drenoLigado ? 'liga' : 'desliga'));
    if (drenoLigado) {
      segDreno = 0;
      timerDrenoInterval = setInterval(() => { segDreno++; document.getElementById('timer_dreno').innerText = segDreno; }, 1000);
    } else {
      clearInterval(timerDrenoInterval);
    }
  } else {
    encheLigado = !encheLigado;
    fetch('/api/bomba?tipo=enche&acao=' + (encheLigado ? 'liga' : 'desliga'));
    if (encheLigado) {
      segEnche = 0;
      timerEncheInterval = setInterval(() => { segEnche++; document.getElementById('timer_enche').innerText = segEnche; }, 1000);
    } else {
      clearInterval(timerEncheInterval);
    }
  }
}

function resetBoots() {
  fetch('/api/reset_boot', { method: 'POST' }).then(() => atualizarDashboard());
}

function carregarConfiguracoes() {
  fetch('/api/config')
    .then(r => r.json())
    .then(cfg => {
      if (cfg.tpa_periodicidade === 2) {
        document.getElementById('rad_2x').checked = true;
        document.getElementById('box_hora2').style.display = 'block';
      } else {
        document.getElementById('rad_1x').checked = true;
        document.getElementById('box_hora2').style.display = 'none';
      }

      document.getElementById('cfg_hora1').value = cfg.tpa_hora1;
      document.getElementById('cfg_hora2').value = cfg.tpa_hora2;
      document.getElementById('cfg_aq_max').value = cfg.aq_dist_max_mm;
      document.getElementById('cfg_aq_min').value = cfg.aq_dist_min_mm;
      document.getElementById('cfg_res_max').value = cfg.res_dist_max_mm;
      document.getElementById('cfg_res_min').value = cfg.res_dist_min_mm;
      document.getElementById('cfg_res_vol').value = cfg.res_volume_total_l;
      document.getElementById('cfg_res_critico').value = cfg.res_percent_critico;
      document.getElementById('cfg_tempo_dreno').value = cfg.tempo_max_drenagem_seg;
      document.getElementById('cfg_tempo_enche').value = cfg.tempo_max_enchimento_seg;
      document.getElementById('wifi_ssid').value = cfg.wifi_ssid;
      document.getElementById('wifi_pass').value = "";
      document.getElementById('cfg_pausa_dreno').value = cfg.tempo_pausa_pos_dreno_seg;
      document.getElementById('cfg_estab_seg').value = cfg.tempo_estabilizacao_seg;
      document.getElementById('cfg_estab_tol').value = cfg.tolerancia_estabilizacao_mm;
    });
}

function salvarConfigTPA() {
  pausarPolling = true; // Libera o ESP32 exclusivamente para o POST
  mostrarLoading('Gravando configurações no ESP32...');

  const periodicidade = document.getElementById('rad_2x').checked ? 2 : 1;
  const dados = {
    tpa_periodicidade: periodicidade,
    tpa_hora1: document.getElementById('cfg_hora1').value,
    tpa_hora2: document.getElementById('cfg_hora2').value,
    aq_dist_max_mm: parseInt(document.getElementById('cfg_aq_max').value),
    aq_dist_min_mm: parseInt(document.getElementById('cfg_aq_min').value),
    res_dist_max_mm: parseInt(document.getElementById('cfg_res_max').value),
    res_dist_min_mm: parseInt(document.getElementById('cfg_res_min').value),
    res_volume_total_l: parseFloat(document.getElementById('cfg_res_vol').value),
    res_percent_critico: parseFloat(document.getElementById('cfg_res_critico').value),
    tempo_max_drenagem_seg: parseInt(document.getElementById('cfg_tempo_dreno').value),
    tempo_max_enchimento_seg: parseInt(document.getElementById('cfg_tempo_enche').value),
    tempo_pausa_pos_dreno_seg: parseInt(document.getElementById('cfg_pausa_dreno').value),
    tempo_estabilizacao_seg: parseInt(document.getElementById('cfg_estab_seg').value),
    tolerancia_estabilizacao_mm: parseInt(document.getElementById('cfg_estab_tol').value)
  };

  fetch('/api/config_tpa', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(dados)
  })
  .then(r => r.json())
  .then(res => {
    mostrarLoading('Configurações salvas! Reiniciando ESP32 e recarregando página...');
    setTimeout(() => { location.reload(); }, 4000);
  })
  .catch(() => {
    mostrarLoading('Reiniciando ESP32 e reconectando...');
    setTimeout(() => { location.reload(); }, 4000);
  });
}

function salvarConfigWiFi() {
  mostrarLoading('Gravando configurações no ESP32...');

  const dados = {
    wifi_ssid: document.getElementById('wifi_ssid').value,
    wifi_pass: document.getElementById('wifi_pass').value
  };

  fetch('/api/config_wifi', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(dados)
  })
  .then(r => r.json())
  .then(res => {
    mostrarLoading('Configurações salvas! Reiniciando ESP32 e recarregando página...');
    setTimeout(() => { location.reload(); }, 5000);
  }).catch(() => {
    mostrarLoading('Reiniciando ESP32 e reconectando...');
    setTimeout(() => { location.reload(); }, 5000);
  });
}

function abrirModalWifi() {
  document.getElementById('modal-wifi').classList.remove('hidden');
  document.getElementById('scan-status').innerText = 'Buscando redes próximas...';
  const lista = document.getElementById('lista-redes');
  lista.innerHTML = '';

  fetch('/api/wifi_scan')
    .then(r => r.json())
    .then(redes => {
      document.getElementById('scan-status').innerText = 'Selecione uma rede abaixo:';
      redes.forEach(rede => {
        const li = document.createElement('li');
        li.innerHTML = `<strong>${rede.ssid}</strong> <span>${rede.rssi} dBm</span>`;
        li.onclick = () => {
          document.getElementById('wifi_ssid').value = rede.ssid;
          fecharModalWifi();
        };
        lista.appendChild(li);
      });
    });
}

function fecharModalWifi() {
  document.getElementById('modal-wifi').classList.add('hidden');
}

function atualizarDashboard() {
  if (pausarPolling) {
    setTimeout(atualizarDashboard, 2500);
    return;
  }

  fetch('/api/status')
    .then(r => r.json())
    .then(data => {
      document.getElementById('temp').innerText = data.temperatura.toFixed(1);
      document.getElementById('data_hora').innerText = data.data_hora;
      document.getElementById('vol_aq').innerText = data.volume_aquario.toFixed(2);
      document.getElementById('dist_aq').innerText = data.dist_aquario;
      document.getElementById('vol_res').innerText = data.volume_reservatorio.toFixed(2);
      document.getElementById('perc_res').innerText = data.percent_reservatorio.toFixed(0);
      document.getElementById('dist_res').innerText = data.dist_reservatorio;
      document.getElementById('ult_tpa').innerText = data.ultima_tpa || 'Nenhuma';
      document.getElementById('vol_ult_tpa').innerText = data.vol_ultima_tpa.toFixed(2);
      document.getElementById('prox_tpa').innerText = data.proxima_tpa;
      document.getElementById('boots').innerText = data.boot_counter;

      // --- BLOCO NOVO: EXIBE OU OCULTA O BOTÃO DE INTERROMPER TPA ---
      const cardTpaAtiva = document.getElementById('card-tpa-ativa');
      const btnExecutarTpa = document.getElementById('btn-executar-tpa');
      if (cardTpaAtiva) {
        if (data.tpa_em_andamento) {
          document.getElementById('tpa_estado').innerText = data.tpa_estado;
          cardTpaAtiva.classList.remove('hidden');
          if (btnExecutarTpa) btnExecutarTpa.classList.add('hidden');
        } else {
          cardTpaAtiva.classList.add('hidden');
          if (btnExecutarTpa) btnExecutarTpa.classList.remove('hidden');
        }
      }
      // -------------------------------------------------------------

      const alerta = document.getElementById('alerta_critico');
      if (data.nivel_critico || data.reservatorio_vazio) {
        alerta.classList.remove('hidden');
      } else {
        alerta.classList.add('hidden');
      }

      const liveAq = document.getElementById('live_dist_aq');
      const liveRes = document.getElementById('live_dist_res');
      const liveTemp = document.getElementById('live_temp_cfg');
      if (liveAq) liveAq.innerText = data.dist_aquario;
      if (liveRes) liveRes.innerText = data.dist_reservatorio;
      if (liveTemp) liveTemp.innerText = data.temperatura.toFixed(1);
    })
    .finally(() => {
      setTimeout(atualizarDashboard, 2500);
    });
}

function carregarLogs() {
  document.getElementById('box-log-tpa').innerText = 'Buscando registros no SD...';
  document.getElementById('box-log-erro').innerText = 'Buscando erros no SD...';

  fetch('/api/logs')
    .then(r => r.json())
    .then(data => {
      document.getElementById('box-log-tpa').innerText = data.tpa_log;
      document.getElementById('box-log-erro').innerText = data.err_log;
    });
}

function limparLog(tipo) {
  if (!confirm('Deseja realmente apagar este arquivo de log do SD Card?')) return;
  fetch('/api/logs/clear?tipo=' + tipo, { method: 'POST' })
    .then(() => carregarLogs());
}

function mostrarLoading(texto) {
  document.getElementById('loading-text').innerText = texto || 'Processando...';
  document.getElementById('loading-overlay').classList.remove('hidden');
}

function esconderLoading() {
  document.getElementById('loading-overlay').classList.add('hidden');
}

function interromperTPA() {
  if (!confirm('Deseja interromper imediatamente a TPA em andamento e desligar as bombas?')) return;

  pausarPolling = true;
  fetch('/api/tpa/abort', { method: 'POST' })
    .then(r => r.json())
    .then(() => {
      alert('TPA interrompida! As bombas foram desligadas.');
    })
    .finally(() => {
      pausarPolling = false;
      atualizarDashboard();
    });
}

function executarTPAAgora() {
  if (!confirm('Deseja iniciar uma TPA completa agora? (O agendamento automático será mantido)')) return;

  pausarPolling = true;
  fetch('/api/tpa/start', { method: 'POST' })
    .then(r => r.json())
    .then(res => {
      if (res.status === 'erro') {
        alert(res.mensagem);
      }
    })
    .finally(() => {
      pausarPolling = false;
      atualizarDashboard();
    });
}

function salvarConfigTemp() {
  const minLiga = parseFloat(document.getElementById('cfg_temp_min_liga').value);
  const minDesliga = parseFloat(document.getElementById('cfg_temp_min_desliga').value);
  const maxLiga = parseFloat(document.getElementById('cfg_temp_max_liga').value);
  const maxDesliga = parseFloat(document.getElementById('cfg_temp_max_desliga').value);

  if (minLiga >= minDesliga) {
    alert('A temperatura para ligar o aquecedor deve ser menor que a de desligar.');
    return;
  }
  if (maxLiga <= maxDesliga) {
    alert('A temperatura para ligar o cooler deve ser maior que a de desligar.');
    return;
  }
  if (minDesliga >= maxDesliga) {
    alert('A faixa do aquecedor e do cooler não pode se cruzar (mantenha uma zona morta entre eles).');
    return;
  }

  pausarPolling = true;
  mostrarLoading('Salvando configurações de temperatura...');

  const dadosTemp = {
    temp_controle_ativo: document.getElementById('cfg_temp_ativo').checked,
    temp_min_liga: minLiga,
    temp_min_desliga: minDesliga,
    temp_max_liga: maxLiga,
    temp_max_desliga: maxDesliga
  };

  fetch('/api/config_temp', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(dadosTemp)
  })
  .then(r => r.json())
  .then(res => {
    esconderLoading();
    alert(res.mensagem || 'Configurações de temperatura salvas e aplicadas!');
  })
  .catch(() => {
    esconderLoading();
    alert('Erro ao salvar configurações de temperatura.');
  })
  .finally(() => {
    pausarPolling = false;
  });
}

// Substitua o antigo setInterval(atualizarDashboard, 2000); por apenas:
window.onload = atualizarDashboard;