/*
 * main.c — Demo CLI do RespiraGyn.
 *
 * Inovatech 2026 — Grupo 13 (Pressao/CO2):
 *   Anthony Prado, Henrique Rodrigues, Julio Cesar Lara, Marcos Piedade, Murilo Felicio.
 * Disciplina: Algoritmos e Estrutura de Dados — Prof. Me. Valdir Prado.
 *
 * Estruturas de dados implementadas:
 *   LINEARES      -> Pilha LIFO (pilha_co2) + Fila FIFO (fila_alertas) + Lista ordenada (lista_ranking)
 *   NAO LINEAR    -> Grafo nao direcionado por adjacencia (grafo_setores)
 *
 * Modos:
 *   ./respiragyn                -> demo completa (6 passos: PUSH/POP/PEEK + FIFO + Lista + Grafo)
 *   ./respiragyn --backend      -> pipe: stdin CSV -> stdout JSON (chamado pelo Flask)
 *   ./respiragyn --prever       -> pipe: stdin PPM/linha -> stdout JSON previsao
 *   ./respiragyn --recomendar   -> pipe: stdin "ppm,area_km2" -> stdout n_arvores
 */

#include "pilha_co2.h"
#include "fila_alertas.h"
#include "lista_ranking.h"
#include "grafo_setores.h"
#include "estimador.h"
#include "forecast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ==========================================================================
 * DEMOS DA PILHA LIFO — 3 requisitos do edital 03/11
 * ========================================================================== */

/* Demo 1: multiplos PUSH sem falha de memoria. */
static void demo_push_multiplo(PilhaCO2 *p) {
    printf("\n==> [DEMO 1] PUSH multiplo — pilha LIFO\n");

    AmostraTrafego amostras[] = {
        {1, 1200.0, 45.0, 2.5, 2},
        {2, 3200.0, 15.0, 1.8, 5},
        {3,  800.0, 55.0, 4.0, 1},
        {4, 2500.0, 22.0, 2.0, 4},
    };
    int n = sizeof(amostras) / sizeof(amostras[0]);

    for (int i = 0; i < n; i++) {
        float ppm = estimar_ppm_co2(&amostras[i]);
        bool  ok  = push_leitura(p, ppm,
                                 amostras[i].id_setor,
                                 amostras[i].nivel_trafego,
                                 time(NULL));
        int arv = recomendacao_arvores(ppm, amostras[i].area_setor_km2);
        printf("  setor=%d  ppm=%.2f  arvores=%d  push=%s\n",
               amostras[i].id_setor, ppm, arv, ok ? "OK" : "DESCARTADO");
    }

    /* Caso explicito do filtro: ppm < 400 (abaixo da atmosfera base). */
    bool filtrado = push_leitura(p, 350.0f, 99, 0, time(NULL));
    printf("  push direto ppm=350 (< 400) -> push=%s (filtro atmosferico)\n",
           filtrado ? "OK" : "DESCARTADO");

    pilha_imprimir(p);
}

/* Demo 2: POP consecutivo + tentativa de POP em pilha vazia = Underflow blindado. */
static void demo_pop_sem_underflow(PilhaCO2 *p) {
    printf("\n==> [DEMO 2] POP sem Underflow — blindagem\n");

    NodoLeitura saida;
    while (pop_leitura(p, &saida)) {
        printf("  pop: setor=%d ppm=%.2f  (restam %d)\n",
               saida.id_setor, saida.ppm, pilha_tamanho(p));
    }
    bool ok = pop_leitura(p, &saida);
    printf("  POP em pilha vazia -> retornou %s (esperado false, sem Segfault)\n",
           ok ? "true" : "false");
}

/* Demo 3: PEEK nao destroi a estrutura (tamanho antes == depois). */
static void demo_peek_nao_destrutivo(PilhaCO2 *p) {
    printf("\n==> [DEMO 3] PEEK — auditoria passiva\n");

    push_leitura(p, 850.0f, 99, 5, time(NULL));
    push_leitura(p, 620.0f, 98, 3, time(NULL));

    int antes = pilha_tamanho(p);
    NodoLeitura topo;
    peek_leitura(p, &topo);
    int depois = pilha_tamanho(p);

    printf("  antes=%d  topo lido: ppm=%.2f setor=%d  depois=%d (OK se iguais)\n",
           antes, topo.ppm, topo.id_setor, depois);
    pilha_imprimir(p);
}

/* ==========================================================================
 * DEMOS DAS NOVAS ESTRUTURAS — exigidas pelo criterio I do seminario
 * ========================================================================== */

/* Demo 4: FILA FIFO de alertas criticos (>= 1200 ppm). Ordem de chegada preservada. */
static void demo_fila_fifo(void) {
    printf("\n==> [DEMO 4] FILA FIFO — alertas criticos em ordem de chegada\n");

    FilaAlertas fa;
    fila_criar(&fa);

    enfileirar(&fa, 1,  900.0f, time(NULL));   /* filtro: nao entra (<1200) */
    enfileirar(&fa, 2, 1300.0f, time(NULL));   /* entra: 1o alerta          */
    enfileirar(&fa, 4, 1500.0f, time(NULL));   /* entra: 2o alerta          */
    enfileirar(&fa, 7, 1800.0f, time(NULL));   /* entra: 3o alerta          */
    fila_imprimir(&fa);

    NodoAlerta primeiro;
    frente_fila(&fa, &primeiro);
    printf("  frente da fila: setor=%d  (deve ser o PRIMEIRO empilhado: 2)\n",
           primeiro.id_setor);

    NodoAlerta saida;
    while (desenfileirar(&fa, &saida)) {
        printf("  desenfileira: setor=%d  ppm=%.2f  (restam %d)\n",
               saida.id_setor, saida.ppm, fila_tamanho(&fa));
    }
    bool ok = desenfileirar(&fa, &saida);
    printf("  desenfileirar em fila vazia -> %s (esperado false)\n",
           ok ? "true" : "false");

    fila_destruir(&fa);
}

/* Demo 5: LISTA ORDENADA por PPM descendente -> top-N critico. */
static void demo_lista_ranking(void) {
    printf("\n==> [DEMO 5] LISTA ORDENADA — top 5 setores criticos\n");

    ListaRanking l;
    lista_criar(&l);

    inserir_ordenado(&l, 1,  980.0f);
    inserir_ordenado(&l, 2, 1500.0f);
    inserir_ordenado(&l, 3,  720.0f);
    inserir_ordenado(&l, 4, 1850.0f);
    inserir_ordenado(&l, 5, 1180.0f);
    inserir_ordenado(&l, 6,  540.0f);
    inserir_ordenado(&l, 7, 1620.0f);

    lista_imprimir_top(&l, 5);

    /* Atualiza o PPM do setor 1 — a lista se reordena sozinha. */
    inserir_ordenado(&l, 1, 1950.0f);
    printf("  Apos atualizar setor 1 (980 -> 1950 ppm):\n");
    lista_imprimir_top(&l, 5);

    float ppm;
    if (buscar_setor(&l, 3, &ppm)) {
        printf("  buscar_setor(3) -> achou  ppm=%.2f\n", ppm);
    }

    lista_destruir(&l);
}

/* Demo 6: GRAFO + BFS -> propaga alerta critico para os vizinhos. */
static void demo_grafo_propagacao(void) {
    printf("\n==> [DEMO 6] GRAFO + BFS — propagacao de alerta pelos vizinhos\n");

    Grafo g;
    grafo_criar(&g, 6);

    /* Registra 6 setores com coordenadas de Goiania. */
    grafo_registrar_setor(&g, 0, -16.7054, -49.2731);  /* Setor Bueno       */
    grafo_registrar_setor(&g, 1, -16.6786, -49.2545);  /* Marginal Botafogo */
    grafo_registrar_setor(&g, 2, -16.6698, -49.2812);  /* Setor Campinas    */
    grafo_registrar_setor(&g, 3, -16.7232, -49.2814);  /* Av T-63           */
    grafo_registrar_setor(&g, 4, -16.6403, -49.2514);  /* Setor Jao         */
    grafo_registrar_setor(&g, 5, -16.6905, -49.2399);  /* Setor Vila Nova   */

    int arestas = grafo_conectar_por_proximidade(&g, 5.0);  /* vizinhos ate 5.0 km */
    printf("  conectado por proximidade (<=5.0 km): %d arestas\n", arestas);
    grafo_imprimir(&g);

    /* Simula alerta critico no setor 0 (Bueno). BFS notifica os vizinhos. */
    FilaAlertas fa;
    fila_criar(&fa);

    int notificados = grafo_propagar_alerta(&g, 0, 1850.0f, &fa);
    printf("\n  Setor 0 (Bueno) em CRITICO (1850 ppm). BFS notificou %d vizinhos:\n",
           notificados);
    fila_imprimir(&fa);

    fila_destruir(&fa);
    grafo_destruir(&g);
}

/* ==========================================================================
 * MODO BACKEND (pipe para o Flask)
 * ========================================================================== */

static void modo_backend(void) {
    PilhaCO2 p;
    pilha_criar(&p);

    char linha[256];
    while (fgets(linha, sizeof(linha), stdin) != NULL) {
        AmostraTrafego a;
        int lidos = sscanf(linha, "%d,%lf,%lf,%lf,%d",
                           &a.id_setor,
                           &a.fluxo_veicular,
                           &a.velocidade_media,
                           &a.area_setor_km2,
                           &a.nivel_trafego);
        if (lidos != 5) continue;

        float ppm = estimar_ppm_co2(&a);
        bool  ok  = push_leitura(&p, ppm, a.id_setor,
                                 a.nivel_trafego, time(NULL));
        int arv = recomendacao_arvores(ppm, a.area_setor_km2);

        printf("{\"id_setor\":%d,\"ppm\":%.2f,\"nivel\":%d,"
               "\"arvores\":%d,\"empilhado\":%s}\n",
               a.id_setor, ppm, a.nivel_trafego, arv,
               ok ? "true" : "false");
        fflush(stdout);
    }

    pilha_destruir(&p);
}

/* ==========================================================================
 * MODO PREVER (pipe para o Flask)
 * ========================================================================== */

static void modo_prever(void) {
    float serie[1024];
    int   n = 0;
    char  linha[64];
    while (fgets(linha, sizeof(linha), stdin) != NULL && n < 1024) {
        float v;
        if (sscanf(linha, "%f", &v) == 1 && v > 0.0f) serie[n++] = v;
    }

    time_t agora = time(NULL);
    struct tm *lt = localtime(&agora);
    int hora_agora = (lt != NULL) ? lt->tm_hour : 12;

    PrevisaoHorizonte out[FORECAST_HORIZONTES];
    prever_serie(serie, n, hora_agora, out);
    float base = wma_ppm(serie + (n > FORECAST_JANELA_MAX ? n - FORECAST_JANELA_MAX : 0),
                         n > FORECAST_JANELA_MAX ? FORECAST_JANELA_MAX : n);

    int pico_proximo = 0;
    for (int i = 0; i < FORECAST_HORIZONTES; i++) {
        if (out[i].em_pico) { pico_proximo = 1; break; }
    }

    printf("{\"base_ppm\":%.1f,\"pontos_usados\":%d,"
           "\"pico_proximo\":%s,\"hora_atual\":%d,\"previsoes\":[",
           base, n, pico_proximo ? "true" : "false", hora_agora);
    for (int i = 0; i < FORECAST_HORIZONTES; i++) {
        printf("%s{\"horas\":%d,\"hora_alvo\":%d,\"ppm\":%.1f,\"pico\":%s}",
               (i > 0) ? "," : "",
               out[i].horas, out[i].hora_alvo,
               out[i].ppm_previsto,
               out[i].em_pico ? "true" : "false");
    }
    printf("]}\n");
    fflush(stdout);
}

/* ==========================================================================
 * MODO RECOMENDAR (pipe para o Flask)
 * ========================================================================== */

static void modo_recomendar(void) {
    char linha[128];
    while (fgets(linha, sizeof(linha), stdin) != NULL) {
        float  ppm;
        double area;
        if (sscanf(linha, "%f,%lf", &ppm, &area) == 2) {
            printf("%d\n", recomendacao_arvores(ppm, area));
            fflush(stdout);
        }
    }
}

/* ==========================================================================
 * main
 * ========================================================================== */

int main(int argc, char **argv) {

    if (argc >= 2) {
        if (strcmp(argv[1], "--backend") == 0)    { modo_backend();    return 0; }
        if (strcmp(argv[1], "--prever") == 0)     { modo_prever();     return 0; }
        if (strcmp(argv[1], "--recomendar") == 0) { modo_recomendar(); return 0; }
    }

    printf("============================================================\n");
    printf("  RespiraGyn — Nucleo em C (Inovatech 2026)\n");
    printf("  Grupo 13 (Pressao/CO2):\n");
    printf("  Anthony, Henrique, Julio, Marcos, Murilo\n");
    printf("============================================================\n");

    PilhaCO2 p;
    pilha_criar(&p);

    demo_push_multiplo(&p);
    demo_pop_sem_underflow(&p);
    demo_peek_nao_destrutivo(&p);
    pilha_destruir(&p);

    demo_fila_fifo();
    demo_lista_ranking();
    demo_grafo_propagacao();

    printf("\n[OK] Todas as estruturas liberadas. Sem memory leak.\n");
    return 0;
}
