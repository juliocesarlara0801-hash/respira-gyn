/*
 * estimador.c — Converte trafego em PPM e calcula arvores recomendadas.
 *
 * Calibracao (defendivel na banca):
 *   extensao_media  = 1.5 km   (tamanho tipico atravessado num setor de ~2 km2)
 *   altura_coluna   = 300 m    (camada de mistura urbana diurna — NOAA)
 *   emissao f(v)    = 90 + 1500/v   (urbano 150-400 g/km — CETESB 2024)
 */

#include "estimador.h"
#include "pilha_co2.h"
#include <math.h>

#define EXTENSAO_MEDIA_KM        1.5
#define ALTURA_COLUNA_AR_M       300.0
#define ATMOSFERA_BASE_PPM       420.0f
#define FATOR_CONVERSAO_PPM      1.8e-6    /* g/m3 -> ppm a 25 C */

/* Curva: v=10 -> 240 g/km ; v=30 -> 140 ; v=60 -> 115 ; v=80 -> 109. */
double fator_emissao_g_por_km(double velocidade_kmh) {
    if (velocidade_kmh < 1.0) velocidade_kmh = 1.0;      /* evita div/0 */
    return 90.0 + (1500.0 / velocidade_kmh);
}

/* PPM = base + (emissao_total / volume_de_ar) convertido. */
float estimar_ppm_co2(const AmostraTrafego *a) {
    if (a == NULL || a->area_setor_km2 <= 0.0) return ATMOSFERA_BASE_PPM;

    double fator        = fator_emissao_g_por_km(a->velocidade_media);
    double emissao_g_h  = a->fluxo_veicular * fator * EXTENSAO_MEDIA_KM;
    double volume_m3    = (a->area_setor_km2 * 1e6) * ALTURA_COLUNA_AR_M;
    double conc_g_m3    = emissao_g_h / volume_m3;
    double contrib_ppm  = conc_g_m3 / FATOR_CONVERSAO_PPM;

    double ppm_total = ATMOSFERA_BASE_PPM + contrib_ppm;
    if (ppm_total > 2000.0) ppm_total = 2000.0;          /* teto defensivo */
    return (float) ppm_total;
}

/* arvores = round(excesso * area * K). K = 0.172 calibrado empiricamente. */
#define ARVORES_K  0.172

int recomendacao_arvores(float ppm, double area_km2) {
    if (ppm <= ATMOSFERA_BASE_PPM || area_km2 <= 0.0) return 0;
    double excesso = (double) ppm - (double) ATMOSFERA_BASE_PPM;
    double bruto   = excesso * area_km2 * ARVORES_K;
    return (int)(bruto + 0.5);                           /* arredonda classico */
}
