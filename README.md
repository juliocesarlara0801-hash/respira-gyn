# RespiraGyn

---

## I. Cabeçalho e Identificação

| Campo | Valor |
|---|---|
| **Nome do Sistema** | RespiraGyn |
| **Nicho Temático** | Pressão / CO₂ (Grupo 13) |
| **Versão** | 1.0 |
| **Disciplina** | Algoritmos e Estrutura de Dados — Prof. Me. Valdir Prado |
| **Evento** | Inovatech 2026 — UniGoiás |
| **Seminário** | 24/11/2026 |

**Integrantes do Grupo 13:**

- Anthony Queiroz do Prado Bastos
- Henrique Rodrigues Gomes
- Julio Cesar Lara Silva
- Marcos Vinicius Gonçalves Piedade
- Murilo Ribeiro Felício

---

## II. Escopo Arquitetural

**Problema físico mapeado.** O RespiraGyn monitora a **concentração de CO₂ veicular por setor da cidade de Goiânia**. Em vez de sensor físico embarcado, o PPM de cada setor é **estimado** a partir de três variáveis vindas de APIs públicas de tráfego (Google Maps ou mock):

| Variável | Unidade | Papel no modelo |
|---|---|---|
| `fluxo_veicular` | veículos/hora | quanto mais tráfego, mais emissão total |
| `velocidade_media` | km/h | marcha lenta emite mais por quilômetro |
| `area_setor_km2` | km² | diluição da emissão na coluna de ar urbana |

O cálculo acontece no núcleo em C (`estimador.c`) a partir de constantes calibradas com literatura (CETESB 2024; NOAA PBL 2024).

**Métrica de acionamento lógico.** Quatro faixas de prioridade por PPM:

| Prioridade | Faixa PPM | Ação do sistema |
|---|---|---|
| BAIXA | < 550 | monitoramento passivo |
| MÉDIA | 550 – 800 | atenção visual no mapa |
| ALTA | 800 – 1200 | destaque na lista lateral |
| **CRÍTICA** | ≥ 1200 | **enfileira alerta** na Fila FIFO; dispara BFS no grafo para alertar vizinhos |

Além da atuação visual, o setor em CRÍTICA entra na **fila de pendências** (FIFO) e seus vizinhos geográficos são notificados por **propagação em grafo** (BFS) — reflete que CO₂ não respeita fronteiras administrativas.

---

## III. Lista de Materiais (BOM)

Projeto 100% em software. **Não há integração com hardware físico**, portanto esta seção não se aplica.

---

## IV. Engenharia de Memória (Inovação)

O núcleo em C implementa **três estruturas lineares** e **uma estrutura não linear**, cada uma com propósito distinto. Todos os motores foram blindados contra Stack Overflow (teto dimensionado) e Underflow (checagem de nulidade) e todas as alocações têm liberação explícita via `free()`.

### IV.1 Pilha LIFO — leituras recentes de CO₂

**Arquivo:** `core/pilha_co2.c`

```c
typedef struct NodoLeitura {
    float   ppm;                    // leitura de CO2
    int     id_setor;               // chave do setor na cidade
    int     nivel_trafego;          // 0 livre .. 5 crítico
    time_t  timestamp;              // momento da coleta (unix epoch)
    struct NodoLeitura *abaixo;     // ponteiro descendente — gravidade LIFO
} NodoLeitura;

typedef struct {
    NodoLeitura *topo;              // única referência que o sistema guarda
    int tamanho;                    // auditoria em O(1)
} PilhaCO2;
```

**Motores (todos O(1)):**

| Motor | Função mecânica | Complexidade |
|---|---|---|
| `push_leitura` | `malloc` + redirecionamento do topo | O(1) |
| `pop_leitura` | guarda temp, desce topo, `free` | O(1) |
| `peek_leitura` | lê topo sem destruir | O(1) |

**Blindagem:**

- **Overflow.** `CO2_PILHA_MAXIMA = 10000` — a implantação física do sensor foi dimensionada para suportar até 10 mil leituras empilhadas antes de recusar novos PUSHes. Com leituras a cada 15 s, isso cobre mais de 41 horas de telemetria contínua sem necessidade de desempilhar.
- **Underflow.** `pop_leitura` verifica `topo == NULL` antes de qualquer acesso. Retorna `false` sem gerar Segmentation Fault.
- **Filtro de 400 ppm.** `push_leitura` recusa leituras abaixo da atmosfera base **antes** do `malloc` — não gasta RAM com ruído (regra do slide "Arquitetura de Código — O Risco do CO₂").
- **Memory Leak.** `pilha_destruir` varre todos os nodos e chama `free` em cada um antes do processo terminar.

### IV.2 Fila FIFO — alertas críticos pendentes

**Arquivo:** `core/fila_alertas.c`

```c
typedef struct NodoAlerta {
    int     id_setor;
    float   ppm;
    time_t  timestamp;
    struct NodoAlerta *proximo;     // aponta para frente — oposto da pilha
} NodoAlerta;

typedef struct {
    NodoAlerta *inicio;             // saída (desenfileirar)
    NodoAlerta *fim;                // entrada (enfileirar)
    int tamanho;
} FilaAlertas;
```

**Motores (todos O(1), graças aos dois ponteiros):**

| Motor | Função mecânica | Complexidade |
|---|---|---|
| `enfileirar` | `malloc`, anexa ao `fim`, atualiza o `fim` | O(1) |
| `desenfileirar` | guarda temp, avança `inicio`, `free` | O(1) |
| `frente_fila` | lê o `inicio` sem destruir | O(1) |

**Justificativa da Fila complementar à Pilha.** A Pilha privilegia o dado **mais recente**. Para despacho de alertas isso falha: se três setores viram críticos simultaneamente, o alerta mais antigo nunca seria atendido. A Fila preserva a **ordem de chegada**, garantindo que nenhum alerta pendente seja esquecido.

**Filtro no `enfileirar`.** Só entra alerta se `ppm ≥ FILA_PPM_CRITICO (1200)` — alertas de setores em MÉDIA ou ALTA não poluem a fila operacional.

**Blindagem.** Teto `FILA_MAXIMA = 5000`; checagem de `inicio == NULL` antes de desenfileirar; limpeza completa em `fila_destruir`.

### IV.3 Lista encadeada ordenada — ranking por PPM

**Arquivo:** `core/lista_ranking.c`

```c
typedef struct NodoRanking {
    int     id_setor;
    float   ppm;
    struct NodoRanking *proximo;
} NodoRanking;

typedef struct {
    NodoRanking *cabeca;            // primeiro -> maior PPM (ordem desc)
} ListaRanking;
```

**Motores:**

| Motor | Função mecânica | Complexidade |
|---|---|---|
| `inserir_ordenado` | varre até achar posição, remove duplicata | O(n) |
| `remover_setor` | busca por id, religa ponteiros | O(n) |
| `buscar_setor` | varredura linear | O(n) |

**Justificativa.** Pilha dá acesso só ao topo; Fila dá só a ordem de chegada. Para responder "quais são os N setores mais críticos **agora**?" precisamos de **ordem total** — e é por isso que a Lista entra no sistema, mesmo pagando o preço de O(n) na inserção.

### IV.4 Grafo de adjacência — vizinhança geográfica (ESTRUTURA NÃO LINEAR)

**Arquivo:** `core/grafo_setores.c`

```c
typedef struct NodoAdj {
    int vizinho;
    struct NodoAdj *proximo;
} NodoAdj;

typedef struct {
    int       n_vertices;
    NodoAdj  *adj[GRAFO_MAX_VERTICES];   // lista de adjacência por vértice
    double    lat[GRAFO_MAX_VERTICES];
    double    lng[GRAFO_MAX_VERTICES];
} Grafo;
```

**Motores:**

| Motor | Função mecânica | Complexidade |
|---|---|---|
| `grafo_adicionar_aresta` | insere na frente das duas listas (bidirecional) | O(1) |
| `grafo_grau` | conta nodos da lista | O(grau) |
| `grafo_conectar_por_proximidade` | testa cada par via fórmula haversine | O(V²) |
| `grafo_propagar_alerta` | BFS a partir da origem, enfileira vizinhos | O(V + E) |

**Justificativa pela escolha de grafo.** CO₂ não respeita fronteira de setor. Quando o Setor Bueno está crítico, o Setor Sul vai respirar o mesmo ar em poucos minutos. O grafo representa essa **continuidade física da atmosfera urbana**. O BFS percorre a vizinhança em largura, garantindo que todo setor alcançável seja notificado uma única vez (controlado por array `visitado[]`).

**Integração com a Fila.** `grafo_propagar_alerta` **reutiliza** a Fila FIFO do IV.2: cada vizinho alcançado pelo BFS é enfileirado como alerta pendente. É a **composição de estruturas** — grafo decide *para quem notificar*, fila decide *em que ordem atender*.

### IV.5 Prova de mitigação de erros físicos

| Risco | Como mitigamos | Onde está |
|---|---|---|
| **Stack Overflow** | Tetos dimensionados: pilha ≤ 10.000; fila ≤ 5.000. Grafo tem vértices alocados estaticamente. Antes do `malloc`, checamos o tamanho corrente. | `pilha_co2.c:48`, `fila_alertas.c:42` |
| **Stack Underflow (Segfault)** | Toda operação de remoção verifica `topo == NULL` / `inicio == NULL` **antes** de dereferenciar. | `pilha_co2.c:58`, `fila_alertas.c:59` |
| **Memory Leak** | Funções `*_destruir` varrem toda a estrutura e chamam `free` em cada nodo. Nenhum caminho de retorno deixa `malloc` sem par. | `pilha_co2.c:21`, `fila_alertas.c:19`, `lista_ranking.c:19`, `grafo_setores.c:26` |
| **Filtro de ruído** | `push_leitura` recusa `ppm < 400` **antes** do `malloc`. Economia de RAM. | `pilha_co2.c:34` |
| **Ponteiro pendente** | Em `pop_leitura`, o campo `abaixo` da cópia de saída é zerado para evitar que o chamador acesse memória liberada. | `pilha_co2.c:72` |

Todos os motores primários (PUSH, POP, PEEK, ENFILEIRAR, DESENFILEIRAR, FRENTE) operam em **O(1)** — a vantagem arquitetural central da família LIFO/FIFO sobre listas genéricas.

---

## V. Topologia de Conexão

Projeto sem hardware. Seção não se aplica.

---

## VI. Manual de Deploy

### Dependências

| Pacote | Versão | Finalidade |
|---|---|---|
| `gcc` | ≥ 9.0 | compilação do núcleo em C |
| `make` | ≥ 4.0 | automação de build |
| `Python` | ≥ 3.10 | backend HTTP (adaptador do núcleo C) |
| `flask` | 3.0.0 | servidor web |
| `flask-cors` | 4.0.0 | liberação cross-origin |
| `requests` | 2.31.0 | chamadas à Google Maps API |
| `gunicorn` | 21.2.0 | servidor em produção |

**Bibliotecas padrão C utilizadas:** `<stdio.h>`, `<stdlib.h>`, `<stdbool.h>`, `<time.h>`, `<math.h>`, `<string.h>`.

### Instalação

```bash
# 1. Núcleo em C
cd core
make                     # gera ./respiragyn
make test                # roda os 24 testes unitários

# 2. Backend HTTP
cd ../backend
pip install -r requirements.txt
python app.py            # sobe em http://localhost:5000
```

### Validação dos três requisitos do Edital 03/11

Rodar `./respiragyn` sem argumentos executa a demo completa, com as **seis validações** no terminal:

| # | Validação | Demo |
|---|---|---|
| 1 | PUSH múltiplo sem erro de memória | DEMO 1 |
| 2 | POP sem Underflow (`false` retornado em pilha vazia, sem Segfault) | DEMO 2 |
| 3 | PEEK não destrutivo (tamanho antes = depois) | DEMO 3 |
| 4 | ENFILEIRAR / DESENFILEIRAR em ordem FIFO | DEMO 4 |
| 5 | Lista ordenada responde top-N e se reorganiza em atualização | DEMO 5 |
| 6 | Grafo BFS propaga alerta para vizinhos geográficos | DEMO 6 |

### Baud Rate

Projeto sem comunicação serial com microcontrolador. Não se aplica.

---

## Referências técnicas

- ZIVIANI, N. *Projeto de Algoritmos*. 3ª ed. Cengage Learning, 2011.
- SCHILDT, H. *C Completo e Total*. 3ª ed. Makron Books, 1997.
- CORMEN, T. et al. *Algoritmos: Teoria e Prática*. 3ª ed. Elsevier, 2012.
- CETESB. *Emissões Veiculares no Estado de São Paulo — Relatório 2024*.
- NOAA. *Planetary Boundary Layer — Urban Mixing Height Data*, 2024.
