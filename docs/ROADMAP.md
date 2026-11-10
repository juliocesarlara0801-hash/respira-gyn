# Roadmap — RespiraGyn

Passo a passo cronológico. Marcado `[x]` o que já está entregue no MVP deste repositório, `[ ]` o que fica para evoluções pós-entrega.

---

## Fase 0 — Setup ✅
- [x] Definir escopo: mapear CO₂ veicular de Goiânia usando APIs de tráfego
- [x] Escolher stack: C (núcleo) + Python Flask (backend) + Leaflet (frontend)
- [x] Cadastrar 8 setores iniciais de Goiânia com lat/lng e espécie arbórea
- [x] Definir critério de entrega alinhado ao edital do Prof. Valdir

## Fase 1 — Núcleo em C ✅
- [x] `pilha_co2.h/.c` — struct Nodo com (ppm, id_setor, nivel_trafego, timestamp)
- [x] `push_leitura()` com filtro ≥400 ppm e proteção contra overflow
- [x] `pop_leitura()` com blindagem contra Underflow
- [x] `peek_leitura()` passivo (lê sem destruir)
- [x] `pilha_destruir()` libera toda a RAM
- [x] `estimador.c` — converte tráfego em PPM
- [x] `main.c` — demo CLI com os 3 critérios do edital
- [x] `main.c --backend` — pipe CSV/JSON para integração
- [x] Makefile com `make`, `make test`, `make clean`
- [x] `tests/test_pilha.c` — suíte unitária (todos passam)

## Fase 2 — Backend Python ✅
- [x] `traffic_client.py` — abstração `TrafficClient`
- [x] `MockTrafficClient` — simula tráfego com padrões de pico
- [x] `GoogleMapsTrafficClient` — consome Distance Matrix com `departure_time=now`
- [x] `app.py` — Flask + CORS + 3 rotas (`/`, `/api/setores`, `/api/leituras`)
- [x] Invocação do binário C via `subprocess.run()` com pipe
- [x] Cálculo de prioridade (BAIXA/MÉDIA/ALTA/CRÍTICA) a partir do PPM
- [x] `setores_goiania.json` com 8 setores + espécie + quantidade sugerida

## Fase 3 — Frontend ✅
- [x] `index.html` — layout em 2 colunas (mapa + painel lateral)
- [x] Leaflet + Leaflet.heat para o mapa de calor
- [x] Marcadores clicáveis com popup (PPM, tráfego, árvore sugerida)
- [x] Painel lateral com cards ordenados por PPM descendente
- [x] Auto-refresh a cada 60s + botão manual
- [x] Clique no card centraliza o mapa no setor
- [x] CSS responsivo e com paleta alinhada ao banner

## Fase 4 — Documentação ✅
- [x] `README.md` principal com quickstart
- [x] `docs/ARQUITETURA.md` — fluxo técnico detalhado
- [x] `docs/EDITAL.md` — mapeamento aos critérios do Prof. Valdir
- [x] `docs/ROADMAP.md` — este arquivo
- [x] `PROMPT_CLAUDE_CODE.md` — prompt para evoluir no Claude Code

---

## Fase 5 — Dia do Inovatech (03/11) 🎯
Para a entrega técnica deste dia:
- [ ] Rodar `make test` no laptop do grupo e salvar screenshot
- [ ] Rodar a demo `./verderotas` e salvar screenshot das 3 demos
- [ ] Rodar backend + frontend localmente e gravar GIF de 20s do mapa de calor atualizando
- [ ] Imprimir o banner (`verderota-co2-banner.pdf`)
- [ ] Levar notebook com Chrome aberto na aplicação funcionando
- [ ] Ensaiar fala de 3 minutos cobrindo: problema → solução → núcleo C → mapa

## Fase 6 — Evoluções (se sobrar tempo antes do 11/11) 🚀
- [x] Persistir leituras em SQLite (uma linha por setor por minuto) — `backend/db.py`
- [x] Dashboard de série temporal por setor (chart.js) — modal de 6h no frontend
- [x] Expandir de 8 → 30 setores com corredores reais de Goiânia
- [x] Previsão +1h/+3h/+6h — **migrada para C** em `core/forecast.c`
- [x] Recomendação de árvores dinâmica (PPM × área) — **em C** em `core/estimador.c`
- [x] Dockerfile multistage (gcc + python-slim) + `render.yaml` + `Procfile`
- [x] Rebranding completo: VerdeRota CO₂ → **RespiraGyn**
- [x] Redesign do frontend — Fraunces + JetBrains Mono, boletim de monitoramento, dark mode
- [ ] Deploy em Render/Railway publicado para a banca acessar via celular
- [ ] Integração real com Google Maps API (obter chave do Julio ou do grupo)
- [ ] Botão "exportar PDF" com snapshot do mapa para relatório final

## Fase 7 — Pós-Inovatech 🌱
- [ ] Publicar como projeto open-source no GitHub do grupo
- [ ] Contatar a Secretaria de Meio Ambiente de Goiânia
- [ ] Artigo curto de 4 páginas para publicação em evento acadêmico
- [ ] Portabilidade: adaptar setores para Anápolis, Brasília, SP
