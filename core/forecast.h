/*
 * forecast.h — Previsao de PPM +1h/+3h/+6h via WMA + fator horario.
 * Sem malloc: opera sobre buffer cedido pelo chamador.
 */

#ifndef RESPIRAGYN_FORECAST_H
#define RESPIRAGYN_FORECAST_H

#define FORECAST_HORIZONTES   3        /* +1h, +3h, +6h               */
#define FORECAST_JANELA_MAX   8        /* amostras usadas no WMA      */
#define FORECAST_PPM_BASE     420.0f   /* fallback sem historico      */

extern const int FORECAST_HORAS[FORECAST_HORIZONTES];

typedef struct {
    int     horas;         /* 1, 3 ou 6                        */
    int     hora_alvo;     /* hora do dia (0-23) do horizonte  */
    float   ppm_previsto;
    int     em_pico;       /* 1 se cai em pico                 */
} PrevisaoHorizonte;

/* WMA — media movel ponderada: indice n-1 recebe maior peso (mais recente). */
float wma_ppm(const float *valores, int n);

/* Multiplicador por hora: 7-9 -> 1.20; 17-19 -> 1.30; 22-4 -> 0.80; outros -> 1.00 */
float fator_hora(int hora_do_dia);

/* Preenche saida[FORECAST_HORIZONTES] com a previsao por horizonte. */
int prever_serie(const float *historico, int n,
                 int hora_agora,
                 PrevisaoHorizonte *saida);

#endif
