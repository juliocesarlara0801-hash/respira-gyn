# 02 — FALAS POR MEMBRO

**Observação importante:** estas falas são GUIA, não script literal. Leia-as duas vezes em casa, entenda a lógica, e fale com suas próprias palavras no palco. O Prof. Valdir penaliza quem lê mecanicamente.

Cada fala tem ~500 a 700 palavras, equivalente a ~4 minutos falando pausado.

---

## FALA 1 — ANTHONY PRADO (00:00 – 04:00)

> **Boa noite, professor. Nós somos o Grupo 13 — tema Pressão e CO₂. Comigo estão o Henrique Rodrigues, o Julio Cesar Lara, o Marcos Piedade e o Murilo Felício. Eu vou abrir a defesa apresentando o problema que a gente escolheu enfrentar, e nos próximos quatro blocos cada colega vai mostrar uma parte diferente da arquitetura que construímos.**
>
> **O sistema que a gente construiu se chama RespiraGyn. O nome vem de "respirar" com "Gyn", a sigla informal de Goiânia. A pergunta de partida foi: por que as prefeituras gastam tanto com arborização urbana e mesmo assim os setores mais carregados de trânsito continuam com ar ruim?**
>
> **A resposta está na direção do plantio. Hoje, a arborização é feita por bairro inteiro, de forma uniforme, sem olhar onde o CO₂ realmente está sendo produzido. O RespiraGyn inverte essa lógica: ele mapeia os focos reais de emissão veicular de Goiânia em quase tempo real, e para cada foco ele recomenda quantas árvores plantar.**
>
> **E aqui tem uma observação técnica importante para o edital do professor. A gente NÃO tem sensor físico. O grupo CO₂ que vai usar sensor MQ-135 é outro. Nós estimamos o CO₂ a partir de três variáveis públicas: o fluxo de veículos no setor, a velocidade média, e a área geográfica dele. Essas três entram num cálculo em C que devolve PPM estimado.**
>
> **Por que isso é relevante para a avaliação? Porque o nosso núcleo está INTEGRALMENTE em C. O Python do backend serve só como adaptador HTTP — ele não calcula CO₂, não decide árvore, não faz previsão. Tudo o que é lógica de decisão roda num binário compilado com gcc, exatamente como o professor pediu.**
>
> **Peço que olhem a tela. Esse é o produto final, rodando aqui no notebook. Vocês veem o mapa de Goiânia com os 30 setores que cadastramos. Cada bolinha de cor é um setor — e a cor não é categórica, é CONTÍNUA. Vai do vinho bordô (crítico extremo) até o caqui claro (ar limpo), passando por laranja, amarelo e verde. O que determina essa cor é o PPM calculado por um núcleo em C que o Henrique vai explicar agora.**
>
> **Só mais uma coisa antes de passar: esse sistema cumpre o critério um do seminário — a gente implementou TODAS as estruturas pedidas. Pilha, Fila, Lista e um Grafo não linear com busca em largura. O Henrique começa pela central, que é a Pilha. Henrique, pode seguir.**

**Checklist do que deve estar na tela antes de começar:**
- Navegador aberto em http://localhost:5000/
- Mapa carregado (se não estiver, aperta F5 e espera 2 segundos)
- Filtro na posição "todas"

---

## FALA 2 — HENRIQUE RODRIGUES (04:00 – 08:00)

> **Obrigado Anthony. Eu vou mostrar a estrutura central do sistema, que é a Pilha LIFO. A sigla significa Last In, First Out — o último dado que entra é o primeiro que sai.**
>
> **Vou abrir o arquivo que define isso. [Julio abre pilha_co2.c no editor.] Olhem a struct NodoLeitura. Ela tem cinco campos: o PPM que é a leitura, o ID do setor na cidade, o nível de tráfego de 0 a 5, o timestamp UNIX, e esse ponteiro aqui chamado `abaixo`. O nome é proposital. Numa pilha, a gravidade aponta para baixo. O topo é a única referência que o sistema guarda — todos os outros nós ficam suspensos pelo nó acima deles.**
>
> **A pilha tem três motores: PUSH, POP e PEEK. Todos operam em tempo constante, O(1), independente do tamanho da pilha. Isso é a vantagem arquitetural central da estrutura LIFO e foi um ponto que o professor fez questão de bater na aula.**
>
> **PUSH — empilha. Ele faz um `malloc` de um novo nó, aponta o `abaixo` dele para o topo antigo, e atualiza o topo global. É O(1) porque não varre a pilha.**
>
> **POP — desempilha. Guarda o endereço do topo num ponteiro temporário, move o topo para o nó abaixo, e chama `free` para devolver a RAM. Isso é importante: o POP não só remove o acesso, ele destrói a caixa — sem isso teria memory leak.**
>
> **PEEK — audita. Lê o valor do topo sem remover. Serve para o painel mostrar o PPM atual na tela sem alterar o estado.**
>
> **Agora, as três blindagens. O professor Valdir disse na aula que a causa número um de reprovação é código em C sem verificação de nulidade. A gente tem três proteções:**
>
> **Primeira: o filtro atmosférico. Antes do malloc, a função push_leitura verifica se o PPM é menor que 400. Se for, ela retorna false sem alocar nada. Por quê? Porque 420 ppm é a atmosfera base do planeta — qualquer leitura abaixo disso é ruído da API. A gente não gasta RAM com lixo.**
>
> **Segunda: o teto contra Stack Overflow. A pilha tem um limite de 10.000 nós. Com leitura a cada 15 segundos isso cobre 41 horas de telemetria contínua antes de precisar desempilhar. Essa é a chamada "regra do engenheiro" — dimensionar o armazém antes de encher.**
>
> **Terceira: a blindagem contra Underflow. O POP começa com `if (p->topo == NULL) return false`. Sem esse if, a tentativa de ler o nó abaixo de um ponteiro nulo geraria Segmentation Fault e o sistema morreria.**
>
> **Vou mostrar isso rodando. [Abre terminal WSL e digita `cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2/core && ./respiragyn`.]**
>
> **Olhem a tela. Na Demo 1, empilhei quatro leituras válidas — todas acima de 400 ppm. Depois tentei empilhar 350 ppm diretamente — ela foi DESCARTADA pelo filtro, como esperado. Na Demo 2, fiz POP até esvaziar, e depois mais um POP que retornou false SEM Segmentation Fault. Na Demo 3, usei PEEK e o tamanho da pilha permaneceu o mesmo antes e depois. Os três requisitos do edital do 03 de novembro estão cumpridos aqui.**
>
> **Mas a pilha só prioriza o dado mais recente. Isso não serve para lista de alertas pendentes. O Julio vai mostrar as estruturas complementares.**

**Checklist:**
- Terminal WSL já aberto na pasta `/mnt/c/Users/julio/RespiraGyn/verderotas-co2/core`
- Comando `./respiragyn` executado (ou prepara pra executar)
- Scroll do terminal começa no topo da saída

---

## FALA 3 — JULIO CESAR LARA (08:00 – 12:00)

> **Obrigado Henrique. Eu vou mostrar duas estruturas que complementam a pilha: a Fila FIFO e a Lista encadeada ordenada.**
>
> **Começo com a Fila. [Abre fila_alertas.c no editor.] A pergunta que motivou criar a fila é essa: imagina que três setores viram críticos ao mesmo tempo. Se a gente usasse só pilha, o alerta mais antigo nunca seria atendido — porque a pilha sempre prioriza o topo. A fila resolve isso: ela preserva a ORDEM DE CHEGADA.**
>
> **Olhem a struct FilaAlertas. Ela tem DOIS ponteiros — `inicio` e `fim`. Isso é essencial. Com um ponteiro só, o enfileirar ou o desenfileirar seria O(n). Com dois, as duas operações viram O(1), porque a gente sempre sabe exatamente onde é a entrada e onde é a saída.**
>
> **Repare também que o ponteiro do nodo agora se chama `proximo`, não `abaixo` como na pilha. É o oposto geométrico. A fila cresce PARA A FRENTE, não para cima.**
>
> **Enfileirar tem um filtro: o alerta só entra na fila se o PPM for maior ou igual a 1200, que é o limite CRÍTICO. Alerta de prioridade média não polui a fila operacional. E também tem blindagem contra Overflow: teto de 5.000 alertas.**
>
> **Desenfileirar tem a blindagem contra Underflow: `if (inicio == NULL) return false` antes de qualquer acesso. Mesma lógica da pilha do Henrique.**
>
> **Agora a Lista. [Abre lista_ranking.c.] A pergunta: "quais são os 5 setores mais críticos AGORA?" Pilha não responde — ela só dá o topo. Fila não responde — ela só dá a ordem de chegada. Para isso preciso de ORDEM TOTAL — todos os setores ordenados pelo PPM descendente. É a função da lista.**
>
> **Olhem a função `inserir_ordenado`. Ela varre a lista até achar a posição correta e insere ali. Isso é O(n), pior que pilha e fila. E isso é um ponto importante da defesa: nem toda estrutura precisa ser O(1). A gente escolhe a complexidade de acordo com a pergunta que precisa responder. Ordem total paga O(n), e esse é o preço justo.**
>
> **Um detalhe: se o mesmo setor já existe na lista, o `inserir_ordenado` REMOVE a entrada antiga e reinsere com o PPM novo. Isso evita duplicata e mantém o ranking consistente em tempo real.**
>
> **Vamos ver rodando. [Rola o terminal para as Demos 4 e 5.] Na Demo 4, enfileirei 3 alertas críticos — setores 2, 4 e 7. Depois desenfileirei. Saíram na mesma ordem: 2 primeiro, depois 4, depois 7. FIFO puro. E uma tentativa de desenfileirar em fila vazia retornou false, sem crash.**
>
> **Na Demo 5, inseri 7 setores com PPMs diferentes. O top 5 saiu em ordem desc: setor 4 com 1850, setor 7 com 1620, e assim por diante. Depois atualizei o setor 1 de 980 para 1950 ppm — e ele subiu para o primeiro lugar automaticamente. A lista reordena sozinha.**
>
> **Mas tudo o que mostrei até agora é LINEAR. CO₂ não é linear — ele se espalha em três dimensões. Para modelar isso, o Marcos vai explicar a estrutura não linear que a gente usou.**

**Checklist:**
- Editor com `fila_alertas.c` e `lista_ranking.c` abertos em abas
- Terminal com saída do `./respiragyn` acessível (não precisa rodar de novo se o Henrique já rodou)

---

## FALA 4 — MARCOS PIEDADE (12:00 – 16:00)

> **Obrigado Julio. O critério um do seminário exige que a gente use uma estrutura NÃO LINEAR, árvore ou grafo. A gente escolheu grafo — e vou explicar por quê.**
>
> **O CO₂ não respeita fronteira administrativa de setor. Se o Setor Bueno está em concentração crítica, o Setor Marista, que fica a 1,5 km, vai respirar o mesmo ar em questão de minutos. A dispersão atmosférica é contínua. Modelar isso com pilha ou fila seria errado — a relação entre setores vizinhos não é sequencial, é EM REDE.**
>
> **Grafo é exatamente a estrutura para relações em rede. [Abre grafo_setores.c.] A implementação usa LISTA DE ADJACÊNCIA. Cada vértice do grafo tem uma cadeia encadeada dos vizinhos dele. É a representação mais eficiente para grafos esparsos, que é o caso aqui — cada setor tem entre 2 e 5 vizinhos, não 30.**
>
> **A struct Grafo guarda um array de ponteiros para as listas de adjacência, mais um array de latitude e longitude de cada vértice. Essas coordenadas servem para uma função importante: `grafo_conectar_por_proximidade`. Ela percorre todos os pares de setores, calcula a distância entre eles com a FÓRMULA DE HAVERSINE — que é a fórmula padrão para distância na superfície de uma esfera — e conecta com uma aresta os setores que estão a até 5 km um do outro. A complexidade dessa função é O(V²) — V ao quadrado — porque ela testa todos os pares.**
>
> **Agora a parte mais importante: `grafo_propagar_alerta`. Essa função implementa uma busca em largura, BFS. Ela parte de um setor crítico e, camada por camada, visita os vizinhos, os vizinhos dos vizinhos, até alcançar todos os setores conectados. A complexidade é O(V + E) — número de vértices mais número de arestas.**
>
> **Olhem o detalhe elegante: a BFS internamente usa uma fila LOCAL para controlar a ordem da visita. Mas para o resultado — notificar os vizinhos alcançados — a BFS enfileira na FILA FIFO que o Julio implementou. Isso é COMPOSIÇÃO DE ESTRUTURAS. O grafo decide QUEM notificar, a fila decide EM QUE ORDEM atender. Nenhuma estrutura faz as duas coisas sozinha.**
>
> **Vou rodar. [Rola o terminal para a Demo 6.] Peguei 6 setores reais de Goiânia — Bueno, Marginal Botafogo, Campinas, T-63, Jaó e Vila Nova — registrei as coordenadas e conectei por proximidade com raio de 5 km. O resultado foi 9 arestas. Depois marquei o setor 0, que é o Bueno, como crítico com 1850 ppm e chamei a propagação. A BFS alcançou 5 vizinhos e enfileirou todos como alertas pendentes.**
>
> **Isso é um sistema integrado em C puro: pilha mantém o histórico recente, fila mantém a ordem de atendimento, lista mantém o ranking, e grafo mantém a topologia física da cidade. O Murilo agora vai mostrar como tudo isso vira um site navegável.**

**Checklist:**
- `grafo_setores.c` aberto no editor
- Scroll do terminal posicionado na Demo 6

---

## FALA 5 — MURILO FELÍCIO (16:00 – 20:00)

> **Obrigado Marcos. Minha parte é mostrar como todo esse núcleo em C vira um produto que uma prefeitura poderia usar.**
>
> **A arquitetura é a seguinte: o binário em C aceita três modos de execução via pipe. Em modo `--backend`, ele recebe CSV por stdin e devolve JSON por stdout. Em modo `--prever`, ele recebe uma série de PPMs e devolve previsão. Em modo `--recomendar`, ele recebe PPM e área e devolve número de árvores.**
>
> **O backend em Python é só um adaptador fino. [Abre backend/app.py.] Olhem essa função aqui: `processar_no_nucleo_c`. Ela usa `subprocess.run` para abrir o binário em C, manda o CSV pela stdin, lê o JSON da stdout, e passa pro Flask serializar como resposta HTTP. O Python literalmente não calcula CO₂ — toda a lógica está no C que vocês acabaram de ver.**
>
> **Agora o site. [Alt-tab para o navegador em http://localhost:5000/.] Vocês veem o mapa de Goiânia com os 30 setores que cadastramos. Cada bolha de cor é um setor — e a cor NÃO é categórica. É um degradê contínuo que vai do vinho bordô, passa por vermelho, laranja, amarelo e verde, até chegar no caqui dos setores limpos. Cada setor tem SUA cor exata, interpolada em RGB com oito pontos de referência.**
>
> **Olhem a lateral. É um boletim ordenado dinamicamente por PPM. Aqui em cima tem os chips de filtro — eu posso filtrar só os setores críticos. [Clica em "critica".] Vejam que o mapa e a lista reagem juntos. Posso combinar — mostrar críticos e altos juntos. [Clica em "alta".]**
>
> **O sistema atualiza em tempo real. Vejam esse contador aqui em cima: "próximo ciclo em 12 segundos". A cada 15 segundos ele rebusca a API, re-estima o CO₂ no núcleo em C, re-grava no SQLite e repinta o mapa. E aqui tem um detalhe sutil: quando o PPM de um setor muda entre ciclos, aparece uma seta com o delta ao lado do nome. Se subiu, aparece em vermelho. Se desceu, verde.**
>
> **Vou clicar num setor para abrir o histórico. [Clica no setor crítico mais alto.] Aparece o modal com o gráfico das últimas 6 horas de PPM e a previsão para mais 1 hora, 3 horas e 6 horas à frente. A previsão também está em C — é uma média móvel ponderada com fator horário que codifica os picos conhecidos de tráfego de Goiânia, manhã e tarde.**
>
> **Para fechar: por que isso importa? Uma árvore jovem em crescimento absorve em média 20 kg de CO₂ por ano. O nosso sistema olha cada setor e recomenda quantas árvores plantar. Para o setor que vocês estão vendo aqui com 1300 ppm numa área de 2 km², a recomendação é 300 árvores — direcionadas, no lugar certo. Em vez de distribuir plantio uniformemente pela cidade, a prefeitura concentraria onde o CO₂ realmente está sendo produzido.**
>
> **Esse é o RespiraGyn. Aceitamos perguntas da banca.**

**Checklist:**
- Navegador aberto em http://localhost:5000/
- Mapa visível, carregado
- Filtros funcionando (teste antes!)
- Pelo menos um setor em ALTA ou CRÍTICA (se tudo estiver em BAIXA, aguarda próximo ciclo ou reinicia Flask)
