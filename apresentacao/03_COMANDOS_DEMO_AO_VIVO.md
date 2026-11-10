# 03 — COMANDOS DEMO AO VIVO

**Este arquivo é o "guia do operador" — para quem estiver digitando no terminal e no editor durante a apresentação. Em geral, esse é o Julio (dono do notebook), mas pode ser qualquer um.**

---

## Ambiente do notebook do Julio

- **SO:** Windows 11
- **Projeto em:** `C:\Users\julio\RespiraGyn\verderotas-co2`
- **Terminal recomendado:** Windows Terminal com WSL Ubuntu
- **Editor recomendado:** VSCode com os arquivos .c abertos em abas
- **Navegador:** Chrome ou Edge em http://localhost:5000/

---

## PREPARAÇÃO — 10 minutos ANTES da apresentação

### Passo 1 — Garantir que o binário C está atualizado

Abra o Windows Terminal, selecione aba WSL Ubuntu, e digite:

```bash
cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2/core
make clean && make
```

**Saída esperada (final):**
```
gcc -Wall -Wextra -std=c11 -O2 -o respiragyn main.o pilha_co2.o ...
```
Nenhum "error" ou "warning". Se aparecer erro, **não apresente** — chame alguém.

### Passo 2 — Rodar os testes (opcional, mas recomendado)

```bash
make test
```

**Saída esperada no final:**
```
=== TODOS PASSARAM ===
```

### Passo 3 — Subir o backend Flask

Abra uma SEGUNDA aba do terminal WSL e deixe esse comando RODANDO:

```bash
cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2/backend
python3 app.py
```

**Saída esperada:**
```
 * Running on http://127.0.0.1:5000
```

**Não feche essa aba durante a apresentação.**

### Passo 4 — Abrir o site no navegador

No PowerShell ou Prompt de Comando (ou clique direto no navegador):

```powershell
Start-Process "http://localhost:5000/"
```

**Verifique:** o mapa carrega em até 3 segundos. Setores coloridos aparecem.

### Passo 5 — Abrir os arquivos no VSCode em abas separadas

Abra VSCode e abra estes 6 arquivos em abas, NA ORDEM:

1. `C:\Users\julio\RespiraGyn\verderotas-co2\core\pilha_co2.c` — para Henrique
2. `C:\Users\julio\RespiraGyn\verderotas-co2\core\fila_alertas.c` — para Julio
3. `C:\Users\julio\RespiraGyn\verderotas-co2\core\lista_ranking.c` — para Julio
4. `C:\Users\julio\RespiraGyn\verderotas-co2\core\grafo_setores.c` — para Marcos
5. `C:\Users\julio\RespiraGyn\verderotas-co2\core\main.c` — para arguição
6. `C:\Users\julio\RespiraGyn\verderotas-co2\backend\app.py` — para Murilo

Deixe na aba do `pilha_co2.c` na hora de começar.

---

## DURANTE A APRESENTAÇÃO

### Comando mestre — rodar as 6 demos

Na aba WSL (NÃO a do Flask), digite UMA ÚNICA VEZ:

```bash
cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2/core
./respiragyn
```

**Importante:** rode ANTES do Henrique começar a falar. Deixe o terminal com a saída inteira visível, e vá rolando conforme cada colega chegar na sua demo:

| Colega | Role o terminal até ver |
|---|---|
| Henrique | `==> [DEMO 1] PUSH multiplo` |
| Henrique | `==> [DEMO 2] POP sem Underflow` |
| Henrique | `==> [DEMO 3] PEEK` |
| Julio | `==> [DEMO 4] FILA FIFO` |
| Julio | `==> [DEMO 5] LISTA ORDENADA` |
| Marcos | `==> [DEMO 6] GRAFO + BFS` |

### Alt-tab entre tela do terminal, do VSCode e do navegador

- Terminal → quando mostrar as demos em execução
- VSCode → quando o colega estiver explicando o código
- Navegador → só no bloco do Murilo

**Treino em casa:** faça pelo menos UMA vez a alternância completa entre as 3 telas antes do dia real.

---

## SE ALGO QUEBRAR — plano de emergência

### Problema 1 — `./respiragyn` não existe

```bash
cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2/core
make clean && make
./respiragyn
```

### Problema 2 — Flask não responde (navegador em branco)

Na aba do Flask (a que estava rodando `python3 app.py`):
- Se o processo morreu, suba de novo:
  ```bash
  cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2/backend
  python3 app.py
  ```
- Aperte F5 no navegador 2 ou 3 vezes.

### Problema 3 — Mapa aparece em branco no navegador

- Abra o Dev Tools (F12) → aba Console → procure erro em vermelho
- Em último caso: Ctrl+Shift+R (hard reload)
- Se nada funcionar: o Marcos ou Murilo pode mostrar o `README.md` renderizado no VSCode (Ctrl+Shift+V) com as capturas de tela, e **explicar com honestidade** que "o backend está rodando mas o navegador travou". A nota técnica permanece — o produto existe.

### Problema 4 — WSL Ubuntu não abre

Se WSL estiver quebrado, há alternativa: rodar pelo próprio Windows, se o grupo tiver gcc nativo. **Preferível preparar isso em casa antes.**

Se tudo quebrar: mostrar o README.md no editor, explicar o código. A nota do critério Arguição Técnica NÃO depende do software rodar — depende do grupo saber explicar.

---

## BACKUP EM NUVEM (opcional — depende da decisão do grupo)

Se o grupo decidir subir pro Render.com como backup, os passos são:

### Antes do dia 24

```bash
# 1. commit tudo no git
cd /mnt/c/Users/julio/RespiraGyn/verderotas-co2
git init
git add .
git commit -m "respiragyn pronto pro seminario"

# 2. criar repo no GitHub (pelo site) e dar push
git remote add origin https://github.com/<usuario>/respiragyn.git
git push -u origin main

# 3. no site render.com: New + > Blueprint > apontar pro repo
# O Dockerfile e o render.yaml já estão configurados no projeto.
```

### No dia da apresentação

Deixe uma aba do Chrome aberta em `https://<nome-do-projeto>.onrender.com/` como backup. Se o localhost falhar, troca a aba ativa e continua.

**Observação:** no Render free tier, o primeiro acesso depois de 15 min de inatividade demora ~30 segundos (cold start). Se for usar como backup, **acesse 5 min antes** da apresentação para "esquentar" o container.

---

## RESUMO VISUAL — o que fica em cada aba

```
┌─────────────────────────────────────────────────────────────┐
│  Windows Terminal                                           │
│                                                             │
│  ┌─────────────────────┐  ┌─────────────────────────────┐  │
│  │ WSL — aba 1          │  │ WSL — aba 2 (Flask RODANDO) │  │
│  │ roda ./respiragyn    │  │ python3 app.py              │  │
│  │ rola demos           │  │ ← NÃO FECHAR                │  │
│  └─────────────────────┘  └─────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  VSCode — 6 abas abertas                                    │
│  [pilha_co2.c] [fila_alertas.c] [lista_ranking.c]           │
│  [grafo_setores.c] [main.c] [app.py]                        │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│  Chrome / Edge                                              │
│  http://localhost:5000/     ← produto final navegável        │
└─────────────────────────────────────────────────────────────┘
```
