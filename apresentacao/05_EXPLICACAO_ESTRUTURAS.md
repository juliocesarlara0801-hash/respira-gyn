# 05 — EXPLICAÇÃO TÉCNICA DE CADA ESTRUTURA

**Este arquivo é referência interna para o estudo em casa, e consulta rápida se o professor pedir "explica essa função aqui".** Cada estrutura tem: motivação, struct comentada campo a campo, big-O das operações, e os pontos de armadilha (onde o código quase dá erro, e por que não dá).

---

## 1. PILHA LIFO — `pilha_co2.c` / `pilha_co2.h`

### Motivação
Precisamos de uma estrutura que priorize o dado **mais recente** com acesso O(1). Qualquer leitura de CO₂ nova sobrescreve o foco — o painel só se importa com o PPM atual do setor, não com o histórico profundo. Pilha é a resposta canônica.

### Struct completa

```c
typedef struct NodoLeitura {
    float   ppm;                  // PPM calculado pelo estimador em C
    int     id_setor;             // ID do setor cadastrado (1-30)
    int     nivel_trafego;        // 0 livre .. 5 congestionado
    time_t  timestamp;            // UNIX epoch do momento da coleta
    struct NodoLeitura *abaixo;   // ponteiro ao nó anterior na pilha
} NodoLeitura;

typedef struct {
    NodoLeitura *topo;            // única referência que o sistema guarda
    int tamanho;                  // para auditar em O(1)
} PilhaCO2;
```

### Por que chamar o ponteiro de `abaixo` e não `proximo`?
Convenção vinda do Prof. Valdir (slide "A Planta de Execução Física"). Reflete a metáfora de gravidade: o nó do topo está "em cima", apontando "para baixo" ao nó anterior. Isso diferencia visualmente pilha de fila nos arquivos — fila usa `proximo` porque cresce horizontalmente.

### Operações e Big-O

| Função | Complexidade | O que faz |
|---|---|---|
| `pilha_criar` | O(1) | zera `topo` e `tamanho` |
| `push_leitura` | O(1) | malloc + ajustar o topo |
| `pop_leitura` | O(1) | guardar temp, mover topo, free |
| `peek_leitura` | O(1) | copiar topo para `saida`, sem tocar |
| `pilha_tamanho` | O(1) | retorna o campo `tamanho` |
| `pilha_vazia` | O(1) | compara `topo == NULL` |
| `pilha_destruir` | O(n) | varre tudo, free em cada nó |

### Três blindagens (onde o código QUASE dá erro)

**Blindagem 1 — filtro atmosférico (push_leitura, linha ~34)**
```c
if (ppm < CO2_PPM_MINIMO) return false;
```
Sem esse `if`, aceitaria qualquer lixo da API. Com ele, protegemos a RAM — não gasta malloc em leitura inválida. Essa regra foi exigida explicitamente pelo Prof. no slide "Arquitetura de Código — O Risco do CO₂".

**Blindagem 2 — teto Overflow (push_leitura, linha ~35)**
```c
if (p->tamanho >= CO2_PILHA_MAXIMA) return false;
```
10.000 nós. Com leitura a cada 15 s, cobre 41 h de telemetria contínua. Além desse teto, novos pushes são recusados — evita Stack Overflow físico da RAM do processo.

**Blindagem 3 — Underflow (pop_leitura, linha ~58)**
```c
if (p == NULL || p->topo == NULL) return false;
```
Sem esse `if`, a linha seguinte faria `temp = p->topo` com `topo = NULL`, e depois `p->topo = temp->abaixo` tentaria dereferenciar NULL — Segmentation Fault. O Prof. chama isso de "erro número um de reprovação".

### Ponto sutil que vale saber

No `pop_leitura`, antes do `free(temp)`, copiamos os dados para `saida` E zeramos `saida->abaixo = NULL`. Por quê? Porque se deixássemos o campo `abaixo` apontando pro próximo nó da pilha, o chamador poderia acidentalmente dereferenciar — mas aquele nó continua existindo, não é use-after-free. Zeramos por **higiene**: o "nó de saída" é uma cópia desconectada, e deixar o ponteiro semi-válido é confuso. Mostra cuidado.

---

## 2. FILA FIFO — `fila_alertas.c` / `fila_alertas.h`

### Motivação
Pilha prioriza o mais recente — mas isso falha para despacho de alertas. Imagine 3 setores críticos simultâneos: o primeiro que chegou seria o ÚLTIMO a ser atendido se usássemos pilha. Fila preserva a **ordem de chegada** — primeiro a chegar, primeiro a sair.

### Struct completa

```c
typedef struct NodoAlerta {
    int     id_setor;
    float   ppm;
    time_t  timestamp;
    struct NodoAlerta *proximo;   // aponta à FRENTE — oposto de `abaixo`
} NodoAlerta;

typedef struct {
    NodoAlerta *inicio;           // saída (quem sai no desenfileirar)
    NodoAlerta *fim;              // entrada (onde entra no enfileirar)
    int tamanho;
} FilaAlertas;
```

### Por que DOIS ponteiros (inicio e fim)?
Para fazer a fila O(1) nas duas pontas. Se só tivesse `inicio`, o `enfileirar` precisaria varrer até achar o fim — O(n). Dois ponteiros custam 8 bytes extras por fila, mas garantem a complexidade constante — vantagem arquitetural.

### Operações e Big-O

| Função | Complexidade | O que faz |
|---|---|---|
| `fila_criar` | O(1) | zera ambos ponteiros |
| `enfileirar` | O(1) | malloc, anexa ao `fim`, atualiza `fim` |
| `desenfileirar` | O(1) | guarda temp, avança `inicio`, free |
| `frente_fila` | O(1) | copia o `inicio` sem destruir |
| `fila_destruir` | O(n) | varre e libera tudo |

### Blindagens

**Filtro de criticidade**
```c
if (ppm < FILA_PPM_CRITICO) return false;
```
A fila só aceita alertas CRÍTICOS (≥ 1200 ppm). Setores em ALTA ou MÉDIA aparecem no mapa mas não geram pendência operacional. Isso mantém a fila enxuta.

**Underflow**
```c
if (f->inicio == NULL) return false;
```
Mesma lógica da pilha — sem esse if, Segfault.

**Armadilha clássica da fila — quando esvazia**
```c
f->inicio = temp->proximo;
if (f->inicio == NULL) f->fim = NULL;
```
Se não zerasse `fim` quando a fila esvazia, o próximo `enfileirar` tentaria fazer `f->fim->proximo = novo` com `fim` apontando para memória LIBERADA. Ponteiro pendente. A gente blindou.

---

## 3. LISTA ENCADEADA ORDENADA — `lista_ranking.c` / `lista_ranking.h`

### Motivação
Pilha dá topo. Fila dá ordem de chegada. **Lista dá ordem total** — o setor mais crítico primeiro, o segundo mais crítico depois, assim por diante. Serve para responder "quais os N piores?".

### Struct completa

```c
typedef struct NodoRanking {
    int     id_setor;
    float   ppm;
    struct NodoRanking *proximo;  // aponta ao próximo na ordem decrescente
} NodoRanking;

typedef struct {
    NodoRanking *cabeca;          // PRIMEIRO -> maior PPM
    int tamanho;
} ListaRanking;
```

### Operações e Big-O

| Função | Complexidade | Nota |
|---|---|---|
| `inserir_ordenado` | O(n) | varre até achar posição |
| `remover_setor` | O(n) | busca por ID, religa |
| `buscar_setor` | O(n) | varredura linear |
| `lista_destruir` | O(n) | libera todos |

### Por que O(n)? Por que não usar árvore?

**Por que lista O(n):** porque a lista só é reconstruída algumas vezes por minuto (quando o backend chama a rota `/api/leituras`). O custo n=30 é 30 operações. Para 30 setores, uma árvore balanceada daria O(log 30) ≈ 5 operações — a diferença prática é nula, e lista simples é mais fácil de defender na arguição.

**Quando usar árvore?** Se fosse 10.000 setores, aí sim uma AVL ou rubro-negra compensaria.

### Detalhe de projeto — evitar duplicata

```c
remover_setor(l, id_setor);  // chamada antes do malloc
```

Se o setor já existe na lista com PPM antigo, a gente remove ANTES de inserir o novo. Sem isso, o mesmo setor apareceria duas vezes no top-N — quebraria a utilidade do ranking. Esse padrão (remove-then-insert) é comum em estruturas ordenadas quando o valor-chave pode mudar.

---

## 4. GRAFO + BFS — `grafo_setores.c` / `grafo_setores.h`

### Motivação
CO₂ não respeita fronteira de setor. A dispersão é **contínua no espaço**. Modelar a cidade como uma coleção de setores INDEPENDENTES (que é o que pilha, fila e lista fazem) ignora isso. O grafo representa explicitamente a **vizinhança geográfica**: setor A está conectado a setor B se eles estão fisicamente próximos.

### Struct completa

```c
typedef struct NodoAdj {
    int vizinho;
    struct NodoAdj *proximo;
} NodoAdj;

typedef struct {
    int       n_vertices;
    NodoAdj  *adj[GRAFO_MAX_VERTICES];   // array de listas de adjacência
    double    lat[GRAFO_MAX_VERTICES];   // latitude de cada setor
    double    lng[GRAFO_MAX_VERTICES];   // longitude de cada setor
} Grafo;
```

### Por que lista de adjacência e não matriz?

**Lista de adjacência** gasta O(V + E) de memória — um nó por aresta.
**Matriz de adjacência** gasta O(V²) sempre — V² bytes.

Para 30 setores com média de 4 vizinhos cada:
- Matriz: 30 × 30 = 900 bytes (cheios de zero na maior parte)
- Lista: 30 + 4×30 = 150 bytes

Como o grafo é **esparso** (poucos vizinhos por vértice), lista é mais eficiente.

### Operações e Big-O

| Função | Complexidade | Descrição |
|---|---|---|
| `grafo_criar` | O(V) | zera arrays |
| `grafo_registrar_setor` | O(1) | guarda lat/lng |
| `grafo_adicionar_aresta` | O(1) | insere na frente de duas listas (bidirecional) |
| `grafo_grau` | O(grau) | conta nós da lista |
| `grafo_conectar_por_proximidade` | O(V²) | testa cada par com haversine |
| `grafo_propagar_alerta` (BFS) | O(V + E) | visita cada vértice uma vez |
| `grafo_destruir` | O(V + E) | libera todas as listas |

### Fórmula de haversine (função `distancia_km`)

Calcula distância em km entre dois pontos lat/lng na superfície da Terra (esfera de 6371 km). A fórmula é:

```
a = sin²(Δlat/2) + cos(lat1)·cos(lat2)·sin²(Δlng/2)
c = 2 · atan2(√a, √(1−a))
d = R · c   (onde R = 6371 km)
```

Por que não distância euclidiana? Porque a Terra é redonda. Para a escala de Goiânia (centenas de km²) o erro euclidiano seria pequeno, mas haversine é a fórmula correta e é só 5 linhas de C.

### BFS passo a passo (função `grafo_propagar_alerta`)

```c
bool visitado[GRAFO_MAX_VERTICES] = {false};   // marca quem já visitamos
int  bfs_fila[GRAFO_MAX_VERTICES];             // fila local de BFS
int  ini = 0, fim = 0;

visitado[origem] = true;
bfs_fila[fim++] = origem;

while (ini < fim) {
    int v = bfs_fila[ini++];                   // tira da frente
    // para cada vizinho w de v:
    if (!visitado[w]) {
        visitado[w] = true;
        bfs_fila[fim++] = w;                   // adiciona ao fim
        enfileirar(destino, w, ppm_origem, ts); // PROPAGA para fila externa
    }
}
```

**Pontos sutis:**
- `visitado[]` evita que a BFS entre em loop (importante em grafos com ciclos).
- A BFS usa uma FILA LOCAL para a travessia interna. A FILA EXTERNA (`FilaAlertas *destino`) recebe os alertas propagados — é a **composição** de estruturas.
- A origem é marcada como visitada mas **não** é enfileirada no destino — o próprio setor crítico já tem seu alerta na pilha, não precisa duplicar.

---

## 5. COMO AS 4 ESTRUTURAS SE INTEGRAM

```
  Leitura da API
        ↓
     push_leitura          [1] PILHA — histórico recente do setor
        ↓
   estimar_ppm_co2
        ↓
    ppm >= 1200 ?
        ↓
   enfileirar()             [2] FILA — pendências a atender
        ↓
   propagar_alerta()        [3] GRAFO — vizinhos geográficos também avisados
        ↓
   inserir_ordenado()       [4] LISTA — ranking atualizado
        ↓
   resposta HTTP ao Flask
```

Cada estrutura tem uma pergunta diferente a responder:

- **Pilha:** "qual o ÚLTIMO estado do setor X?"
- **Fila:** "qual o PRÓXIMO alerta a atender?"
- **Lista:** "quais os TOP N piores?"
- **Grafo:** "QUEM MAIS devo notificar quando X virar crítico?"

Nenhuma das quatro sozinha resolve tudo. A composição delas é que forma o núcleo do RespiraGyn.

---

## 6. TESTES UNITÁRIOS (`core/tests/test_pilha.c`)

Atualmente cobrem:
- Pilha LIFO (push, pop, peek, underflow, overflow, destroy)
- Estimador (PPM plausível, teto, crescimento com tráfego)
- Recomendação de árvores
- WMA e fator_hora do forecast
- `prever_serie` com 3 horizontes

**Gap conhecido:** não há testes unitários dos 3 módulos novos (fila, lista, grafo). As 6 demos em `main.c` servem como **testes de integração** — mas se o Prof. Valdir pedir cobertura unitária, isso é 1-2 horas de trabalho adicional. Avisar o grupo antes do dia.
