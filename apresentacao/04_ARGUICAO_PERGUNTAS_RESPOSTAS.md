# 04 — ARGUIÇÃO: perguntas prováveis e respostas

**Esse é o arquivo mais importante do kit.** São 5 minutos de arguição após a defesa, e o Prof. Valdir pode perguntar a QUALQUER integrante sobre QUALQUER bloco. A leitura mecânica anula a nota do critério IV.

Abaixo, 15 perguntas prováveis, divididas por tema. Cada resposta é curta (2 a 4 frases). **Treine respondê-las sem olhar.**

---

## TEMA A — Pilha LIFO

### A1. "Por que vocês escolheram pilha e não lista simples para armazenar as leituras?"
**Resposta:** Porque o sistema prioriza o dado mais RECENTE. O painel de emergência só precisa olhar o topo — o PPM da última medição. Pilha dá essa resposta em O(1); lista daria em O(n) se precisasse varrer. A rigidez da pilha é a vantagem aqui, não a limitação.

### A2. "O que acontece se dois threads chamarem `push_leitura` ao mesmo tempo?"
**Resposta:** Teria race condition clássica — dois PUSHes concorrentes sobrescrevendo o topo. A gente não implementou mutex porque o Flask chama o binário como subprocesso single-threaded, então na prática isso não ocorre. Se precisássemos de multi-thread, colocaria um `pthread_mutex_lock` antes de atribuir p->topo.

### A3. "Por que a struct tem `float` para PPM e não `int`? PPM poderia ser inteiro, não?"
**Resposta:** Porque o estimador calcula valores fracionários — 584,44 ppm por exemplo. Arredondar na origem perde precisão em cascata nas médias móveis e no ranking. O display frontend é que arredonda na hora de mostrar.

### A4. "Mostra onde o Stack Overflow é prevenido."
**Resposta:** Em `pilha_co2.c`, dentro do `push_leitura`, tem a linha `if (p->tamanho >= CO2_PILHA_MAXIMA) return false`. A constante é 10.000 nós, definida no header. Isso impede que `malloc` continue sendo chamado indefinidamente.

---

## TEMA B — Fila FIFO

### B1. "Por que dois ponteiros (inicio e fim) e não um só?"
**Resposta:** Porque fila precisa ser O(1) nas DUAS pontas — entrada e saída. Com um ponteiro só, uma das duas operações seria O(n). Dois ponteiros custa apenas 8 bytes a mais por fila, mas garante a complexidade constante.

### B2. "O que acontece quando a fila fica vazia depois de um desenfileirar?"
**Resposta:** O código verifica: `if (f->inicio == NULL) f->fim = NULL`. Sem essa linha, `fim` apontaria para memória liberada — ponteiro pendente. Essa é uma armadilha clássica de fila que a gente blindou.

### B3. "Qual a diferença mecânica entre o ponteiro `abaixo` da pilha e o `proximo` da fila?"
**Resposta:** Nome diferente, posição semântica inversa. Na pilha, `abaixo` aponta para o nó ANTERIOR (empilhado antes). Na fila, `proximo` aponta para o nó POSTERIOR (enfileirado depois). O direção que a estrutura cresce é oposta.

### B4. "Por que o filtro do enfileirar é 1200 ppm e não 1000 ou 1500?"
**Resposta:** Porque 1200 é o limite CRÍTICO definido na nossa escala de prioridade — a partir daí o setor precisa de resposta imediata. Setores em MÉDIA ou ALTA são visualmente destacados mas não disparam alerta operacional. Se baixássemos o limiar, a fila encheria de ruído.

---

## TEMA C — Lista ordenada

### C1. "Por que a lista é O(n) na inserção e a pilha é O(1)? Não dá pra fazer lista em O(1)?"
**Resposta:** Pilha insere sempre no mesmo lugar: o topo. Lista ordenada precisa achar a POSIÇÃO CORRETA para manter a ordem — isso exige varredura. Se usássemos árvore binária balanceada, cairia pra O(log n), mas pra uma lista de 30 setores a diferença prática é zero, e o código fica mais simples de defender.

### C2. "Por que remover o setor antes de reinserir, em vez de atualizar o PPM in place?"
**Resposta:** Porque o PPM novo pode mudar a POSIÇÃO do setor no ranking. Atualizar in place quebraria a invariante de ordem decrescente. Remover e reinserir garante que a lista continua ordenada mesmo depois de atualizações.

---

## TEMA D — Grafo

### D1. "Por que lista de adjacência e não matriz de adjacência?"
**Resposta:** Porque o grafo é ESPARSO — cada setor tem no máximo 4 ou 5 vizinhos, não 30. Matriz gastaria O(V²) de memória sempre; lista gasta O(V + E), que é muito menor para grafo esparso. Além disso, iterar sobre vizinhos é mais rápido na lista.

### D2. "Por que BFS e não DFS para a propagação de alerta?"
**Resposta:** Porque BFS visita por CAMADAS — primeiro os vizinhos diretos, depois os de segundo grau. É o comportamento físico correto: o setor mais próximo é notificado antes do mais distante. DFS iria até o fim de um ramo antes de voltar.

### D3. "O que é a fórmula haversine? Por que não distância euclidiana?"
**Resposta:** Haversine calcula distância entre dois pontos na superfície de uma ESFERA, usando latitude e longitude. A Terra é redonda; distância euclidiana (como se fosse plano) funciona bem só pra curtas distâncias. Pra Goiânia inteira, o erro euclidiano seria pequeno, mas haversine é a fórmula correta.

### D4. "A BFS enfileira na fila da classe FilaAlertas. Isso é bom ou ruim?"
**Resposta:** É excelente, e intencional. Mostra COMPOSIÇÃO de estruturas: o grafo decide quem alertar, a fila decide a ordem de atendimento. Reutiliza código testado (`enfileirar`) em vez de duplicar lógica dentro da função de propagação.

---

## TEMA E — Big-O e arquitetura

### E1. "Qual a complexidade do seu sistema inteiro?"
**Resposta:** Depende da operação. Push, Pop, Peek, Enfileirar e Desenfileirar são O(1). Inserção na lista ordenada e BFS são O(n) e O(V+E). Conectar grafo por proximidade é O(V²) — mas é feito uma vez na inicialização.

### E2. "Por que O(1) é tão importante para a Pilha mas O(n) é aceitável para a Lista?"
**Resposta:** Porque a pilha é chamada MUITAS VEZES por segundo (a cada leitura de setor); já a lista ordenada é reconstruída poucas vezes por minuto (só pro ranking do top-N). A complexidade escolhida reflete a frequência de uso.

### E3. "Vocês têm `malloc` em quantos lugares? E tem `free` para todos?"
**Resposta:** `malloc` aparece em 4 lugares: push_leitura (pilha), enfileirar (fila), inserir_ordenado (lista) e grafo_adicionar_aresta (grafo). Para cada um existe `free` correspondente: pop_leitura, desenfileirar, remover_setor, grafo_destruir. Além disso, funções `*_destruir` varrem toda a estrutura ao final — nenhum caminho de retorno deixa alocação pendente.

---

## TEMA F — Produto

### F1. "O CO₂ é real ou simulado?"
**Resposta:** ESTIMADO. A gente não usa sensor físico. O PPM de cada setor é CALCULADO a partir de fluxo veicular, velocidade e área, usando constantes calibradas com CETESB 2024 e NOAA. É modelo, não medição — e isso está documentado no README e no código.

### F2. "Se o CO₂ é estimado, por que o professor deveria confiar no número de árvores recomendadas?"
**Resposta:** Porque a recomendação é PROPORCIONAL — mesmo se o valor absoluto de PPM estiver impreciso, a comparação RELATIVA entre setores continua válida. O setor com mais tráfego recebe mais árvores. É uma heurística de priorização, não uma métrica certificada.

### F3. "Esse sistema funcionaria em produção real?"
**Resposta:** O NÚCLEO em C sim — é robusto, testado, com blindagem completa. Para produção real, trocaríamos o estimador por sensores MQ-135 reais e a camada de tráfego mock pela API do Google Maps paga. A arquitetura está pronta — a implementação do `GoogleMapsTrafficClient` em `traffic_client.py` já existe, só precisa da chave.

### F4. "Qual integrante fez qual parte?"
**Resposta:** O desenvolvimento foi colaborativo, com revisão de código entre todos. A divisão nominal: Anthony no estimador e documentação; Henrique na pilha; Julio na fila, lista e integração; Marcos no grafo; Murilo no backend Flask e frontend. Mas qualquer um aqui consegue explicar qualquer parte — é por isso que estamos todos defendendo.

---

## REGRAS DE OURO DA ARGUIÇÃO

1. **Nunca disser "não sei".** Prefira: "Boa pergunta, deixa eu olhar o código um segundo." Peça pro Julio abrir o arquivo. Ninguém é penalizado por consultar o código — só por recitar do papel.

2. **Se travar, PASSA.** Combine antes: "Nessa parte, quem conhece melhor é o <nome>, deixa ele complementar." Mostra equipe, não fraqueza.

3. **Se o professor te corrigir, aceite.** Nunca discuta. "Verdade professor, eu troquei os termos aqui, obrigado pela correção."

4. **Se ele pedir pra ver o código fonte, o Julio alt-tab no VSCode e abre o arquivo pedido.** NÃO abra o arquivo tentando responder — abra pra que a resposta seja APOIADA no código.

5. **Confie na estrutura.** Vocês implementaram DE VERDADE pilha, fila, lista, grafo com BFS, blindagens, free, tudo. O código está correto. Respondam com segurança.
