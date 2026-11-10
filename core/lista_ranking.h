/*
 * lista_ranking.h — Lista simples ORDENADA por PPM descendente.
 * Serve para responder "quais sao os N setores mais criticos agora?"
 * Pilha so da o topo, fila so da a ordem de chegada; a lista da ordem total.
 */

#ifndef LISTA_RANKING_H
#define LISTA_RANKING_H

#include <stdbool.h>

/* Nodo com ponteiro para o PROXIMO (padrao de lista simples). */
typedef struct NodoRanking {
    int     id_setor;
    float   ppm;
    struct NodoRanking *proximo;
} NodoRanking;

typedef struct {
    NodoRanking *cabeca;     /* primeiro -> maior PPM */
    int tamanho;
} ListaRanking;

void  lista_criar          (ListaRanking *l);
bool  inserir_ordenado     (ListaRanking *l, int id_setor, float ppm); /* O(n) — varre ate achar posicao */
bool  remover_setor        (ListaRanking *l, int id_setor);            /* O(n)                           */
bool  buscar_setor         (const ListaRanking *l, int id_setor, float *ppm_out); /* O(n)                */
int   lista_tamanho        (const ListaRanking *l);
bool  lista_vazia          (const ListaRanking *l);
void  lista_destruir       (ListaRanking *l);                          /* libera tudo  O(n)              */
void  lista_imprimir_top   (const ListaRanking *l, int n);             /* top-N critico                   */

#endif
