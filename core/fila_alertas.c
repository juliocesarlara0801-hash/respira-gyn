/*
 * fila_alertas.c — Motores de Enfileiramento e Remocao (FIFO).
 * Dois ponteiros (inicio, fim) garantem O(1) nas duas pontas.
 * Filtro no enfileirar: so entra alerta se ppm >= FILA_PPM_CRITICO.
 */

#include "fila_alertas.h"
#include <stdio.h>
#include <stdlib.h>

void fila_criar(FilaAlertas *f) {
    if (f == NULL) return;
    f->inicio = NULL;
    f->fim    = NULL;
    f->tamanho = 0;
}

/* Libera cada nodo do inicio ate o fim. Sem leak. */
void fila_destruir(FilaAlertas *f) {
    if (f == NULL) return;
    while (f->inicio != NULL) {
        NodoAlerta *temp = f->inicio;
        f->inicio = f->inicio->proximo;
        free(temp);
    }
    f->fim = NULL;
    f->tamanho = 0;
}

/* ENFILEIRAR  O(1).
   Filtro: so entra se ppm >= FILA_PPM_CRITICO (1200). Leitura normal nao ocupa fila.
   Novo nodo vai pro FIM; se fila vazia, inicio tambem aponta pra ele. */
bool enfileirar(FilaAlertas *f, int id_setor, float ppm, time_t ts) {
    if (f == NULL) return false;
    if (ppm < FILA_PPM_CRITICO) return false;            /* filtro critico     */
    if (f->tamanho >= FILA_MAXIMA) return false;         /* blindagem Overflow */

    NodoAlerta *novo = (NodoAlerta*) malloc(sizeof(NodoAlerta));
    if (novo == NULL) return false;

    novo->id_setor  = id_setor;
    novo->ppm       = ppm;
    novo->timestamp = ts;
    novo->proximo   = NULL;                              /* ultimo da fila */

    if (f->fim == NULL) {                                /* fila vazia -> dois ponteiros apontam pro mesmo nodo */
        f->inicio = novo;
        f->fim    = novo;
    } else {
        f->fim->proximo = novo;                          /* antigo fim aponta pro novo */
        f->fim = novo;                                   /* atualiza o fim             */
    }
    f->tamanho++;
    return true;
}

/* DESENFILEIRAR  O(1).
   Remove o INICIO (o alerta que chegou PRIMEIRO). Blindagem: se vazia, retorna false. */
bool desenfileirar(FilaAlertas *f, NodoAlerta *saida) {
    if (f == NULL || f->inicio == NULL) return false;    /* blindagem Underflow */

    NodoAlerta *temp = f->inicio;
    if (saida != NULL) {
        saida->id_setor  = temp->id_setor;
        saida->ppm       = temp->ppm;
        saida->timestamp = temp->timestamp;
        saida->proximo   = NULL;
    }

    f->inicio = temp->proximo;
    if (f->inicio == NULL) f->fim = NULL;                /* ficou vazia -> zera o fim tambem */
    free(temp);
    f->tamanho--;
    return true;
}

/* FRENTE — espia o inicio sem remover. */
bool frente_fila(const FilaAlertas *f, NodoAlerta *saida) {
    if (f == NULL || f->inicio == NULL || saida == NULL) return false;
    saida->id_setor  = f->inicio->id_setor;
    saida->ppm       = f->inicio->ppm;
    saida->timestamp = f->inicio->timestamp;
    saida->proximo   = NULL;
    return true;
}

int  fila_tamanho(const FilaAlertas *f) { return (f == NULL) ? 0 : f->tamanho; }
bool fila_vazia  (const FilaAlertas *f) { return (f == NULL || f->inicio == NULL); }

/* Varre do inicio ate o fim — ordem de chegada. */
void fila_imprimir(const FilaAlertas *f) {
    if (f == NULL || f->inicio == NULL) { printf("[fila vazia]\n"); return; }
    printf("--- Fila (inicio -> fim) | tamanho=%d ---\n", f->tamanho);
    NodoAlerta *cur = f->inicio;
    int pos = 0;
    while (cur != NULL) {
        printf("  [%02d] setor=%d  ppm=%.2f  ts=%ld\n",
               pos, cur->id_setor, cur->ppm, (long) cur->timestamp);
        cur = cur->proximo;
        pos++;
    }
    printf("-------------------------------------------\n");
}
