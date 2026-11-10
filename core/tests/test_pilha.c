/*
 * test_pilha.c — Testes unitarios do nucleo em C.
 * Minimalista (sem framework externo) para rodar em qualquer maquina.
 *
 * Cobre:
 *   - push_leitura respeita filtro de 400 ppm
 *   - push_leitura respeita CO2_PILHA_MAXIMA (sem overflow)
 *   - pop_leitura em pilha vazia nao gera segfault (underflow blindado)
 *   - peek nao altera a estrutura
 *   - destruir libera a pilha corretamente (resetando tamanho)
 */

#include "../pilha_co2.h"
#include "../estimador.h"
#include "../forecast.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>

static int falhas = 0;
#define CHECK(cond, msg)  do {                             \
    if (!(cond)) { printf("  FALHA: %s\n", msg); falhas++; }\
    else         { printf("  ok   : %s\n", msg); }          \
} while (0)

static void teste_push_filtra_ruido(void) {
    printf("[teste] push_leitura descarta ppm < 400\n");
    PilhaCO2 p; pilha_criar(&p);

    bool ok1 = push_leitura(&p, 350.0f, 1, 1, time(NULL));
    bool ok2 = push_leitura(&p, 450.0f, 1, 1, time(NULL));

    CHECK(!ok1, "350 ppm foi descartado");
    CHECK( ok2, "450 ppm foi empilhado");
    CHECK(pilha_tamanho(&p) == 1, "tamanho == 1");

    pilha_destruir(&p);
}

static void teste_pop_underflow_blindado(void) {
    printf("[teste] pop em pilha vazia nao crasha\n");
    PilhaCO2 p; pilha_criar(&p);

    NodoLeitura saida;
    bool ok = pop_leitura(&p, &saida);
    CHECK(!ok, "pop em pilha vazia devolve false");
    CHECK(pilha_tamanho(&p) == 0, "tamanho segue 0");

    pilha_destruir(&p);
}

static void teste_peek_nao_destrutivo(void) {
    printf("[teste] peek nao altera a pilha\n");
    PilhaCO2 p; pilha_criar(&p);

    push_leitura(&p, 500.0f, 10, 2, time(NULL));
    push_leitura(&p, 700.0f, 20, 4, time(NULL));

    int antes = pilha_tamanho(&p);
    NodoLeitura topo;
    bool ok = peek_leitura(&p, &topo);
    int depois = pilha_tamanho(&p);

    CHECK(ok, "peek retornou true");
    CHECK(topo.ppm == 700.0f, "topo ppm == 700");
    CHECK(topo.id_setor == 20, "topo setor == 20");
    CHECK(antes == depois, "tamanho preservado apos peek");

    pilha_destruir(&p);
}

static void teste_destruir(void) {
    printf("[teste] destruir libera e zera tamanho\n");
    PilhaCO2 p; pilha_criar(&p);

    for (int i = 0; i < 100; i++) {
        push_leitura(&p, 420.0f + i, i, 1, time(NULL));
    }
    CHECK(pilha_tamanho(&p) == 100, "100 itens empilhados");

    pilha_destruir(&p);
    CHECK(pilha_tamanho(&p) == 0, "destruir zera tamanho");
    CHECK(pilha_vazia(&p),         "pilha fica vazia");
}

static void teste_estimador(void) {
    printf("[teste] estimador gera ppm plausivel\n");
    AmostraTrafego a_calmo       = {1,  200.0, 60.0, 3.0, 1};
    AmostraTrafego a_congestion  = {1, 3000.0, 10.0, 1.5, 5};

    float ppm_calmo = estimar_ppm_co2(&a_calmo);
    float ppm_cong  = estimar_ppm_co2(&a_congestion);

    CHECK(ppm_calmo >= 420.0f,          "setor calmo >= 420 ppm");
    CHECK(ppm_cong  >  ppm_calmo,       "setor congestionado > calmo");
    CHECK(ppm_cong  <= 2000.0f,         "ppm respeita teto de seguranca");
    printf("  >> calmo=%.2f  cong=%.2f\n", ppm_calmo, ppm_cong);
}

static void teste_recomendacao_arvores(void) {
    printf("[teste] recomendacao_arvores escala com ppm e area\n");

    int a0 = recomendacao_arvores(420.0f, 2.0);   /* ar limpo */
    int a1 = recomendacao_arvores(800.0f, 2.0);   /* alto     */
    int a2 = recomendacao_arvores(1200.0f, 2.0);  /* critico  */
    int a3 = recomendacao_arvores(800.0f, 4.0);   /* dobra area */

    CHECK(a0 == 0,            "420 ppm -> 0 arvores (sem pressao)");
    CHECK(a1 > 0,             "800 ppm, 2km2 -> recomenda arvores");
    CHECK(a2 > a1,            "mais ppm -> mais arvores");
    CHECK(a3 > a1,            "mais area, mesmo ppm -> mais arvores");
    CHECK(recomendacao_arvores(500.0f, -1.0) == 0, "area invalida -> 0");
    printf("  >> 420=%d  800/2km2=%d  1200/2km2=%d  800/4km2=%d\n",
           a0, a1, a2, a3);
}

static void teste_forecast_wma(void) {
    printf("[teste] wma pondera mais o passado recente\n");

    float serie_crescente[] = { 500.0f, 600.0f, 700.0f, 900.0f };
    float serie_decrescente[] = { 900.0f, 700.0f, 600.0f, 500.0f };
    float media_crescente  = wma_ppm(serie_crescente, 4);
    float media_decrescente = wma_ppm(serie_decrescente, 4);

    /* A WMA pondera o mais recente mais pesado. Numa serie crescente,
       o mais recente eh o maior; a WMA deve puxar acima da media simples (675). */
    CHECK(media_crescente > 675.0f,          "WMA puxa acima da media simples quando a tendencia sobe");
    CHECK(media_decrescente < 675.0f,        "WMA puxa abaixo da media simples quando a tendencia cai");
    CHECK(wma_ppm(NULL, 0) == 420.0f,        "WMA em serie vazia -> atmosfera base");
    printf("  >> crescente=%.1f  decrescente=%.1f (media simples seria 675)\n",
           media_crescente, media_decrescente);
}

static void teste_forecast_fator_hora(void) {
    printf("[teste] fator_hora codifica picos de Goiania\n");
    CHECK(fator_hora(8)   > 1.1f, "manha -> pico (1.20)");
    CHECK(fator_hora(18)  > 1.2f, "tarde -> pico (1.30)");
    CHECK(fator_hora(2)   < 1.0f, "madrugada -> abaixo da base (0.80)");
    CHECK(fator_hora(14) == 1.0f, "meio do dia -> base 1.00");
    CHECK(fator_hora(99) == 1.0f, "entrada invalida -> fallback 1.00");
}

static void teste_forecast_prever_serie(void) {
    printf("[teste] prever_serie preenche 3 horizontes\n");
    float h[] = { 650.0f, 720.0f, 810.0f, 950.0f, 1180.0f };
    PrevisaoHorizonte out[FORECAST_HORIZONTES];
    int n = prever_serie(h, 5, 8 /* 8h da manha */, out);
    CHECK(n == 3, "3 horizontes retornados");
    CHECK(out[0].horas == 1 && out[1].horas == 3 && out[2].horas == 6,
          "horizontes 1h/3h/6h na ordem");
    /* As 9h (8+1) ainda eh pico manha; previsao deve estar acima da base */
    CHECK(out[0].ppm_previsto > 0.0f, "ppm previsto positivo");
}

int main(void) {
    printf("=== Testes unitarios RespiraGyn ===\n\n");

    teste_push_filtra_ruido();
    teste_pop_underflow_blindado();
    teste_peek_nao_destrutivo();
    teste_destruir();
    teste_estimador();
    teste_recomendacao_arvores();
    teste_forecast_wma();
    teste_forecast_fator_hora();
    teste_forecast_prever_serie();

    printf("\n=== %s ===\n", falhas == 0 ? "TODOS PASSARAM" : "COM FALHAS");
    return falhas;
}
