/*
 * forecast.c — WMA + fator horario para prever PPM nos horizontes +1h/+3h/+6h.
 */

#include "forecast.h"
#include <stddef.h>

const int FORECAST_HORAS[FORECAST_HORIZONTES] = {1, 3, 6};

/* WMA  O(n). Pesos lineares 1..n. Soma dos pesos = n(n+1)/2 (Gauss). */
float wma_ppm(const float *valores, int n) {
    if (valores == NULL || n <= 0) return FORECAST_PPM_BASE;

    double soma_valores = 0.0;
    double soma_pesos   = 0.0;
    for (int i = 0; i < n; i++) {
        double peso = (double)(i + 1);
        soma_valores += (double)valores[i] * peso;
        soma_pesos   += peso;
    }
    return (float) (soma_valores / soma_pesos);
}

/* Picos de Goiania: manha 7-9, tarde 17-19 (pior), madrugada 22-4 baixa. */
float fator_hora(int hora_do_dia) {
    if (hora_do_dia < 0 || hora_do_dia > 23)       return 1.0f;
    if (hora_do_dia >= 7  && hora_do_dia <= 9)     return 1.20f;
    if (hora_do_dia >= 17 && hora_do_dia <= 19)    return 1.30f;
    if (hora_do_dia >= 22 || hora_do_dia <= 4)     return 0.80f;
    return 1.00f;
}

/* Previsao por horizonte: PPM_base_WMA * fator_hora(hora_alvo). */
int prever_serie(const float *historico, int n,
                 int hora_agora,
                 PrevisaoHorizonte *saida) {
    if (saida == NULL) return 0;

    /* Trunca para os FORECAST_JANELA_MAX pontos mais recentes. */
    int inicio = (n > FORECAST_JANELA_MAX) ? (n - FORECAST_JANELA_MAX) : 0;
    int usados = n - inicio;
    float base = wma_ppm(historico + inicio, usados);

    for (int k = 0; k < FORECAST_HORIZONTES; k++) {
        int h = FORECAST_HORAS[k];
        int hora_alvo = (hora_agora + h) % 24;
        float f = fator_hora(hora_alvo);

        saida[k].horas        = h;
        saida[k].hora_alvo    = hora_alvo;
        saida[k].ppm_previsto = base * f;
        saida[k].em_pico      = (f > 1.05f) ? 1 : 0;
    }
    return FORECAST_HORIZONTES;
}
