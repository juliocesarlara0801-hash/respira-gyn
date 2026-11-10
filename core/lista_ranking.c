/*
 * lista_ranking.c — Lista encadeada simples, ORDENADA por PPM desc.
 * Insercao: O(n) pior caso (vs O(1) da pilha e da fila). Justificativa:
 * aqui precisamos de ORDEM TOTAL, nao so da ponta.
 */

#include "lista_ranking.h"
#include <stdio.h>
#include <stdlib.h>

void lista_criar(ListaRanking *l) {
    if (l == NULL) return;
    l->cabeca = NULL;
    l->tamanho = 0;
}

void lista_destruir(ListaRanking *l) {
    if (l == NULL) return;
    while (l->cabeca != NULL) {
        NodoRanking *temp = l->cabeca;
        l->cabeca = l->cabeca->proximo;
        free(temp);
    }
    l->tamanho = 0;
}

/* INSERIR ORDENADO  O(n).
   Se setor ja existe, atualiza o PPM (nao duplica). Reinsere para manter a ordem. */
bool inserir_ordenado(ListaRanking *l, int id_setor, float ppm) {
    if (l == NULL) return false;

    remover_setor(l, id_setor);                          /* evita duplicata */

    NodoRanking *novo = (NodoRanking*) malloc(sizeof(NodoRanking));
    if (novo == NULL) return false;
    novo->id_setor = id_setor;
    novo->ppm      = ppm;
    novo->proximo  = NULL;

    /* Caso 1: lista vazia ou novo e o maior -> entra na cabeca. */
    if (l->cabeca == NULL || ppm > l->cabeca->ppm) {
        novo->proximo = l->cabeca;
        l->cabeca     = novo;
        l->tamanho++;
        return true;
    }

    /* Caso 2: varre ate achar o primeiro cujo PPM e menor -> insere ANTES dele. */
    NodoRanking *cur = l->cabeca;
    while (cur->proximo != NULL && cur->proximo->ppm >= ppm) {
        cur = cur->proximo;
    }
    novo->proximo = cur->proximo;
    cur->proximo  = novo;
    l->tamanho++;
    return true;
}

/* REMOVER POR CHAVE  O(n). */
bool remover_setor(ListaRanking *l, int id_setor) {
    if (l == NULL || l->cabeca == NULL) return false;

    NodoRanking *cur  = l->cabeca;
    NodoRanking *prev = NULL;
    while (cur != NULL && cur->id_setor != id_setor) {
        prev = cur;
        cur = cur->proximo;
    }
    if (cur == NULL) return false;                       /* nao achou */

    if (prev == NULL) {                                  /* era a cabeca */
        l->cabeca = cur->proximo;
    } else {
        prev->proximo = cur->proximo;
    }
    free(cur);
    l->tamanho--;
    return true;
}

/* BUSCAR POR CHAVE  O(n). Devolve o PPM via ponteiro. */
bool buscar_setor(const ListaRanking *l, int id_setor, float *ppm_out) {
    if (l == NULL) return false;
    NodoRanking *cur = l->cabeca;
    while (cur != NULL) {
        if (cur->id_setor == id_setor) {
            if (ppm_out != NULL) *ppm_out = cur->ppm;
            return true;
        }
        cur = cur->proximo;
    }
    return false;
}

int  lista_tamanho(const ListaRanking *l) { return (l == NULL) ? 0 : l->tamanho; }
bool lista_vazia  (const ListaRanking *l) { return (l == NULL || l->cabeca == NULL); }

/* Top-N: imprime os primeiros n (ja estao em ordem desc). */
void lista_imprimir_top(const ListaRanking *l, int n) {
    if (l == NULL || l->cabeca == NULL) { printf("[ranking vazio]\n"); return; }
    printf("--- Top %d setores mais criticos ---\n", n);
    NodoRanking *cur = l->cabeca;
    int pos = 1;
    while (cur != NULL && pos <= n) {
        printf("  #%d  setor=%d  ppm=%.2f\n", pos, cur->id_setor, cur->ppm);
        cur = cur->proximo;
        pos++;
    }
    printf("------------------------------------\n");
}
