/*
 * pilha_co2.h — Pilha LIFO de leituras de CO2.
 * RespiraGyn — Inovatech 2026 — Grupo 13 (Pressao/CO2).
 * Anthony, Henrique, Julio, Marcos, Murilo.
 * Aula 13 — Prof. Me. Valdir Prado.
 */

#ifndef PILHA_CO2_H
#define PILHA_CO2_H

#include <stdbool.h>
#include <time.h>

#define CO2_PPM_MINIMO    400.0f   /* filtro atmosfera base */
#define CO2_PILHA_MAXIMA  10000    /* teto anti-Overflow    */

/* Nodo da pilha. O gancho aponta para baixo (gravidade LIFO). */
typedef struct NodoLeitura {
    float   ppm;
    int     id_setor;
    int     nivel_trafego;       /* 0=livre .. 5=critico */
    time_t  timestamp;
    struct NodoLeitura *abaixo;
} NodoLeitura;

/* Pilha encapsulada: evita variavel global e permite N pilhas. */
typedef struct {
    NodoLeitura *topo;
    int tamanho;
} PilhaCO2;

void  pilha_criar    (PilhaCO2 *p);                /* inicializa vazia       */
bool  push_leitura   (PilhaCO2 *p,                 /* PUSH  O(1) + filtro    */
                      float ppm, int id_setor,
                      int nivel_trafego, time_t ts);
bool  pop_leitura    (PilhaCO2 *p, NodoLeitura *saida); /* POP   O(1) blindado */
bool  peek_leitura   (const PilhaCO2 *p, NodoLeitura *saida); /* PEEK  O(1)   */
int   pilha_tamanho  (const PilhaCO2 *p);
bool  pilha_vazia    (const PilhaCO2 *p);
void  pilha_destruir (PilhaCO2 *p);                /* libera tudo  O(n)      */
void  pilha_imprimir (const PilhaCO2 *p);          /* debug: topo -> base    */

#endif
