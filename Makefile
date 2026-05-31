# Makefile — Sistema de Processamento de Cartões
CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=c11
TARGET  = processador
SRCS    = main.c modulo_csv.c modulo_ordenacao.c modulo_estruturas.c modulo_relatorio.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c transacao.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET) transacoes.dat relatorio_consolidado.txt

.PHONY: all clean
