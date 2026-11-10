# Prompt para o Claude Code

Copie e cole o bloco abaixo no Claude Code (na raiz do projeto `verderotas-co2/`). Ele dá todo o contexto que o Claude Code precisa para continuar evoluindo o VerdeRota CO₂ sem precisar reexplicar nada.

---

```
Você está trabalhando no projeto VerdeRota CO₂, uma plataforma web do
Inovatech 2026 (UniGoiás) que mapeia focos de emissão de CO₂ veicular em
Goiânia usando APIs de tráfego (Google Maps/Waze) e recomenda arborização
direcionada. Autores: Anthony Prado, Henrique Rodrigues, Julio Cesar Lara
e Marcos Piedade. Disciplina: Algoritmos e Estrutura de Dados 1 — Prof.
Me. Valdir Prado.

CONTEXTO DO QUE JÁ EXISTE
=========================
O repositório já tem uma base funcional:

- core/           Núcleo em C (pilha LIFO adaptada do código do professor)
                  pilha_co2.{h,c}  estimador.{h,c}  main.c  Makefile
                  tests/test_pilha.c (todos os testes passam)
                  Rodar: make && ./verderotas  (demo CLI)
                         make test              (testes unitários)
                         ./verderotas --backend (pipe CSV→JSON)
- backend/        Flask que coleta tráfego, invoca o binário C via
                  subprocess pipe e serve JSON para o frontend.
                  app.py  traffic_client.py  setores_goiania.json
- frontend/       Mapa de calor em Leaflet + painel lateral com
                  recomendação de árvore por setor.
                  index.html  app.js  style.css
- docs/           ARQUITETURA.md, ROADMAP.md, EDITAL.md

A regra inviolável: a Pilha LIFO em C é o núcleo acadêmico do projeto.
Qualquer evolução DEVE preservar os 3 critérios do edital — PUSH múltiplo
sem erro, POP blindado contra Underflow, PEEK não-destrutivo — e o
filtro de 400 ppm antes do malloc. docs/EDITAL.md mapeia cada regra ao
arquivo onde ela vive.

PRIORIDADES AGORA
=================
Faça nesta ordem, uma por vez, sempre rodando os testes (make test) após
cada mudança estrutural:

1) Persistir leituras em SQLite.
   - Criar backend/db.py com schema: tabela `leituras` (id, id_setor,
     ppm, nivel_trafego, fluxo, velocidade, ts).
   - app.py deve gravar cada leitura após processar no núcleo C.
   - Nova rota GET /api/historico/<id_setor>?horas=6 devolvendo a série.

2) Dashboard de série temporal no frontend.
   - Adicionar chart.js via CDN ao index.html.
   - Ao clicar num card lateral, abrir um modal com gráfico das últimas
     6h de PPM daquele setor, puxando da nova rota.

3) Expandir setores_goiania.json.
   - Pesquisar na web (web_search) os principais corredores viários de
     Goiânia. Chegar a pelo menos 25 setores com lat/lng reais,
     área_km2 estimada e espécie arbórea nativa do Cerrado adequada.
   - Mantenha o formato JSON existente; o backend não precisa mudar.

4) Modo previsão.
   - Adicionar função em backend/forecast.py que, dado o histórico,
     prevê o PPM para +1h, +3h e +6h usando média móvel ponderada.
   - Rota GET /api/previsao/<id_setor>.
   - Frontend mostra badge "↑ pico previsto para 18h" quando aplicável.

5) Deploy.
   - Dockerfile multistage que compila o C (stage 1) e empacota com o
     Flask (stage 2).
   - render.yaml OU Procfile para Render.com ou Railway.
   - README seção "Deploy" com os passos.

PADRÕES A SEGUIR
================
- Código em C: preservar o estilo do Prof. Valdir (comentários em PT-BR
  explicando a mecânica, nomes de variáveis em português para a pilha,
  todas as operações da pilha em O(1)).
- Backend: Flask simples, sem SQLAlchemy — use sqlite3 da stdlib.
- Frontend: sem build step, sem framework. HTML + JS vanilla + Leaflet.
- Testes: cada mudança no núcleo C precisa ganhar teste em tests/.
- Commits: pequenos e descritivos ("db: cria schema de leituras",
  "api: rota /historico", etc.).

NÃO QUEBRAR
===========
- Os 3 critérios do edital (rodar ./core/verderotas sem crash).
- make test precisa continuar verde.
- /api/leituras continua respondendo no mesmo formato (frontend depende).
- Filtro de 400 ppm no push_leitura (é regra do professor).

QUANDO TERMINAR CADA ITEM
=========================
- Rode `make -C core test` e `curl http://localhost:5000/api/leituras`
  para garantir que nada quebrou.
- Atualize docs/ROADMAP.md marcando [x] o que ficou pronto.
- Comite.

Comece pelo item 1 (SQLite). Confirme comigo antes de ir pro próximo
passo se tiver qualquer decisão arquitetural não óbvia.
```

---

## Dicas de uso

- Rode o Claude Code dentro da pasta `verderotas-co2/` (ele lê o `README.md` e os docs como contexto).
- Se quiser executar um passo isolado, peça só aquele item (ex: "faça só o item 3 — expansão de setores").
- Se quiser reaproveitar o contexto em outro modelo, cole apenas o conteúdo entre as marcações ``` acima.
