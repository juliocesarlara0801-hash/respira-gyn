/*
 * fila_alertas.h — Fila FIFO de alertas criticos de CO2.
 * Setor acima do limite entra pelo FIM; despachante processa pelo INICIO.
 * Complementa a Pilha: pilha prioriza o mais RECENTE, fila preserva a ORDEM de chegada.
 */

#ifndef FILA_ALERTAS_H
#define FILA_ALERTAS_H

#include <stdbool.h>
#include <time.h>

#define FILA_PPM_CRITICO   1200.0f    /* so alerta se ultrapassou o limite CRITICO */
#define FILA_MAXIMA        5000       /* teto anti-Overflow da fila                */

/* Nodo da fila. Ponteiro aponta para o PROXIMO (frente) — oposto da pilha. */
typedef struct NodoAlerta {
    int     id_setor;
    float   ppm;
    time_t  timestamp;
    struct NodoAlerta *proximo;
} NodoAlerta;

/* Fila com dois ponteiros: inicio (saida) e fim (entrada). */
typedef struct {
    NodoAlerta *inicio;
    NodoAlerta *fim;
    int         tamanho;
} FilaAlertas;

void  fila_criar      (FilaAlertas *f);
bool  enfileirar      (FilaAlertas *f, int id_setor, float ppm, time_t ts); /* O(1) + filtro critico */
bool  desenfileirar   (FilaAlertas *f, NodoAlerta *saida);                  /* O(1) blindado        */
bool  frente_fila     (const FilaAlertas *f, NodoAlerta *saida);            /* espia sem remover    */
int   fila_tamanho    (const FilaAlertas *f);
bool  fila_vazia      (const FilaAlertas *f);
void  fila_destruir   (FilaAlertas *f);                                     /* libera tudo  O(n)    */
void  fila_imprimir   (const FilaAlertas *f);                               /* inicio -> fim         */

#endif
