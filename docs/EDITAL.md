# Mapeamento ao Edital — Prof. Me. Valdir Prado

Este documento mostra, linha a linha, como o RespiraGyn atende aos critérios da **entrega técnica de 03/11** da Aula 13 (Pilhas) e aos pontos que o professor marcou como obrigatórios no PDF da aula.

---

## 📋 Edital de Avaliação (slide "Especificações de Entrega Híbrida — 03/11")

> "A esteira exige **três validações funcionais no terminal**. A validação ocorrerá de forma cruzada — os grupos testarão as arquiteturas dos oponentes."

| # | Critério do professor | Onde está no código | Como validar |
|---|-----------------------|---------------------|--------------|
| 1 | **Empilhamento múltiplo sem erro** — capacidade de empilhar múltiplos sensores com PUSH sem falha de memória | `main.c` → `demo_push_multiplo()` + `pilha_co2.c` → `push_leitura()` | `./core/verderotas` → seção "[DEMO 1]" empilha 5 setores sem erro |
| 2 | **POP sem Underflow** — exclusão de sensor com POP sem apresentar Segmentation Fault | `pilha_co2.c` → `pop_leitura()` com `if (p->topo == NULL) return false;` | `./core/verderotas` → seção "[DEMO 2]" esvazia a pilha e tenta um POP extra; devolve `false` sem crashar |
| 3 | **Auditoria via PEEK** — leitura do topo antes e depois de cada manobra, sem destruir a estrutura | `pilha_co2.c` → `peek_leitura()` + `main.c` → `demo_peek_nao_destrutivo()` | `./core/verderotas` → seção "[DEMO 3]" imprime tamanho antes/depois do peek (iguais) |

**Alerta do edital:** *"Grupos que cometerem falha de segmento serão penalizados na revisão de pares."*
→ Blindagem explícita em `pop_leitura()` e `peek_leitura()`. Teste `teste_pop_underflow_blindado()` em `tests/test_pilha.c` cobre esse caso.

---

## 🧱 Regras específicas que o professor marcou para o grupo CO₂

### Slide "Arquitetura de Código — O Risco do CO₂"
> "A maquete **não deve aceitar o PUSH** se a leitura for menor que 400 PPM (atmosfera base). Inserir lixo atinge o teto do armazém inutilmente. **Confirme a presença do `if` antes do motor dinâmico.**"

- Implementado em `pilha_co2.c` → `push_leitura()`:
  ```c
  if (ppm < CO2_PPM_MINIMO) {
      return false;          // filtro antes do malloc!
  }
  ```
- Teste: `teste_push_filtra_ruido()` valida que 350 ppm é descartado e 450 ppm é aceito.

### Slide "Escopo de Arquitetura 3 — Pilha de CO₂"
> "A arquitetura deve empilhar todas as medições e, a cada novo PUSH, acionar a função analítica de teto de PPM. **A Pilha de CO₂ não pode sofrer Overflow de maneira alguma** — isso travaria o painel de evacuação. Validem o tamanho em bytes do sensor para maximizar a quantidade de nós no topo."

- `CO2_PILHA_MAXIMA = 10000` em `pilha_co2.h` — teto configurável de segurança.
- `push_leitura()` rejeita (retorna `false`) se `p->tamanho >= CO2_PILHA_MAXIMA`.
- Backend em Python chama `pilha_destruir()` indiretamente a cada requisição (processo C termina → sistema operacional reclama a RAM), garantindo que em telemetria contínua a pilha não cresce indefinidamente.

### Slide "O Limite Físico — Stack Overflow"
> "Calcule o limite de caixas que a memória suporta antes de programar a implantação física."

- `sizeof(NodoLeitura)` ≈ 32 bytes (ppm 4 + id_setor 4 + nivel 4 + timestamp 8 + ponteiro 8 + padding).
- Com `CO2_PILHA_MAXIMA = 10000` → ~320 KB por pilha. Cabe tranquilamente em qualquer microcontrolador Cortex-M4 (128+ KB) ou PC.

### Slide "O Motor de Destruição — Operação POP"
> "O POP não apenas remove o acesso à caixa — ele a **tritura**. Guarda o endereço do topo em ponteiro temporário, move o topo para o andar abaixo e libera a RAM com `free()`."

Nossa `pop_leitura()` segue a receita literalmente:
```c
NodoLeitura *temp = p->topo;   // guarda referência temporária
// ... copia dado para `saida` ...
p->topo = temp->abaixo;        // desce o topo um andar
free(temp);                    // tritura e devolve RAM
```

### Slide "Comparação Arquitetural — Pilha vs Lista Simples"
> "Em sistemas fechados, rigidez é uma vantagem competitiva de performance — não uma limitação."

- Para o VerdeRota a rigidez da pilha é exatamente o que queremos: sempre precisamos do dado **mais recente** por setor (acionar alerta e priorizar plantio). Não precisamos varrer histórico antigo — ele pode ser arquivado para análise posterior.

---

## 🧪 Checklist para validação cruzada (grupos oponentes)

- [ ] `make -C core` compila sem warnings
- [ ] `./core/respiragyn` roda as 3 demos sem crash
- [ ] `make -C core test` imprime `=== TODOS PASSARAM ===` (24 testes)
- [ ] `./core/respiragyn --backend` com stdin vazio encerra limpo
- [ ] `./core/respiragyn --backend` com 100 linhas CSV produz 100 JSONs
- [ ] `./core/respiragyn --prever` com histórico válido devolve JSON de previsão
- [ ] `./core/respiragyn --recomendar` com "ppm,area" devolve nº de árvores
- [ ] `python backend/app.py` responde em `/api/leituras` com 30 setores
- [ ] Mapa de calor atualiza visualmente no navegador a cada 60s

## 📎 Entrega sugerida para o professor

Zipar a pasta inteira e entregar + rodar as 3 demos ao vivo no laptop. Se possível, projetar o frontend com o mapa de calor animando para mostrar o uso real da pilha.
