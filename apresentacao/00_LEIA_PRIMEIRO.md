# 00 — LEIA PRIMEIRO

**Seminário: 24/11/2026 — Grupo 13 (Pressão/CO₂)**
**Tempo: 20 min de defesa + 5 min de arguição técnica**
**Local da apresentação: notebook do Julio Cesar Lara Silva**

---

## Antes de começar, abra estes 4 arquivos na ordem:

1. **Este arquivo** (00_LEIA_PRIMEIRO.md) — visão geral
2. **01_ROTEIRO_COMPLETO.md** — minuto a minuto, quem fala o quê
3. **02_FALAS_POR_MEMBRO.md** — a fala detalhada de cada integrante
4. **03_COMANDOS_DEMO_AO_VIVO.md** — os comandos exatos a digitar
5. **04_ARGUICAO_PERGUNTAS_RESPOSTAS.md** — FAQ do professor
6. **05_EXPLICACAO_ESTRUTURAS.md** — para quem for chamado no "code review ao vivo"

---

## Divisão de falas

| Minuto | Integrante | Tópico | Arquivo-chave |
|---|---|---|---|
| 00:00 – 04:00 | **Anthony Prado** | Problema + visão geral | slides / fala |
| 04:00 – 08:00 | **Henrique Rodrigues** | Pilha LIFO (PUSH/POP/PEEK) | `core/pilha_co2.c` |
| 08:00 – 12:00 | **Julio Cesar Lara** | Fila FIFO + Lista ordenada | `core/fila_alertas.c`, `core/lista_ranking.c` |
| 12:00 – 16:00 | **Marcos Piedade** | Grafo + BFS (não linear) | `core/grafo_setores.c` |
| 16:00 – 20:00 | **Murilo Felício** | Backend Flask + Frontend ao vivo | `backend/app.py`, navegador |

---

## Antes de sair de casa — checklist

- [ ] Notebook do Julio **100 % carregado** + carregador na mochila
- [ ] WSL Ubuntu instalado e funcionando (`wsl -d Ubuntu`)
- [ ] Projeto em `C:\Users\julio\RespiraGyn\verderotas-co2` sincronizado
- [ ] README.md atualizado com os 5 nomes (conferir)
- [ ] 5 integrantes presentes (critério IV penaliza ausência)
- [ ] Combinado quem fica dono de cada estrutura para a arguição

---

## Combinado de responsabilidade para arguição

**Regra:** o professor Valdir pode perguntar a **qualquer** integrante sobre **qualquer** bloco. A leitura mecânica anula a nota do critério IV.

Mesmo assim, cada um é o "dono" de uma estrutura e deve saber **a fundo**:

| Dono | Estrutura | Arquivo |
|---|---|---|
| Henrique | Pilha LIFO | `pilha_co2.c`, `pilha_co2.h` |
| Julio | Fila FIFO | `fila_alertas.c`, `fila_alertas.h` |
| Julio | Lista ordenada | `lista_ranking.c`, `lista_ranking.h` |
| Marcos | Grafo + BFS | `grafo_setores.c`, `grafo_setores.h` |
| Anthony | Estimador + Forecast | `estimador.c`, `forecast.c` |
| Murilo | Backend Flask + Frontend | `backend/app.py`, `frontend/app.js` |

Cada um deve ler **duas vezes** o arquivo 05 antes do dia 24/11.

---

## Hospedagem — decisão em aberto

O projeto **já tem deploy pronto para Render.com** (ver `render.yaml` e `Dockerfile`). Precisamos decidir:

| Opção | Prós | Contras |
|---|---|---|
| **Localhost (notebook do Julio)** | sem risco de internet cair; sem custo | depende de WSL + Python local; se o notebook travar, acaba a demo |
| **Render.com (nuvem)** | funciona de qualquer máquina; URL pública | se o wifi da sala falhar, apaga a demo; precisa subir antes |

**Recomendação do roteiro:** usar **localhost** no notebook do Julio como fonte primária, e **deixar o Render como backup** (subir o deploy antes do dia 24 e ter a URL salva). Se o localhost falhar durante a apresentação, alterna para o celular com o 4G abrindo o Render.

Veja `03_COMANDOS_DEMO_AO_VIVO.md` seção "Backup em nuvem" para as instruções do Render.

---

## O que o júri vai avaliar (critério IV do briefing)

> "Postura profissional e clareza. Qualquer membro do esquadrão poderá ser convocado ao vivo para explicar blocos específicos do código submetido. **A leitura mecanizada de material anula a pontuação deste critério.**"

Portanto: **não leiam este roteiro no celular durante a fala**. Estude antes, pratique em casa pelo menos **duas vezes** em grupo. Use este roteiro só como lembrança da ORDEM dos tópicos, não como script literal.
