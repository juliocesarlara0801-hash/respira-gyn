/* ============================================================================
 * app.js — Frontend do RespiraGyn
 * Carrega /api/leituras, pinta o boletim lateral, desenha o mapa de calor
 * sobre tiles neutros e abre o modal histórico com gráfico ao clicar.
 *
 * Tempo real: pesquisa /api/leituras a cada INTERVALO_MS e pode ser
 * pausado pelo usuário. Um contador regressivo mostra o próximo ciclo.
 *
 * Filtros de prioridade: 4 chips (BAIXA/MÉDIA/ALTA/CRÍTICA) em multi-seleção,
 * aplicam simultaneamente ao mapa (marcadores + calor) e à lista lateral.
 * O estado é persistido em localStorage.
 * ============================================================================ */

const GOIANIA_CENTER = [-16.686, -49.265];
const API_LEITURAS   = "/api/leituras";
const INTERVALO_MS   = 15000;                      // ciclo de atualização
const PRIORIDADES    = ["BAIXA", "MEDIA", "ALTA", "CRITICA"];
const LS_FILTROS     = "respiragyn.filtros";       // chave do localStorage

/* Paleta sincronizada com style.css — evita CSS-vars dentro do canvas. */
const COR = {
  moss:  "#3D5F3A",
  dry:   "#C09427",
  brick: "#9B2D21",
  wine:  "#4A1113",   // vinho / bordô (extremo crítico)
  rule:  "#B7B19F",
  ink:   "#171A14",
  mut:   "#4A4F42",
  bg:    "#F5F3EC",
};

const CORES_PRIO = {
  CRITICA: COR.brick,
  ALTA:    COR.dry,
  MEDIA:   COR.moss,
  BAIXA:   COR.rule,
};

/* ---------------------------------------------------------------------------
 *  Paleta contínua PPM → cor
 *  8 stops interpolados em RGB. Vai de "caqui limpo" (ar de base) até
 *  "vinho bordô" (extremo crítico). As cores intermediárias seguem o
 *  gradiente natural do Cerrado em estresse hídrico: olive → dourado →
 *  terra queimada → tijolo → vinho.
 * --------------------------------------------------------------------------- */
const PALETA_PPM = [
  { ppm:  420, cor: [183, 177, 159] },  // --rule     (atmosférico limpo)
  { ppm:  560, cor: [111, 150, 100] },  // moss vivo  (zona saudável)
  { ppm:  720, cor: [176, 150,  55] },  // olive dourado (transição)
  { ppm:  880, cor: [201, 143,  39] },  // --dry      (atenção)
  { ppm: 1050, cor: [200, 104,  38] },  // laranja queimado
  { ppm: 1250, cor: [155,  45,  33] },  // --brick    (crítico)
  { ppm: 1450, cor: [113,  30,  25] },  // vermelho profundo
  { ppm: 1700, cor: [ 74,  17,  19] },  // --wine     (extremo crítico)
];
const PPM_MIN_ESCALA = PALETA_PPM[0].ppm;
const PPM_MAX_ESCALA = PALETA_PPM[PALETA_PPM.length - 1].ppm;

function hex(c) { return c.toString(16).padStart(2, "0"); }
function rgbToHex([r, g, b]) { return "#" + hex(r) + hex(g) + hex(b); }

function corPorPpm(ppm) {
  if (ppm <= PALETA_PPM[0].ppm) return rgbToHex(PALETA_PPM[0].cor);
  const last = PALETA_PPM[PALETA_PPM.length - 1];
  if (ppm >= last.ppm) return rgbToHex(last.cor);
  for (let i = 0; i < PALETA_PPM.length - 1; i++) {
    const a = PALETA_PPM[i], b = PALETA_PPM[i + 1];
    if (ppm <= b.ppm) {
      const t = (ppm - a.ppm) / (b.ppm - a.ppm);
      return rgbToHex([
        Math.round(a.cor[0] + (b.cor[0] - a.cor[0]) * t),
        Math.round(a.cor[1] + (b.cor[1] - a.cor[1]) * t),
        Math.round(a.cor[2] + (b.cor[2] - a.cor[2]) * t),
      ]);
    }
  }
  return rgbToHex(last.cor);
}

/* Raio em metros da "zona de impacto" no mapa. Setor com PPM de base
   ainda aparece como círculo pequeno (presença); crítico extremo ocupa
   até ~1 km², consistente com o alcance de dispersão do CO₂ veicular. */
function raioZona(ppm) {
  const t = Math.max(0, Math.min(1, (ppm - PPM_MIN_ESCALA) / (PPM_MAX_ESCALA - PPM_MIN_ESCALA)));
  return 280 + t * 680;   // 280 m → 960 m
}

/* Versão mais escura da cor (contorno sutilmente mais intenso). */
function corContorno(ppm) {
  return corPorPpm(Math.min(ppm + 160, PPM_MAX_ESCALA));
}

const mapa = L.map("mapa", {
  zoomControl: true,
  attributionControl: true,
}).setView(GOIANIA_CENTER, 12);

/* OpenStreetMap padrão — grátis, sem chave, sem referrer check.
   Uso respeita a política de "small-scale use" da OSM Foundation. */
L.tileLayer("https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png", {
  maxZoom: 19,
  attribution: "© OpenStreetMap contributors",
}).addTo(mapa);

/* Força o Leaflet a recomputar o tamanho do container após o 1º layout
   e em qualquer resize — evita o bug clássico do mapa em branco quando
   o container ganha altura depois do map init. */
function recalcularMapa() { mapa.invalidateSize(true); }
window.addEventListener("load", recalcularMapa);
window.addEventListener("resize", recalcularMapa);
setTimeout(recalcularMapa, 300);  // teto defensivo para fontes/tiles lentas

let camadaCalor = null;
let camadaMarcadores = null;
let graficoChart = null;

/* Estado global */
let leiturasAtuais = [];            // último payload de /api/leituras
let ppmAnterior    = {};            // id → ppm da leitura anterior (para animar deltas)
let filtrosAtivos  = carregarFiltros();  // Set<PRIORIDADE>
let temporizador   = null;
let countdown      = null;
let proximaEm      = INTERVALO_MS / 1000;
let pausado        = false;

function pad2(n) { return n < 10 ? "0" + n : "" + n; }
function corPorPrioridade(prio) { return CORES_PRIO[prio] || COR.rule; }
function prioClass(prio)        { return "p-" + prio.toLowerCase(); }
function fmtPpm(n)              { return Math.round(n).toLocaleString("pt-BR"); }

/* ----------------------------------------------------------------------------
 *  Filtros de prioridade
 * ---------------------------------------------------------------------------- */

function carregarFiltros() {
  try {
    const raw = localStorage.getItem(LS_FILTROS);
    if (raw) {
      const arr = JSON.parse(raw);
      if (Array.isArray(arr) && arr.length > 0) {
        return new Set(arr.filter(p => PRIORIDADES.includes(p)));
      }
    }
  } catch (_) { /* localStorage indisponível em private mode etc. */ }
  return new Set(PRIORIDADES);   // default: mostra tudo
}

function salvarFiltros() {
  try {
    localStorage.setItem(LS_FILTROS, JSON.stringify([...filtrosAtivos]));
  } catch (_) { /* silencioso */ }
}

function passaFiltro(l) {
  return filtrosAtivos.has(l.prioridade);
}

function montarChipsFiltro() {
  const container = document.getElementById("filtros-prio");
  if (!container) return;

  container.innerHTML = "";

  const todas = document.createElement("button");
  todas.className = "chip-filtro chip-filtro-todas";
  todas.textContent = "todas";
  todas.setAttribute("type", "button");
  todas.addEventListener("click", () => {
    filtrosAtivos = new Set(PRIORIDADES);
    salvarFiltros();
    sincronizarChips();
    aplicarFiltros();
  });
  container.appendChild(todas);

  PRIORIDADES.forEach(p => {
    const btn = document.createElement("button");
    btn.className = `chip-filtro ${prioClass(p)}`;
    btn.dataset.prio = p;
    btn.setAttribute("type", "button");
    btn.setAttribute("aria-pressed", filtrosAtivos.has(p) ? "true" : "false");
    btn.innerHTML = `
      <span class="chip-ponto" aria-hidden="true"></span>
      <span class="chip-rotulo">${p.toLowerCase()}</span>
      <span class="chip-contagem" data-prio="${p}">0</span>
    `;
    btn.addEventListener("click", () => {
      if (filtrosAtivos.has(p)) {
        if (filtrosAtivos.size === 1) {
          // Clicar no único ativo expande pra todos (evita estado vazio inútil).
          filtrosAtivos = new Set(PRIORIDADES);
        } else {
          filtrosAtivos.delete(p);
        }
      } else {
        filtrosAtivos.add(p);
      }
      salvarFiltros();
      sincronizarChips();
      aplicarFiltros();
    });
    container.appendChild(btn);
  });

  sincronizarChips();
}

function sincronizarChips() {
  document.querySelectorAll(".chip-filtro[data-prio]").forEach(btn => {
    const p = btn.dataset.prio;
    const ativo = filtrosAtivos.has(p);
    btn.classList.toggle("ativo", ativo);
    btn.setAttribute("aria-pressed", ativo ? "true" : "false");
  });
  const todas = document.querySelector(".chip-filtro-todas");
  if (todas) {
    const mostrandoTudo = filtrosAtivos.size === PRIORIDADES.length;
    todas.classList.toggle("ativo", mostrandoTudo);
  }
}

function atualizarContagens() {
  const cont = { BAIXA: 0, MEDIA: 0, ALTA: 0, CRITICA: 0 };
  leiturasAtuais.forEach(l => { cont[l.prioridade] = (cont[l.prioridade] || 0) + 1; });
  PRIORIDADES.forEach(p => {
    const span = document.querySelector(`.chip-contagem[data-prio="${p}"]`);
    if (span) span.textContent = cont[p] || 0;
  });
}

function aplicarFiltros() {
  const visiveis = leiturasAtuais.filter(passaFiltro);
  desenharMapa(visiveis);
  preencherLista(visiveis);

  const painelVazio = document.getElementById("painel-vazio");
  if (painelVazio) {
    painelVazio.classList.toggle("oculto", visiveis.length > 0);
  }
}

/* ----------------------------------------------------------------------------
 *  Mapa
 * ---------------------------------------------------------------------------- */

function desenharMapa(leituras) {
  /* Heatmap de fundo — intensidade = ppm normalizado na escala contínua.
     Os stops seguem a paleta PPM para dar continuidade visual entre o
     mapa de calor e os círculos de zona. */
  const pontos = leituras.map(l => [
    l.lat, l.lng,
    Math.max(0.05, Math.min(1, (l.ppm - PPM_MIN_ESCALA) / (PPM_MAX_ESCALA - PPM_MIN_ESCALA))),
  ]);

  if (camadaCalor) mapa.removeLayer(camadaCalor);
  if (pontos.length > 0) {
    camadaCalor = L.heatLayer(pontos, {
      radius: 65, blur: 45, maxZoom: 15,
      gradient: {
        0.00: rgbToHex(PALETA_PPM[0].cor),
        0.14: rgbToHex(PALETA_PPM[1].cor),
        0.28: rgbToHex(PALETA_PPM[2].cor),
        0.42: rgbToHex(PALETA_PPM[3].cor),
        0.56: rgbToHex(PALETA_PPM[4].cor),
        0.70: rgbToHex(PALETA_PPM[5].cor),
        0.85: rgbToHex(PALETA_PPM[6].cor),
        1.00: rgbToHex(PALETA_PPM[7].cor),
      },
    }).addTo(mapa);
  } else {
    camadaCalor = null;
  }

  if (camadaMarcadores) mapa.removeLayer(camadaMarcadores);
  camadaMarcadores = L.layerGroup().addTo(mapa);

  /* Desenho em dois passos: primeiro todas as ZONAS (circles grandes, abaixo),
     depois todos os CENTROS (pontos pequenos, acima). Assim um crítico não
     fica atrás da zona de um vizinho crítico ainda maior. */
  const zonas  = [];
  const centros = [];

  leituras.forEach(l => {
    const cor      = corPorPpm(l.ppm);
    const contorno = corContorno(l.ppm);
    const raio     = raioZona(l.ppm);

    const popup = `
      <b>${l.nome}</b>
      <div class="popup-ppm" style="color:${contorno}">${fmtPpm(l.ppm)} ppm</div>
      Prioridade: <b>${l.prioridade}</b><br>
      Fluxo ${l.fluxo_veicular} veíc/h · ${l.velocidade_media} km/h<br>
      <div class="popup-especie">${l.especie_recomendada}</div>
      Plantar <b>${l.arvores_sugeridas}</b> árvores novas.
    `;

    const zona = L.circle([l.lat, l.lng], {
      radius: raio,
      color: contorno,
      weight: 1.4,
      fillColor: cor,
      fillOpacity: 0.42,
    });
    zona.bindPopup(popup);
    zonas.push(zona);

    const centro = L.circleMarker([l.lat, l.lng], {
      radius: 4.5,
      color: contorno,
      fillColor: cor,
      fillOpacity: 1,
      weight: 1.2,
    });
    centro.bindPopup(popup);
    centros.push(centro);
  });

  zonas.forEach(z => z.addTo(camadaMarcadores));
  centros.forEach(c => c.addTo(camadaMarcadores));
}

/* ----------------------------------------------------------------------------
 *  Boletim lateral
 * ---------------------------------------------------------------------------- */

function preencherLista(leituras) {
  const lista = document.getElementById("lista-setores");
  lista.innerHTML = "";

  const ordenadas = [...leituras].sort((a, b) => b.ppm - a.ppm);

  ordenadas.forEach(l => {
    const li = document.createElement("li");
    li.className = `registro-item ${prioClass(l.prioridade)}`;
    li.setAttribute("tabindex", "0");
    li.setAttribute("role", "button");
    /* Barra vertical lateral pintada com a cor contínua do PPM atual.
       Isso faz a lista "pulsar" em sintonia com o mapa. */
    li.style.setProperty("--barra-cor", corPorPpm(l.ppm));

    const prev = ppmAnterior[l.id];
    const delta = (prev !== undefined) ? (l.ppm - prev) : 0;
    const deltaClass =
      Math.abs(delta) < 1 ? "" : (delta > 0 ? "delta-sobe" : "delta-desce");
    const deltaFmt = (prev === undefined || Math.abs(delta) < 1)
      ? "" : (delta > 0 ? "↑ " : "↓ ") + Math.round(Math.abs(delta));

    li.innerHTML = `
      <span class="registro-id">${pad2(l.id)}</span>
      <span class="registro-nome">${l.nome}</span>
      <span class="registro-ppm">
        ${fmtPpm(l.ppm)}
        <small>ppm · ${l.prioridade.toLowerCase()}</small>
      </span>
      <div class="registro-detalhe">
        <span class="registro-especie">${l.especie_recomendada}</span>
        <span class="registro-arvores">${l.arvores_sugeridas}<small>árvores</small></span>
      </div>
      ${deltaFmt
        ? `<span class="registro-delta ${deltaClass}" aria-label="variação desde o ciclo anterior">${deltaFmt}</span>`
        : ""}
    `;
    const abrir = () => {
      mapa.setView([l.lat, l.lng], 15);
      abrirModalHistorico(l);
    };
    li.addEventListener("click", abrir);
    li.addEventListener("keydown", (e) => {
      if (e.key === "Enter" || e.key === " ") { e.preventDefault(); abrir(); }
    });
    lista.appendChild(li);
  });
}

/* ----------------------------------------------------------------------------
 *  Modal de histórico + previsão
 * ---------------------------------------------------------------------------- */

async function abrirModalHistorico(leitura) {
  const modal   = document.getElementById("modal-historico");
  const idElem  = document.getElementById("modal-id");
  const titulo  = document.getElementById("modal-titulo");
  const sub     = document.getElementById("modal-sub");
  const vazio   = document.getElementById("modal-vazio");
  const badgeA  = document.getElementById("modal-badge-atual");
  const badgeP  = document.getElementById("modal-badge-prev");
  const canvas  = document.getElementById("grafico-ppm");

  idElem.textContent = pad2(leitura.id);
  titulo.textContent = leitura.nome;
  sub.textContent    = `${fmtPpm(leitura.ppm)} ppm agora · ${leitura.especie_recomendada}`;

  badgeA.textContent = `agora ${fmtPpm(leitura.ppm)} ppm`;
  badgeA.className   = "chip chip-neutro";
  badgeA.classList.remove("oculto");
  badgeP.classList.add("oculto");

  modal.classList.remove("oculto");
  modal.setAttribute("aria-hidden", "false");

  try {
    const [hist, prev] = await Promise.all([
      fetch(`/api/historico/${leitura.id}?horas=6`).then(r => r.json()),
      fetch(`/api/previsao/${leitura.id}`).then(r => r.json()),
    ]);

    const pontos = hist.pontos || [];
    if (pontos.length === 0) {
      vazio.classList.remove("oculto");
      canvas.classList.add("oculto");
      if (graficoChart) { graficoChart.destroy(); graficoChart = null; }
    } else {
      vazio.classList.add("oculto");
      canvas.classList.remove("oculto");
      desenharChart(canvas, pontos, leitura.prioridade);
    }

    renderBadgePrevisao(badgeP, prev);
  } catch (err) {
    console.error(err);
  }
}

function desenharChart(canvas, pontos, prio) {
  const labels = pontos.map(p =>
    new Date(p.ts * 1000).toLocaleTimeString("pt-BR",
      { hour: "2-digit", minute: "2-digit" })
  );
  const dados = pontos.map(p => p.ppm);
  const cor   = corPorPrioridade(prio);

  if (graficoChart) graficoChart.destroy();
  graficoChart = new Chart(canvas, {
    type: "line",
    data: {
      labels,
      datasets: [{
        label: "PPM",
        data: dados,
        borderColor: cor,
        backgroundColor: cor + "22",
        fill: true,
        tension: 0.3,
        pointRadius: 2,
        pointHoverRadius: 5,
        pointBackgroundColor: cor,
        borderWidth: 2,
      }],
    },
    options: {
      responsive: true,
      maintainAspectRatio: false,
      interaction: { mode: "index", intersect: false },
      plugins: {
        legend: { display: false },
        tooltip: {
          backgroundColor: COR.ink,
          titleFont: { family: "JetBrains Mono" },
          bodyFont:  { family: "JetBrains Mono" },
          padding: 10,
          cornerRadius: 2,
        },
      },
      scales: {
        y: {
          beginAtZero: false,
          grid: { color: COR.rule + "55" },
          ticks: {
            font: { family: "JetBrains Mono", size: 11 },
            color: COR.mut,
          },
          title: {
            display: true,
            text: "ppm",
            font: { family: "Fraunces", style: "italic", size: 11 },
            color: COR.mut,
          },
        },
        x: {
          grid: { display: false },
          ticks: {
            maxTicksLimit: 8,
            font: { family: "JetBrains Mono", size: 11 },
            color: COR.mut,
          },
        },
      },
    },
  });
}

function renderBadgePrevisao(elem, prev) {
  if (!prev || !prev.previsoes || prev.previsoes.length === 0) {
    elem.classList.add("oculto");
    return;
  }
  const pico = prev.previsoes.find(p => p.pico);
  if (pico) {
    const quando = pad2(pico.hora_alvo) + "h";
    elem.textContent = `↑ pico previsto para ${quando} · ${fmtPpm(pico.ppm)} ppm`;
    elem.className = "chip chip-pico";
  } else {
    const prox = prev.previsoes[0];
    elem.textContent = `→ +${prox.horas}h: ${fmtPpm(prox.ppm)} ppm`;
    elem.className = "chip chip-neutro";
  }
  elem.classList.remove("oculto");
}

function fecharModal() {
  const modal = document.getElementById("modal-historico");
  modal.classList.add("oculto");
  modal.setAttribute("aria-hidden", "true");
}

document.getElementById("btn-fechar-modal").addEventListener("click", fecharModal);
document.getElementById("modal-historico").addEventListener("click", (e) => {
  if (e.target.id === "modal-historico") fecharModal();
});
document.addEventListener("keydown", (e) => {
  if (e.key === "Escape") fecharModal();
});

/* ----------------------------------------------------------------------------
 *  Ciclo de atualização em tempo real
 * ---------------------------------------------------------------------------- */

async function carregar() {
  const status = document.getElementById("status");
  const ponto  = document.getElementById("status-ponto");
  status.textContent = "coletando tráfego…";

  try {
    const resp = await fetch(API_LEITURAS);
    const data = await resp.json();

    // Guarda ppm anterior antes de sobrescrever (pra desenhar deltas).
    ppmAnterior = {};
    leiturasAtuais.forEach(l => { ppmAnterior[l.id] = l.ppm; });

    leiturasAtuais = data.leituras;
    atualizarContagens();
    aplicarFiltros();

    const horario = new Date().toLocaleTimeString("pt-BR",
      { hour: "2-digit", minute: "2-digit", second: "2-digit" });
    const fonte = data.fonte_trafego === "GoogleMapsTrafficClient" ? "Google Maps" : "mock";
    status.textContent = `${data.total_setores} setores · ${fonte} · ${horario}`;

    ponto.classList.remove("piscando");
    void ponto.offsetWidth;        // force reflow para re-disparar animação
    ponto.classList.add("piscando");
  } catch (err) {
    status.textContent = "erro: " + err.message;
    console.error(err);
  }
}

function tickCountdown() {
  const el = document.getElementById("countdown");
  if (!el) return;
  if (pausado) {
    el.textContent = "pausado";
    return;
  }
  proximaEm = Math.max(0, proximaEm - 1);
  el.textContent = `próximo ciclo em ${proximaEm}s`;
}

function iniciarCiclo() {
  pararCiclo();
  proximaEm = INTERVALO_MS / 1000;
  temporizador = setInterval(async () => {
    if (pausado) return;
    await carregar();
    proximaEm = INTERVALO_MS / 1000;
  }, INTERVALO_MS);
  countdown = setInterval(tickCountdown, 1000);
  tickCountdown();
}

function pararCiclo() {
  if (temporizador) { clearInterval(temporizador); temporizador = null; }
  if (countdown)    { clearInterval(countdown);    countdown    = null; }
}

function alternarPausa() {
  pausado = !pausado;
  const btn = document.getElementById("btn-pausar");
  const el  = document.getElementById("countdown");
  if (pausado) {
    btn.textContent = "Retomar";
    btn.setAttribute("aria-pressed", "true");
    if (el) el.textContent = "pausado";
  } else {
    btn.textContent = "Pausar";
    btn.setAttribute("aria-pressed", "false");
    proximaEm = INTERVALO_MS / 1000;
    tickCountdown();
  }
}

/* ----------------------------------------------------------------------------
 *  Bootstrap
 * ---------------------------------------------------------------------------- */

montarChipsFiltro();

document.getElementById("btn-atualizar").addEventListener("click", async () => {
  await carregar();
  proximaEm = INTERVALO_MS / 1000;
});
document.getElementById("btn-pausar").addEventListener("click", alternarPausa);

carregar();
iniciarCiclo();
