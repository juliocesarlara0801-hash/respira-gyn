/*
 * estimador.h — Estimativa de CO2 a partir de dados de trafego.
 * Grupo nao possui sensor fisico: a leitura e CALCULADA com fluxo/velocidade/area.
 */

#ifndef ESTIMADOR_H
#define ESTIMADOR_H

/* Amostra bruta da API de trafego (Google Maps / mock). */
typedef struct {
    int    id_setor;
    double fluxo_veicular;       /* veiculos/hora */
    double velocidade_media;     /* km/h          */
    double area_setor_km2;       /* area em km2   */
    int    nivel_trafego;        /* 0..5          */
} AmostraTrafego;

/* Converte trafego em PPM estimado. Usa fator de emissao + diluicao em coluna de ar.
   Teto defensivo em 2000 ppm. Nunca retorna abaixo da atmosfera base (420 ppm). */
float estimar_ppm_co2(const AmostraTrafego *amostra);

/* Fator de emissao em g CO2/km, dependente da velocidade.
   Veiculo em marcha lenta emite muito mais que em fluxo livre. */
double fator_emissao_g_por_km(double velocidade_kmh);

/* Numero de arvores recomendadas para o setor. Cresce com excesso_ppm * area.
   Retorna 0 se ppm <= 420 (ar limpo) ou area <= 0. */
int recomendacao_arvores(float ppm, double area_km2);

#endif
