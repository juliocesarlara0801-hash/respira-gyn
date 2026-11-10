/*
 * grafo_setores.c — Grafo NAO direcionado por lista de adjacencia.
 * BFS propaga alertas aos vizinhos usando uma FILA (reuso da fila_alertas).
 *
 * Big-O:
 *   adicionar aresta   -> O(1)
 *   grau               -> O(grau)
 *   BFS                -> O(V + E)
 *   conectar_por_proximidade -> O(V^2) (pares de setores)
 */

#include "grafo_setores.h"
#include "fila_alertas.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAIO_TERRA_KM  6371.0

/* Inicializa o grafo vazio: nenhum vertice registrado, arestas = 0. */
void grafo_criar(Grafo *g, int n_vertices) {
    if (g == NULL) return;
    if (n_vertices > GRAFO_MAX_VERTICES) n_vertices = GRAFO_MAX_VERTICES;
    g->n_vertices = n_vertices;
    for (int i = 0; i < GRAFO_MAX_VERTICES; i++) {
        g->adj[i] = NULL;
        g->lat[i] = 0.0;
        g->lng[i] = 0.0;
    }
}

/* Libera todas as listas de adjacencia. Sem leak. */
void grafo_destruir(Grafo *g) {
    if (g == NULL) return;
    for (int i = 0; i < g->n_vertices; i++) {
        NodoAdj *cur = g->adj[i];
        while (cur != NULL) {
            NodoAdj *temp = cur;
            cur = cur->proximo;
            free(temp);
        }
        g->adj[i] = NULL;
    }
    g->n_vertices = 0;
}

/* Guarda as coordenadas do setor (usadas depois no conectar_por_proximidade). */
bool grafo_registrar_setor(Grafo *g, int id, double lat, double lng) {
    if (g == NULL || id < 0 || id >= g->n_vertices) return false;
    g->lat[id] = lat;
    g->lng[id] = lng;
    return true;
}

/* Insere na frente das duas listas (bidirecional) — O(1) por aresta. */
bool grafo_adicionar_aresta(Grafo *g, int a, int b) {
    if (g == NULL || a < 0 || b < 0) return false;
    if (a >= g->n_vertices || b >= g->n_vertices) return false;
    if (a == b) return false;                             /* sem laco */

    NodoAdj *na = (NodoAdj*) malloc(sizeof(NodoAdj));
    NodoAdj *nb = (NodoAdj*) malloc(sizeof(NodoAdj));
    if (na == NULL || nb == NULL) { free(na); free(nb); return false; }

    na->vizinho = b;  na->proximo = g->adj[a];  g->adj[a] = na;
    nb->vizinho = a;  nb->proximo = g->adj[b];  g->adj[b] = nb;
    return true;
}

/* Conta quantos vizinhos sao acessiveis do setor id. */
int grafo_grau(const Grafo *g, int id) {
    if (g == NULL || id < 0 || id >= g->n_vertices) return 0;
    int grau = 0;
    NodoAdj *cur = g->adj[id];
    while (cur != NULL) { grau++; cur = cur->proximo; }
    return grau;
}

/* Haversine — distancia em km entre duas coordenadas. */
static double distancia_km(double lat1, double lng1, double lat2, double lng2) {
    double dlat = (lat2 - lat1) * M_PI / 180.0;
    double dlng = (lng2 - lng1) * M_PI / 180.0;
    double a = sin(dlat/2.0) * sin(dlat/2.0) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dlng/2.0) * sin(dlng/2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return RAIO_TERRA_KM * c;
}

/* Liga todo par de setores cuja distancia e <= raio_km. O(V^2). */
int grafo_conectar_por_proximidade(Grafo *g, double raio_km) {
    if (g == NULL) return 0;
    int arestas = 0;
    for (int i = 0; i < g->n_vertices; i++) {
        for (int j = i + 1; j < g->n_vertices; j++) {
            double d = distancia_km(g->lat[i], g->lng[i], g->lat[j], g->lng[j]);
            if (d <= raio_km) {
                if (grafo_adicionar_aresta(g, i, j)) arestas++;
            }
        }
    }
    return arestas;
}

/* BFS a partir de `origem`. Para cada vizinho alcancado, enfileira um alerta
   com o PPM da origem. O proprio vertice origem NAO entra na fila destino.
   Big-O: O(V + E). Visitados marcados com um array local. */
int grafo_propagar_alerta(const Grafo *g, int origem,
                          float ppm_origem, FilaAlertas *destino) {
    if (g == NULL || destino == NULL) return 0;
    if (origem < 0 || origem >= g->n_vertices) return 0;

    bool visitado[GRAFO_MAX_VERTICES] = {false};
    int  bfs_fila[GRAFO_MAX_VERTICES];                    /* fila local para BFS */
    int  ini = 0, fim = 0;
    int  notificados = 0;
    time_t agora = time(NULL);

    visitado[origem] = true;
    bfs_fila[fim++] = origem;

    while (ini < fim) {
        int v = bfs_fila[ini++];
        NodoAdj *cur = g->adj[v];
        while (cur != NULL) {
            int w = cur->vizinho;
            if (!visitado[w]) {
                visitado[w] = true;
                bfs_fila[fim++] = w;
                /* enfileirar usa filtro PPM_CRITICO; forcamos ppm_origem (>=1200). */
                if (enfileirar(destino, w, ppm_origem, agora)) notificados++;
            }
            cur = cur->proximo;
        }
    }
    return notificados;
}

void grafo_imprimir(const Grafo *g) {
    if (g == NULL) { printf("[grafo nulo]\n"); return; }
    printf("--- Grafo de setores | V=%d ---\n", g->n_vertices);
    for (int i = 0; i < g->n_vertices; i++) {
        printf("  [%d] grau=%d  vizinhos: ", i, grafo_grau(g, i));
        NodoAdj *cur = g->adj[i];
        while (cur != NULL) {
            printf("%d ", cur->vizinho);
            cur = cur->proximo;
        }
        printf("\n");
    }
    printf("--------------------------------\n");
}
