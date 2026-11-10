# 06 — CHECKLIST DIA DA APRESENTAÇÃO (24/11/2026)

**Imprima este arquivo ou abra no celular UMA pessoa só (não todos). Marque cada item antes de subir pro palco.**

---

## 24h ANTES (23/11 à noite)

- [ ] Rodar `make clean && make` — verificar que compila sem warning
- [ ] Rodar `./respiragyn` — conferir que as 6 demos passam
- [ ] Rodar `make test` — conferir "=== TODOS PASSARAM ==="
- [ ] Subir backend Flask e abrir http://localhost:5000/ — mapa carrega?
- [ ] Clicar em 1 setor — modal com histórico aparece?
- [ ] Testar filtros (BAIXA/MÉDIA/ALTA/CRÍTICA) — funcionam?
- [ ] Testar countdown — chega até 0 e refaz a leitura?
- [ ] Carregar notebook do Julio a 100%
- [ ] Separar carregador do notebook, o padrão (não esquecer!)
- [ ] Combinar com os 5 membros: horário de encontro, roupa, local
- [ ] Cada um lê sua fala em `02_FALAS_POR_MEMBRO.md` DUAS vezes em voz alta
- [ ] Cada um estuda as perguntas de arguição em `04_ARGUICAO_PERGUNTAS_RESPOSTAS.md` da SUA estrutura

---

## 2h ANTES de subir ao palco

- [ ] Chegar ao local da apresentação
- [ ] Ligar o notebook e verificar bateria ≥ 80%
- [ ] Testar que o WiFi do local funciona (ou saber que não precisa)
- [ ] Abrir Windows Terminal — abrir aba WSL
- [ ] `cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2/core && make clean && make`
- [ ] Abrir SEGUNDA aba WSL — `cd ../backend && python3 app.py`
- [ ] Abrir Chrome em http://localhost:5000/ — confirmar mapa carregado
- [ ] Abrir VSCode com as 6 abas:
   1. `core/pilha_co2.c`
   2. `core/fila_alertas.c`
   3. `core/lista_ranking.c`
   4. `core/grafo_setores.c`
   5. `core/main.c`
   6. `backend/app.py`
- [ ] Posicionar a aba ativa no `pilha_co2.c` (primeira a mostrar)
- [ ] Testar alt-tab entre as 3 janelas: Terminal / VSCode / Chrome
- [ ] Testar o zoom do VSCode (Ctrl +) — fonte grande para a plateia ver
- [ ] Testar o zoom do Chrome (Ctrl +) — mapa visível de longe

---

## 5 MIN ANTES DE COMEÇAR

- [ ] Rodar `./respiragyn` NO TERMINAL — deixar a saída inteira disponível para rolar
- [ ] Posicionar scroll do terminal no TOPO da saída (onde começa DEMO 1)
- [ ] Verificar que o mapa do Chrome tem setores em cores variadas (não todos vermelhos nem todos verdes) — se estiverem monocromáticos, aguardar 15 s até próximo ciclo e verificar de novo
- [ ] Silenciar notificações do Windows (Notification Panel)
- [ ] Silenciar notificações do celular
- [ ] Respirar fundo. Está tudo pronto.

---

## DURANTE A APRESENTAÇÃO (20 min)

### Ordem fixa

| Min | Quem | Ação de tela |
|---|---|---|
| 0–4 | Anthony | **Chrome aberto** no mapa |
| 4–8 | Henrique | **Alt-tab para VSCode** → `pilha_co2.c`. Depois **alt-tab para terminal** → mostra Demo 1, 2, 3 |
| 8–12 | Julio | **Alt-tab para VSCode** → `fila_alertas.c` e `lista_ranking.c`. Rola terminal para Demo 4, 5 |
| 12–16 | Marcos | **VSCode** → `grafo_setores.c`. Rola terminal para Demo 6 |
| 16–20 | Murilo | **Alt-tab para Chrome** — demonstra o produto. Alt-tab rápido para `backend/app.py` para explicar o `subprocess.run` |

### Lembretes

- **Não ler o celular** com este roteiro. Leia a sala.
- **Falar devagar.** Nervoso acelera — prefira pausas.
- **Olhar para o professor** quando responder arguição.
- **Confiar no código.** Está correto. Está testado. Está defensável.

---

## APÓS A APRESENTAÇÃO

- [ ] Rodar `./respiragyn 2>&1 | tee ../logs_apresentacao.txt` se o professor pedir log
- [ ] Guardar o `logs_apresentacao.txt` no projeto
- [ ] Commit git com tag "seminario-24-11-2026"
- [ ] Beber água. Ir comer alguma coisa. Vocês merecem.

---

## SE ALGO QUEBRAR durante a apresentação

Veja `03_COMANDOS_DEMO_AO_VIVO.md` seção "SE ALGO QUEBRAR".

**Mantra:** a nota técnica não depende do software rodar. Depende do grupo saber EXPLICAR. Se o Flask morrer, abra o código no VSCode e continue a defesa falando.
