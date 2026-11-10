# 01 — ROTEIRO COMPLETO DA APRESENTAÇÃO

**Total: 20 min de defesa expositiva + 5 min de arguição técnica**

Cada bloco de ~4 min tem:
- O que falar (resumo)
- Qual arquivo abrir no notebook
- Qual comando rodar

---

## BLOCO 1 — Anthony Prado (00:00 – 04:00)

### ABERTURA + PROBLEMA

**Tela a mostrar:** `frontend` aberto no navegador em http://localhost:5000/
(mapa de Goiânia com setores coloridos)

**Fala resumida:**
1. Cumprimentar banca. Apresentar os 5 integrantes do Grupo 13.
2. Anunciar o sistema: **RespiraGyn** — monitor de CO₂ veicular de Goiânia.
3. Problema real: trânsito denso em Goiânia concentra CO₂ em setores específicos. Prefeitura planta árvores sem direcionamento.
4. Nossa proposta: **estimar CO₂ por setor usando dados de tráfego** (Google Maps / Waze) e **recomendar árvores** onde a pressão é maior.
5. Diferencial para o Prof. Valdir: **toda a lógica de decisão está em C** — o Python só serializa HTTP.

**Mostrar na tela:**
- O mapa com os círculos coloridos (vinho → verde)
- Clicar em 1 setor → abrir o modal com histórico
- Apontar: "Essa cor sai da pilha LIFO em C que meu colega vai explicar"

**Encerrar com:** "Passo pro Henrique, que vai mostrar a estrutura central: a Pilha LIFO."

---

## BLOCO 2 — Henrique Rodrigues (04:00 – 08:00)

### PILHA LIFO (estrutura central)

**Tela a mostrar:** VSCode / editor com `core/pilha_co2.c` aberto.
Caminho: `C:\Users\julio\RespiraGyn\verderotas-co2\core\pilha_co2.c`

**Fala resumida:**
1. "Pilha é LIFO — Last In, First Out. O último empilhado é o primeiro a sair."
2. Mostrar a struct `NodoLeitura` (ppm, id_setor, nivel, timestamp, abaixo).
3. "O ponteiro se chama `abaixo` porque a gravidade da pilha aponta para baixo — o nó do topo é a única referência que o sistema guarda."
4. Explicar os três motores: `push_leitura` (O(1)), `pop_leitura` (O(1)), `peek_leitura` (O(1)).
5. Explicar as três blindagens:
   - **Filtro atmosférico**: `if (ppm < 400) return false;` ANTES do malloc
   - **Overflow**: teto `CO2_PILHA_MAXIMA = 10000` (dimensiona o armazém)
   - **Underflow**: `if (p->topo == NULL) return false;` antes de qualquer dereferência
6. "O POP ainda chama `free()` no nó removido — sem memory leak."

**Comando a rodar no terminal (WSL):**
```bash
cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2/core
./respiragyn
```

**O que a saída vai mostrar:**
- DEMO 1 → 4 PUSHes aceitos + 1 PUSH com 350 ppm **DESCARTADO pelo filtro**
- DEMO 2 → POPs consecutivos, e um POP em pilha vazia retornando `false` **sem Segfault**
- DEMO 3 → PEEK lendo o topo e comprovando que o tamanho não mudou

**Encerrar com:** "Pilha só prioriza o mais recente. Para a lista de alertas pendentes a gente precisou de outra estrutura — passo pro Julio."

---

## BLOCO 3 — Julio Cesar Lara (08:00 – 12:00)

### FILA FIFO + LISTA ORDENADA

**Tela a mostrar:** editor com dois arquivos abertos lado a lado:
- `core/fila_alertas.c`
- `core/lista_ranking.c`

Caminhos:
- `C:\Users\julio\RespiraGyn\verderotas-co2\core\fila_alertas.c`
- `C:\Users\julio\RespiraGyn\verderotas-co2\core\lista_ranking.c`

**Fala resumida — Fila:**
1. "Pilha funciona para o dado mais novo. Mas quando três setores viram críticos juntos, o alerta antigo nunca seria atendido se usássemos pilha."
2. Mostrar a struct `FilaAlertas` — **dois ponteiros**, `inicio` e `fim`. "Isso garante que enfileirar e desenfileirar sejam O(1) nas duas pontas."
3. "O ponteiro agora se chama `proximo` em vez de `abaixo` — aponta para a FRENTE da fila, oposto da pilha."
4. Filtro no `enfileirar`: só entra se `ppm >= 1200` (CRÍTICO). Alerta médio não polui a fila operacional.
5. Blindagem Underflow: `if (inicio == NULL) return false;` antes do desenfileirar.

**Fala resumida — Lista:**
1. "Pilha dá topo. Fila dá a ordem de chegada. Mas para responder 'quais os 5 setores mais críticos AGORA?' precisamos de ordem total."
2. Mostrar `inserir_ordenado` em `lista_ranking.c`: varre até achar a posição correta. "É O(n), pior que pilha e fila — e isso é o preço da ordem total."
3. "Se o mesmo setor já existe na lista, a gente remove primeiro e reinsere — evita duplicata. Isso é importante para o ranking em tempo real."

**Comando a rodar (continuação da mesma execução):**
As DEMOS 4 e 5 já apareceram depois do Henrique. Role o terminal para elas:
- DEMO 4 → 3 alertas enfileirados, desenfileirados na MESMA ordem (FIFO)
- DEMO 5 → top 5 setores; depois atualizo o setor 1 de 980 → 1950 e ele pula pra #1

**Encerrar com:** "Pilha, Fila e Lista são LINEARES. Mas o CO₂ não respeita fronteira de setor — para isso precisamos de uma estrutura NÃO linear. Passo pro Marcos."

---

## BLOCO 4 — Marcos Piedade (12:00 – 16:00)

### GRAFO + BFS (estrutura NÃO LINEAR)

**Tela a mostrar:** editor com `core/grafo_setores.c` aberto.
Caminho: `C:\Users\julio\RespiraGyn\verderotas-co2\core\grafo_setores.c`

**Fala resumida:**
1. "O critério I do seminário exige estrutura **não linear**: árvore ou grafo. Escolhemos **grafo** — e tem motivo técnico."
2. "CO₂ não respeita fronteira administrativa. Se o Setor Bueno está crítico, o Setor Sul vai respirar o mesmo ar em minutos. O grafo representa essa continuidade física."
3. Mostrar a struct `Grafo`: **lista de adjacência** — cada vértice tem uma cadeia de vizinhos.
4. Explicar `grafo_conectar_por_proximidade`: percorre pares de setores, calcula distância com **haversine**, conecta os que estão a ≤ 5 km. Complexidade **O(V²)**.
5. Mostrar `grafo_propagar_alerta`: faz **BFS** (busca em largura) a partir do setor crítico, marca visitados, enfileira os vizinhos alcançados na Fila FIFO do Julio. Complexidade **O(V + E)**.
6. **Ponto de ouro:** "Isso é COMPOSIÇÃO de estruturas. O grafo decide **quem** notificar, a fila decide **em que ordem** atender."

**Comando a rodar (continuação):**
DEMO 6 já aparece no final do `./respiragyn`:
- 6 setores reais de Goiânia conectados por proximidade (9 arestas)
- Setor 0 (Bueno) marcado como CRÍTICO (1850 ppm)
- BFS notifica **5 vizinhos**, todos enfileirados

**Encerrar com:** "Tudo isso que mostrei até agora é o núcleo em C. Quem transforma isso num site de verdade é o Murilo."

---

## BLOCO 5 — Murilo Felício (16:00 – 20:00)

### BACKEND FLASK + FRONTEND (produto final)

**Tela a mostrar:** navegador em http://localhost:5000/
Se não estiver aberto, abra antes.

**Fala resumida:**
1. "A arquitetura do Julio é um binário em C que recebe CSV por stdin e devolve JSON por stdout. Para transformar isso num site, usamos **Flask em Python como adaptador fino**."
2. Abrir num segundo terminal (ou alt-tab) o arquivo `backend/app.py`. Mostrar `processar_no_nucleo_c()` que chama `subprocess.run([BIN_C, "--backend"])`. "O Python não calcula CO₂ — só traduz HTTP para pipe."
3. **Mostrar o site rodando ao vivo:**
   - Mapa de Goiânia com **30 setores reais** coloridos pelo degradê contínuo (vinho → verde)
   - Filtros de cor (BAIXA/MÉDIA/ALTA/CRÍTICA) — clicar em CRÍTICA, mostrar que só os piores ficam
   - **Tempo real**: countdown no header mostra "atualiza em X s" — a cada 15 s re-chama a API
   - Clicar em 1 setor → modal com histórico de 6 h e previsão +1/+3/+6 h
4. **Fechar com impacto:**
   - "Ao plantar árvores direcionadas aos setores vermelhos, cada árvore jovem absorve cerca de 20 kg de CO₂ por ano. Nosso sistema sugere **centenas de árvores** nos pontos certos."
   - "Dúvidas?"

**Comandos úteis (se precisar reiniciar algo ao vivo):**
```bash
# subir Flask se cair
wsl -d Ubuntu -- bash -c 'cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2/backend && python3 app.py'

# abrir o navegador
start http://localhost:5000/
```

---

## ARGUIÇÃO (5 min) — reação a perguntas

Veja **04_ARGUICAO_PERGUNTAS_RESPOSTAS.md** para as 12 perguntas mais prováveis e suas respostas curtas.

Regras combinadas durante a arguição:
- Se a pergunta cair na estrutura "dono" de alguém, ele responde PRIMEIRO.
- Se a pessoa travar, **outro membro completa** — isso mostra equipe.
- **Nunca dizer "não sei"**. Prefira "boa pergunta, deixa eu pensar um segundo" e consultar o colega.
- Se o professor pedir pra ver o código, **o Julio abre no VSCode**, quem é dono explica.

---

## Diagrama mental da defesa

```
Anthony           Henrique            Julio              Marcos           Murilo
(problema)   →    (Pilha LIFO)   →   (Fila + Lista) →   (Grafo+BFS) →   (Site rodando)
 visão geral      estrutura CORE     complemento        não linear       produto final
     4 min           4 min              4 min              4 min            4 min

                           ↓   ↓   ↓   ↓   ↓
                          ARGUIÇÃO TÉCNICA (5 min)
                     professor pergunta a QUALQUER UM
```
