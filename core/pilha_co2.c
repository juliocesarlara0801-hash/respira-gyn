/*
 * pilha_co2.c — PUSH / POP / PEEK da pilha LIFO.
 * Blindagem contra Underflow e Overflow. Filtro de 400 ppm no PUSH.
 */

#include "pilha_co2.h"
#include <stdio.h>
#include <stdlib.h>

/* Inicializa a pilha vazia. */
void pilha_criar(PilhaCO2 *p) {
    if (p == NULL) return;
    p->topo = NULL;
    p->tamanho = 0;
}

/* Desempilha tudo ate zerar. Libera cada caixa com free -> sem leak. */
void pilha_destruir(PilhaCO2 *p) {
    if (p == NULL) return;
    while (p->topo != NULL) {
        NodoLeitura *temp = p->topo;
        p->topo = p->topo->abaixo;
        free(temp);
    }
    p->tamanho = 0;
}

/* PUSH  O(1).
   Filtro: ppm < 400 (abaixo da atmosfera base) nao vira caixa — nao gasta malloc.
   Teto: respeita CO2_PILHA_MAXIMA para evitar Stack Overflow em telemetria continua. */
bool push_leitura(PilhaCO2 *p,
                  float ppm, int id_setor,
                  int nivel_trafego, time_t timestamp) {

    if (p == NULL) return false;
    if (ppm < CO2_PPM_MINIMO) return false;              /* filtro atmosferico */
    if (p->tamanho >= CO2_PILHA_MAXIMA) return false;    /* blindagem Overflow */

    NodoLeitura *novo = (NodoLeitura*) malloc(sizeof(NodoLeitura));
    if (novo == NULL) return false;                      /* RAM esgotada       */

    novo->ppm           = ppm;
    novo->id_setor      = id_setor;
    novo->nivel_trafego = nivel_trafego;
    novo->timestamp     = timestamp;
    novo->abaixo        = p->topo;                       /* aponta ao topo antigo */
    p->topo             = novo;                          /* novo topo             */
    p->tamanho++;
    return true;
}

/* POP  O(1).
   Blindagem obrigatoria: se topo == NULL, retorna false sem acessar memoria.
   Copia o dado para `saida` antes de free — evita use-after-free no chamador. */
bool pop_leitura(PilhaCO2 *p, NodoLeitura *saida) {
    if (p == NULL || p->topo == NULL) return false;      /* blindagem Underflow */

    NodoLeitura *temp = p->topo;
    if (saida != NULL) {
        saida->ppm           = temp->ppm;
        saida->id_setor      = temp->id_setor;
        saida->nivel_trafego = temp->nivel_trafego;
        saida->timestamp     = temp->timestamp;
        saida->abaixo        = NULL;
    }
    p->topo = temp->abaixo;                              /* desce um andar */
    free(temp);                                          /* devolve RAM    */
    p->tamanho--;
    return true;
}

/* PEEK  O(1) — leitura passiva, nao mexe na estrutura. */
bool peek_leitura(const PilhaCO2 *p, NodoLeitura *saida) {
    if (p == NULL || p->topo == NULL || saida == NULL) return false;
    saida->ppm           = p->topo->ppm;
    saida->id_setor      = p->topo->id_setor;
    saida->nivel_trafego = p->topo->nivel_trafego;
    saida->timestamp     = p->topo->timestamp;
    saida->abaixo        = NULL;
    return true;
}

int  pilha_tamanho(const PilhaCO2 *p) { return (p == NULL) ? 0 : p->tamanho; }
bool pilha_vazia  (const PilhaCO2 *p) { return (p == NULL || p->topo == NULL); }

/* Varre do topo ate a base — util para auditar em arguicao. */
void pilha_imprimir(const PilhaCO2 *p) {
    if (p == NULL || p->topo == NULL) { printf("[pilha vazia]\n"); return; }
    printf("--- Pilha (topo -> base) | tamanho=%d ---\n", p->tamanho);
    NodoLeitura *cur = p->topo;
    int pos = 0;
    while (cur != NULL) {
        printf("  [%02d] setor=%d  ppm=%.2f  trafego=%d  ts=%ld\n",
               pos, cur->id_setor, cur->ppm,
               cur->nivel_trafego, (long) cur->timestamp);
        cur = cur->abaixo;
        pos++;
    }
    printf("-------------------------------------------\n");
}
