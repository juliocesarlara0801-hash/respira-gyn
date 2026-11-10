# Arquitetura — RespiraGyn

Documento técnico que explica **como as três camadas conversam entre si** e por que cada escolha foi feita.

---

## 1. Visão geral em três camadas

```
┌─────────────────────────────────────────────────────────────┐
│ 1. COLETA                                                   │
│    traffic_client.py                                        │
│    - MockTrafficClient      (desenvolvimento / sala)        │
│    - GoogleMapsTrafficClient (produção, precisa de API key) │
└──────────────────────────┬──────────────────────────────────┘
                           │  AmostraTrafego (CSV)
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ 2. PROCESSAMENTO — NÚCLEO EM C                              │
│    core/verderotas --backend                                │
│    estimador.c  →  converte tráfego em PPM                  │
│    pilha_co2.c  →  empilha leitura (LIFO) com filtro ≥400   │
└──────────────────────────┬──────────────────────────────────┘
                           │  JSON por setor
                           ▼
┌─────────────────────────────────────────────────────────────┐
│ 3. APRESENTAÇÃO                                             │
│    backend/app.py       (Flask + CORS)                      │
│    frontend/*           (Leaflet + heatmap)                 │
└─────────────────────────────────────────────────────────────┘
```

## 2. Por que separar C e Python

| Responsabilidade | Linguagem | Motivo |
|------------------|-----------|--------|
| Pilha LIFO + lógica de ponteiros | **C** | Requisito da disciplina (Prof. Valdir). Também é onde o professor avalia manipulação de memória. |
| Integração com APIs REST, JSON, servir HTTP | **Python/Flask** | Faz isso em 150 linhas; fazer em C puro seria 10× mais código e nenhum ganho acadêmico. |
| Visualização, mapa, interatividade | **JS/HTML/CSS** | O navegador já é o ambiente natural. Leaflet renderiza heatmap sem depender de tile paga. |

A comunicação C ↔ Python é via **pipe stdin/stdout** (CSV entra, JSON sai). Isso evita:
- escrever binding Python/C (cffi, ctypes),
- escrever socket TCP só pra isso,
- ter duas implementações da mesma lógica.

## 3. Fluxo de uma requisição `/api/leituras`

1. Frontend chama `GET /api/leituras` ao abrir ou a cada 60s.
2. `app.py` carrega `setores_goiania.json` (8 setores de Goiânia).
3. Para cada setor, `criar_cliente().coletar(setor)` devolve `AmostraTrafego`.
4. As amostras são serializadas em CSV: `"id,fluxo,velocidade,area,nivel"`.
5. O CSV é enviado via `subprocess.run()` ao binário `./core/verderotas --backend`.
6. No C:
   - `estimar_ppm_co2()` calcula PPM com modelo de emissão veicular + diluição em coluna de ar.
   - `push_leitura()` filtra (`ppm ≥ 400`) e empilha.
   - Cada amostra devolve uma linha JSON por `stdout`.
7. `app.py` lê o JSON, junta com metadados do setor (nome, espécie arbórea sugerida, quantidade de árvores) e calcula `prioridade` (BAIXA/MÉDIA/ALTA/CRÍTICA).
8. Resposta JSON vai pro frontend, que:
   - Redesenha o `L.heatLayer` com os novos pontos.
   - Reconstrói a lista lateral ordenada por PPM descendente.

## 4. Decisões de modelagem (estimador CO₂)

O estimador converte tráfego em PPM usando três ideias:

1. **Fator de emissão depende da velocidade.** Veículo em marcha lenta queima combustível sem se mover → alto g/km. Curva empírica: `f(v) = 100 + 4500/v`.
2. **Emissão total = fluxo × fator × extensão_média (3,5 km)** — gramas de CO₂ por hora no setor.
3. **Diluição na coluna de ar.** O volume é `área_setor × altura_de_mistura (150 m)`. Divide a emissão total por esse volume e converte g/m³ → ppm.
4. **Soma à base atmosférica** (420 ppm — valor NOAA 2026). Isso garante que leituras válidas nunca ficam abaixo do filtro `CO2_PPM_MINIMO`.

Teto defensivo em 2000 ppm para rejeitar ruído absurdo.

## 5. Segurança e robustez

| Risco | Mitigação |
|-------|-----------|
| Stack Underflow (POP em pilha vazia) | `pop_leitura()` checa `topo == NULL` antes de desreferenciar. |
| Stack Overflow em telemetria contínua | `CO2_PILHA_MAXIMA = 10000` limita crescimento. |
| Memory Leak | `pilha_destruir()` desempilha e `free()` cada nó. Chamado no final de cada requisição. |
| Ruído da API (PPM < 400) | Filtro no `push_leitura()` antes do `malloc`. |
| Falha de malloc | `push_leitura()` retorna `false`, backend segue com os outros setores. |
| Google Maps retornando erro | Fallback silencioso para velocidade 40 km/h. |

## 6. Como testar fim-a-fim

```bash
# Terminal 1 — valida o núcleo C
cd core && make test && ./verderotas

# Terminal 2 — sobe backend
cd backend && python app.py

# Terminal 3 — chama a API direto
curl http://localhost:5000/api/leituras | python -m json.tool
```

## 7. Persistência (SQLite) e previsão

Após a Fase 6, cada ciclo de `/api/leituras` grava as leituras empilhadas pelo núcleo C em um SQLite local (`backend/verderotas.db`):

```
┌────────────┐
│ /api/leituras ──> db.inserir_leituras_batch()  ──┐
└────────────┘                                     │
                                                   ▼
┌────────────────────────────────────┐    tabela `leituras`
│ /api/historico/<id>?horas=6  ──── ─┴─>  (id, id_setor, ppm,
│                                         nivel_trafego, fluxo,
│ /api/previsao/<id>  ───> forecast.py    velocidade, ts)
└────────────────────────────────────┘    + índice (id_setor, ts DESC)
```

`forecast.py` aplica média móvel ponderada sobre os últimos 8 pontos e aplica um fator de hora (pico manhã 7-10h, pico tarde 17-20h, madrugada 22-4h). A previsão não substitui o sensor — ela é um sinal antecipatório para destacar setores que vão piorar nas próximas horas.

## 8. Pontos a evoluir (não bloqueiam a entrega)

- Substituir mock pelo Waze for Cities quando a prefeitura liberar.
- Modelo de emissão por tipo de via (anel viário, residencial, arterial).
- Trocar WMA por ARIMA quando o histórico passar de 7 dias.
- Disco persistente no Render para o `.db` sobreviver a deploys.
