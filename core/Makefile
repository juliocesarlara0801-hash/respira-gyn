# Makefile - RespiraGyn (Inovatech 2026 - Grupo 13)
#
#   make           -> gera ./respiragyn
#   make test      -> compila e roda testes unitarios
#   make backend   -> roda em modo backend (stdin CSV)
#   make clean     -> remove binarios e objetos

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -O2
LDFLAGS := -lm

OBJS    := pilha_co2.o fila_alertas.o lista_ranking.o grafo_setores.o estimador.o forecast.o
BIN     := respiragyn
TESTBIN := tests/test_pilha

all: $(BIN)

$(BIN): main.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TESTBIN)
	@echo "--- Executando testes unitarios ---"
	@./$(TESTBIN)

$(TESTBIN): tests/test_pilha.c $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

backend: $(BIN)
	./$(BIN) --backend

clean:
	rm -f *.o $(BIN) $(TESTBIN)

.PHONY: all test backend clean
