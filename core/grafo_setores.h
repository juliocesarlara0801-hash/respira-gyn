/*
 * grafo_setores.h — Grafo NAO direcionado de setores vizinhos.
 * Estrutura NAO LINEAR (critério I do seminário).
 * Lista de adjacencia: cada vertice guarda uma cadeia dos vizinhos.
 * Uso real: BFS para propagar alertas — CO2 nao respeita fronteira de setor.
 */

#ifndef GRAFO_SETORES_H
#define GRAFO_SETORES_H

#include <stdbool.h>
#include "fila_alertas.h"

#define GRAFO_MAX_VERTICES  64            /* folga sobre os 30 setores reais */

/* Nodo da lista de adjacencia. */
typedef struct NodoAdj {
    int vizinho;                          /* id do setor vizinho          */
    struct NodoAdj *proximo;
} NodoAdj;

/* Grafo: V vertices, cada um com uma lista de adjacencia. */
typedef struct {
    int       n_vertices;
    NodoAdj  *adj[GRAFO_MAX_VERTICES];
    double    lat[GRAFO_MAX_VERTICES];    /* guardadas para ligacao por proximidade */
    double    lng[GRAFO_MAX_VERTICES];
} Grafo;

void  grafo_criar            (Grafo *g, int n_vertices);
bool  grafo_registrar_setor  (Grafo *g, int id, double lat, double lng);
bool  grafo_adicionar_aresta (Grafo *g, int a, int b);              /* bidirecional  O(1) */
int   grafo_grau             (const Grafo *g, int id);              /* numero de vizinhos */
void  grafo_destruir         (Grafo *g);                            /* libera todas as listas */

/* Conecta automaticamente pares de setores a <= raio_km de distancia.
   Usa formula haversine simplificada. Retorna o numero de arestas criadas. */
int   grafo_conectar_por_proximidade(Grafo *g, double raio_km);

/* BFS a partir de `origem`. Para cada vertice ALCANCADO (exceto a origem),
   enfileira um alerta na fila `destino` com o ppm_origem (propagacao).
   Retorna quantos vizinhos foram notificados. */
int   grafo_propagar_alerta  (const Grafo *g, int origem,
                              float ppm_origem, FilaAlertas *destino);

void  grafo_imprimir         (const Grafo *g);

#endif
